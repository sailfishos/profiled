
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

#ifndef SYMTAB_H_
# define SYMTAB_H_

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif

typedef struct symtab_t symtab_t;

typedef void       *(*symtab_new_fn)(const char*);
typedef const char *(*symtab_key_fn)(const void*);
typedef void        (*symtab_del_fn)(void*);

/* ------------------------------------------------------------------------- *
 * symtab_t
 * ------------------------------------------------------------------------- */

struct symtab_t
{
  size_t  st_count;
  size_t  st_alloc;
  void  **st_elem;

  symtab_new_fn  st_new;
  symtab_del_fn  st_del;
  symtab_key_fn  st_key;
};

void     *symtab_insert   (symtab_t *self, const void *key);
void     *symtab_lookup   (const symtab_t *self, const void *key);
void      symtab_remove   (symtab_t *self, const void *key);
void      symtab_clear    (symtab_t *self);
void      symtab_ctor     (symtab_t *self,
                           symtab_new_fn new,
                           symtab_del_fn del,
                           symtab_key_fn key);
void      symtab_dtor     (symtab_t *self);
symtab_t *symtab_create   (symtab_new_fn new,
                           symtab_del_fn del,
                           symtab_key_fn key);
void      symtab_delete   (symtab_t *self);
void      symtab_delete_cb(void *self);

# ifdef __cplusplus
};
# endif

#endif /* SYMTAB_H_ */
