#include <switch.h>
#include "freeswitch_perl.h"
#include "mod_perl_extra.h"

static STRLEN n_a;

#define init_me() cb_function = hangup_func_str = NULL; hh = mark = 0; my_perl = NULL; cb_arg = NULL

Session::Session() : CoreSession()
{
	init_me();
}

Session::Session(char *uuid) : CoreSession(uuid)
{
	init_me();
}

Session::Session(switch_core_session_t *new_session) : CoreSession(new_session)
{
	init_me();
}
static switch_status_t perl_hanguphook(switch_core_session_t *session_hungup);
Session::~Session()
{
	switch_safe_free(cb_function);
	switch_safe_free(cb_arg);
	switch_safe_free(hangup_func_str);
	switch_core_event_hook_remove_state_change(session, perl_hanguphook);
}

bool Session::begin_allow_threads() 
{
	do_hangup_hook();
	return true;
}

bool Session::end_allow_threads() 
{
	do_hangup_hook();
	return true;
}

void Session::setPERL(PerlInterpreter *pi)
{
	my_perl = pi;
}

PerlInterpreter *Session::getPERL()
{
	if (!my_perl) {
		switch_log_printf(SWITCH_CHANNEL_LOG, SWITCH_LOG_ERROR, "Doh!\n");		
	}
	return my_perl;
}


bool Session::ready() {
	bool r;

	sanity_check(false);	
	r = switch_channel_ready(channel) != 0;
	do_hangup_hook();

	return r;
}

void Session::check_hangup_hook() 
{
	if (hangup_func_str && (hook_state == CS_HANGUP || hook_state == CS_ROUTING)) {
		hh++;
	}
}

void Session::do_hangup_hook() 
{
	if (hh && !mark) {
		const char *err = NULL;
		mark++;
		char *code;
		if (!getPERL()) {
			return;
		}

		code = switch_mprintf("%s(%s)", hangup_func_str, hook_state == CS_HANGUP ? "hangup" : "transfer");
		Perl_eval_pv(my_perl, code, TRUE);
		free(code);
	}
}

static switch_status_t perl_hanguphook(switch_core_session_t *session_hungup) 
{
	switch_channel_t *channel = switch_core_session_get_channel(session_hungup);
	CoreSession *coresession = NULL;
	switch_channel_state_t state = switch_channel_get_state(channel);

	if ((coresession = (CoreSession *) switch_channel_get_private(channel, "CoreSession"))) {
		if (coresession->hook_state != state) {
			coresession->hook_state = state;
			coresession->check_hangup_hook();
		}
	}

	return SWITCH_STATUS_SUCCESS;
}


void Session::setHangupHook(char *func) {

	sanity_check_noreturn;

	switch_safe_free(hangup_func_str);

	if (func) {
		hangup_func_str = strdup(func);
		switch_channel_set_private(channel, "CoreSession", this);
		hook_state = switch_channel_get_state(channel);
		switch_core_event_hook_add_state_change(session, perl_hanguphook);
	}
}

void Session::setInputCallback(char *cbfunc, char *funcargs) {

	sanity_check_noreturn;

	switch_safe_free(cb_function);
	if (cbfunc) {
		cb_function = strdup(cbfunc);
	}

	switch_safe_free(cb_arg);
	if (funcargs) {
		cb_arg = strdup(funcargs);
	}
	
	args.buf = this;
	switch_channel_set_private(channel, "CoreSession", this);

	args.input_callback = dtmf_callback;  
	ap = &args;
}

switch_status_t Session::run_dtmf_callback(void *input, switch_input_type_t itype) 
{
	if (!getPERL()) {
		return SWITCH_STATUS_FALSE;;
	}

	switch (itype) {
    case SWITCH_INPUT_TYPE_DTMF:
		{
			switch_dtmf_t *dtmf = (switch_dtmf_t *) input;
			char str[32] = "";
			int arg_count = 2;
			HV *hash;
			SV *this_sv;
			char *code;

			if (!(hash = get_hv("__dtmf", TRUE))) {
				abort();
			}

			str[0] = dtmf->digit;
			this_sv = newSV(strlen(str)+1);
			sv_setpv(this_sv, str);
			hv_store(hash, "digit", 5, this_sv, 0);

			switch_snprintf(str, sizeof(str), "%d", dtmf->duration);
			this_sv = newSV(strlen(str)+1);
			sv_setpv(this_sv, str);
			hv_store(hash, "duration", 8, this_sv, 0);			

			code = switch_mprintf("$__RV = %s('dtmf', \\%%__dtmf, %s);", cb_function, switch_str_nil(cb_arg));
			Perl_eval_pv(my_perl, code, FALSE);
			free(code);

			return process_callback_result(SvPV(get_sv("__RV", TRUE), n_a));
		}
		break;
    case SWITCH_INPUT_TYPE_EVENT:
		{
			switch_event_t *event = (switch_event_t *) input;
			int arg_count = 2;
			char *code;

			mod_perl_conjure_event(my_perl, event, "__Input_Event__");
			
			code = switch_mprintf("$__RV = %s('event', $__Input_Event__, %s);", cb_function, switch_str_nil(cb_arg));
			Perl_eval_pv(my_perl, code, TRUE);
			free(code);
			
			return process_callback_result(SvPV(get_sv("__RV", TRUE), n_a));
		}
		break;
	}

	return SWITCH_STATUS_SUCCESS;
}
