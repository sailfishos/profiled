
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

#include "confmon.h"
#include "database.h"
#include "xutil.h"
#include "logging.h"

#include <glib.h>
#include <sys/inotify.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

// TODO: do we need to worry about recovering after whole /etc is lost?

#define DEBUG 0

#if DEBUG
# define OUTPUT_FUNCTION_NAME printf("@ %s()\n", __FUNCTION__);
#else
# define OUTPUT_FUNCTION_NAME do{}while(0);
#endif

/* ========================================================================= *
 * Misc utilities
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * lea  --  helper for address + offset calculations
 * ------------------------------------------------------------------------- */

static inline void *lea(void *base, int offs)
{
  return ((char *)base) + offs;
}

/* ------------------------------------------------------------------------- *
 * unpk  --  inotify masks to human readable strings
 * ------------------------------------------------------------------------- */

#if DEBUG
static const char *unpk(unsigned mask)
{
  static const struct
  {
    unsigned mask; const char *name;
  } lut[] =
  {
    { IN_ACCESS,         "IN_ACCESS" },
    { IN_MODIFY,         "IN_MODIFY" },
    { IN_ATTRIB,         "IN_ATTRIB" },
    { IN_CLOSE_WRITE,    "IN_CLOSE_WRITE" },
    { IN_CLOSE_NOWRITE,  "IN_CLOSE_NOWRITE" },
    { IN_OPEN,           "IN_OPEN" },
    { IN_MOVED_FROM,     "IN_MOVED_FROM" },
    { IN_MOVED_TO,       "IN_MOVED_TO" },
    { IN_CREATE,         "IN_CREATE" },
    { IN_DELETE,         "IN_DELETE" },
    { IN_DELETE_SELF,    "IN_DELETE_SELF" },
    { IN_MOVE_SELF,      "IN_MOVE_SELF" },
    { IN_UNMOUNT,        "IN_UNMOUNT" },
    { IN_Q_OVERFLOW,     "IN_Q_OVERFLOW" },
    { IN_IGNORED,        "IN_IGNORED" },
    { IN_ONLYDIR,        "IN_ONLYDIR" },
    { IN_DONT_FOLLOW,    "IN_DONT_FOLLOW" },
    { IN_MASK_ADD,       "IN_MASK_ADD" },
    { IN_ISDIR,          "IN_ISDIR" },
    { IN_ONESHOT,        "IN_ONESHOT" },
    {0, 0}
  };

  static char tmp[1024];
  char *pos = tmp;

  *tmp = 0;

  auto void add(const char *s);

  auto void add(const char *s)
  {
    if( *tmp ) *pos++ = ' ';
    while( *s ) *pos++ = *s++;
    *pos = 0;
  }

  for(int i = 0; lut[i].mask; ++i )
  {
    if( mask & lut[i].mask )
    {
      add(lut[i].name);
      mask ^= lut[i].mask;
    }
  }

  if( mask )
  {
    char hex[64];
    snprintf(hex, sizeof hex, "0x%x", mask);
    add(hex);
  }

  return tmp;
};
#endif

/* ========================================================================= *
 * Module Configuration
 * ========================================================================= */

enum
{
  CONFMON_RELOAD_DELAY = 5, /* [seconds] amount of time between the last
                             * modification to configuration files and
                             * doing the configuration reload */
};

/* ========================================================================= *
 * Module Data
 * ========================================================================= */

static char *confmon_parent  = 0;  // "/etc"
static char *confmon_config  = 0;  // "profiled"
static guint confmon_timeout = 0;  // delayed reaload g_timeout
static int   confmon_inotify = -1; // inotify file descriptor
static guint confmon_iowatch = 0;  // inotify input callback

static int   wd_config = -1;       // watch descriptor for config dir
static int   wd_parent = -1;       // watch descriptor for parent dir

static void confmon_disable(void);
static int  confmon_restart(void);

/* ========================================================================= *
 * DELAYED RELOAD
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * confmon_do_reload  --  g_timeout callback that does the actual reload
 * ------------------------------------------------------------------------- */

static
gboolean
confmon_do_reload(gpointer aptr)
{
  OUTPUT_FUNCTION_NAME

  confmon_timeout = 0;
  database_reload();
  return FALSE;
}

