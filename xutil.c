
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

#include "xutil.h"
#include "logging.h"

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

/* ------------------------------------------------------------------------- *
 * xexists  --  file exists
 * ------------------------------------------------------------------------- */

int
xexists(const char *path)
{
  return access(path, F_OK) == 0;
}

/* ------------------------------------------------------------------------- *
 * xloadfile  --  load file in ram, zero padded for c string compatibility
 * ------------------------------------------------------------------------- */

int
xloadfile(const char *path, char **pdata, size_t *psize)
{
  int     err  = -1;
  int     file = -1;
  size_t  size = 0;
  char   *data = 0;

  struct stat st;

  if( (file = open(path, O_RDONLY)) == -1 )
  {
    log_debug("%s: load/open: %s\n", path, strerror(errno));
    goto cleanup;
  }

  if( fstat(file, &st) == -1 )
  {
    log_err("%s: load/stat: %s\n", path, strerror(errno));
    goto cleanup;
  }

  size = st.st_size;

  // calloc size+1 -> text files are zero terminated

  if( (data = calloc(size+1, 1)) == 0 )
  {
    log_err("%s: load/calloc %zd: %s\n", path, size, strerror(errno));
    goto cleanup;
  }

  errno = 0;
  if( read(file, data, size) != (ssize_t)size )
  {
    log_warning("%s: load/read: %s\n", path, strerror(errno));
    goto cleanup;
  }

  err = 0;

  cleanup:

  if( err  !=  0 ) free(data), data = 0, size = 0;
  if( file != -1 ) close(file);

  free(*pdata), *pdata = data, *psize = size;

  return err;
}

/* ------------------------------------------------------------------------- *
 * xsavefile  --  save buffer to file
 * ------------------------------------------------------------------------- */

int
xsavefile(const char *path, int mode, const void *data, size_t size)
{
  int err  = -1;
  int file = -1;

  if( (file = open(path, O_WRONLY|O_CREAT, mode)) == -1 )
  {
    remove(path);
    if( (file = open(path, O_WRONLY|O_CREAT|O_TRUNC, mode)) == -1 )
    {
      log_err("%s: save/open: %s\n", path, strerror(errno));
      goto cleanup;
    }
  }

  const char *base = data;
  size_t done = 0;
  while( done < size )
  {
    errno = 0;
    int n = TEMP_FAILURE_RETRY(write(file, base+done, size-done));
    if( n <= 0 )
    {
      log_err("%s: save/write: %s (%Zd/%Zd)\n", path, strerror(errno),
              done, size);
      goto cleanup;
    }
    done += n;
  }

  if( ftruncate(file, size) == -1 )
  {
    log_err("%s: save/truncate: %s\n", path, strerror(errno));
    goto cleanup;
  }

  if( fsync(file) == -1 )
  {
    log_err("%s: save/fsync: %s\n", path, strerror(errno));
    goto cleanup;
  }

  err = 0;

  cleanup:

  if( file != -1 && close(file) == -1 )
  {
    log_err("%s: save/close: %s\n", path, strerror(errno));
    err = -1;
  }

  //log_crit_F("error=%d\n", err);
  return err;
}

/* ------------------------------------------------------------------------- *
 * xcyclefiles  --  rename: temp -> current -> backup
 * ------------------------------------------------------------------------- */

int
xcyclefiles(const char *temp, const char *path, const char *back)
{
  /* - - - - - - - - - - - - - - - - - - - *
   * not foolproof, but certainly better
   * than directly overwriting datafiles
   * - - - - - - - - - - - - - - - - - - - */

  int err = -1;

  if( !xexists(temp) )
  {
    log_warning("%s: missing new file: %s\n", temp, strerror(errno));
    goto done;
  }

  if( xexists(path) )
  {
    // backup -> /dev/null
    remove(back);

    // current -> backup
    if( rename(path, back) == -1 )
    {
      log_err("rename %s -> %s: %s\n", path, back, strerror(errno));
      goto done;
    }
  }

  // temporary -> current
  if( rename(temp, path) == -1 )
  {
    log_err("rename %s -> %s: %s\n", temp, path, strerror(errno));

    // try backup -> current
    if( xexists(back) )
    {
      rename(back, path);
    }
    goto done;
  }

  // success
  err = 0;

  done:

  //log_crit_F("error=%d\n", err);
  return err;
}

/* ------------------------------------------------------------------------- *
 * xfetchstats  --  get current stats for file
 * ------------------------------------------------------------------------- */

void
xfetchstats(const char *path, struct stat *cur)
{
  if( stat(path, cur) == -1 )
  {
    memset(cur, 0, sizeof *cur);
  }
}

/* ------------------------------------------------------------------------- *
 * xcheckstats  --  check if current stats differ from given
 * ------------------------------------------------------------------------- */

int
xcheckstats(const char *path, const struct stat *old)
{
  int ok = 1;

  struct stat cur;

  xfetchstats(path, &cur);

  if( old->st_dev   != cur.st_dev  ||
      old->st_ino   != cur.st_ino  ||
      old->st_size  != cur.st_size ||
      old->st_mtime != cur.st_mtime )
  {
    ok = 0;
  }

  return ok;
}

/* ------------------------------------------------------------------------- *
 * xstrfmt  --  bit like asprintf, but without undefined state on error
 * ------------------------------------------------------------------------- */

char *
xstrfmt(const char *fmt, ...)
{
  char *res = 0;
  va_list va;
  va_start(va, fmt);
  if( vasprintf(&res, fmt, va) < 0 )
  {
    res = 0;
  }
  va_end(va);

  return res;
}
