
/******************************************************************************
** This file is part of profile-qt
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
**
** Contact: Sakari Poussa <sakari.poussa@nokia.com>
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are met:
**
** Redistributions of source code must retain the above copyright notice,
** this list of conditions and the following disclaimer. Redistributions in
** binary form must reproduce the above copyright notice, this list of
** conditions and the following disclaimer in the documentation  and/or
** other materials provided with the distribution.
**
** Neither the name of Nokia Corporation nor the names of its contributors
** may be used to endorse or promote products derived from this software 
** without specific prior written permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
** AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
** THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
** PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
** CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
** EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
** PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
** OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
** WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
** OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
** ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
******************************************************************************/

#include "profiled_config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>

#include <dbus/dbus-glib-lowlevel.h>

#include "codec.h"
#include "libprofile-internal.h"
#include "profile_dbus.h"

#include "logging.h"

/* ========================================================================= *
 * Configuration
 * ========================================================================= */

#define FEED_INITIAL_VALUES_TO_CB 0

#define PROFILED_MATCH \
  "type='signal'"\
  ",interface='"PROFILED_INTERFACE"'"\
  ",member='"PROFILED_CHANGED"'"

/* ========================================================================= *
 * Callback Array Handling
 * ========================================================================= */

typedef struct
{
  void *user_cb;
  void *data;
  void (*free_cb)(void*);
} profile_hook_t;

/* ------------------------------------------------------------------------- *
 * profile_hook_add
 * ------------------------------------------------------------------------- */

static
void
profile_hook_add(profile_hook_t **parr, size_t *pcnt,
                        void *user_cb,
                        void *data,
                        void (*free_cb)(void*))
{
  profile_hook_t *arr = *parr;
  size_t          cnt = *pcnt;

  //log_warning_F("cb=%p, data=%p, free=%p\n",user_cb,data,free_cb);

  if( user_cb != 0 )
  {
    int i = cnt++;
    arr = realloc(arr, cnt * sizeof *arr);
    arr[i].user_cb = user_cb;
    arr[i].free_cb = free_cb;
    arr[i].data    = data;
  }

  *parr = arr;
  *pcnt = cnt;

}

/* ------------------------------------------------------------------------- *
 * profile_hook_rem
 * ------------------------------------------------------------------------- */

static
int
profile_hook_rem(profile_hook_t **parr, size_t *pcnt,
                        void *user_cb,
                        void *data)
{
  int             res = 0;
  profile_hook_t *arr = *parr;
  size_t          cnt = *pcnt;

  //log_warning_F("cb=%p, data=%p\n",user_cb,data);

  for( size_t i = cnt; i--; )
  {
    if( arr[i].user_cb != user_cb ) continue;
    if( arr[i].data != data ) continue;

    if( arr[i].free_cb != 0 && arr[i].data != 0 )
    {
      arr[i].free_cb(arr[i].data);
    }

    for( --cnt; i < cnt; ++i )
    {
      arr[i] = arr[i+1];
    }

    arr = realloc(arr, cnt * sizeof *arr);
    res = 1;
    break;
  }

  *parr = arr;
  *pcnt = cnt;

  return res;
}

/* ========================================================================= *
 * State Data
 * ========================================================================= */

/* Has profile tracker been activated */
static bool            profile_tracker_on  = FALSE;

/* D-Bus connection used by profile tracker */
static DBusConnection *profile_tracker_con = NULL;

/* Callback function pointers */
static profile_track_profile_fn_data profile_track_profile_func = NULL;
static void                         *profile_track_profile_data = NULL;

static profile_track_value_fn_data   profile_track_active_func  = NULL;
static void                         *profile_track_active_data  = NULL;

static profile_track_value_fn_data   profile_track_change_func  = NULL;
static void                         *profile_track_change_data  = NULL;

/* Callback arrays */
static profile_hook_t *profile_hook  = 0;
static size_t          profile_hooks = 0;

static profile_hook_t *active_hook   = 0;
static size_t          active_hooks  = 0;

static profile_hook_t *change_hook   = 0;
static size_t          change_hooks  = 0;

/* ========================================================================= *
 * Wrappers for calling change indication callbacks
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * dummy_track_profile  --  callback conversion thunk
 * ------------------------------------------------------------------------- */

static void dummy_track_profile(const char *profile, void *user_data)
{
  profile_track_profile_fn func = user_data;
  func(profile);
}

