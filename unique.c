
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "unique.h"

/* ========================================================================= *
 * unique_t  --  methods
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * unique_ctor
 * ------------------------------------------------------------------------- */

void
unique_ctor(unique_t *self)
{
  self->un_count = 0;
  self->un_alloc = 64;
  self->un_string = malloc(self->un_alloc * sizeof *self->un_string);
  *self->un_string = 0;
}

/* ------------------------------------------------------------------------- *
 * unique_dtor
 * ------------------------------------------------------------------------- */

void
unique_dtor(unique_t *self)
{
  for( size_t i = 0; i < self->un_count; ++i )
  {
    free(self->un_string[i]);
  }
  free(self->un_string);
}

/* ------------------------------------------------------------------------- *
 * unique_create
 * ------------------------------------------------------------------------- */

unique_t *
unique_create(void)
{
  unique_t *self = calloc(1, sizeof *self);
  unique_ctor(self);
  return self;
}

/* ------------------------------------------------------------------------- *
 * unique_delete
 * ------------------------------------------------------------------------- */

void
unique_delete(unique_t *self)
{
  if( self != 0 )
  {
    unique_dtor(self);
    free(self);
  }
}

/* ------------------------------------------------------------------------- *
 * unique_delete_cb
 * ------------------------------------------------------------------------- */

void
unique_delete_cb(void *self)
{
  unique_delete(self);
}

/* ------------------------------------------------------------------------- *
 * unique_final
 * ------------------------------------------------------------------------- */

char **
unique_final(unique_t *self, size_t *pcount)
{
  auto int cmp(const void *a, const void *b);

  auto int cmp(const void *a, const void *b)
  {
    return strcmp(*(const char **)a, *(const char **)b);
  }

  if( self->un_dirty != 0 )
  {
    qsort(self->un_string, self->un_count, sizeof *self->un_string, cmp);

    size_t si = 0, di = 0;

    char *prev = NULL;

    while( si < self->un_count )
    {
      char *curr = self->un_string[si++];
      if( prev == 0 || strcmp(prev, curr) )
      {
        self->un_string[di++] = prev = curr;
      }
      else
      {
        free(curr);
      }
    }
    self->un_count = di;
    self->un_dirty = 0;
    self->un_string[self->un_count] = 0;
  }

  if( pcount != 0 )
  {
    *pcount = self->un_count;
  }

  return self->un_string;
}

/* ------------------------------------------------------------------------- *
 * unique_steal
 * ------------------------------------------------------------------------- */

char **
unique_steal(unique_t *self, size_t *pcount)
{
  char **res = unique_final(self, pcount);
  self->un_count  = 0;
  self->un_alloc  = 0;
  self->un_dirty  = 0;
  self->un_string = 0;
  return res;
}

/* ------------------------------------------------------------------------- *
 * unique_add
 * ------------------------------------------------------------------------- */

void
unique_add(unique_t *self, const char *str)
{
  if( self->un_count + 2 > self->un_alloc )
  {
    if( self->un_alloc == 0 )
    {
      self->un_alloc = 32;
    }
    else
    {
      self->un_alloc = self->un_alloc * 3 / 2;
    }
    self->un_string = realloc(self->un_string,
                              self->un_alloc * sizeof *self->un_string);
  }

  self->un_string[self->un_count++] = strdup(str ?: "");
  self->un_string[self->un_count] = 0;
  self->un_dirty = 1;
}
