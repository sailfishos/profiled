
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

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>

#ifdef LOGGING_ENABLED

static int log_level   = LOG_WARNING;
static int log_promote = LOG_WARNING;
static int log_opened = 0;

/* ------------------------------------------------------------------------- *
 * get_progname
 * ------------------------------------------------------------------------- */

static const char *get_progname(void)
{
  static char *progname = 0;

  if( !progname )
  {
    char tmp[256];

    memset(tmp, 0, sizeof tmp);
    if( readlink("/proc/self/exe", tmp, sizeof tmp - 1) > 0 )
    {
      char *s = strrchr(tmp, '/');
      s = s ? (s+1) : tmp;
      if( s && *s ) progname = strdup(s);
    }

    if( !progname )
    {
      if( asprintf(&progname, "unkn%d", (int)getpid()) < 0 )
      {
	// we're not going to free this anyway
	progname = (char *)"unknown";
      }
    }
  }

  return progname;
}

/* ------------------------------------------------------------------------- *
 * log_set_level
 * ------------------------------------------------------------------------- */

void
log_set_level(int level)
{
  log_level = level;
}

/* ------------------------------------------------------------------------- *
 * log_cmp_level
 * ------------------------------------------------------------------------- */

int
log_cmp_level(int level)
{
  return level <= log_level;
}

/* ------------------------------------------------------------------------- *
 * log_open
 * ------------------------------------------------------------------------- */

void
log_open(const char *ident, int daemon)
{
  if( !log_opened )
  {
    log_opened = 1;
    openlog(ident ?: get_progname(), LOG_PID, daemon ? LOG_DAEMON : LOG_USER);
  }
}

/* ------------------------------------------------------------------------- *
 * log_close
 * ------------------------------------------------------------------------- */

void
log_close(void)
{
  if( log_opened )
  {
    log_opened = 0;
    closelog();
  }
}

/* ------------------------------------------------------------------------- *
 * log_emit_syslog
 * ------------------------------------------------------------------------- */

static
void
log_emit_syslog(int level, const char *fmt, va_list va)
{
  vsyslog(level, fmt, va);
}

/* ------------------------------------------------------------------------- *
 * log_emit_stderr
 * ------------------------------------------------------------------------- */

static
void
log_emit_stderr(int level, const char *fmt, va_list va)
{
  (void)level;

  char msg[256];
  char *eol;

  vsnprintf(msg, sizeof msg, fmt, va);

  eol = strrchr(msg, '\n');
  dprintf(STDERR_FILENO, "%s[%d]: %s%s",
          get_progname(), getpid(), msg, (!eol || eol[1]) ? "\n" : "");

}

/* ------------------------------------------------------------------------- *
 * log_emit
 * ------------------------------------------------------------------------- */

void
log_emit(int level, const char *fmt, ...)
{
  int saved = errno;

  if( level <= log_level )
  {
    /* Ugly hack: promote level so that the messages
     * end up in the syslog on the target device too */
    if( level > log_promote )
    {
      level = log_promote;
    }

    /* Recursive syslog calls - due to signal handlers
     * for example - can result in deadlock.
     *
     * We can protect against deadlocks as long as
     * all logging goes through this function.
     *
     * Possible direct calls to syslog from elsewhere
     * means that this can't be made 100% safe.
     */

    va_list va;
    va_start(va, fmt);

    static volatile int syslog_cnt = 0;

    if( ++syslog_cnt == 1 )
    {
      log_emit_syslog(level, fmt, va);
    }
    else
    {
      log_emit_stderr(level, fmt, va);
    }
    --syslog_cnt;

    va_end(va);
  }

  errno = saved;
}

# ifdef LOGGING_FNCALLS
static int log_depth = 0;
static const char bar[] = "................................................................";

void log_enter(const char *func)
{
  log_emit(LOG_CRIT, "|%.*s> %s() {\n", log_depth*2, bar, func);
  ++log_depth;
}

void log_leave(const char *func)
{
  if( --log_depth < 0 ) log_depth = 0;
  log_emit(LOG_CRIT, "|%.*s> } %s()\n", log_depth*2, bar, func);
}
# endif // LOGGING_FNCALLS

#endif // LOGGING_ENABLED
