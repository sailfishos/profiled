
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

#include "logging.h"
#include "mainloop.h"
#include "server.h"
#include "database.h"
#include "codec.h"
#include "profile_dbus.h"

#include <sys/types.h>
#include <sys/socket.h>

#include <string.h>
#include <stdlib.h>

#include <dbus/dbus-glib-lowlevel.h>

#ifndef DBUS_ERROR_INIT
# define DBUS_ERROR_INIT { NULL, NULL, TRUE, 0, 0, 0, 0, NULL }
#endif

enum
{
  /* Avoiding bursts of change signals: delay broadcast by
   * BROADCAST_DELAY_MIN ms after each change to profile data,
   * but no longer than BROADCAST_DELAY_MAX from the first
   * change */
  // broadcast 100ms after last change
  BROADCAST_DELAY_MIN =  100, /* [ms] */
  // send change broadcast in one second even if changes keep coming in
  BROADCAST_DELAY_MAX = 1000, /* [ms] */
};

/* ========================================================================= *
 * PROFILE DBUS SERVER FUNCTIONS
 * ========================================================================= */

static DBusConnection *server_bus    = NULL;

/* ------------------------------------------------------------------------- *
 * server_make_reply  --  create method call responce message
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_make_reply(DBusMessage *msg, int type, ...)
{
  DBusMessage *rsp = 0;

  va_list va;

  va_start(va, type);

  if( (rsp = dbus_message_new_method_return(msg)) != 0 )
  {
    if( !dbus_message_append_args_valist(rsp, type, va) )
    {
      dbus_message_unref(rsp), rsp = 0;
    }
  }

  va_end(va);

  return rsp;
}

static
DBusMessage *
introspect(DBusMessage *msg)
{
  const char *xml = 
    "<!DOCTYPE node PUBLIC \"-//freedesktop//DTD D-BUS Object Introspection 1.0//EN\"\n"
    "\"http://www.freedesktop.org/standards/dbus/1.0/introspect.dtd\">\n"
    "<node>\n"
    "   <interface name=\"com.nokia.profiled\">\n"
    "      <signal name=\"profile_changed\">\n"
    "         <arg type=\"b\" direction=\"out\"/>\n"
    "         <arg type=\"b\" direction=\"out\"/>\n"
    "         <arg type=\"s\" direction=\"out\"/>\n"
    "         <arg type=\"a(sss)\" direction=\"out\"/>\n"
    "      </signal>\n"
    "   </interface>\n"
    "</node>";
  log_info("%s -> reply: '%s'\n", __FUNCTION__, xml);
  return server_make_reply(msg, DBUS_TYPE_STRING, &xml, DBUS_TYPE_INVALID);
}



/* ------------------------------------------------------------------------- *
 * server_get_profiles  --  handle PROFILED_GET_PROFILES method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_profiles(DBusMessage *msg)
{
  DBusMessage *rsp = 0;
  int          len = 0;
  char       **vec = database_get_profiles(&len);

// QUARANTINE   debugf("@ server_get_profiles() -> %p %d\n", vec, len);

  rsp = server_make_reply(msg,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &vec, len,
                          DBUS_TYPE_INVALID);
  database_free_profiles(vec);

  log_info("%s -> reply: %d names\n", __FUNCTION__, len);
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_get_profile  --  handle PROFILED_GET_PROFILE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_profile(DBusMessage *msg)
{
  const char *prof = database_get_profile();

  log_info("%s -> reply: '%s'\n", __FUNCTION__, prof);
  return server_make_reply(msg, DBUS_TYPE_STRING, &prof, DBUS_TYPE_INVALID);
}

/* ------------------------------------------------------------------------- *
 * server_set_profile  --  handle PROFILED_SET_PROFILE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_set_profile(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *prof = 0;
  dbus_bool_t   res  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &prof,
                            DBUS_TYPE_INVALID) )
  {
    res = (database_set_profile(prof) == 0);
  }
  else
  {
    log_err("%s: %s: %s\n",
              dbus_message_get_member(msg),
              err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_BOOLEAN, &res, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: %s\n", __FUNCTION__, res ? "True" : "False");

  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_has_profile  --  handle PROFILED_HAS_PROFILE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_has_profile(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *prof = 0;
  dbus_bool_t   res  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &prof,
                            DBUS_TYPE_INVALID) )
  {
    res = (database_has_profile(prof) != 0);
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_BOOLEAN, &res, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: %s\n", __FUNCTION__, res ? "True" : "False");
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_get_keys  --  handle PROFILED_GET_KEYS method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_keys(DBusMessage *msg)
{
  char       **vec = 0;
  int          len = 0;
  DBusMessage *rsp = 0;

  vec = database_get_keys(&len);
  rsp = server_make_reply(msg,
                          DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &vec, len,
                          DBUS_TYPE_INVALID);
  database_free_keys(vec);

  log_info("%s -> reply: %d names\n", __FUNCTION__, len);
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_get_values  --  handle PROFILED_GET_VALUES method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_values(DBusMessage *msg)
{
  char         *prof = 0;
  profileval_t  *vec  = 0;
  int           len  = 0;
  DBusMessage  *rsp  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &prof,
                            DBUS_TYPE_INVALID) )
  {
    vec = database_get_values(prof, &len);
    rsp = server_make_reply(msg, DBUS_TYPE_INVALID);

    DBusMessageIter iter, item;

    dbus_message_iter_init_append(rsp, &iter);

    static const char sgn[] =
    DBUS_STRUCT_BEGIN_CHAR_AS_STRING
    DBUS_TYPE_STRING_AS_STRING
    DBUS_TYPE_STRING_AS_STRING
    DBUS_TYPE_STRING_AS_STRING
    DBUS_STRUCT_END_CHAR_AS_STRING;

    dbus_message_iter_open_container(&iter, DBUS_TYPE_ARRAY, sgn, &item);

    for( int i = 0; i < len; ++i )
    {
      const char *key  = vec[i].pv_key;
      const char *val  = vec[i].pv_val;
      const char *type = vec[i].pv_type;
      encode_triplet(&item, &key, &val, &type);
    }

    dbus_message_iter_close_container(&iter, &item);
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  database_free_values(vec);

  log_info("%s -> reply: %d values\n", __FUNCTION__, len);
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_set_value  --  handle PROFILED_SET_VALUE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_set_value(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *prof = 0;
  char         *key  = 0;
  char         *val  = 0;
  dbus_bool_t   res  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &prof,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_STRING, &val,
                            DBUS_TYPE_INVALID) )
  {
    res = (database_set_value(prof, key, val) == 0 );
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_BOOLEAN, &res, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: %s\n", __FUNCTION__, res ? "True" : "False");
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_has_value  --  handle PROFILED_HAS_VALUE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_has_value(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *key  = 0;
  dbus_bool_t   res  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_INVALID) )
  {
    res = (database_has_value(key) != 0);
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_BOOLEAN, &res, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: %s\n", __FUNCTION__, res ? "True" : "False");
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_is_writable  --  handle PROFILED_IS_WRITABLE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_is_writable(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *key  = 0;
  dbus_bool_t   res  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_INVALID) )
  {
    res = (database_is_writable(key) != 0);
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_BOOLEAN, &res, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: %s\n", __FUNCTION__, res ? "True" : "False");
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_get_value  --  handle PROFILED_GET_VALUE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_value(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *prof = 0;
  char         *key  = 0;
  const char   *val  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &prof,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_INVALID) )
  {
    val = database_get_value(prof, key, "");
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_STRING, &val, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: '%s'\n", __FUNCTION__, val);
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_get_type  --  handle PROFILED_GET_TYPE method call
 * ------------------------------------------------------------------------- */

