
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

#include "inifile.h"
#include "logging.h"
#include "unique.h"

#include <ctype.h>
#include <errno.h>

/* ========================================================================= *
 * inival_t  --  methods
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * inival_emit
 * ------------------------------------------------------------------------- */

int
inival_emit(const inival_t *self, FILE *file)
{
  int rc = fprintf(file, "%s%c%s\n", self->iv_key, SEP, self->iv_val);
  return (rc < 0) ? -1 : 0;
}

/* ------------------------------------------------------------------------- *
 * inival_set
 * ------------------------------------------------------------------------- */

void
inival_set(inival_t *self, const char *val)
{
  xstrset(&self->iv_val, val);
}

/* ------------------------------------------------------------------------- *
 * inival_create
 * ------------------------------------------------------------------------- */

inival_t *
inival_create(const char *key, const char *val)
{
  inival_t *self = calloc(1, sizeof *self);

  self->iv_key = strdup(key ?: "");
  self->iv_val = strdup(val ?: "");

  return self;
}

/* ------------------------------------------------------------------------- *
 * inival_delete
 * ------------------------------------------------------------------------- */

void
inival_delete(inival_t *self)
{
  if( self != 0 )
  {
    free(self->iv_key);
    free(self->iv_val);
    free(self);
  }
}

/* ------------------------------------------------------------------------- *
 * inival_compare
 * ------------------------------------------------------------------------- */

int
inival_compare(const inival_t *self, const char *key)
{
  return xstrsame(self->iv_key, key);
}

/* ------------------------------------------------------------------------- *
 * inival_delete_cb
 * ------------------------------------------------------------------------- */

void
inival_delete_cb(void *self)
{
  inival_delete(self);
}

/* ------------------------------------------------------------------------- *
 * inival_create_cb
 * ------------------------------------------------------------------------- */

static
void *
inival_create_cb(const char *key)
{
  return inival_create(key, "");
}

/* ------------------------------------------------------------------------- *
 * inival_getkey_cb
 * ------------------------------------------------------------------------- */

static
const char *
inival_getkey_cb(const void *self)
{
  return inival_get_key(self);
}

/* ========================================================================= *
 * inisec_t  --  methods
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * inisec_ctor
 * ------------------------------------------------------------------------- */

void
inisec_ctor(inisec_t *self)
{
  self->is_name   = 0;

  symtab_ctor(&self->is_values,
              inival_create_cb,
              inival_delete_cb,
              inival_getkey_cb);
}

/* ------------------------------------------------------------------------- *
 * inisec_dtor
 * ------------------------------------------------------------------------- */

void
inisec_dtor(inisec_t *self)
{
  symtab_dtor(&self->is_values);

  free(self->is_name);
}

/* ------------------------------------------------------------------------- *
 * inisec_create
 * ------------------------------------------------------------------------- */

inisec_t *
inisec_create(const char *name)
{
  inisec_t *self = calloc(1, sizeof *self);
  inisec_ctor(self);

  inisec_set_name(self, name);

  return self;
}

/* ------------------------------------------------------------------------- *
 * inisec_delete
 * ------------------------------------------------------------------------- */

void
inisec_delete(inisec_t *self)
{
  if( self != 0 )
  {
    inisec_dtor(self);
    free(self);
  }
}

/* ------------------------------------------------------------------------- *
 * inisec_compare
 * ------------------------------------------------------------------------- */

int
inisec_compare(const inisec_t *self, const char *name)
{
  return xstrsame(self->is_name, name);
}

/* ------------------------------------------------------------------------- *
 * inisec_compare_cb
 * ------------------------------------------------------------------------- */

int
inisec_compare_cb(const void *self, const void *name)
{
  return inisec_compare(self, name);
}

/* ------------------------------------------------------------------------- *
 * inisec_delete_cb
 * ------------------------------------------------------------------------- */

void
inisec_delete_cb(void *self)
{
  inisec_delete(self);
}

/* ------------------------------------------------------------------------- *
 * inisec_getkey_cb
 * ------------------------------------------------------------------------- */

static
const char *
inisec_getkey_cb(const void *self)
{
  return inisec_get_name(self);
}

/* ------------------------------------------------------------------------- *
 * inisec_create_cb
 * ------------------------------------------------------------------------- */

static
void *
inisec_create_cb(const char *name)
{
  return inisec_create(name);
}

/* ------------------------------------------------------------------------- *
 * inisec_set
 * ------------------------------------------------------------------------- */

void
inisec_set(inisec_t *self, const char *key, const char *val)
{
  inival_t *res = symtab_insert(&self->is_values, key);
  inival_set(res, val);
}

/* ------------------------------------------------------------------------- *
 * inisec_get
 * ------------------------------------------------------------------------- */

