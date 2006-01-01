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
 *
 *
 * switch_mutex.h -- Mutex Locking
 *
 */
/*! \file switch_mutex.h
    \brief Mutex Locking
*/
#ifndef SWITCH_MUTEX_H
#define SWITCH_MUTEX_H

#ifdef __cplusplus
extern "C" {
#endif

#include <switch.h>

SWITCH_DECLARE(switch_status) switch_mutex_init(switch_mutex_t **lock,
								  switch_lock_flag flags,
								  switch_memory_pool *pool);

SWITCH_DECLARE(switch_status) switch_mutex_destroy(switch_mutex_t *lock);
SWITCH_DECLARE(switch_status) switch_mutex_lock(switch_mutex_t *lock);
SWITCH_DECLARE(switch_status) switch_mutex_unlock(switch_mutex_t *lock);
SWITCH_DECLARE(switch_status) switch_mutex_trylock(switch_mutex_t *lock);


#ifdef __cplusplus
}
#endif


#endif