static
DBusMessage *
server_get_type(DBusMessage *msg)
{

  DBusMessage  *rsp  = 0;
  char         *key  = 0;
  const char   *val  = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( dbus_message_get_args(msg, &err,
                            DBUS_TYPE_STRING, &key,
                            DBUS_TYPE_INVALID) )
  {
    val = database_get_type(key, "");
  }
  else
  {
    log_err("%s: %s: %s\n",
           dbus_message_get_member(msg),
           err.name, err.message);
  }

  rsp = server_make_reply(msg, DBUS_TYPE_STRING, &val, DBUS_TYPE_INVALID);

  dbus_error_free(&err);

  log_info("%s -> reply: '%s'\n", __FUNCTION__, val);
  return rsp;
}

/* ------------------------------------------------------------------------- *
 * server_filter  -- handle requests coming via dbus
 * ------------------------------------------------------------------------- */

static
DBusHandlerResult
server_filter(DBusConnection *conn,
              DBusMessage *msg,
              void *user_data)
{
  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);
  int                 type      = dbus_message_get_type(msg);
  DBusMessage        *rsp       = 0;

// QUARANTINE   log_debug("@%s(%s, %s, %s)", __FUNCTION__, object, interface, member);

  if( !interface || !member || !object )
  {
    goto cleanup;
  }

  if( dbus_message_is_signal(msg, DBUS_INTERFACE_LOCAL, "Disconnected") )
  {
    /* Make a orderly shutdown if we get disconnected from
     * session bus, so that we do not leave behind defunct
     * profiledaemon process that might overwrite data saved
     * by another profile daemon connected to functioning
     * session bus. */
    log_warning("disconnected from session bus - terminating\n");
    mainloop_stop();
    goto cleanup;
  }

  if (!strcmp(interface, "org.freedesktop.DBus.Introspectable") 
      && !strcmp(member, "Introspect")
      && type == DBUS_MESSAGE_TYPE_METHOD_CALL) {
    result = DBUS_HANDLER_RESULT_HANDLED;
    log_info("introspect");
    rsp = introspect(msg);
    if(rsp == 0)
      rsp = dbus_message_new_error(msg, DBUS_ERROR_FAILED, member);
    if(rsp != 0)
      dbus_connection_send(conn, rsp, 0);
    goto cleanup;
  }
    

  if( !strcmp(interface, PROFILED_INTERFACE) && !strcmp(object, PROFILED_PATH) )
  {
    /* - - - - - - - - - - - - - - - - - - - *
     * as far as dbus message filtering is
     * considered handling of PROFILED_INTERFACE
     * should happen right here
     * - - - - - - - - - - - - - - - - - - - */

    result = DBUS_HANDLER_RESULT_HANDLED;

    /* - - - - - - - - - - - - - - - - - - - *
     * handle method_call messages
     * - - - - - - - - - - - - - - - - - - - */

    if( type == DBUS_MESSAGE_TYPE_METHOD_CALL )
    {
      static const struct
      {
        const char *member;
        DBusMessage *(*func)(DBusMessage *);
      } lut[] =
      {
        {PROFILED_GET_PROFILES, server_get_profiles},

        {PROFILED_GET_PROFILE,  server_get_profile},
        {PROFILED_SET_PROFILE,  server_set_profile},
        {PROFILED_HAS_PROFILE,  server_has_profile},

        {PROFILED_GET_KEYS,     server_get_keys},
        {PROFILED_GET_VALUES,   server_get_values},

        {PROFILED_GET_TYPE,     server_get_type},

        {PROFILED_GET_VALUE,    server_get_value},
        {PROFILED_SET_VALUE,    server_set_value},
        {PROFILED_HAS_VALUE,    server_has_value},
        {PROFILED_IS_WRITABLE,  server_is_writable},

        {0,0}
      };

      for( int i = 0; ; ++i )
      {
        if( lut[i].member == 0 )
        {
          log_err("unknown method call: %s\n", member);
          rsp = dbus_message_new_error(msg, DBUS_ERROR_UNKNOWN_METHOD, member);
          break;
        }

        if( !strcmp(lut[i].member, member) )
        {
          log_info("handling method call: %s\n", member);
          rsp = lut[i].func(msg);
          break;
        }
      }
    }

    /* - - - - - - - - - - - - - - - - - - - *
     * if no responce message was created
     * above and the peer expects reply,
     * create a generic error message
     * - - - - - - - - - - - - - - - - - - - */

    if( rsp == 0 && !dbus_message_get_no_reply(msg) )
    {
      rsp = dbus_message_new_error(msg, DBUS_ERROR_FAILED, member);
    }

    /* - - - - - - - - - - - - - - - - - - - *
     * send responce if we have something
     * to send
     * - - - - - - - - - - - - - - - - - - - */

    if( rsp != 0 )
    {
      dbus_connection_send(conn, rsp, 0);
      //dbus_connection_flush(conn);
    }
  }

  cleanup:

  if( rsp != 0 )
  {
    dbus_message_unref(rsp);
  }

  return result;
}

