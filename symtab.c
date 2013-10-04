
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#include "symtab.h"

/* ========================================================================= *
 * symtab_t  --  methods
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * symtab_insert
 * ------------------------------------------------------------------------- */

void *
symtab_insert(symtab_t *self, const void *key)
{
  size_t l = 0;
  size_t h = self->st_count;

  while( l < h )
  {
    size_t i = (l + h) / 2;
    void  *p = self->st_elem[i];
    int    r = strcmp(self->st_key(p), key);

    if( r < 0 ) { l = i + 1; continue; }
    if( r > 0 ) { h = i + 0; continue; }
    return p;
  }

  if( self->st_count == self->st_alloc )
  {
    if( self->st_alloc < 16 )
    {
      self->st_alloc = 16;
    }
    else
    {
      self->st_alloc = self->st_alloc * 3 / 2;
    }
    self->st_elem = realloc(self->st_elem,
                            self->st_alloc * sizeof *self->st_elem);
  }

  for( h = self->st_count++; h > l; --h )
  {
    self->st_elem[h] = self->st_elem[h-1];
  }

  return self->st_elem[l] = self->st_new(key);
}

/* ------------------------------------------------------------------------- *
 * symtab_lookup
 * ------------------------------------------------------------------------- */

void *
symtab_lookup(const symtab_t *self, const void *key)
{
  size_t l = 0;
  size_t h = self->st_count;

  while( l < h )
  {
    size_t i = (l + h) / 2;
    void  *p = self->st_elem[i];
    int    r = strcmp(self->st_key(p), key);

    if( r < 0 ) { l = i + 1; continue; }
    if( r > 0 ) { h = i + 0; continue; }
    return p;
  }

  return 0;
}

/* ------------------------------------------------------------------------- *
 * symtab_remove
 * ------------------------------------------------------------------------- */

void
symtab_remove(symtab_t *self, const void *key)
{
  size_t l = 0;
  size_t h = self->st_count;

  while( l < h )
  {
    size_t i = (l + h) / 2;
    void  *p = self->st_elem[i];
    int    r = strcmp(self->st_key(p), key);

    if( r < 0 ) { l = i + 1; continue; }
    if( r > 0 ) { h = i + 0; continue; }

    self->st_del(p);

    for( h = --self->st_count; i < h; ++i )
    {
      self->st_elem[i] = self->st_elem[i+1];
    }
    break;
  }
}

/* ------------------------------------------------------------------------- *
 * symtab_clear
 * ------------------------------------------------------------------------- */

void
symtab_clear(symtab_t *self)
{
  if( self->st_del != 0 )
  {
    for( size_t i = 0; i < self->st_count; ++i )
    {
      self->st_del(self->st_elem[i]);
    }
  }
  self->st_count = 0;
}

/* ------------------------------------------------------------------------- *
 * symtab_ctor
 * ------------------------------------------------------------------------- */

void
symtab_ctor(symtab_t *self,
            symtab_new_fn new,
            symtab_del_fn del,
            symtab_key_fn key)
{
  self->st_count = 0;
  self->st_alloc = 0;
  self->st_elem  = 0;
  self->st_new   = new;
  self->st_key   = key;
  self->st_del   = del;
}

/* ------------------------------------------------------------------------- *
 * symtab_dtor
 * ------------------------------------------------------------------------- */

void
symtab_dtor(symtab_t *self)
{
  symtab_clear(self);
  free(self->st_elem);
}

/* ------------------------------------------------------------------------- *
 * symtab_create
 * ------------------------------------------------------------------------- */

symtab_t *
symtab_create(symtab_new_fn new,
              symtab_del_fn del,
              symtab_key_fn key)
{
  symtab_t *self = calloc(1, sizeof *self);
  symtab_ctor(self, new, del, key);
  return self;
}

/* ------------------------------------------------------------------------- *
 * symtab_delete
 * ------------------------------------------------------------------------- */

void
symtab_delete(symtab_t *self)
{
  if( self != 0 )
  {
    symtab_dtor(self);
    free(self);
  }
}

/* ------------------------------------------------------------------------- *
 * symtab_delete_cb
 * ------------------------------------------------------------------------- */

void
symtab_delete_cb(void *self)
{
  symtab_delete(self);
}
