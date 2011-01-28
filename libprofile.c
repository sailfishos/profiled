
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
#include <stdlib.h>
#include <string.h>
#include <dbus/dbus.h>
#include <unistd.h>

#include "logging.h"

#include "xutil.h"
#include "libprofile-internal.h"
#include "profile_dbus.h"

static inline void client_check_profile(const char **pprofile)
{
  if( ! *pprofile ) *pprofile = "";
}

static const char * const profile_bool_true_values[] =
{
  "On", "True", "Yes", "Y", "T", 0
};

static const char * const profile_bool_false_values[] =
{
  "Off", "False", "No", "N", "F", 0
};

/* ------------------------------------------------------------------------- *
 * client_make_method_message  --  construct dbus method call message
 * ------------------------------------------------------------------------- */

static
DBusMessage *
client_make_method_message(const char *method, int dbus_type, ...)
{
  DBusMessage    *msg  = 0;

  va_list         va;

  va_start(va, dbus_type);

  if( !(msg = dbus_message_new_method_call(PROFILED_SERVICE,
                                           PROFILED_PATH,
                                           PROFILED_INTERFACE,
                                           method)) )
  {
    log_err_F("could not create method call message\n");
    goto cleanup;
  }

  if( !dbus_message_append_args_valist(msg, dbus_type, va) )
  {
    log_err_F("could not add args to method call message\n");
    goto cleanup;
  }

  cleanup:

  va_end(va);

  return msg;
}

/* ------------------------------------------------------------------------- *
 * client_exec_method_call  --  send method call & wait for reply
 * ------------------------------------------------------------------------- */

static
DBusMessage *
client_exec_method_call(DBusMessage *msg)
{
  DBusConnection *conn = 0;
  DBusMessage    *rsp  = 0;
  DBusError       err  = DBUS_ERROR_INIT;

  if( (conn = profile_connection_get()) == 0 )
  {
    goto cleanup;
  }

  if( !(rsp = dbus_connection_send_with_reply_and_block(conn, msg, -1, &err)) )
  {
    log_err_F("%s: %s: %s\n", "dbus_connection_send_with_reply_and_block", err.name, err.message);
    goto cleanup;
  }

  cleanup:

  if( conn != 0 ) dbus_connection_unref(conn);

  dbus_error_free(&err);

  log_debug("%s: %s -> %s\n", "libprofile",
             dbus_message_get_member(msg),
             (rsp == 0 ) ? "NO REPLY" :
             dbus_message_type_to_string(dbus_message_get_type(rsp)));

  return rsp;
}

/* ------------------------------------------------------------------------- *
 * profile_get_profiles  --  handle PROFILED_GET_PROFILES method call
 * ------------------------------------------------------------------------- */

char **
profile_get_profiles(void)
{
  char         **res = 0;
  DBusMessage   *msg = 0;
  DBusMessage   *rsp = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_GET_PROFILES,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      char **v = 0;
      int    n = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &v, &n,
                                DBUS_TYPE_INVALID) )
      {
        res = malloc((n+1)*sizeof *res);
        for( int i = 0; i < n; ++i )
        {
          res[i] = strdup(v[i] ?: "");
        }
        res[n] = 0;

        //res = v;
      }
      dbus_free_string_array(v);
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_free_profiles
 * ------------------------------------------------------------------------- */

void
profile_free_profiles(char **v)
{
  xfreev(v);
  //dbus_free_string_array(v);
}

/* ------------------------------------------------------------------------- *
 * profile_get_keys  --  handle PROFILED_GET_KEYS method call
 * ------------------------------------------------------------------------- */

char **
profile_get_keys(void)
{
  char         **res = 0;
  DBusMessage   *msg = 0;
  DBusMessage   *rsp = 0;
  DBusError     err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_GET_KEYS,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      char **v = 0;
      int    n = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_ARRAY, DBUS_TYPE_STRING, &v, &n,
                                DBUS_TYPE_INVALID) )
      {
        res = v;
      }

// QUARANTINE       printf("@ %s() -> %p %d -> %p\n", __FUNCTION__, v,n,res);

    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_free_keys
 * ------------------------------------------------------------------------- */

void
profile_free_keys(char **v)
{
  dbus_free_string_array(v);
}

/* ------------------------------------------------------------------------- *
 * profile_get_values  --  handle PROFILED_GET_VALUES method call
 * ------------------------------------------------------------------------- */