const char *
inisec_get(inisec_t *self, const char *key, const char *val)
{
  inival_t *res = symtab_lookup(&self->is_values, key);
  return res ? res->iv_val : val;
}

/* ------------------------------------------------------------------------- *
 * inisec_has
 * ------------------------------------------------------------------------- */

int
inisec_has(inisec_t *self, const char *key)
{
  return symtab_lookup(&self->is_values, key) != 0;
}

/* ------------------------------------------------------------------------- *
 * inisec_del
 * ------------------------------------------------------------------------- */

void
inisec_del(inisec_t *self, const char *key)
{
  symtab_remove(&self->is_values, key);
}

/* ------------------------------------------------------------------------- *
 * inisec_emit
 * ------------------------------------------------------------------------- */

int
inisec_emit(const inisec_t *self, FILE *file)
{
  int err = -1;

  if( fprintf(file, "%c%s%c\n\n", BRA, self->is_name, KET) < 0 ) goto cleanup;

  for( size_t i = 0; i < self->is_values.st_count; ++i )
  {
    if( inival_emit(self->is_values.st_elem[i], file) < 0 ) goto cleanup;
  }

  if( fprintf(file, "\n") < 0 ) goto cleanup;

  err = 0;
  cleanup:
  return err;
}

/* ========================================================================= *
 * inifile_t  --  methods
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * inifile_get_path
 * ------------------------------------------------------------------------- */

const char *
inifile_get_path(inifile_t *self)
{
  return self->if_path;
}

/* ------------------------------------------------------------------------- *
 * inifile_set_path
 * ------------------------------------------------------------------------- */

void
inifile_set_path(inifile_t *self, const char *path)
{
  xstrset(&self->if_path, path);
}

/* ------------------------------------------------------------------------- *
 * inifile_ctor
 * ------------------------------------------------------------------------- */

void
inifile_ctor(inifile_t *self)
{
  self->if_path = 0;

  symtab_ctor(&self->if_sections,
              inisec_create_cb,
              inisec_delete_cb,
              inisec_getkey_cb);
}

/* ------------------------------------------------------------------------- *
 * inifile_dtor
 * ------------------------------------------------------------------------- */

void
inifile_dtor(inifile_t *self)
{
  symtab_dtor(&self->if_sections);

  free(self->if_path);
}

/* ------------------------------------------------------------------------- *
 * inifile_create
 * ------------------------------------------------------------------------- */

inifile_t *
inifile_create(void)
{
  inifile_t *self = calloc(1, sizeof *self);
  inifile_ctor(self);
  return self;
}

/* ------------------------------------------------------------------------- *
 * inifile_delete
 * ------------------------------------------------------------------------- */

void
inifile_delete(inifile_t *self)
{
  if( self != 0 )
  {
    inifile_dtor(self);
    free(self);
  }
}

/* ------------------------------------------------------------------------- *
 * inifile_delete_cb
 * ------------------------------------------------------------------------- */

void
inifile_delete_cb(void *self)
{
  inifile_delete(self);
}

/* ------------------------------------------------------------------------- *
 * inifile_has_section
 * ------------------------------------------------------------------------- */

int
inifile_has_section(const inifile_t *self, const char *sec)
{
  return symtab_lookup(&self->if_sections, sec) != 0;
}

/* ------------------------------------------------------------------------- *
 * inifile_get_section
 * ------------------------------------------------------------------------- */

inisec_t *
inifile_get_section(const inifile_t *self, const char *sec)
{
  return symtab_lookup(&self->if_sections, sec);
}

/* ------------------------------------------------------------------------- *
 * inifile_add_section
 * ------------------------------------------------------------------------- */

inisec_t *
inifile_add_section(inifile_t *self, const char *sec)
{
  return symtab_insert(&self->if_sections, sec);
}

/* ------------------------------------------------------------------------- *
 * inifile_del_section
 * ------------------------------------------------------------------------- */

void
inifile_del_section(inifile_t *self, const char *sec)
{
  symtab_remove(&self->if_sections, sec);
}

/* ------------------------------------------------------------------------- *
 * inifile_set
 * ------------------------------------------------------------------------- */

void
inifile_set(inifile_t *self, const char *sec, const char *key, const char *val)
{
  inisec_set(inifile_add_section(self, sec), key, val);
}

/* ------------------------------------------------------------------------- *
 * inifile_setfmt
 * ------------------------------------------------------------------------- */

void
inifile_setfmt(inifile_t *self, const char *sec, const char *key, const char *fmt, ...)
{
  va_list va;
  char *val = 0;

  va_start(va, fmt);
  if( vasprintf(&val, fmt, va) < 0 )
  {
    val = 0;
  }
  va_end(va);

  if( val != 0 )
  {
    inisec_set(inifile_add_section(self, sec), key, val);
    free(val);
  }
}

