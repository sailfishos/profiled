
/******************************************************************************
** This file is part of profile-qt
**
** Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include <profiled/libprofile.h>

#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <sys/time.h>
#include <time.h>

static
void
track_profile(const char *profile)
{
  printf("CB ACTIVATED '%s'\n", profile);
  fflush(0);
}

static
void
track_active(const char *profile, const char *key, const char *val, const char *type)
{
  printf("CB ACTIVE: %s = %s (%s)\n", key, val, type);
  fflush(0);
}

static
void
track_change(const char *profile, const char *key, const char *val, const char *type)
{
  printf("CB '%s': %s = %s (%s)\n", profile, key, val, type);
  fflush(0);
}

static DBusConnection *connection = 0;

static void tracker_init(void)
{
  DBusError err = DBUS_ERROR_INIT;

  connection = dbus_bus_get(DBUS_BUS_SESSION, &err);
  assert( connection != 0 );

  profile_track_set_profile_cb(track_profile);
  profile_track_set_active_cb(track_active);
  profile_track_set_change_cb(track_change);
  int rc = profile_tracker_init();
  assert( rc == 0 );

  dbus_error_free(&err);
}

#if 01

static void getmonotime(struct timeval *tv, void *aptr)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  TIMESPEC_TO_TIMEVAL(tv, &ts);
}

static void timeout_init(struct timeval *tmo, int msec)
{
  struct timeval now;
  getmonotime(&now,0);
  tmo->tv_sec  = (msec / 1000);
  tmo->tv_usec = (msec % 1000) * 1000;
  timeradd(tmo, &now, tmo);
}

static int timeout_left(const struct timeval *tmo)
{
  int res = 0;
  struct timeval now;
  getmonotime(&now,0);
  if( timercmp(&now, tmo, <) )
  {
    timersub(tmo,&now,&now);
    res = now.tv_sec * 1000 + now.tv_usec / 1000;
  }
  return res;
}

static void mysleep(int msec)
{
  struct timeval tmo;
  struct timeval t1,t2;
  int ms;

  //printf("SLEEP %d\n", msec);
  timeout_init(&tmo, msec);
  getmonotime(&t1,0);

  while( (ms = timeout_left(&tmo)) > 0 )
  {
    printf("dbus wait io %d\n", ms);
    dbus_connection_read_write(connection, ms);
    do
    {
      printf("dbus dispatch\n");
    }
    while( dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS );
  }

  getmonotime(&t2,0);
  timersub(&t2,&t1,&t1);
  printf("SLEPT  %.3f / %.3f s\n", t1.tv_sec + t1.tv_usec * 1e-6, msec * 1e-3);

}
#else
static void mysleep(int msec)
{
  //printf("SLEEP %d ms\n", msec);

  struct timeval t1,t2;

  getmonotime(&t1,0);

  if( dbus_connection_read_write(connection, msec) )
  {
    do
    {
      puts("dbus dispatch");
    }
    while( dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS );
  }

  getmonotime(&t2,0);
  timersub(&t2,&t1,&t1);
  printf("SLEPT  %.3f / %.3f s\n", t1.tv_sec + t1.tv_usec * 1e-6, msec * 1e-3);
}
#endif

#define numof(a) (sizeof(a)/sizeof*(a))

int main(int argc, char **argv)
{
  static const char * const profiles[] =
  {
    "general", "silent",
  };
  static const char * const keys[] =
  {
#if 0
    "ringing.alert.tone",
#else
    "calendar.alarm.enabled",
    "clock.alarm.enabled",
    "email.alert.tone",
    "email.alert.volume",
    "im.alert.tone",
    "im.alert.volume",
    "keypad.sound.level",
    "ringing.alert.tone",
    "ringing.alert.type",
    "ringing.alert.volume",
    "sms.alert.tone",
    "sms.alert.volume",
    "system.sound.level",
    "touchscreen.sound.level",
    "vibrating.alert.enabled",
#endif
  };

  printf("\n================ %s ================\n", "init");
  tracker_init();

  printf("\n================ %s ================\n", "profiles");

  for( int n = 0; n < 2; ++n )
  {
    for( size_t i = 0; i < numof(profiles); ++i )
    {
      printf("--\nSET '%s'\n", profiles[i]);
      profile_set_profile(profiles[i]);
      mysleep(1500);
    }
  }

  printf("\n================ %s ================\n", "values");

  for( size_t i = 0; i < 8; ++i )
  {
    const char *p = profiles[i % numof(profiles)];
    const char *k = keys[i % numof(keys)];

    char v[32];
    snprintf(v, sizeof v, "dummy-%Zd.mp3", i);

    printf("--\nSET %s:%s=%s\n", p,k,v);

    profile_set_value(p, k, v);
    mysleep(1500);
  }

  printf("\n================ %s ================\n", "reset");

  for( size_t p = 0; p < numof(profiles); ++p )
  {
    for( size_t k = 0; k < numof(keys); ++k )
    {
      profile_set_value(profiles[p], keys[k], "");
    }
  }

  mysleep(2000);

  printf("\n================ %s ================\n", *profiles);
  profile_set_profile(*profiles);
  mysleep(1500);

  return 0;
}