static
int
server_append_values_to_iter(DBusMessageIter *iter,
                             profileval_t *vec, int cnt)
{
  int res = -1;
  DBusMessageIter item, memb;

  static const char sgn[] =
  DBUS_STRUCT_BEGIN_CHAR_AS_STRING
  DBUS_TYPE_STRING_AS_STRING
  DBUS_TYPE_STRING_AS_STRING
  DBUS_TYPE_STRING_AS_STRING
  DBUS_STRUCT_END_CHAR_AS_STRING;

  // TODO: need to check success / failure
  dbus_message_iter_open_container(iter, DBUS_TYPE_ARRAY, sgn, &item);

  for( int i = 0; i < cnt; ++i )
  {
    const char *key  = vec[i].pv_key;
    const char *val  = vec[i].pv_val;
    const char *type = vec[i].pv_type;

    dbus_message_iter_open_container(&item, DBUS_TYPE_STRUCT, 0, &memb);
    dbus_message_iter_append_basic(&memb, DBUS_TYPE_STRING, &key);
    dbus_message_iter_append_basic(&memb, DBUS_TYPE_STRING, &val);
    dbus_message_iter_append_basic(&memb, DBUS_TYPE_STRING, &type);
    dbus_message_iter_close_container(&item, &memb);
  }

  dbus_message_iter_close_container(iter, &item);

  res = 0;
  //cleanup:

  return res;
}

