
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
#include "../dbus-gmain/dbus-gmain.h"
#include <profiled/libprofile.h>

static char           *ex_busaddr    = NULL;
static DBusConnection *ex_connection = NULL;
static GMainLoop      *ex_mainloop   = NULL;

#if 0
# define ENTER printf("ENTER %s\n",__FUNCTION__);
# define LEAVE printf("LEAVE %s\n",__FUNCTION__);
#else
# define ENTER do{}while(0);
# define LEAVE do{}while(0);
#endif

static
int
ex_init(void)
{
  ENTER
  int         res = -1;
  DBusError   err;

  dbus_error_init(&err);

  if( (ex_connection = dbus_bus_get(DBUS_BUS_SESSION, &err)) == 0 )
  {
    //CLEANUP fatalf("%s: %s\n", "dbus_bus_get", err.message);
    goto cleanup;
  }

  dbus_gmain_set_up_connection(ex_connection, NULL);
  dbus_connection_set_exit_on_disconnect(ex_connection, 0);

  res = 0;

  /* - - - - - - - - - - - - - - - - - - - *
   * cleanup & return
   * - - - - - - - - - - - - - - - - - - - */

  cleanup:

  dbus_error_free(&err);
  LEAVE
  return res;
}

static
void
ex_quit(void)
{
  ENTER
  if( ex_connection != 0 )
  {
    dbus_connection_unref(ex_connection);
    ex_connection = 0;
  }
  LEAVE
}

static
void
track_profile(const char *profile)
{
  ENTER
  printf("PROFILE -> '%s'\n", profile);
  LEAVE
}

static
void
track_active(const char *profile, const char *key, const char *val, const char *type)
{
  ENTER
  printf("ACTIVE: %s = %s (%s)\n", key, val, type);
  LEAVE
}

static
void
track_change(const char *profile, const char *key, const char *val, const char *type)
{
  ENTER
  printf("%s: %s = %s (%s)\n", profile, key, val, type);
  LEAVE
}

#define PLAN_A 1

static void setup_tracking(void)
{
  ENTER
  profile_connection_disable_autoconnect();
  profile_track_set_profile_cb(track_profile);
  profile_track_set_active_cb(track_active);
  profile_track_set_change_cb(track_change);

#if !PLAN_A
  profile_tracker_init();
#endif
  LEAVE
}
#define numof(a) (sizeof(a)/sizeof*(a))
static
gboolean
ex_toggle(gpointer data)
{
  static const char * const v[] =
  {
    "foo.mp3",
    "bar.mp3",
    "baf.mp3",
    "tic.mp3",
    "tac.mp3",
    "toe.mp3",
  };
  static const char * const p[] =
  {
    "general","silent"
  };

  static unsigned n = 0;

  ENTER
  unsigned i = n++;

  //profile_set_value(0, "ringing.alert.tone", v[i%numof(v)]);

  profile_set_value(p[i%numof(p)], "ringing.alert.tone", v[i%numof(v)]);

  LEAVE
  return TRUE;
}

static
gboolean
ex_wait(gpointer data)
{
  ENTER
  // inform libprofile about the connection
  profile_connection_set(ex_connection);
#if PLAN_A
  profile_tracker_init();
#endif
  g_timeout_add(2000, ex_toggle, 0);
  LEAVE
  return FALSE;
}

static
gboolean
ex_idle(gpointer data)
{
  ENTER

  // make session bus accessible
  setenv("DBUS_SESSION_BUS_ADDRESS", ex_busaddr, 1);

  // initialize connection
  ex_init();

  // hide session bus address again
  unsetenv("DBUS_SESSION_BUS_ADDRESS");

  g_timeout_add(2000, ex_wait, 0);
  LEAVE
  return FALSE;
}

int main(int ac, char **av)
{
  ENTER

  // get dbus address
  ex_busaddr = strdup(getenv("DBUS_SESSION_BUS_ADDRESS") ?: "");

  // hide it for now
  unsetenv("DBUS_SESSION_BUS_ADDRESS");

  int exit_code = EXIT_FAILURE;

  ex_mainloop = g_main_loop_new(NULL, FALSE);

  // set up profile tracking
  setup_tracking();

  // delayed session bus connection
  //g_idle_add(ex_idle, 0);
  g_timeout_add(2000, ex_idle, 0);

  // enter mainloop
  printf("mainloop...\n");
  g_main_loop_run(ex_mainloop);

  exit_code = EXIT_SUCCESS;

  ex_quit();

  if( ex_mainloop != 0 )
  {
    g_main_loop_unref(ex_mainloop);
    ex_mainloop = 0;
  }

  LEAVE
  return exit_code;
}