/* ------------------------------------------------------------------------- *
 * inifile_get
 * ------------------------------------------------------------------------- */

const char *
inifile_get(inifile_t *self, const char *sec, const char *key, const char *val)
{
  inisec_t *s = symtab_lookup(&self->if_sections, sec);
  return s ? inisec_get(s, key, val) : val;
}

/* ------------------------------------------------------------------------- *
 * inifile_getfmt
 * ------------------------------------------------------------------------- */

int
inifile_getfmt(inifile_t *self, const char *sec, const char *key, const char *fmt, ...)
{
  const char *val = inifile_get(self, sec, key, "");
  int         res = 0;
  va_list     va;

  va_start(va, fmt);
  res = vsscanf(val, fmt, va);
  va_end(va);

  return res;
}

/* ------------------------------------------------------------------------- *
 * inifile_del
 * ------------------------------------------------------------------------- */

void
inifile_del(inifile_t *self, const char *sec, const char *key)
{
  inisec_t *s = symtab_lookup(&self->if_sections, sec);
  if( s != 0 )
  {
    inisec_del(s, key);
  }
}

/* ------------------------------------------------------------------------- *
 * inifile_has
 * ------------------------------------------------------------------------- */

int
inifile_has(inifile_t *self, const char *sec, const char *key)
{
  inisec_t *s = symtab_lookup(&self->if_sections, sec);
  return s ? inisec_has(s, key) : 0;
}

/* ------------------------------------------------------------------------- *
 * inifile_emit
 * ------------------------------------------------------------------------- */

int
inifile_emit(const inifile_t *self, FILE *file)
{
  int err = 0;
  for( size_t i = 0; i < self->if_sections.st_count; ++i )
  {
    if( inisec_emit(self->if_sections.st_elem[i], file) < 0 )
    {
      err = -1; break;
    }
  }
  return err;
}

/* ------------------------------------------------------------------------- *
 * inifile_save
 * ------------------------------------------------------------------------- */

int
inifile_save(const inifile_t *self, const char *path)
{
  int   err  = -1;
  FILE *file = 0;

  if( (file = fopen(path, "w")) == 0 )
  {
    log_err("%s: inisave/open: %s\n", path, strerror(errno));
    goto cleanup;
  }

  if( inifile_emit(self, file) < 0 )
  {
    log_err("%s: inisave/write: %s\n", path, strerror(errno));
    goto cleanup;
  }

  err = 0;

  cleanup:

  if( file != 0 && fclose(file) == EOF )
  {
    log_err("%s: inisave/close: %s\n", path, strerror(errno));
    err = -1;
  }
  return err;
}

/* ------------------------------------------------------------------------- *
 * inifile_save_to_memory
 * ------------------------------------------------------------------------- */

int
inifile_save_to_memory(const inifile_t *self, char **pdata, size_t *psize,
                       const char *comment, size_t minsize)
{
  int         err  = -1;
  FILE       *file = 0;
  const char *path = "<ram>";

  if( (file = open_memstream (pdata, psize)) == 0 )
  {
    log_err("%s: inisave/open: %s\n", path, strerror(errno));
    goto cleanup;
  }

  if( comment != 0 && fprintf(file, "# %s\n", comment) < 0 )
  {
    log_err("%s: inisave/comment: %s\n", path, "error");
    goto cleanup;
  }

  if( inifile_emit(self, file) < 0 )
  {
    log_err("%s: inisave/write: %s\n", path, strerror(errno));
    goto cleanup;
  }

  for( ;; )
  {
    static const char pad[] =
    "#######################################"
    "#######################################";

    long size = ftell(file);

    if( size < 0 || size >= (long)minsize )
    {
      break;
    }

    size_t todo = minsize - (size_t)size;

    if( todo >= sizeof pad ) todo = sizeof pad - 1;

    if( fprintf(file, "%.*s\n", (int)(todo-1), pad) < 0 )
    {
      log_err("%s: inisave/padding: %s\n", path, "error");
      goto cleanup;
    }
  }

  err = 0;

  cleanup:

  if( file != 0 && fclose(file) == EOF )
  {
    log_err("%s: inisave/close: %s\n", path, strerror(errno));
    err = -1;
  }

  return err;
}

/* ------------------------------------------------------------------------- *
 * inifile_load
 * ------------------------------------------------------------------------- */

