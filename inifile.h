
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

#ifndef INIFILE_H_
# define INIFILE_H_

# include <stdio.h>

# include "xutil.h"
# include "symtab.h"

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif

enum {
  BRA = '[',
  KET = ']',
  SEP = '=',
  //SEP = ':',
};

typedef struct inifile_t inifile_t;
typedef struct inisec_t  inisec_t;
typedef struct inival_t  inival_t;

/* ------------------------------------------------------------------------- *
 * inival_t
 * ------------------------------------------------------------------------- */

struct inival_t
{
  char *iv_key;
  char *iv_val;
};

static inline const char *inival_get_key(const inival_t *self)
{
  return self->iv_key;
}
static inline const char *inival_get_val(const inival_t *self)
{
  return self->iv_val;
}

int       inival_emit      (const inival_t *self, FILE *file);
void      inival_set       (inival_t *self, const char *val);
inival_t *inival_create    (const char *key, const char *val);
void      inival_delete    (inival_t *self);
int       inival_compare   (const inival_t *self, const char *key);
int       inival_compare_cb(const void *self, const void *key);
void      inival_delete_cb (void *self);

/* ------------------------------------------------------------------------- *
 * inisec_t
 * ------------------------------------------------------------------------- */

struct inisec_t
{
  char      *is_name;
  symtab_t   is_values;
};

void        inisec_ctor      (inisec_t *self);
void        inisec_dtor      (inisec_t *self);
inisec_t   *inisec_create    (const char *name);
void        inisec_delete    (inisec_t *self);
int         inisec_compare   (const inisec_t *self, const char *name);
int         inisec_compare_cb(const void *self, const void *name);
void        inisec_delete_cb (void *self);
void        inisec_set       (inisec_t *self, const char *key, const char *val);
const char *inisec_get       (inisec_t *self, const char *key, const char *val);
int         inisec_has       (inisec_t *self, const char *key);
void        inisec_del       (inisec_t *self, const char *key);
int         inisec_emit      (const inisec_t *self, FILE *file);

static inline void inisec_set_name(inisec_t *self, const char *name)
{
  xstrset(&self->is_name, name);
}

static inline const char *inisec_get_name(const inisec_t *self)
{
  return self->is_name;
}

/* ------------------------------------------------------------------------- *
 * inifile_t
 * ------------------------------------------------------------------------- */

struct inifile_t
{
  char      *if_path;
  symtab_t   if_sections;
};

const char * inifile_get_path         (inifile_t *self);
void         inifile_set_path         (inifile_t *self, const char *path);
void         inifile_ctor             (inifile_t *self);
void         inifile_dtor             (inifile_t *self);
inifile_t  * inifile_create           (void);
void         inifile_delete           (inifile_t *self);
void         inifile_delete_cb        (void *self);
int          inifile_has_section      (const inifile_t *self, const char *sec);
inisec_t   * inifile_get_section      (const inifile_t *self, const char *sec);
inisec_t   * inifile_add_section      (inifile_t *self, const char *sec);
void         inifile_del_section      (inifile_t *self, const char *sec);
void         inifile_set              (inifile_t *self, const char *sec, const char *key, const char *val);
void         inifile_setfmt           (inifile_t *self, const char *sec, const char *key, const char *fmt, ...);
const char * inifile_get              (inifile_t *self, const char *sec, const char *key, const char *val);
int          inifile_getfmt           (inifile_t *self, const char *sec, const char *key, const char *fmt, ...);
void         inifile_del              (inifile_t *self, const char *sec, const char *key);
int          inifile_has              (inifile_t *self, const char *sec, const char *key);
int          inifile_emit             (const inifile_t *self, FILE *file);
int          inifile_save             (const inifile_t *self, const char *path);
int          inifile_load             (inifile_t *self, const char *path);
int          inifile_save_to_memory   (const inifile_t *self, char **pdata, size_t *psize, const char *comment, size_t minsize);
inisec_t   * inifile_scan_sections    (const inifile_t *self, int (*cb)(const inisec_t*, void*), void *aptr);
inival_t   * inifile_scan_values      (const inifile_t *self, int (*cb)(const inisec_t *, const inival_t*, void*), void *aptr);
char       **inifile_get_section_names(const inifile_t *self, size_t *pcount);
char       **inifile_get_value_keys   (const inifile_t *self, size_t *pcount);
void         inifile_to_csv           (const inifile_t *self);
char       **inifile_get_section_keys (const inifile_t *self, const char *sec_name, int *pcount);

# ifdef __cplusplus
};
# endif

#endif /* INIFILE_H_ */