/* ------------------------------------------------------------------------- *
 * server_change_broadcast  --  send profile change signals to dbus
 * ------------------------------------------------------------------------- */

static
int
server_change_broadcast(int changed, int active, const char *profile)
{
// QUARANTINE   debugf("@ %s\n", __FUNCTION__);

  int           cnt = 0;
  profileval_t *set = database_get_changed_values(profile, &cnt);
  DBusMessage  *msg = 0;
  DBusMessageIter iter;

  log_notice("%s: changed=%d, active=%d, profile=%s, values=%d\n",
             __FUNCTION__, changed, active, profile, cnt);

  msg = dbus_message_new_signal(PROFILED_PATH,
                                PROFILED_INTERFACE,
                                PROFILED_CHANGED);

  dbus_bool_t cflag = (changed != 0);
  dbus_bool_t aflag = (active  != 0);

  dbus_message_append_args(msg,
                           DBUS_TYPE_BOOLEAN, &cflag,
                           DBUS_TYPE_BOOLEAN, &aflag,
                           DBUS_TYPE_STRING,  &profile,
                           DBUS_TYPE_INVALID);

  dbus_message_iter_init_append(msg, &iter);
  server_append_values_to_iter(&iter, set, cnt);
  database_free_changed_values(set);

  dbus_connection_send(server_bus, msg, 0);
  dbus_connection_flush(server_bus);

  dbus_message_unref(msg);
  return 0;
}

