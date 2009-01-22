/* 
 * FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 * Copyright (C) 2005/2006, Anthony Minessale II <anthmct@yahoo.com>
 *
 * Version: MPL 1.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is FreeSWITCH Modular Media Switching Software Library / Soft-Switch Application
 *
 * The Initial Developer of the Original Code is
 * Anthony Minessale II <anthmct@yahoo.com>
 * Portions created by the Initial Developer are Copyright (C)
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 * 
 * Anthony Minessale II <anthmct@yahoo.com>
 * Andrew Thompson <andrew@hijacked.us>
 * Rob Charlton <rob.charlton@savageminds.com>
 *
 *
 * ei_helpers.c -- helper functions for ei
 *
 */
#include <switch.h>
#include <ei.h>
#include "mod_erlang_event.h"

/* Stolen from code added to ei in R12B-5.
 * Since not everyone has this version yet;
 * provide our own version. 
 * */

#define put8(s,n) do { \
	  (s)[0] = (char)((n) & 0xff); \
	  (s) += 1; \
} while (0)

#define put32be(s,n) do {  \
	  (s)[0] = ((n) >>  24) & 0xff; \
	  (s)[1] = ((n) >>  16) & 0xff; \
	  (s)[2] = ((n) >>  8) & 0xff;  \
	  (s)[3] = (n) & 0xff; \
	  (s) += 4; \
} while (0)

void ei_link(listener_t *listener, erlang_pid *from, erlang_pid *to) {
	char msgbuf[2048];
	char *s;
	int index = 0;
	/*int n;*/

	index = 5;                                     /* max sizes: */
	ei_encode_version(msgbuf,&index);                     /*   1 */
	ei_encode_tuple_header(msgbuf,&index,3);
	ei_encode_long(msgbuf,&index,ERL_LINK);
	ei_encode_pid(msgbuf,&index,from);                    /* 268 */
	ei_encode_pid(msgbuf,&index,to);                      /* 268 */

	/* 5 byte header missing */
	s = msgbuf;
	put32be(s, index - 4);                                /*   4 */
	put8(s, ERL_PASS_THROUGH);                            /*   1 */
	/* sum:  542 */

	switch_mutex_lock(listener->sock_mutex);
	write(listener->sockfd, msgbuf, index);
	switch_mutex_unlock(listener->sock_mutex);
}

void ei_encode_switch_event_headers(ei_x_buff *ebuf, switch_event_t *event)
{
	int i;
	char *uuid = switch_event_get_header(event, "unique-id");

	switch_event_header_t *hp;

	for (i = 0, hp = event->headers; hp; hp = hp->next, i++);

	if (event->body)
		i++;

	ei_x_encode_list_header(ebuf, i+1);

	if (uuid) {
		ei_x_encode_string(ebuf, switch_event_get_header(event, "unique-id"));
	} else {
		ei_x_encode_atom(ebuf, "undefined");
	}

	for (hp = event->headers; hp; hp = hp->next) {
		ei_x_encode_tuple_header(ebuf, 2);
		ei_x_encode_string(ebuf, hp->name);
		ei_x_encode_string(ebuf, hp->value);
	}

	if (event->body) {
		ei_x_encode_tuple_header(ebuf, 2);
		ei_x_encode_string(ebuf, "body");
		ei_x_encode_string(ebuf, event->body);
	}

	ei_x_encode_empty_list(ebuf);
}


void ei_encode_switch_event_tag(ei_x_buff *ebuf, switch_event_t *event, char *tag)
{

	ei_x_encode_tuple_header(ebuf, 2);
	ei_x_encode_atom(ebuf, tag);
	ei_encode_switch_event_headers(ebuf, event);
}


switch_status_t initialise_ei(struct ei_cnode_s *ec)
{
	switch_status_t rv;
	struct sockaddr_in server_addr;

	/* zero out the struct before we use it */
	memset(&server_addr, 0, sizeof(server_addr));
	
	/* convert the configured IP to network byte order, handing errors */
	rv = inet_pton(AF_INET, prefs.ip, &server_addr.sin_addr.s_addr);
	if (rv == 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Could not parse invalid ip address: %s\n", prefs.ip);
		return SWITCH_STATUS_FALSE;
	} else if (rv == -1) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Error when parsing ip address %s : %s\n", prefs.ip, strerror(errno));
		return SWITCH_STATUS_FALSE;
	}
	
	/* set the address family and port */
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(prefs.port);
	
	struct hostent *nodehost = gethostbyaddr(&server_addr.sin_addr.s_addr, sizeof(server_addr.sin_addr.s_addr), AF_INET);
	
	char *thishostname = nodehost->h_name;
	char thisnodename[MAXNODELEN+1];
	
	if (!strcmp(thishostname, "localhost"))
		gethostname(thishostname, EI_MAXHOSTNAMELEN);
	
	if (prefs.shortname) {
		char *off;
		if ((off = strchr(thishostname, '.'))) {
			*off = '\0';
		}
	}
	
	snprintf(thisnodename, MAXNODELEN+1, "%s@%s", prefs.nodename, thishostname);
	
	/* init the ei stuff */
	if (ei_connect_xinit(ec, thishostname, prefs.nodename, thisnodename, (Erl_IpAddr)(&server_addr.sin_addr.s_addr), prefs.cookie, 0) < 0) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Failed to init ei connection\n");
		return SWITCH_STATUS_FALSE;
	}

	switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "ei initialized at %s\n", thisnodename);
	return SWITCH_STATUS_SUCCESS;
}

/* For Emacs:
 * Local Variables:
 * mode:c
 * indent-tabs-mode:t
 * tab-width:4
 * c-basic-offset:4
 * End:
 * For VIM:
 * vim:set softtabstop=4 shiftwidth=4 tabstop=4:
 */