/* ------------------------------------------------------------------------- *
 * dummy_track_value  --  callback conversion thunk
 * ------------------------------------------------------------------------- */

static void dummy_track_value(const char *profile,
                              const char *key,
                              const char *val,
                              const char *type,
                              void *user_data)
{
  profile_track_value_fn func = user_data;
  func(profile, key, val, type);
}

/* ------------------------------------------------------------------------- *
 * profile_track_profile  --  profile changed
 * ------------------------------------------------------------------------- */

static inline void profile_track_profile(const char *profile)
{
  if( profile_track_profile_func != 0 )
  {
    log_debug("%s - %s\n", __FUNCTION__, profile);
    profile_track_profile_func(profile, profile_track_profile_data);
  }

  for( size_t i = 0; i < profile_hooks; ++i )
  {
    profile_hook_t *h = &profile_hook[i];
    profile_track_profile_fn_data cb = h->user_cb;
    log_debug("%s - %s @ %p %p %p\n", __FUNCTION__, profile,
              h->user_cb, h->data, h->free_cb);
    if( cb ) cb(profile, h->data);
  }
}

/* ------------------------------------------------------------------------- *
 * profile_track_active  --  value changed in current profile
 * ------------------------------------------------------------------------- */

static inline void profile_track_active(const char *profile,
                                        const char *key,
                                        const char *val,
                                        const char *type)
{
  if( profile_track_active_func != 0 )
  {
    log_debug("%s - %s: %s = %s (%s)\n", __FUNCTION__,
              profile, key, val, type);

    profile_track_active_func(profile, key, val, type,
                              profile_track_active_data);
  }

  for( size_t i = 0; i < active_hooks; ++i )
  {
    profile_hook_t *h = &active_hook[i];
    profile_track_value_fn_data cb = h->user_cb;

    log_debug("%s - %s: %s = %s (%s)@ %p %p %p\n",
              __FUNCTION__,
              profile, key, val, type,
              h->user_cb, h->data, h->free_cb);

    if( cb ) cb(profile, key, val, type, h->data);
  }
}

/* ------------------------------------------------------------------------- *
 * profile_track_change  --  value changed in non-current profile
 * ------------------------------------------------------------------------- */

static inline void profile_track_change(const char *profile,
                                        const char *key,
                                        const char *val,
                                        const char *type)
{
  if( profile_track_change_func != 0 )
  {
    log_debug("%s - %s: %s = %s (%s)\n", __FUNCTION__,
              profile, key, val, type);

    profile_track_change_func(profile, key, val, type,
                              profile_track_change_data);
  }

  for( size_t i = 0; i < change_hooks; ++i )
  {
    profile_hook_t *h = &change_hook[i];
    profile_track_value_fn_data cb = h->user_cb;

    log_debug("%s - %s: %s = %s (%s)@ %p %p %p\n",
              __FUNCTION__,
              profile, key, val, type,
              h->user_cb, h->data, h->free_cb);

    if( cb ) cb(profile, key, val, type, h->data);
  }
}

/* ========================================================================= *
 * D-Bus message filter
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * profile_tracker_filter  -- handle requests coming via dbus
 * ------------------------------------------------------------------------- */

static
DBusHandlerResult
profile_tracker_filter(DBusConnection *conn,
                       DBusMessage *msg,
                       void *user_data)
{
  (void)conn; (void)user_data;

  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);

#if 0
  const char         *typename  = "UNKNOWN";
  switch( type )
  {
  case DBUS_MESSAGE_TYPE_INVALID:       typename = "INVALID"; break;
  case DBUS_MESSAGE_TYPE_METHOD_CALL:   typename = "M-CALL"; break;
  case DBUS_MESSAGE_TYPE_METHOD_RETURN: typename = "M-RET"; break;
  case DBUS_MESSAGE_TYPE_ERROR:         typename = "ERROR"; break;
  case DBUS_MESSAGE_TYPE_SIGNAL:        typename = "SIGNAL"; break;
  }
  log_debug("%s: %s, %s, %s\n", typename, interface, object, member);