int
inifile_load(inifile_t *self, const char *path)
{
  int     err  = -1;
  FILE   *file = 0;
  size_t  size = 0;
  char   *data = 0;

  inisec_t *sec = 0;
  char     *key = 0;
  char     *val = 0;

  if( (file = fopen(path, "r")) == 0 )
  {
    log_debug("%s: iniload/open: %s\n", path, strerror(errno));
    goto cleanup;
  }

  while( getline(&data, &size, file) != -1 )
  {
    char *pos = xstrip(data);

    if( *pos == 0 ) continue;

    if( *pos == '#' ) continue;

    if( *pos == BRA )
    {
      char *name = xsplit(pos+1, 0, KET);
      sec = inifile_add_section(self, xstripall(name));
      continue;
    }

    key = xsplit(pos, &val, SEP);
    xstripall(key);
    xstrip(val);

    if( sec && *key )
    {
      inisec_set(sec, key, val);
    }
  }

  err = 0;

  cleanup:

  free(data);

  if( file != 0 && fclose(file) == EOF )
  {
    log_err("%s: iniload/close: %s\n", path, strerror(errno));
    err = -1;
  }

  return err;
}

/* ------------------------------------------------------------------------- *
 * inifile_scan_sections
 * ------------------------------------------------------------------------- */

inisec_t *
inifile_scan_sections(const inifile_t *self,
                      int (*cb)(const inisec_t*, void*),
                      void *aptr)
{
  for( size_t i = 0; i < self->if_sections.st_count; ++i )
  {
    inisec_t *sec = self->if_sections.st_elem[i];
    if( cb(sec, aptr) != 0 ) return sec;
  }
  return 0;
}

/* ------------------------------------------------------------------------- *
 * inifile_scan_values
 * ------------------------------------------------------------------------- */

inival_t *
inifile_scan_values(const inifile_t *self,
                    int (*cb)(const inisec_t *, const inival_t*, void*),
                    void *aptr)
{
  for( size_t i = 0; i < self->if_sections.st_count; ++i )
  {
    inisec_t *sec = self->if_sections.st_elem[i];

    for( size_t k = 0; k < sec->is_values.st_count; ++k )
    {
      inival_t *val = sec->is_values.st_elem[k];

      if( cb(sec, val, aptr) != 0 ) return val;
    }
  }
  return 0;
}

/* ------------------------------------------------------------------------- *
 * inifile_get_section_names
 * ------------------------------------------------------------------------- */
static int inifile_get_section_names_cb(const inisec_t *sec, void *aptr)
{
  unique_t *unique = aptr;
  unique_add(unique, sec->is_name);
  return 0;
}

char **
inifile_get_section_names(const inifile_t *self, size_t *pcount)
{
  unique_t unique;

  unique_ctor(&unique);
  inifile_scan_sections(self, inifile_get_section_names_cb, &unique);
  char **v = unique_steal(&unique, pcount);
  unique_dtor(&unique);

  return v;
}

/* ------------------------------------------------------------------------- *
 * inifile_get_value_keys
 * ------------------------------------------------------------------------- */
static int inifile_get_value_keys_cb(const inisec_t *sec,
                                     const inival_t *val, void *aptr)
{
  (void)sec;

  unique_t *unique = aptr;
  unique_add(unique, val->iv_key);
  return 0;
}

char **
inifile_get_value_keys(const inifile_t *self, size_t *pcount)
{
  unique_t unique;
  unique_ctor(&unique);
  inifile_scan_values(self, inifile_get_value_keys_cb, &unique);
  char **names = unique_steal(&unique, pcount);
  unique_dtor(&unique);
  return names;
}

/* ------------------------------------------------------------------------- *
 * inifile_to_csv
 * ------------------------------------------------------------------------- */

static int inifile_to_csv_cb(const inisec_t *s, const inival_t *v, void *aptr)
{
  FILE *file = aptr;
  fprintf(file, "%s;%s;%s\n", s->is_name, v->iv_key, v->iv_val);
  return 0;
}

void
inifile_to_csv(const inifile_t *self)
{
  inifile_scan_values(self, inifile_to_csv_cb, stderr);
}

/* ------------------------------------------------------------------------- *
 * inifile_get_section_keys
 * ------------------------------------------------------------------------- */

char **
inifile_get_section_keys(const inifile_t *self, const char *sec_name,
			 int *pcount)
{
  char **res = 0;
  int    cnt = 0;

  inisec_t *sec = inifile_get_section(self, sec_name);

  if( sec )
  {
    res = calloc(sec->is_values.st_count + 1, sizeof *res);
    for( size_t i = 0; i < sec->is_values.st_count; ++i )
    {
      inival_t *val =  sec->is_values.st_elem[i];
      res[cnt++] = strdup(val->iv_key);
    }
    res[cnt] = 0;
  }

  if( pcount ) *pcount = cnt;

  return res;
}
