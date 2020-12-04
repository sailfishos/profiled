
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

#ifndef DATABASE_H_
# define DATABASE_H_

# include "profileval.h"

# define FS ""

# ifndef FS
#  define FS "fsroot"
# endif

# define CONFIG_DIR  FS"/etc/profiled"

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif

int             database_init                 (void);
void            database_quit                 (void);

char          **database_get_profiles         (int *pcount);
void            database_free_profiles        (char **profiles);

const char     *database_get_profile          (void);
const char     *database_get_previous         (void);
int             database_set_profile          (const char *profile);
int             database_has_profile          (const char *profile);

char          **database_get_keys             (int *pcount);
void            database_free_keys            (char **keys);

int             database_has_value            (const char *key);
int             database_is_writable          (const char *key);
const char     *database_get_value            (const char *profile, const char *key, const char *val);
int             database_set_value            (const char *profile, const char *key, const char *val);
const char     *database_get_type             (const char *key, const char *def);
profileval_t   *database_get_values           (const char *profile, int *pcount);
void            database_free_values          (profileval_t *values);

void            database_set_changed_cb       (void (*cb)(void));
void            database_clear_changes        (void);

char          **database_get_changed_profiles (int *pcount);
void            database_free_changed_profiles(char **profiles);

profileval_t   *database_get_changed_values   (const char *profile, int *pcount);
void            database_free_changed_values  (profileval_t *values);

void            database_reload(void);

void            database_set_restart_request_cb(void (*cb)(void));

# ifdef __cplusplus
};
# endif

#endif /* DATABASE_H_ */