/* ------------------------------------------------------------------------- *
 * server_changes_max_id  --  timeout id for unhandled profile data changes
 * ------------------------------------------------------------------------- */

static guint server_changes_min_id = 0;
static guint server_changes_max_id = 0;

static int server_changes_broadcast_cancel(void)
{
  int cancelled = 0;

  if( server_changes_min_id != 0 )
  {
    g_source_remove(server_changes_min_id);
    server_changes_min_id = 0;
    cancelled = 1;
  }

  if( server_changes_max_id != 0 )
  {
    g_source_remove(server_changes_max_id);
    server_changes_max_id = 0;
    cancelled = 1;
  }
  return cancelled;
}

/* ------------------------------------------------------------------------- *
 * server_changes_broadcast_cb  --  broadcasts changes signals after timeout
 * ------------------------------------------------------------------------- */

static
gboolean
server_changes_broadcast_cb(gpointer data)
{
// QUARANTINE   debugf("@ %s\n", __FUNCTION__);

  const char *current  = database_get_profile();
  const char *previous = database_get_previous();
  int         changed  = strcmp(current, previous);

// QUARANTINE   debugf("prev=%s, curr=%s, diff=%d\n",
// QUARANTINE          previous,current,changed);

  int    count    = 0;
  char **profiles = database_get_changed_profiles(&count);

// QUARANTINE   debugf("\tp=%p, n=%d\n", profiles, count);

  for( int i = 0; profiles[i]; ++i )
  {
    const char *profile = profiles[i];
    int active = !strcmp(profile, current);

    if( changed && active )
    {
      continue;
    }
// QUARANTINE     debugf("\tPROF %s\n", profile);

    server_change_broadcast(0, active, profiles[i]);
  }

  if( changed )
  {
    server_change_broadcast(1, 1, current);
  }

  database_free_changed_profiles(profiles);

  database_clear_changes();

  server_changes_broadcast_cancel();
  return FALSE;
}

static void server_changes_broadcast_request(void)
{
  if( BROADCAST_DELAY_MAX > 0 )
  {
    if( server_changes_max_id == 0 )
    {
      server_changes_max_id = g_timeout_add(BROADCAST_DELAY_MAX,
                                            server_changes_broadcast_cb, 0);
    }
  }

  if( server_changes_min_id != 0 )
  {
    g_source_remove(server_changes_min_id);
    server_changes_min_id = 0;
  }

  if( BROADCAST_DELAY_MIN > 0 )
  {
    server_changes_min_id = g_timeout_add(BROADCAST_DELAY_MIN,
                                          server_changes_broadcast_cb, 0);
  }
}

/* ------------------------------------------------------------------------- *
 * server_changes_handler_cb  --  get called when profile data changes
 * ------------------------------------------------------------------------- */

static
void
server_changes_handler_cb(void)
{
  // QUARANTINE   debugf("@ %s\n", __FUNCTION__);
  server_changes_broadcast_request();

}

/* ------------------------------------------------------------------------- *
 * server_changes_save  --  skip change broadcast, only save changes
 * ------------------------------------------------------------------------- */

static
void
server_changes_save(void)
{
  if( server_changes_broadcast_cancel() )
  {
    database_clear_changes();
  }
}

/* ------------------------------------------------------------------------- *
 * If database engine notices that somebody (i.e. osso-backup doing restore)
 * has touched the data files it disables data saving and makes a
 * restart request.
 * ------------------------------------------------------------------------- */

static guint server_restart_id = 0;

static
gboolean
server_restart_idle_cb(gpointer data)
{
// QUARANTINE   log_debug("@%s", __FUNCTION__);
  server_restart_id = 0;

  // terminate - we will be restarted by dbus when our interface
  //             is used by somebody
  mainloop_stop();
  return FALSE;
}

