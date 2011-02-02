
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
#include <unistd.h>
#include <glib.h>
#include <time.h>
#include <sys/time.h>

#include "libprofile-internal.h"
#include "xutil.h"

static char *slice(char **p, int c)
{
  char *b = *p;
  char *e = strchr(b, c);

  if( e != 0 )
  {
    if( *e ) *e++ = 0;
  }
  else
  {
    e = strchr(b, 0);
  }
  *p = e;
  return b;
}

static void parse(char *s, char **p, char **k, char **v)
{
  *p = 0;
  if( strchr(s, ':') )
  {
    *p = slice(&s, ':');
  }
  *k = slice(&s, '=');
  *v = slice(&s, 0);
}

static void getmonotime(struct timeval *tv)
{
  struct timespec ts;
  clock_gettime(CLOCK_MONOTONIC, &ts);
  TIMESPEC_TO_TIMEVAL(tv, &ts);
}

static void message(const char *profile, const char *fmt, ...)
__attribute__((format(printf,2,3)));

static void message(const char *profile, const char *fmt, ...)
{
  static char *previous = 0;
  static struct timeval zen;
  static const struct timeval tmo = { 0, 100 * 1000 };
  struct timeval now, dif;
  int gap = 0;

  getmonotime(&now);
  timersub(&now, &zen, &dif);
  if( timercmp(&tmo, &dif, <) )
  {
    zen = now;
    gap = 1;
  }

  if( !xstrsame(previous, profile) )
  {
    xstrset(&previous, profile);
    gap = 1;
  }
  if( gap ) printf("\n");

  va_list va;
  va_start(va, fmt);
  vprintf(fmt, va);
  va_end(va);
}

static
void
track_profile(const char *profile)
{
  message(profile, "PROFILE -> '%s'\n", profile);
}

static
void
track_active(const char *profile, const char *key, const char *val, const char *type)
{
  message(profile, "%s*: %s = %s (%s)\n", profile, key, val, type);
}

static
void
track_change(const char *profile, const char *key, const char *val, const char *type)
{
  message(profile, "%s: %s = %s (%s)\n", profile, key, val, type);
}

static int track(void)
{
  int        exit_code = EXIT_FAILURE;
  GMainLoop *mainloop  = g_main_loop_new(NULL, FALSE);

  profile_track_set_profile_cb(track_profile);
  profile_track_set_active_cb(track_active);
  profile_track_set_change_cb(track_change);

  profile_tracker_init();

  g_main_loop_run(mainloop);

  exit_code = EXIT_SUCCESS;

  profile_tracker_quit();

  if( mainloop != 0 )
  {
    g_main_loop_unref(mainloop);
    mainloop = 0;
  }

  return exit_code;
}

static const char usage[] =
"NAME\n"
"  profileclient  --  command line utility for profile daemon access\n"
"\n"
"SYNOPSIS\n"
"  profileclient <options>\n"
"\n"
"DESCRIPTION\n"
"    This tools can be used to set and query profile data from profile\n"
"    daemon over dbus interface.\n"
"    interface.\n"
"\n"
"OPTIONS\n"
"  -h\n"
"       This help text\n"
"  -p <profile>\n"
"       Set currently active profile.\n"
"  -l\n"
"       List available profiles, active marked with asterisk (*).\n"
"  -k\n"
"       List available profile keys.\n"
"  -v\n"
"       List values for current profile.\n"
"  -V <profile>\n"
"       List values for named profile.\n"
"  -T\n"
"       Start tracking profile changes.\n"
"  -s [profile:]<key>=<value>\n"
"        Set profile value in named/current profile.\n"
"  -g [profile:]<key>\n"
"        Get profile value in named/current profile.\n"
"  -G [profile:]<key>\n"
"        Get profile value in named/current profile.\n"
"        Print results also as parsed bool/int/double.\n"
"  -d [profile:]<key>\n"
"        Reset profile value in named/current profile.\n"
"  -t <key>\n"
"       Get type of given key. \n"
"  -r\n"
"       Reset current profile to default values\n"
"  -R <profile>\n"
"       Reset named profile to default values\n"
"\n"
"EXAMPLES\n"
"  % profileclient -psilent\n"
"  \n"
"    Activate 'silent' profile.\n"
"    \n"
"  % profileclient -T\n"
"  \n"
"    Any changes to profile data are emitted to stdout.\n"
"\n"
"\n"
"COPYRIGHT\n"
"  Copyright (C) 2008 Nokia Corporation.\n"
"  \n"
"SEE ALSO\n"
"  profiled\n";

