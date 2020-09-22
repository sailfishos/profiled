
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

#ifndef LOGGING_H_
# define LOGGING_H_

# include <stdarg.h>
# include <syslog.h>

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif

/* ------------------------------------------------------------------------- *
 * Configurable Logging
 *
 * -D LOGGING_ENABLED   -> without this no logging will be done
 *
 * -D LOGGING_FNCALLS   -> calltree via ENTER/LEAVE macros
 *
 * -D LOGGING_EXTRA     -> syslog calls use promoted priority
 *
 * -D LOGGING_CHECK1ST  -> check level before evaluating message args
 *
 * -D LOGGING_LEVEL=lev -> enable logging up to level
 *    0 - emerg
 *    1 - alert
 *    2 - crit
 *    3 - err
 *    4 - warning
 *    5 - notice
 *    6 - info
 *    7 - debug
 * ------------------------------------------------------------------------- */

/* ------------------------------------------------------------------------- *
 * Logging Functions
 * ------------------------------------------------------------------------- */

# ifdef LOGGING_ENABLED
void log_open(const char *ident, int daemon);
void log_close(void);
void log_emit(int lev, const char *fmt, ...) __attribute__ ((format (printf, 2, 3)));
void log_set_level(int level);
int  log_cmp_level(int level);

# else
#  define log_open(IDENT,DAEMON)   do{}while(0)
#  define log_close()              do{}while(0)
#  define log_emit(LEV,FMT,ARG...) do{}while(0)
#  define log_set_level(LEV)       do{}while(0)
#  define log_cmp_level(LEV)       0
# endif

/* ------------------------------------------------------------------------- *
 * Logging Macros
 * ------------------------------------------------------------------------- */

# ifdef LOGGING_ENABLED

#  ifdef LOGGING_CHECK1ST
#   define log_emitif(LEV,FMT,ARG...) \
  do{\
    if(log_cmp_level(LEV)) {\
      log_emit(LEV,FMT,##ARG);\
    }\
  }while(0)
#  else
#   define log_emitif(LEV,FMT,ARG...) log_emit(LEV,FMT,##ARG)
#  endif

#  if LOGGING_LEVEL >= 0
#   define log_emerg(FMT, ARG...)     log_emitif(LOG_EMERG,   FMT, ## ARG)
#   define log_emerg_F(FMT, ARG...)   log_emitif(LOG_EMERG,   "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 1
#   define log_alert(FMT, ARG...)     log_emitif(LOG_ALERT,   FMT, ## ARG)
#   define log_alert_F(FMT, ARG...)   log_emitif(LOG_ALERT,   "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 2
#   define log_crit(FMT, ARG...)      log_emitif(LOG_CRIT,    FMT, ## ARG)
#   define log_crit_F(FMT, ARG...)    log_emitif(LOG_CRIT,    "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 3
#   define log_err(FMT, ARG...)       log_emitif(LOG_ERR,     FMT, ## ARG)
#   define log_err_F(FMT, ARG...)     log_emitif(LOG_ERR,     "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 4
#   define log_warning(FMT, ARG...)   log_emitif(LOG_WARNING, FMT, ## ARG)
#   define log_warning_F(FMT, ARG...) log_emitif(LOG_WARNING, "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 5
#   define log_notice(FMT, ARG...)    log_emitif(LOG_NOTICE,  FMT, ## ARG)
#   define log_notice_F(FMT, ARG...)  log_emitif(LOG_NOTICE,  "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 6
#   define log_info(FMT, ARG...)      log_emitif(LOG_INFO,    FMT, ## ARG)
#   define log_info_F(FMT, ARG...)    log_emitif(LOG_INFO,    "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  if LOGGING_LEVEL >= 7
#   define log_debug(FMT, ARG...)     log_emitif(LOG_DEBUG,   FMT, ## ARG)
#   define log_debug_F(FMT, ARG...)   log_emitif(LOG_DEBUG,   "%s: "FMT, __FUNCTION__, ## ARG)
#  endif

#  ifdef LOGGING_FNCALLS
void log_enter(const char *func);
void log_leave(const char *func);
#   define ENTER log_enter(__FUNCTION__);
#   define LEAVE log_leave(__FUNCTION__);
#  endif
# endif

/* ------------------------------------------------------------------------- *
 * Provide dummies if not defined above
 * ------------------------------------------------------------------------- */

# ifndef log_emerg
#  define log_emerg(FMT, ARG...)     do{}while(0)
#  define log_emerg_F(FMT, ARG...)   do{}while(0)
# endif

# ifndef log_alert
#  define log_alert(FMT, ARG...)     do{}while(0)
#  define log_alert_F(FMT, ARG...)   do{}while(0)
# endif

# ifndef log_crit
#  define log_crit(FMT, ARG...)      do{}while(0)
#  define log_crit_F(FMT, ARG...)    do{}while(0)
# endif

# ifndef log_err
#  define log_err(FMT, ARG...)       do{}while(0)
#  define log_err_F(FMT, ARG...)     do{}while(0)
# endif

# ifndef log_warning
#  define log_warning(FMT, ARG...)   do{}while(0)
#  define log_warning_F(FMT, ARG...) do{}while(0)
# endif

# ifndef log_notice
#  define log_notice(FMT, ARG...)    do{}while(0)
#  define log_notice_F(FMT, ARG...)  do{}while(0)
# endif

# ifndef log_info
#  define log_info(FMT, ARG...)      do{}while(0)
#  define log_info_F(FMT, ARG...)    do{}while(0)
# endif

# ifndef log_debug
#  define log_debug(FMT, ARG...)     do{}while(0)
#  define log_debug_F(FMT, ARG...)   do{}while(0)
# endif

# ifndef ENTER
#  define ENTER /*nop*/
# endif

# ifndef LEAVE
#  define LEAVE /*nop*/
# endif

# ifdef __cplusplus
};
# endif

#endif /* LOGGING_H_ */
