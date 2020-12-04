
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

#ifndef XUTIL_H_
# define XUTIL_H_

# include <sys/types.h>
# include <sys/stat.h>

# include <stdlib.h>
# include <string.h>

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif

int  xexists(const char *path);
int  xloadfile(const char *path, char **pdata, size_t *psize);
int  xsavefile(const char *path, int mode, const void *data, size_t size);
int  xcyclefiles(const char *temp, const char *path, const char *back);
void xfetchstats(const char *path, struct stat *cur);
int  xcheckstats(const char *path, const struct stat *old);
char *xstrfmt(const char *fmt, ...) __attribute__ ((format (printf, 1, 2)));

static inline int xiswhite(int c)
{
  return (c > 0) && (c <= 32);
}

static inline int xisblack(int c)
{
  return (unsigned)c > 32u;
}

static inline char *xstripall(char *str)
{
  // trim white space at start & end of string
  // compress whitespace in middle to one space

  char *s = str;
  char *d = str;

  for( ;; )
  {
    while( xisblack(*s) ) { *d++ = *s++; }

    while( xiswhite(*s) ) { ++s; }

    if( *s == 0 )
    {
      *d = 0;
      return str;
    }

    *d++ = ' ';
  }
}

static inline char *xstrip(char *str)
{
  // trim white space at start & end of string
  char *s = str;
  char *d = str;

  while( xiswhite(*s) ) { ++s; }

  char *e = str;

  while( *s )
  {
    if( xisblack(*s) ) e = d+1;
    *d++ = *s++;
  }
  *e = 0;
  return str;
}

static inline char *xsplit(char *pos, char **ppos, int sep)
{
  char *res = pos;

  for( ; ; ++pos )
  {
    if( *pos == sep )
    {
      if( sep != 0 ) *pos++ = 0;
      break;
    }
    if( *pos == 0 )
    {
      break;
    }
  }

  if( ppos != 0 ) *ppos = pos;

  return res;
}

static inline void xstrset(char **pdst, const char *src)
{
  char *tmp = src ? strdup(src) : 0;
  free(*pdst);
  *pdst = tmp;
}

static inline void xfreev(char **v)
{
  if( v != 0 )
  {
    for( size_t i = 0; v[i]; ++i )
    {
      free(v[i]);
    }
    free(v);
  }
}

static inline int xendswith(const char *str, const char *end)
{
  int ns = strlen(str);
  int ne = strlen(end);
  return (ns >= ne) && !strcmp(str+ns-ne, end);
}

static inline int xstrnull(const char *s)
{
  return !s || !*s;
}

static inline int xstrsame(const char *s1, const char *s2)
{
  return (!s1 || !s2) ? (s1 == s2) : !strcmp(s1,s2);
}

static inline int xisempty(const char *s)
{
  return (s == 0) || (*s == 0);
}

# ifdef __cplusplus
};
# endif

#endif /* XUTIL_H_ */
