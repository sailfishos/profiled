
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

#include "codec.h"

/* ========================================================================= *
 * Utilites for encoding/decoding dbus messages using message
 * iterators. Useful only because libdbus does not expose some
 * internal functions via the public API.
 * ========================================================================= */

int
encode_bool(DBusMessageIter *iter, const int *pval)
{
  int          err = -1;
  dbus_bool_t  val = (*pval != 0);

  if( dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &val) )
  {
    err = 0;
  }
  return err;
}

int
decode_bool(DBusMessageIter *iter, int *pval)
{
  int          err = -1;
  dbus_bool_t  val = 0;

  if( dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_BOOLEAN )
  {
    dbus_message_iter_get_basic(iter, &val);
    dbus_message_iter_next(iter);
    err = 0;
  }
  *pval = (val != 0);
  return err;
}

int
encode_int(DBusMessageIter *iter, const int *pval)
{
  int          err = -1;
  dbus_int32_t val = *pval;

  if( dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &val) )
  {
    err = 0;
  }
  return err;
}

int
decode_int(DBusMessageIter *iter, int *pval)
{
  int          err = -1;
  dbus_int32_t val = 0;
  if( dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_INT32 )
  {
    dbus_message_iter_get_basic(iter, &val);
    dbus_message_iter_next(iter);
    err = 0;
  }
  *pval = val;
  return err;
}

int
encode_string(DBusMessageIter *iter, const char **pval)
{
  int   err = -1;
  const char *val = *pval ?: "";
  if( dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &val) )
  {
    err = 0;
  }
  return err;
}

int
decode_string(DBusMessageIter *iter, const char **pval)
{
  int err = -1;
  if( dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRING )
  {
    dbus_message_iter_get_basic(iter, pval);
    dbus_message_iter_next(iter);
    err = 0;
  }
  return err;
}

int
encode_triplet(DBusMessageIter *iter, const char **pkey, const char **pval, const char **ptype)
{
  int err = -1;

  DBusMessageIter memb;

  if( dbus_message_iter_open_container(iter, DBUS_TYPE_STRUCT, 0, &memb) )
  {
    if( !encode_string(&memb, pkey) &&
        !encode_string(&memb, pval) &&
        !encode_string(&memb, ptype) )
    {
      err = 0;
    }

    if( !dbus_message_iter_close_container(iter, &memb) )
    {
      err = -1;
    }
  }

  return err;
}

int
decode_triplet(DBusMessageIter *iter, const char **pkey, const char **pval, const char **ptype)
{
  int err = -1;

  DBusMessageIter memb;

  if( dbus_message_iter_get_arg_type(iter) == DBUS_TYPE_STRUCT )
  {
    dbus_message_iter_recurse(iter, &memb);

    if( !decode_string(&memb, pkey) &&
        !decode_string(&memb, pval) &&
        !decode_string(&memb, ptype) )
    {
      dbus_message_iter_next(iter);
      err = 0;
    }
  }
  return err;
}