profileval_t *
profile_get_values(const char *profile)
{
  profileval_t     *res = 0;
  int           cnt = 0;
  int           top = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;

  client_check_profile(&profile);

  if( (msg = client_make_method_message(PROFILED_GET_VALUES,
                                        DBUS_TYPE_STRING, &profile,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      // FIXME: quick and dirty struct array decoding

      DBusMessageIter iter, item, memb;

      dbus_message_iter_init(rsp, &iter);

      if( dbus_message_iter_get_arg_type(&iter) == DBUS_TYPE_ARRAY )
      {
        dbus_message_iter_recurse(&iter, &item);

        top = 16;
        res = malloc(top * sizeof *res);

        while( dbus_message_iter_get_arg_type(&item) == DBUS_TYPE_STRUCT )
        {
          dbus_message_iter_recurse(&item, &memb);

          char *k = 0, *v = 0, *t = 0;
          dbus_message_iter_get_basic(&memb, &k);
          dbus_message_iter_next(&memb);

          dbus_message_iter_get_basic(&memb, &v);
          dbus_message_iter_next(&memb);

          dbus_message_iter_get_basic(&memb, &t);
          dbus_message_iter_next(&memb);

          if( cnt == top )
          {
            res = realloc(res, (top *= 2) * sizeof *res);
          }

          profileval_ctor_ex(&res[cnt++], k, v, t);
          dbus_message_iter_next(&item);
        }
      }
    }
  }

  res = realloc(res, (cnt+1) * sizeof *res);
  profileval_ctor(&res[cnt]);

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_free_values
 * ------------------------------------------------------------------------- */

void
profile_free_values(profileval_t *values)
{
  profileval_free_vector(values);
}

/* ------------------------------------------------------------------------- *
 * profile_has_profile  --  handle PROFILED_HAS_PROFILE method call
 * ------------------------------------------------------------------------- */

int
profile_has_profile(const char *profile)
{
  int           res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  client_check_profile(&profile);

  if( (msg = client_make_method_message(PROFILED_HAS_PROFILE,
                                        DBUS_TYPE_STRING, &profile,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      dbus_bool_t v = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_BOOLEAN, &v,
                                DBUS_TYPE_INVALID) )
      {
        if( v != 0 ) res = 1;
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_get_profile  --  handle PROFILED_GET_PROFILE method call
 * ------------------------------------------------------------------------- */

char *
profile_get_profile(void)
{
  char         *res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_GET_PROFILE,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      char *v = 0;
      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_STRING, &v,
                                DBUS_TYPE_INVALID) )
      {
        res = strdup(v ?: "");
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_set_profile  --  handle PROFILED_SET_PROFILE method call
 * ------------------------------------------------------------------------- */

int
profile_set_profile(const char *profile)
{
  int           res = -1;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  client_check_profile(&profile);

  if( (msg = client_make_method_message(PROFILED_SET_PROFILE,
                                        DBUS_TYPE_STRING, &profile,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      dbus_bool_t v = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_BOOLEAN, &v,
                                DBUS_TYPE_INVALID) )
      {
        if( v != 0 ) res = 0;
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_has_value  --  handle PROFILED_HAS_VALUE method call
 * ------------------------------------------------------------------------- */

int
profile_has_value(const char *key)
{
  int           res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_HAS_VALUE,
                                        DBUS_TYPE_STRING, &key,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      dbus_bool_t v = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_BOOLEAN, &v,
                                DBUS_TYPE_INVALID) )
      {
        if( v != 0 ) res = 1;
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_is_writable  --  handle PROFILED_IS_WRITABLE method call
 * ------------------------------------------------------------------------- */

int
profile_is_writable(const char *key)
{
  int           res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_IS_WRITABLE,
                                        DBUS_TYPE_STRING, &key,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      dbus_bool_t v = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_BOOLEAN, &v,
                                DBUS_TYPE_INVALID) )
      {
        if( v != 0 ) res = 1;
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_get_value  --  handle PROFILED_GET_VALUE method call
 * ------------------------------------------------------------------------- */

char *
profile_get_value(const char *profile, const char *key)
{
  char         *res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  client_check_profile(&profile);

  if( (msg = client_make_method_message(PROFILED_GET_VALUE,
                                        DBUS_TYPE_STRING, &profile,
                                        DBUS_TYPE_STRING, &key,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      char *v = 0;
      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_STRING, &v,
                                DBUS_TYPE_INVALID) )
      {
        res = strdup(v ?: "");
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  log_debug_F("%s(%s) = %s\n", key, profile, res);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_set_value  --  handle PROFILED_SET_VALUE method call
 * ------------------------------------------------------------------------- */

int
profile_set_value(const char *profile, const char *key, const char *val)
{
  int           res = -1;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  client_check_profile(&profile);

  if( (msg = client_make_method_message(PROFILED_SET_VALUE,
                                        DBUS_TYPE_STRING, &profile,
                                        DBUS_TYPE_STRING, &key,
                                        DBUS_TYPE_STRING, &val,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      dbus_bool_t v = 0;

      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_BOOLEAN, &v,
                                DBUS_TYPE_INVALID) )
      {
        if( v != 0 ) res = 0;
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_get_type  --  handle PROFILED_GET_TYPE method call
 * ------------------------------------------------------------------------- */

char *
profile_get_type(const char *key)
{
  char         *res = 0;
  DBusMessage  *msg = 0;
  DBusMessage  *rsp = 0;
  DBusError    err  = DBUS_ERROR_INIT;

  if( (msg = client_make_method_message(PROFILED_GET_TYPE,
                                        DBUS_TYPE_STRING, &key,
                                        DBUS_TYPE_INVALID)) )
  {
    if( (rsp = client_exec_method_call(msg)) )
    {
      char *v = 0;
      if( dbus_message_get_args(rsp, &err,
                                DBUS_TYPE_STRING, &v,
                                DBUS_TYPE_INVALID) )
      {
        res = strdup(v ?: "");
      }
    }
  }

  if( rsp != 0 ) dbus_message_unref(rsp);
  if( msg != 0 ) dbus_message_unref(msg);

  dbus_error_free(&err);

  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_parse_bool
 * ------------------------------------------------------------------------- */

int
profile_parse_bool(const char *text)
{
  if( text != 0 )
  {
    for( size_t i = 0; profile_bool_true_values[i]; ++i )
    {
      if( !strcasecmp(profile_bool_true_values[i], text) )
      {
        return 1;
      }
    }

    for( size_t i = 0; profile_bool_false_values[i]; ++i )
    {
      if( !strcasecmp(profile_bool_false_values[i], text) )
      {
        return 0;
      }
    }
    return strtol(text, 0, 0) != 0;
  }

  return 0;
}

/* ------------------------------------------------------------------------- *
 * profile_parse_int
 * ------------------------------------------------------------------------- */

int
profile_parse_int(const char *text)
{
  return text ? strtol(text, 0, 0) : 0;
}

/* ------------------------------------------------------------------------- *
 * profile_parse_double
 * ------------------------------------------------------------------------- */

double
profile_parse_double(const char *text)
{
  return text ? strtod(text, 0) : 0;
}

/* ------------------------------------------------------------------------- *
 * profile_get_value_as_bool
 * ------------------------------------------------------------------------- */

int
profile_get_value_as_bool(const char *profile, const char *key)
{
  char *val = profile_get_value(profile, key);
  int   res = profile_parse_bool(val);
  free(val);
  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_set_value_as_bool
 * ------------------------------------------------------------------------- */

int
profile_set_value_as_bool(const char *profile, const char *key, int val)
{
  return profile_set_value(profile, key, val ?
                           *profile_bool_true_values :
                           *profile_bool_false_values);

}

/* ------------------------------------------------------------------------- *
 * profile_get_value_as_int  --
 * ------------------------------------------------------------------------- */

int
profile_get_value_as_int(const char *profile, const char *key)
{
  char *val = profile_get_value(profile, key);
  int   res = profile_parse_int(val);
  free(val);
  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_set_value_as_int  --
 * ------------------------------------------------------------------------- */

int
profile_set_value_as_int(const char *profile, const char *key, int val)
{
  char tmp[32];
  snprintf(tmp, sizeof tmp, "%d", val);
  return profile_set_value(profile, key, tmp);
}

/* ------------------------------------------------------------------------- *
 * profile_get_value_as_double  --
 * ------------------------------------------------------------------------- */

int
profile_get_value_as_double(const char *profile, const char *key)
{
  char  *val = profile_get_value(profile, key);
  double res = profile_parse_double(val);
  free(val);
  return res;
}

/* ------------------------------------------------------------------------- *
 * profile_set_value_as_double  --
 * ------------------------------------------------------------------------- */

int
profile_set_value_as_double(const char *profile, const char *key, double val)
{
  char tmp[32];
  snprintf(tmp, sizeof tmp, "%.16g", val);
  return profile_set_value(profile, key, tmp);
}