int main(int argc, char **argv)
{
  int opt;

  char *p,*k,*v,**w;

  while( (opt = getopt(argc, argv, "hp:s:d:g:t:TlkvV:B:I:D:G:rR:")) != -1 )
  {
    switch (opt)
    {
    case 'h':
      write(STDOUT_FILENO, usage, sizeof usage - 1);
      exit(0);

    case 'p':
      profile_set_profile(optarg);
      break;

    case 'l':
      p = profile_get_profile();
      w = profile_get_profiles();
      for( size_t i = 0; w && w[i]; ++i )
      {
        printf("%s%s\n", w[i], strcmp(w[i],p) ? "" : "*");
      }
      profile_free_profiles(w);
      free(p);
      break;

    case 'k':
      w = profile_get_keys();
      for( size_t i = 0; w && w[i]; ++i )
      {
        printf("%s\n", w[i]);
      }
      profile_free_keys(w);
      break;

    case 'r':
      w = profile_get_keys();
      for( size_t i = 0; w && w[i]; ++i )
      {
        profile_set_value(0, w[i], "");
      }
      profile_free_keys(w);
      break;

    case 'R':
      w = profile_get_keys();
      for( size_t i = 0; w && w[i]; ++i )
      {
        profile_set_value(optarg, w[i], "");
      }
      profile_free_keys(w);
      break;

    case 'v':
    case 'V':
      {
        p = (opt == 'v') ? 0 : optarg;
        profileval_t *t = profile_get_values(p);

        for( size_t i = 0; t && t[i].pv_key; ++i )
        {
          printf("%s = %s (%s)\n", t[i].pv_key, t[i].pv_val, t[i].pv_type);
        }
        profile_free_values(t);
      }
      break;

    case 'T':
      exit(track());

    case 's':
      parse(optarg, &p,&k,&v);
      profile_set_value(p, k, v);
      break;

    case 'd':
      parse(optarg, &p,&k,&v);
      profile_set_value(p, k, "");
      break;

    case 'g':
      parse(optarg, &p,&k,&v);
      v = profile_get_value(p,k);
      printf("%s\n", v);
      free(v);
      break;

    case 'G':
      {
        parse(optarg, &p,&k,&v);

        char *s = profile_get_value(p,k);
        printf("STRING: '%s'\n", s);
        free(s);

        int b = profile_get_value_as_bool(p,k);
        printf("BOOL: %s (%d)\n", b ? "On" : "Off", b);

        int i = profile_get_value_as_int(p,k);
        printf("INT: %d\n", i);

        double d = profile_get_value_as_double(p,k);
        printf("DOUBLE:%g\n", d);
      }
      break;

    case 'B':
      {
        parse(optarg, &p,&k,&v);
        int v = profile_get_value_as_bool(p,k);
        printf("BOOL:%s (%d)\n", v ? "On" : "Off", v);
      }
      break;

    case 'I':
      {
        parse(optarg, &p,&k,&v);
        int v = profile_get_value_as_int(p,k);
        printf("INT:%d\n", v);
      }
      break;

    case 'D':
      {
        parse(optarg, &p,&k,&v);
        double v = profile_get_value_as_double(p,k);
        printf("DOUBLE:%g\n", v);
      }
      break;

    case 't':
      v = profile_get_type(optarg);
      printf("%s\n", v);
      free(v);
      break;

    default: /* '?' */
      fprintf(stderr, "(use -h for usage info)\n");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}

#if 0

int        profile_has_profile  (const char *profile);

int        profile_has_value    (const char *key);

int        profile_get_value_as_int   (const char *profile, const char *key);
int        profile_set_value_as_int   (const char *profile, const char *key, int val);
int        profile_get_value_as_double(const char *profile, const char *key);
int        profile_set_value_as_double(const char *profile, const char *key, double val);

#endif