#endif

  if( !interface || !member || !object )
  {
    goto cleanup;
  }

  if( type == DBUS_MESSAGE_TYPE_SIGNAL &&
      !strcmp(interface, PROFILED_SERVICE) &&
      !strcmp(object, PROFILED_PATH) &&
      !strcmp(member, PROFILED_CHANGED) )
  {
    DBusMessageIter iter;
    DBusMessageIter item;

    int         changed = 0;
    int         active  = 0;
    const char *profile = 0;

    dbus_message_iter_init(msg, &iter);

    if( decode_bool(&iter, &changed) || decode_bool(&iter, &active) ||
        decode_string(&iter, &profile) )
    {
      goto cleanup;
    }

    if( changed != 0 )
    {
      profile_track_profile(profile);
    }

    dbus_message_iter_recurse(&iter, &item);

    const char *key, *val, *type;

    while( decode_triplet(&item, &key,&val,&type) == 0 )
    {
      if( active != 0 )
      {
        profile_track_active(profile, key,val,type);
      }
      else
      {
        profile_track_change(profile, key,val,type);
      }
    }
  }

  cleanup:

  return result;
}

/* ------------------------------------------------------------------------- *
 * profile_tracker_disconnect  --  release tracker dbus connection
 * ------------------------------------------------------------------------- */

void
profile_tracker_disconnect(void)
{
  DBusError err = DBUS_ERROR_INIT;

  if( profile_tracker_con != 0 )
  {
    ENTER
    /* stop listening to profiled signals */
    if( dbus_connection_get_is_connected(profile_tracker_con) )
    {
      dbus_bus_remove_match(profile_tracker_con, PROFILED_MATCH, &err);

      if( dbus_error_is_set(&err) )
      {
	log_err("%s: %s: %s\n", "dbus_bus_remove_match",
		err.name, err.message);
	dbus_error_free(&err);
      }
    }

    /* remove message filter function */
    dbus_connection_remove_filter(profile_tracker_con,
                                  profile_tracker_filter, 0);

    /* release connection reference */
    dbus_connection_unref(profile_tracker_con);
    profile_tracker_con = 0;
    LEAVE
  }

  dbus_error_free(&err);
}

/* ------------------------------------------------------------------------- *
 * profile_tracker_connect  --  establish tracker dbus connection
 * ------------------------------------------------------------------------- */