/* ------------------------------------------------------------------------- *
 * confmon_cancel_reload  --  cancel pending config reload
 * ------------------------------------------------------------------------- */

static
void
confmon_cancel_reload(void)
{
  if( confmon_timeout != 0 )
  {
    OUTPUT_FUNCTION_NAME
    g_source_remove(confmon_timeout);
    confmon_timeout = 0;
  }
}
/* ------------------------------------------------------------------------- *
 * confmon_request_reload  --  request delayed config reload
 * ------------------------------------------------------------------------- */

static
void
confmon_request_reload(void)
{
  confmon_cancel_reload();
  OUTPUT_FUNCTION_NAME
  confmon_timeout = g_timeout_add_seconds(CONFMON_RELOAD_DELAY,
                                          confmon_do_reload,0);
}

/* ========================================================================= *
 * MONITOR CONFIGURATION FILES
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * confmon_inotify_cb  --  callback for reading inotify events
 * ------------------------------------------------------------------------- */

static
gboolean
confmon_inotify_cb(GIOChannel   *source,
                        GIOCondition  condition,
                        gpointer      data)
{
  OUTPUT_FUNCTION_NAME

  int disable = 0;
  int restart = 0;
  int reload  = 0;

  char buf[2<<10];
  struct inotify_event *eve;

  int n = read(confmon_inotify, buf, sizeof buf);

  if( n == -1 )
  {
    if( errno != EINTR )
    {
      log_warning("inotify read: %s\n", strerror(errno));
      disable = 1;
    }
  }
  else if( n < (int)sizeof *eve )
  {
    log_warning("inotify read: %d / %Zd\n", n, sizeof *eve);
    disable = 1;
  }
  else
  {
    eve = lea(buf, 0);
    while( n >= sizeof *eve )
    {
      int k = sizeof *eve + eve->len;

      if( (k < sizeof *eve) || (k > n) )
      {
        break;
      }

#if DEBUG
      printf("EVE wd=%d, mask=%s, cookie=0x%x, len=%u, %s\n",
             eve->wd,
             unpk(eve->mask),
             (unsigned)eve->cookie,
             (unsigned)eve->len,
             eve->name);
#endif

      if( eve->wd == wd_parent )
      {
        if( eve->mask & IN_IGNORED )
        {
          disable = 1;
        }
        else if( eve->mask & (IN_CREATE|IN_MOVED_TO) )
        {
          if( !strcmp(confmon_config, eve->name) )
          {
            restart = 1;
          }
        }
      }
      else if( eve->wd == wd_config )
      {
        reload = 1;
      }

      n -= k;
      eve = lea(eve, k);
    }
  }

  if( disable )
  {
    log_warning("config file inotify monitoring disabled\n");
    confmon_disable();
  }
  else
  {
    if( reload )
    {
      confmon_request_reload();
    }
    if( restart )
    {
      confmon_restart();
    }
  }

  return !disable;
}

/* ------------------------------------------------------------------------- *
 * confmon_disable  --  cease all monitoring activities
 * ------------------------------------------------------------------------- */

static
void
confmon_disable(void)
{
  OUTPUT_FUNCTION_NAME

  if( confmon_iowatch != 0 )
  {
    g_source_remove(confmon_iowatch);
    confmon_iowatch = 0;
  }

  if( confmon_inotify != -1 )
  {
    if( wd_parent != -1 )
    {
      inotify_rm_watch(confmon_inotify, wd_parent);
      wd_parent = -1;
    }
    if( wd_config != -1 )
    {
      inotify_rm_watch(confmon_inotify, wd_config);
      wd_config = -1;
    }

    close(confmon_inotify);
    confmon_inotify = -1;
  }
}

/* ------------------------------------------------------------------------- *
 * confmon_restart  --  restart configuration directory watching
 * ------------------------------------------------------------------------- */