static
gboolean
server_restart_delay_cb(gpointer data)
{
// QUARANTINE   log_debug("@%s", __FUNCTION__);

  // wait for idle
  server_restart_id = g_idle_add(server_restart_idle_cb, 0);
  return FALSE;
}

static
void
server_restart_cb(void)
{
// QUARANTINE   log_debug("@%s", __FUNCTION__);
  if( server_restart_id == 0 )
  {
    // wait couple of secs
    server_restart_id = g_timeout_add(5 * 1000, server_restart_delay_cb, 0);
  }
}

/* ------------------------------------------------------------------------- *
 * server_init
 * ------------------------------------------------------------------------- */

int
server_init(void)
{
// QUARANTINE   log_debug("@%s()", __FUNCTION__);
  int         res = -1;
  DBusError   err = DBUS_ERROR_INIT;

  /* - - - - - - - - - - - - - - - - - - - *
   * set data changed notification callback
   * - - - - - - - - - - - - - - - - - - - */

  database_set_changed_cb(server_changes_handler_cb);

  /* - - - - - - - - - - - - - - - - - - - *
   * set callback that will be called if
   * database notices that the data files
   * have been modified by somebody else
   * (like osso-backup during restore)
   * - - - - - - - - - - - - - - - - - - - */

  database_set_restart_request_cb(server_restart_cb);

  /* - - - - - - - - - - - - - - - - - - - *
   * connect to dbus
   * - - - - - - - - - - - - - - - - - - - */

#ifdef USE_SYSTEM_BUS
  if( (server_bus = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_get", err.message);
    goto cleanup;
  }
#else
  if( (server_bus = dbus_bus_get(DBUS_BUS_SESSION, &err)) == 0 )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_get", err.message);
    goto cleanup;
  }
#endif

  /* - - - - - - - - - - - - - - - - - - - *
   * register message filter
   * - - - - - - - - - - - - - - - - - - - */

  if( !dbus_connection_add_filter(server_bus, server_filter, 0, 0) )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_connection_add_filter", "FAILED");
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * request service name
   * - - - - - - - - - - - - - - - - - - - */

  int ret = dbus_bus_request_name(server_bus, PROFILED_SERVICE,
                                  DBUS_NAME_FLAG_DO_NOT_QUEUE,
                                  //DBUS_NAME_FLAG_REPLACE_EXISTING,
                                  &err);

  if( ret != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER )
  {
    if( dbus_error_is_set(&err) )
    {
      log_err("%s: %s: %s\n", "dbus_bus_request_name", err.name, err.message);
    }
    else
    {
      log_err("%s: %s\n", "dbus_bus_request_name", "not primary owner of connection\n");
    }
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * attach connection to mainloop
   * - - - - - - - - - - - - - - - - - - - */

  dbus_connection_setup_with_g_main(server_bus, NULL);
  dbus_connection_set_exit_on_disconnect(server_bus, 0);

  /* - - - - - - - - - - - - - - - - - - - *
   * success
   * - - - - - - - - - - - - - - - - - - - */

  res = 0;

  /* - - - - - - - - - - - - - - - - - - - *
   * cleanup & return
   * - - - - - - - - - - - - - - - - - - - */

  cleanup:

  dbus_error_free(&err);
  return res;
}

/* ------------------------------------------------------------------------- *
 * server_quit
 * ------------------------------------------------------------------------- */

void
server_quit(void)
{
// QUARANTINE   log_debug("@%s()", __FUNCTION__);

  // detach from database notifications
  database_set_changed_cb(0);
  database_set_restart_request_cb(0);

  // save data if we have unhandled changes
  server_changes_save();

  // detach from dbus
  if( server_bus != 0 )
  {
    dbus_connection_remove_filter(server_bus, server_filter, 0);

    dbus_connection_unref(server_bus);
    server_bus = 0;
  }
}