static
int
profile_tracker_connect(void)
{
  ENTER

  int             res = -1;
  DBusError       err = DBUS_ERROR_INIT;

  /* Check if we are already connected */
  if( profile_tracker_con != 0 )
  {
    res = 0; goto cleanup;
  }

  /* Connect to session bus */
  if( (profile_tracker_con = profile_connection_get()) == 0 )
  {
    goto cleanup;
  }

  /* attach connection to glib mainloop */
  dbus_connection_setup_with_g_main(profile_tracker_con, NULL);

  /* Register message filter */
  if( !dbus_connection_add_filter(profile_tracker_con, profile_tracker_filter, 0, 0) )
  {
    log_err("%s: %s: %s\n", "dbus_connection_add_filter",
            err.name, err.message);
    goto cleanup;
  }

  /* Listen to signals from profiled */
  dbus_bus_add_match(profile_tracker_con, PROFILED_MATCH, &err);

  if( dbus_error_is_set(&err) )
  {
    log_err("%s: %s: %s\n", "dbus_bus_add_match",
            err.name, err.message);
    goto cleanup;
  }

  /* Success */
  res = 0;

  cleanup:

  if( res == -1 )
  {
    profile_tracker_disconnect();
  }

  dbus_error_free(&err);

  LEAVE
  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_tracker_reconnect  --  disconnect and connect if tracking
 * ------------------------------------------------------------------------- */

void
profile_tracker_reconnect(void)
{
  ENTER

  profile_tracker_disconnect();
  if( profile_tracker_on )
  {
    profile_tracker_connect();
  }

  LEAVE
}

/* ========================================================================= *
 * API Functions
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * profile_track_set_profile_cb  --  set callback for profile changed
 * ------------------------------------------------------------------------- */

void
profile_track_set_profile_cb(profile_track_profile_fn cb)
{
  ENTER
  profile_track_profile_func = cb ? dummy_track_profile : 0;
  profile_track_profile_data = cb;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_track_set_profile_cb_with_data  --  set callback for profile changed
 * ------------------------------------------------------------------------- */

void
profile_track_set_profile_cb_with_data(profile_track_profile_fn_data cb,
                                       void *user_data)
{
  ENTER
  profile_track_profile_func = cb;
  profile_track_profile_data = user_data;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_track_set_active_cb  --  set callback for changes in active profile
 * ------------------------------------------------------------------------- */

void
profile_track_set_active_cb(profile_track_value_fn cb)
{
  ENTER
  profile_track_active_func = cb ? dummy_track_value : 0;
  profile_track_active_data = cb;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_track_set_active_cb_with_data  --  set callback for changes in active profile
 * ------------------------------------------------------------------------- */

void
profile_track_set_active_cb_with_data(profile_track_value_fn_data cb,
                                      void *user_data)
{
  ENTER
  profile_track_active_func = cb;
  profile_track_active_data = user_data;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_track_set_change_cb  -- set callback for changes in non-active profiles
 * ------------------------------------------------------------------------- */

void
profile_track_set_change_cb(profile_track_value_fn cb)
{
  ENTER
  profile_track_change_func = cb ? dummy_track_value : 0;
  profile_track_change_data = cb;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_track_set_change_cb_with_data  -- set callback for changes in non-active profiles
 * ------------------------------------------------------------------------- */

void
profile_track_set_change_cb_with_data(profile_track_value_fn_data cb, void *user_data)
{
  ENTER
  profile_track_change_func = cb;
  profile_track_change_data = user_data;
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_tracker_quit  --  stop profile tracking
 * ------------------------------------------------------------------------- */

void
profile_tracker_quit(void)
{
  ENTER
  profile_tracker_on = FALSE;
  profile_tracker_disconnect();
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_tracker_init  --  start profile tracking
 * ------------------------------------------------------------------------- */

int
profile_tracker_init(void)
{
  ENTER

  int           res = -1;

#if FEED_INITIAL_VALUES_TO_CB
  char         *cur = 0;
  profileval_t *val = 0;
#endif

  log_debug("init tracker ...\n");

  /* Establish dbus connection */
  if( profile_tracker_connect() == -1 )
  {
    goto cleanup;
  }

  /* Simulate full profile change */
#if FEED_INITIAL_VALUES_TO_CB
  if( profile_track_active_cb )
  {
    cur = profile_get_profile();
    val = profile_get_values(cur);

    if( cur && val )
    {
      //profile_track_profile(cur);

      for( size_t i = 0; val[i].pv_key; ++i )
      {
        profile_track_active(cur,
                             val[i].pv_key,
                             val[i].pv_val,
                             val[i].pv_type);
      }
    }
  }
#endif

  /* Success */
  res = 0;

  cleanup:

#if FEED_INITIAL_VALUES_TO_CB
  profile_free_values(val);
  free(cur);
#endif

  log_debug("init tracker -> %s\n",
            (res == 0) ? "OK" : "FAILED");

  /* Flag: client wants to use tracker */
  profile_tracker_on = TRUE;

  LEAVE
  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_track_add_profile_cb
 * ------------------------------------------------------------------------- */

void
profile_track_add_profile_cb(profile_track_profile_fn_data cb,
                             void *user_data,
                             profile_user_data_free_fn free_cb)
{
  profile_hook_add(&profile_hook, &profile_hooks, cb, user_data, free_cb);
}

/* ------------------------------------------------------------------------- *
 * profile_track_remove_profile_cb
 * ------------------------------------------------------------------------- */

int
profile_track_remove_profile_cb(profile_track_profile_fn_data cb,
                                void *user_data)
{
  return profile_hook_rem(&profile_hook, &profile_hooks, cb, user_data);
}

/* ------------------------------------------------------------------------- *
 * profile_track_add_active_cb
 * ------------------------------------------------------------------------- */

void
profile_track_add_active_cb(profile_track_value_fn_data cb,
                            void *user_data,
                            profile_user_data_free_fn free_cb)
{
  profile_hook_add(&active_hook, &active_hooks, cb, user_data, free_cb);
}

/* ------------------------------------------------------------------------- *
 * profile_track_remove_active_cb
 * ------------------------------------------------------------------------- */

int
profile_track_remove_active_cb(profile_track_value_fn_data cb,
                            void *user_data)
{
  return profile_hook_rem(&active_hook, &active_hooks, cb, user_data);
}

/* ------------------------------------------------------------------------- *
 * profile_track_add_change_cb
 * ------------------------------------------------------------------------- */

void
profile_track_add_change_cb(profile_track_value_fn_data cb,
                            void *user_data,
                            profile_user_data_free_fn free_cb)
{
  profile_hook_add(&change_hook, &change_hooks, cb, user_data, free_cb);
}

/* ------------------------------------------------------------------------- *
 * profile_track_remove_change_cb
 * ------------------------------------------------------------------------- */

int
profile_track_remove_change_cb(profile_track_value_fn_data cb,
                            void *user_data)
{
  return profile_hook_rem(&change_hook, &change_hooks, cb, user_data);
}
