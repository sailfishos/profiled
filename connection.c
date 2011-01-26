
/*
* This file is part of profile-qt
*
* Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies). All rights reserved.
*
* Contact: Sakari Poussa <sakari.poussa@nokia.com>
*
* Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
*
* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer 
* in the documentation  and/or other materials provided with the distribution.
* Neither the name of Nokia Corporation nor the names of its contributors may be used to endorse or promote products derived 
* from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, 
* BUT NOT LIMITED TO, THE * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
* IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, * INDIRECT, INCIDENTAL, SPECIAL,
* EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; 
* LOSS OF USE, DATA, OR PROFITS; OR * * BUSINESS INTERRUPTION) HOWEVER CAUSED
* AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING 
* IN ANY WAY OUT OF THE USE * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/

#include "profiled_config.h"

#include "libprofile-internal.h"
#include "logging.h"

#include <stdlib.h>

static int             zz_blocked = 0;
static DBusConnection *zz_conn = 0;

/* ========================================================================= *
 * Internal Functions
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * profile_connection_disconnect
 * ------------------------------------------------------------------------- */

static
void
profile_connection_disconnect(void)
{
  if( zz_conn != 0 )
  {
    ENTER
    dbus_connection_unref(zz_conn);
    zz_conn = 0;
    profile_tracker_disconnect();
    LEAVE
  }
}

/* ------------------------------------------------------------------------- *
 * profile_connection_attach
 * ------------------------------------------------------------------------- */

static
void
profile_connection_attach(DBusConnection *conn)
{
  profile_connection_disconnect();

  ENTER
  if( (zz_conn = conn) != 0 )
  {
    dbus_connection_ref(zz_conn);
    profile_tracker_reconnect();
  }
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_connection_connect
 * ------------------------------------------------------------------------- */

static
DBusConnection *
profile_connection_connect(void)
{
  ENTER
  if( zz_conn == 0 )
  {
    DBusError       err = DBUS_ERROR_INIT;
    DBusConnection *con = NULL;

    if( getenv("DBUS_SESSION_BUS_ADDRESS") == 0 )
    {
      log_warning("$DBUS_SESSION_BUS_ADDRESS is not set"
                  " - sessionbus autoconnect blocked\n");
      zz_blocked = 1;
    }
    else if( (con = dbus_bus_get(DBUS_BUS_SESSION, &err)) == 0 )
    {
      log_err("can't connect to session bus: %s: %s\n",
              err.name, err.message);
    }
    else
    {
      dbus_connection_set_exit_on_disconnect(con, 0);
      profile_connection_attach(con);
      dbus_connection_unref(con);
    }

    dbus_error_free(&err);
  }

  LEAVE
  return zz_conn;
}

/* ========================================================================= *
 * API Functions
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * profile_connection_get
 * ------------------------------------------------------------------------- */

DBusConnection *
profile_connection_get(void)
{
  ENTER
  DBusConnection *conn = zz_conn;

  if( conn == 0 )
  {
    if( zz_blocked )
    {
      log_warning("session bus connection requested while blocked\n");
    }
    else
    {
      conn = profile_connection_connect();
    }
  }
  if( conn != 0 )
  {
    dbus_connection_ref(conn);
  }
  LEAVE
  return conn;
}

/* ------------------------------------------------------------------------- *
 * profile_connection_set
 * ------------------------------------------------------------------------- */

void
profile_connection_set(DBusConnection *conn)
{
  ENTER
  profile_connection_disconnect();
  profile_connection_attach(conn);
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_connection_disable_autoconnect
 * ------------------------------------------------------------------------- */

void
profile_connection_disable_autoconnect(void)
{
  ENTER
  zz_blocked = TRUE;
  profile_connection_disconnect();
  LEAVE
}

/* ------------------------------------------------------------------------- *
 * profile_connection_enable_autoconnect
 * ------------------------------------------------------------------------- */

void
profile_connection_enable_autoconnect(void)
{
  ENTER
  zz_blocked = FALSE;
  LEAVE
}