static
int
confmon_restart(void)
{
  OUTPUT_FUNCTION_NAME

  int error = -1;

  /* - - - - - - - - - - - - - - - - - - - *
   * need to have valid inotify file
   * descriptor to do anything ...
   * - - - - - - - - - - - - - - - - - - - */

  if( confmon_inotify == -1 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * remove stale configuration directory
   * watch
   * - - - - - - - - - - - - - - - - - - - */

  if( wd_config != -1 )
  {
    inotify_rm_watch(confmon_inotify, wd_config);
    wd_config = -1;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * try to watch config directory, no
   * need to panic if it fails right now
   * - - - - - - - - - - - - - - - - - - - */

  wd_config = inotify_add_watch(confmon_inotify,
                                CONFIG_DIR,
                                IN_CLOSE_WRITE|IN_DELETE);

  if( wd_config == -1 )
  {
    log_err("failed to add inotify watch for '%s': %s\n",
            CONFIG_DIR, strerror(errno));
  }

  error = 0;

  cleanup:

  return error;
}

/* ------------------------------------------------------------------------- *
 * confmon_start  --  start monitoring config directory via inotify
 * ------------------------------------------------------------------------- */

static
int
confmon_start(void)
{
  OUTPUT_FUNCTION_NAME

  int         error   = -1;
  GIOChannel *io_chan = NULL;

  /* - - - - - - - - - - - - - - - - - - - *
   * initialize inotify descriptor
   * - - - - - - - - - - - - - - - - - - - */

  if( (confmon_inotify = inotify_init()) == -1 )
  {
    log_err("inotify init failed: %s\n", strerror(errno));
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * as configuration files are packaged
   * separately from the daemon, we might
   * not have a configuration directory
   *
   * -> watch out for it to appear
   * - - - - - - - - - - - - - - - - - - - */

  if( wd_parent == -1 )
  {
    wd_parent = inotify_add_watch(confmon_inotify,
                                  confmon_parent,
                                  IN_CREATE|IN_MOVED_TO);
    if( wd_parent == -1 )
    {
      log_err("failed to add inotify watch for '%s': %s\n",
              confmon_parent, strerror(errno));
      goto cleanup;
    }
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * let gmainloop wait for input on the
   * inotify descriptor and then call our
   * event handler callback
   * - - - - - - - - - - - - - - - - - - - */

  if( (io_chan = g_io_channel_unix_new(confmon_inotify)) == 0 )
  {
    goto cleanup;
  }

  confmon_iowatch = g_io_add_watch(io_chan, G_IO_IN,
                                   confmon_inotify_cb, 0);

  if( confmon_iowatch == 0 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * try to track the actual configuration
   * file directory too
   * - - - - - - - - - - - - - - - - - - - */

  error = confmon_restart();

  /* - - - - - - - - - - - - - - - - - - - *
   * cleanup & retirn
   * - - - - - - - - - - - - - - - - - - - */

  cleanup:

  if( io_chan != 0 )
  {
    g_io_channel_unref(io_chan);
  }

  if( error != 0 )
  {
    confmon_disable();
  }

  return error;
}

/* ------------------------------------------------------------------------- *
 * confmon_init_paths  --  set up parent of config dir paths
 * ------------------------------------------------------------------------- */

static
void
confmon_init_paths(void)
{
  char *path = 0;
  char *base = 0;

  if( (path = strdup(CONFIG_DIR)) != 0 )
  {
    if( (base = strrchr(path, '/')) != 0 )
    {
      *base++ = 0;
    }
  }
  xstrset(&confmon_parent, (path && *path) ? path : "/etc");
  xstrset(&confmon_config, (base && *base) ? base : "profiled");

  free(path);
}

/* ------------------------------------------------------------------------- *
 * confmon_quit_paths  --  free parent of config dir paths
 * ------------------------------------------------------------------------- */

static
void
confmon_quit_paths(void)
{
  xstrset(&confmon_parent, 0);
  xstrset(&confmon_config, 0);
}

/* ========================================================================= *
 * EXTERNAL API
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * confmon_init  --  start config file monitoring subsystem
 * ------------------------------------------------------------------------- */

int
confmon_init(void)
{
  confmon_init_paths();
  return confmon_start();
}

/* ------------------------------------------------------------------------- *
 * confmon_quit  --  stop config file monitoring subsystem
 * ------------------------------------------------------------------------- */

void
confmon_quit(void)
{
  confmon_cancel_reload();
  confmon_disable();
  confmon_quit_paths();
}
