
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

#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <glib.h>

#include "logging.h"
#include "sighnd.h"
#include "server.h"
#include "database.h"
#include "confmon.h"
#include "mainloop.h"

/* ========================================================================= *
 * Module Data
 * ========================================================================= */

static GMainLoop  *mainloop_addon     = 0;

/* ========================================================================= *
 * Module Functions
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * mainloop_stop
 * ------------------------------------------------------------------------- */

void
mainloop_stop(void)
{
  if( mainloop_addon == 0 )
  {
    log_err("no main loop to stop, terminating immediately\n");
    exit(1);
  }

  g_main_loop_quit(mainloop_addon);
}

/* ------------------------------------------------------------------------- *
 * mainloop_run
 * ------------------------------------------------------------------------- */

int
mainloop_run(int argc, char* argv[])
{
  int exit_code = EXIT_FAILURE;

  log_debug("STARTUP");

  /* - - - - - - - - - - - - - - - - - - - *
   * make writing to closed sockets
   * return -1 instead of raising a
   * SIGPIPE signal
   * - - - - - - - - - - - - - - - - - - - */

  signal(SIGPIPE, SIG_IGN);

  /* - - - - - - - - - - - - - - - - - - - *
   * install signal handlers
   * - - - - - - - - - - - - - - - - - - - */

  if( sighnd_setup() == -1 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * initialize profile database
   * - - - - - - - - - - - - - - - - - - - */

  if( database_init() == -1 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * start profile dbus server
   * - - - - - - - - - - - - - - - - - - - */

  if( server_init() == -1 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * start monitoring config directory
   * - - - - - - - - - - - - - - - - - - - */

  if( confmon_init() == -1 )
  {
    goto cleanup;
  }

  /* - - - - - - - - - - - - - - - - - - - *
   * enter mainloop
   * - - - - - - - - - - - - - - - - - - - */

  mainloop_addon = g_main_loop_new(NULL, FALSE);

  log_debug("ENTER MAIN LOOP");
  g_main_loop_run(mainloop_addon);
  log_debug("LEAVE MAIN LOOP");

  exit_code = EXIT_SUCCESS;

  /* - - - - - - - - - - - - - - - - - - - *
   * cleanup & exit
   * - - - - - - - - - - - - - - - - - - - */

  cleanup:

  log_debug("CLEANUP");

  if( mainloop_addon != 0 )
  {
    g_main_loop_unref(mainloop_addon);
    mainloop_addon = 0;
  }

  confmon_quit();

  server_quit();

  database_quit();

  log_debug("EXIT %d", exit_code);

  return exit_code;
}
