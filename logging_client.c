
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

#ifdef LOGGING_ENABLED

/* ------------------------------------------------------------------------- *
 * logging level control
 * ------------------------------------------------------------------------- */

# ifdef LOGGING_EXTRA
static int log_level_promote = LOG_WARNING; // promote to warnings
static int log_level_cutoff  = LOG_DEBUG;   // everything passes through
# else // normal values
static int log_level_promote = LOG_DEBUG;   // no promotion
static int log_level_cutoff  = LOG_WARNING; // warnings or more severe
# endif

/* ------------------------------------------------------------------------- *
 * log_cmp_level
 * ------------------------------------------------------------------------- */

int
log_cmp_level(int level)
{
  return level <= log_level_cutoff;
}

/* ------------------------------------------------------------------------- *
 * log_emit
 * ------------------------------------------------------------------------- */

void
log_emit(int level, const char *fmt, ...)
{
  if( level <= log_level_cutoff )
  {
    if( level > log_level_promote )
    {
      level = log_level_promote;
    }

    va_list va;
    va_start(va, fmt);
    //vsyslog(LOG_USER | level, fmt, va);
    vsyslog(level, fmt, va);
    va_end(va);
  }
}

/* ------------------------------------------------------------------------- *
 * call tree logging
 * ------------------------------------------------------------------------- */

# ifdef LOGGING_FNCALLS
#  if 0
#   warning client side function call logging enabled
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
#  else
void log_enter(const char *func) { }
void log_leave(const char *func) { }
#  endif
# endif // LOGGING_FNCALLS

#endif // LOGGING_ENABLED
