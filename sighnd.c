
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

#include "sighnd.h"
#include "mainloop.h"
#include "logging.h"

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <glib.h>

/* ========================================================================= *
 * SIGNAL HANDING FUNCTIONALITY
 * ========================================================================= */

/* list of signals to trap */
static const int sighnd_trap[] =
{
  SIGINT,
  SIGQUIT,
  SIGTERM,
  SIGHUP,
  SIGUSR1,
  SIGUSR2,
  -1
};

static int   sighnd_exit    = 0;
static int   sighnd_pipe[2] = {-1, -1};
static guint sighnd_watch   = 0;

/* ------------------------------------------------------------------------- *
 * sighnd_terminate  --  terminate process from signal handler
 * ------------------------------------------------------------------------- */

static
void
sighnd_terminate(void)
{
  static int done = 0;

  switch( ++done )
  {
  case 1:
    mainloop_stop();
    break;
  case 2:
    log_crit("Terminating NOW\n");
    // fall through
  default:
    _exit(1);
    break;
  }
}

/* ------------------------------------------------------------------------- *
 * sighnd_handler  --  handle trapped signals
 * ------------------------------------------------------------------------- */

static
void
sighnd_handler(int sig)
{
  log_warning("Got signal [%d] %s\n", sig, strsignal(sig));

  switch( sig )
  {
  case SIGUSR1:
    log_set_level(LOG_DEBUG);
    log_emit(LOG_WARNING, "log verbosity -> DEBUG\n");
    break;

  case SIGUSR2:
    log_set_level(LOG_WARNING);
    log_emit(LOG_WARNING, "log verbosity -> WARNING\n");
    break;

  default:
    sighnd_terminate();
    break;
  }
}

/* ------------------------------------------------------------------------- *
 * sighnd_pipe_rx_cb
 * ------------------------------------------------------------------------- */

static
gboolean
sighnd_pipe_rx_cb(GIOChannel *channel, GIOCondition condition, gpointer data)
{
  (void)channel; (void)condition; (void)data;

  int sig = 0;

  if( read(sighnd_pipe[0], &sig, sizeof sig) == sizeof sig )
  {
    sighnd_handler(sig);
    // keep io watch alive
    return TRUE;
  }
  // disable io watch
  return FALSE;
}

/* ------------------------------------------------------------------------- *
 * sighnd_pipe_tx_cb  --  signal handling callback
 * ------------------------------------------------------------------------- */

static void sighnd_pipe_tx_cb(int sig)
{
  // restore signal handler
  signal(sig, sighnd_pipe_tx_cb);

  // make sure that stuck mainloop does not stop
  // us from handling terminating signals
  switch( sig )
  {
  case SIGINT:
  case SIGQUIT:
  case SIGTERM:
    if( sighnd_exit != 0 )
    {
      _exit(1);
    }
    sighnd_exit = 1;
    break;
  }

  // transfer the signal to mainloop via pipe
  if( TEMP_FAILURE_RETRY(write(sighnd_pipe[1], &sig, sizeof sig)) != sizeof sig )
  {
    abort();
  }
}

/* ------------------------------------------------------------------------- *
 * sighnd_setup_forwarding
 * ------------------------------------------------------------------------- */

static
int
sighnd_setup_forwarding(void)
{
  int error = -1;
  GIOChannel *channel = 0;

  if( pipe(sighnd_pipe) == -1 )
  {
    log_err("could not create signal pipe: %s\n", strerror(errno));
    goto cleanup;
  }

  if( (channel = g_io_channel_unix_new(sighnd_pipe[0])) == 0 )
  {
    log_err("could not create signal io channel\n");
    goto cleanup;
  }

  sighnd_watch = g_io_add_watch(channel, G_IO_IN, sighnd_pipe_rx_cb, 0);
  if( sighnd_watch == 0 )
  {
    log_err("could not create signal io watch\n");
    goto cleanup;
  }

  error = 0;

  cleanup:

  if( channel != 0 )
  {
    g_io_channel_unref(channel);
  }

  return error;
}

/* ------------------------------------------------------------------------- *
 * sighnd_setup_callbacks
 * ------------------------------------------------------------------------- */

static
int
sighnd_setup_callbacks(void)
{
  int err = 0;

  for( int i = 0; sighnd_trap[i] != -1; ++i )
  {
    if( signal(sighnd_trap[i], sighnd_pipe_tx_cb) == SIG_ERR )
    {
      log_warning("could not install handler for signal %d - %s",
                  sighnd_trap[i], strsignal(sighnd_trap[i]));
      err = -1;
    }
  }

  return err;
}

/* ------------------------------------------------------------------------- *
 * sighnd_setup  --  setup signal trapping
 * ------------------------------------------------------------------------- */

int
sighnd_setup(void)
{
  int error = -1;

  if( sighnd_setup_forwarding() != -1 &&
      sighnd_setup_callbacks() != -1 )
  {
    error = 0;
  }

  return error;
}
