
/******************************************************************************
** This file is part of profile-qt
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
** Copyright (C) 2020 Jolla Ltd.
** All rights reserved.
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

#include <sys/time.h>

#include <stdio.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>

#include "dbus-gmain/dbus-gmain.h"

#include "profile_dbus.h"

static void longpausebar(void)
{
  static struct timeval       bar = {0,0};
  static struct timeval const tmo = {0,500*1000};
  auto   struct timeval       now;

  gettimeofday(&now, 0);

  if( timercmp(&bar,&now,<) )
  {
    printf("--------------------------------"
           "--------------------------------\n");
  }
  timeradd(&now,&tmo,&bar);
}

/* ========================================================================= *
 * PROFILE DBUS FUNCTIONS
 * ========================================================================= */

static DBusConnection *tracker_bus    = NULL;

/* ------------------------------------------------------------------------- *
 * tracker_filter  -- handle requests coming via dbus
 * ------------------------------------------------------------------------- */

static
DBusHandlerResult
tracker_filter(DBusConnection *conn,
               DBusMessage *msg,
               void *user_data)
{
  (void)conn; (void)user_data;

  DBusHandlerResult   result    = DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
  const char         *interface = dbus_message_get_interface(msg);
  const char         *member    = dbus_message_get_member(msg);
  const char         *object    = dbus_message_get_path(msg);

  if( !interface || !member || !object )
  {
    goto cleanup;
  }

  if( !strcmp(interface, PROFILED_SERVICE) && !strcmp(object, PROFILED_PATH) )
  {
    result = DBUS_HANDLER_RESULT_HANDLED;

    longpausebar();

// QUARANTINE     extern void dbusif_show_message(DBusMessage *);
// QUARANTINE     dbusif_show_message(msg);
  }

  cleanup:

  return result;
}

/* ------------------------------------------------------------------------- *
 * tracker_init
 * ------------------------------------------------------------------------- */

static
int
tracker_init(void)
{
  int         res = -1;
  DBusError   err;

  dbus_error_init(&err);

  /* - - - - - - - - - - - - - - - - - - - *
   * connect to system bus
   * - - - - - - - - - - - - - - - - - - - */

#ifdef USE_SYSTEM_BUS
  if( (tracker_bus = dbus_bus_get(DBUS_BUS_SYSTEM, &err)) == 0 )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_get", err.message);
    goto cleanup;
  }
#else
  if( (tracker_bus = dbus_bus_get(DBUS_BUS_SESSION, &err)) == 0 )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_get", err.message);
    goto cleanup;
  }
#endif

  dbus_gmain_set_up_connection(tracker_bus, NULL);
  dbus_connection_set_exit_on_disconnect(tracker_bus, 0);

  /* - - - - - - - - - - - - - - - - - - - *
   * register message filter
   * - - - - - - - - - - - - - - - - - - - */

  if( !dbus_connection_add_filter(tracker_bus, tracker_filter, 0, 0) )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_connection_add_filter", "FAILED");
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * listen to signals
   * - - - - - - - - - - - - - - - - - - - */

  dbus_bus_add_match(tracker_bus,
                     "type='signal', interface='"PROFILED_INTERFACE"'",
                     &err);

  if( dbus_error_is_set(&err) )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_add_match", err.message);
    goto cleanup;
  }

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
 * tracker_quit
 * ------------------------------------------------------------------------- */

static
void
tracker_quit(void)
{
  if( tracker_bus != 0 )
  {
    dbus_connection_unref(tracker_bus);
    tracker_bus = 0;
  }
}

static GMainLoop  *tracker_mainloop = 0;

int main(int ac, char **av)
{
  (void)ac; (void)av;

  int exit_code = EXIT_FAILURE;

  tracker_mainloop = g_main_loop_new(NULL, FALSE);

  tracker_init();

  g_main_loop_run(tracker_mainloop);

  exit_code = EXIT_SUCCESS;

  tracker_quit();

  if( tracker_mainloop != 0 )
  {
    g_main_loop_unref(tracker_mainloop);
    tracker_mainloop = 0;
  }

  return exit_code;
}
