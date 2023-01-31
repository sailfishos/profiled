
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

#include "profiled_config.h"

#include "database.h"
#include "logging.h"
#include "inifile.h"
#include "unique.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <glob.h>
#include <unistd.h>
#include <errno.h>
#include <glib.h>

/* ========================================================================= *
 * Configuration
 * ========================================================================= */

#define CUSTOM_INI   "custom.ini"
#define CURRENT_TXT  "current"

#define OVERRIDE "override"
#define FALLBACK "fallback"
#define DATATYPE "datatype"

enum
{
  RETRY_SAVE_PERIOD = 60, /* seconds between retries, when data
                           * saving fails (disk full etc) */

  MINIMUM_CUSTOM_INI_SIZE = 4<<10, /* the file will be padded to this
                                    * length so that we have some reserve
                                    * space available on filesystem full
                                    * situations */
};

/* ========================================================================= *
 * Module Data
 * ========================================================================= */

static char *database_current  = 0; // currently active profile
static char *database_previous = 0; // previously this was active

static inifile_t *database_static  = 0; // loaded from CONFIG_DIR/*.ini
static inifile_t *database_custom  = 0; // stored at CUSTOM_INI

static char *custom_work = 0; // path to custom values save file
static char *custom_path = 0;
static char *custom_back = 0;

static char *current_work = 0; // path to current profile name save file
static char *current_path = 0;
static char *current_back = 0;

// stat() info for custom data files, used for detecting situations
// where somebody else than profiled changes the files
static struct stat database_custom_stat;
static struct stat database_current_stat;

// sections that can be modified only via configuration files
char const* const database_specials[] =
{
  OVERRIDE, FALLBACK, DATATYPE, 0
};

// names of profiles that will be always available
char const* const database_builtins[] =
{
  "general",
  "silent",
  "meeting",
  "outdoors",
  0
};

static void database_generate_changes(void);

/* ========================================================================= *
 * Module Callbacks
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * database_restart_request_cb  --  called when custom data has been changed
 * by something else than profiled itself (normally: restoring a backup)
 * ------------------------------------------------------------------------- */

static void (*database_restart_request_cb)(void) = 0;

void
database_set_restart_request_cb(void (*cb)(void))
{
  database_restart_request_cb = cb;
}

static void
database_restart_request(void)
{
  if( database_restart_request_cb != 0 )
  {
    log_warning("database is requesting restart");
    database_restart_request_cb();
  }
}

/* ------------------------------------------------------------------------- *
 * database_changed_cb  --  called when profile data has been changed, and
 * change broadcast is needed
 * ------------------------------------------------------------------------- */

static void (*database_changed_cb)(void) = 0;

static inline void
database_notify_changes(void)
{
  if( database_changed_cb != 0 )
  {
    database_changed_cb();
  }
}

void
database_set_changed_cb(void (*cb)(void))
{
  database_changed_cb = cb;
}

/* ========================================================================= *
 * Helpers for Accessing Profile Values
 * ========================================================================= */

/* ------------------------------------------------------------------------- *
 * database_check_profile  --  NULL or empty profiles -> current
 * ------------------------------------------------------------------------- */

static inline void database_check_profile(const char **pprofile)
{
  if( xisempty(*pprofile) ) *pprofile = database_current;
}

/* ------------------------------------------------------------------------- *
 * database_is_special  --  check if profile name is reserved section name
 * ------------------------------------------------------------------------- */

static int database_is_special(const char *profile)
{
  for( int i = 0; database_specials[i]; ++i )
  {
    if( !strcmp(database_specials[i], profile) )
    {
      return 1;
    }
  }
  return 0;
}

/* ------------------------------------------------------------------------- *
 * database_is_builtin  --  check if profile name is one of builtin ones
 * ------------------------------------------------------------------------- */

static int database_is_builtin(const char *profile)
{
  for( int i = 0; database_builtins[i]; ++i )
  {
    if( !strcmp(database_builtins[i], profile) )
    {
      return 1;
    }
  }
  return 0;
}

/* ------------------------------------------------------------------------- *
 * datatype_  --  helper for getting datatype for key
 * ------------------------------------------------------------------------- */

static const char *datatype_(const char *key)
{
  return inifile_get(database_static, DATATYPE, key, NULL);
}

/* ------------------------------------------------------------------------- *
 * fallback_  --  helper for getting fallback value for key
 * ------------------------------------------------------------------------- */

static const char *fallback_(const char *key)
{
  return inifile_get(database_static, FALLBACK, key, NULL);
}

/* ------------------------------------------------------------------------- *
 * override_  -- helper for getting override value for key
 * ------------------------------------------------------------------------- */

static const char *override_(const char *key)
{
  return inifile_get(database_static, OVERRIDE, key, NULL);
}

/* ------------------------------------------------------------------------- *
 * config_  --  helper for getting configured value for key
 * ------------------------------------------------------------------------- */

static const char *config_(const char *profile, const char *key)
{
  return inifile_get(database_static, profile, key, NULL);
}

/* ------------------------------------------------------------------------- *
 * custom_  --  helper for getting user set value for key
 * ------------------------------------------------------------------------- */

static const char *custom_(const char *profile, const char *key)
{
  return inifile_get(database_custom, profile, key, NULL);
}

/* ------------------------------------------------------------------------- *
 * current_  --  helper for getting current value for key
 * ------------------------------------------------------------------------- */

static const char *current_(const char *profile, const char *key)
{
  static const char general[] = "general";

  /* - - - - - - - - - - - - - - - - - - - *
   * profile lookup order:
   * 1. config: "override"
   * 2. custom: caller provided
   * 3. config: caller provided
   * 4. custom: "general"
   * 5. config: "general"
   * 6. config: "fallback"
   * - - - - - - - - - - - - - - - - - - - */

  const char *res = 0;

  if( (res = override_(key)) )
  {
    // have override value
    goto cleanup;
  }

  if( !xstrnull(res = custom_(profile, key)) )
  {
    // have non-empty custom value for requested profile
    goto cleanup;
  }

  if( (res = config_(profile, key)) )
  {
    // have non-empty default value for requested profile
    goto cleanup;
  }

  if( !xstrsame(profile, general) )
  {
    if( !xstrnull(res = custom_(general, key)) )
    {
      // have non-empty custom value for general profile
      goto cleanup;
    }
    if( (res = config_(general, key)) )
    {
      // have non-empty default value for general profile
      goto cleanup;
    }
  }

  // use the fallback value
  res = fallback_(key);

cleanup:

  return res;
}

static const char *datadir(void)
{
  static gchar *path = NULL;

  if( !path )
  {
    path = g_strdup_printf("%s/profiled", g_get_user_config_dir() ?: "/tmp");
  }
  return path;
}

static const char *legacydir(void)
{
  static char path[256] = "";

  if( *path == 0 )
  {
    snprintf(path, sizeof path, "%s/.profiled", getenv("HOME") ?: "/tmp");
  }
  return path;
}

static int isdir(const char *path)
{
  struct stat st;
  return path && !stat(path, &st) && S_ISDIR(st.st_mode);
}

static int prepdir(char *path)
{
  if( !isdir(path) )
  {
    char *end = strrchr(path, '/');
    int   res = -1;

    if( end != 0 && end > path )
    {
      *end = 0;
      res = prepdir(path);
      *end = '/';
    }

    if( res == 0 )
    {
      if( mkdir(path, 0755) == -1 )
      {
        log_err("%s: mkdir: %s\n", path, strerror(errno));
      }
      return isdir(path) ? 0 : -1;
    }
    return -1;
  }
  return 0;
}

static int prepfile(char *path)
{
  char *end = strrchr(path, '/');
  int   res = 0;

  if( end != 0 && end > path )
  {
    *end = 0;
    res = prepdir(path);
    *end = '/';
  }
  return res;
}

/* ------------------------------------------------------------------------- *
 * database_load_config  --  load static profile data
 * ------------------------------------------------------------------------- */

static void
database_load_config(void)
{
  glob_t globbuf;

  glob(CONFIG_DIR"/[0-9][0-9].*.ini", GLOB_MARK, 0, &globbuf);
  for( size_t i = 0; i < globbuf.gl_pathc; ++i )
  {
    inifile_load(database_static, globbuf.gl_pathv[i]);
  }
  globfree(&globbuf);
}

/* ------------------------------------------------------------------------- *
 * database_load_custom  --  load custom profile data
 * ------------------------------------------------------------------------- */

static void
database_load_custom(void)
{
  inifile_load(database_custom, custom_path);
}

/* ------------------------------------------------------------------------- *
 * database_save_custom  --  save custom profile data
 * ------------------------------------------------------------------------- */

static int
database_save_custom_cb(const inisec_t *s, const inival_t *v, void *aptr)
{
  const char *prf = s->is_name;
  const char *key = v->iv_key;
  const char *val = v->iv_val;
  inifile_t  *ini = aptr;

  /* empty value means "use default inheritance" -> do not save them */
  if( !xstrnull(val) )
  {
    inifile_set(ini, prf, key, val);
  }
  return 0;
}

static int
database_save_custom(void)
{
  int     error    = -1;
  char   *old_data = 0;
  size_t  old_size = 0;
  char   *new_data = 0;
  size_t  new_size = 0;

  // make sure we have data directory
  if( prepfile(custom_work) == -1 )
  {
    goto cleanup;
  }

  // create new content
  inifile_t ini;
  inifile_ctor(&ini);
  inifile_scan_values(database_custom, database_save_custom_cb, &ini);
  inifile_save_to_memory(&ini, &new_data, &new_size,
                         "custom profile values",
                         MINIMUM_CUSTOM_INI_SIZE);
  inifile_dtor(&ini);

  // load old content
  xloadfile(custom_path, &old_data, &old_size);

  // write to flash only if needed
  if( old_size != new_size || memcmp(old_data, new_data, new_size) )
  {
    if( xexists(custom_back) && !xexists(custom_work) )
    {
      rename(custom_back, custom_work);
    }
    if( xsavefile(custom_work, 0666, new_data, new_size) == -1 )
    {
      goto cleanup;
    }
    if( xcyclefiles(custom_work, custom_path, custom_back) == -1 )
    {
      goto cleanup;
    }
  }

  if( !xexists(custom_back) && !xexists(custom_work) )
  {
    xsavefile(custom_back, 0666, new_data, new_size);
  }

  // success
  error = 0;

  cleanup:

  free(old_data);
  free(new_data);

  //log_crit_F("error=%d\n", error);

  return error;
}

/* ------------------------------------------------------------------------- *
 * database_save_current  --  store currently active profile to file
 * ------------------------------------------------------------------------- */

static int
database_save_current(void)
{
  int     error = -1;
  char   *old_data = 0;
  size_t  old_size = 0;
  char   *new_data = 0;
  size_t  new_size = 0;

  // make sure we have data directory
  if( prepfile(current_work) == -1 )
  {
    goto cleanup;
  }

  // create new content
  if( !(new_data = xstrfmt("%s\n", database_current)) )
  {
    goto cleanup;
  }
  new_size = strlen(new_data);

  // load old content
  xloadfile(current_path, &old_data, &old_size);

  // write to flash only if needed
  if( old_size != new_size || memcmp(old_data, new_data, new_size) )
  {
    if( xexists(current_back) && !xexists(current_work) )
    {
      rename(current_back, current_work);
    }
    if( xsavefile(current_work, 0666, new_data, new_size) == -1 )
    {
      goto cleanup;
    }
    if( xcyclefiles(current_work, current_path, current_back) == -1 )
    {
      goto cleanup;
    }
  }

  if( !xexists(current_back) && !xexists(current_work) )
  {
    xsavefile(current_back, 0666, new_data, new_size);
  }

  // success
  error = 0;

  cleanup:

  free(old_data);
  free(new_data);

  //log_crit_F("error=%d\n", error);
  return error;
}

/* ------------------------------------------------------------------------- *
 * database_load_current  --  restore currently active profile from file
 * ------------------------------------------------------------------------- */

static void
database_load_current(void)
{
  char   *old_data = 0;
  size_t  old_size = 0;

  if( xloadfile(current_path, &old_data, &old_size) == 0 )
  {
    old_data[strcspn(old_data,"\r\n")] = 0;
    xstripall(old_data);
  }

  if( xisempty(old_data) || database_set_profile(old_data) == -1 )
  {
    xstrset(&database_current, *database_builtins);
  }

  free(old_data);
}

/* ------------------------------------------------------------------------- *
 * database_load  --  load all profile data
 * ------------------------------------------------------------------------- */

static void
database_load(void)
{
  database_load_config();
  database_load_custom();
  database_load_current();

  xfetchstats(custom_path, &database_custom_stat);
  xfetchstats(current_path, &database_current_stat);
}

/* ------------------------------------------------------------------------- *
 * database_reload  --  reload configurarion files & broadcast changes
 * ------------------------------------------------------------------------- */

void
database_reload(void)
{
  // reload config files

  inifile_delete(database_static);
  database_static  = inifile_create();
  database_load_config();


  if( !database_has_profile(database_current) )
  {
    // switch back to default profile if the
    // configuration updates happened to get
    // rid of the currently active profile
    xstrset(&database_current, *database_builtins);
  }

  // send change notification if needed
  database_notify_changes();
}

/* ------------------------------------------------------------------------- *
 * database_save  --  save all profile data
 * ------------------------------------------------------------------------- */

static int
database_save_now(void)
{
  static int disabled = 0;

  int error = 0;

  if( disabled )
  {
    log_warning("%s: saving disabled, ignoring save request\n", custom_path);
  }
  else
  {
    if( !xcheckstats(custom_path,  &database_custom_stat) )
    {
      log_warning("%s: modified - disabling saving\n", custom_path);
      disabled = 1;
    }

    if( !xcheckstats(current_path, &database_current_stat) )
    {
      log_warning("%s: modified - disabling saving\n", current_path);
      disabled = 1;
    }

    if( disabled )
    {
      database_restart_request();
    }
  }

  if( !disabled )
  {
    if( database_save_custom()  == -1 ) error = -1;
    if( database_save_current() == -1 ) error = -1;

    xfetchstats(custom_path,  &database_custom_stat);
    xfetchstats(current_path, &database_current_stat);
  }

  //log_crit_F("error=%d\n", error);
  return error;
}

static guint database_save_retry_id = 0;
static guint database_save_later_id = 0;

static
gboolean
database_save_retry_cb(gpointer data)
{
  (void)data;

  if( database_save_now() == -1 )
  {
      return TRUE;
  }

  log_warning("database save retry - success\n");
  database_save_retry_id = 0;
  return FALSE;
}

static
void
database_save_cancel(void)
{
  if( database_save_later_id != 0 )
  {
    g_source_remove(database_save_later_id);
    database_save_later_id = 0;
  }

  if( database_save_retry_id != 0 )
  {
    log_warning("database save retry - cancelled\n");
    g_source_remove(database_save_retry_id);
    database_save_retry_id = 0;
  }
}

static
void
database_save_request(void)
{
  database_save_cancel();

  if( database_save_now() == -1 )
  {
    log_warning("database save retry - started\n");
    database_save_retry_id = g_timeout_add_seconds(RETRY_SAVE_PERIOD,
                                                   database_save_retry_cb, 0);
  }
}

static
gboolean
database_save_later_cb(gpointer data)
{
  (void)data;

  database_save_later_id = 0;
  database_save_request();
  return FALSE;
}

static
void
database_save_later(int secs)
{
  if( database_save_later_id == 0 && database_save_retry_id == 0 )
  {
    if( secs < 1 ) secs = 1;
    database_save_later_id = g_timeout_add_seconds(secs,
                                                   database_save_later_cb, 0);
  }
}

static
int
copy_file(const char *source, const char *destination)
{
  FILE *src, *dest;
  int ch;
  src = fopen(source, "r");
  if( src == NULL )
  {
    log_err("File open failed: %s\n", source);
    return -1;
  }
  dest = fopen(destination, "w");
  if( dest == NULL )
  {
    log_err("File open failed: %s\n", destination);
    fclose(src);
    return -1;
  }
  while( (ch = fgetc(src)) != EOF )
  {
    fputc(ch, dest);
  }
  fclose(src);
  fclose(dest);
  return 0;
}

static
void
migrate_configs_hard_way(const char *legacydir, const char *currentdir)
{
  DIR *dptr;
  struct dirent *dir;
  char srcname[260], destname[260];
  int result = 0;
  if( mkdir(currentdir, 0755) < 0 && errno != EEXIST )
  {
    log_err("%s: mkdir: %s\n", currentdir, strerror(errno));
    return;
  }
  dptr = opendir(legacydir);
  if( !dptr )
  {
    log_err("Directory open failed: %s %s\n", legacydir, strerror(errno));
    return;
  }
  while( (dir = readdir(dptr)) != NULL )
  {
    if( dir->d_type == DT_REG )
    {
      snprintf(srcname, sizeof srcname, "%s/%s", legacydir, dir->d_name);
      snprintf(destname, sizeof destname, "%s/%s", currentdir, dir->d_name);
      if( copy_file(srcname, destname) < 0 )
      {
        result = -1;
        continue;
      }
      if( remove(srcname) != 0 )
      {
        log_warning("Failed to remove %s with %s\n", srcname, strerror(errno));
      }
    }
  }
  closedir(dptr);
  if( result < 0 || remove(legacydir) != 0 )
  {
    log_warning("Failed to remove %s with %s\n", legacydir, strerror(errno));
  }
}

static
void
migrate_configs(const char *legacydir, const char *currentdir)
{
  if( rename(legacydir, currentdir) < 0 )
  {
    log_warning("Rename failed with %s. Falling back to copy/delete.\n", strerror(errno));
    migrate_configs_hard_way(legacydir, currentdir);
  }
}

/* ------------------------------------------------------------------------- *
 * database_init
 * ------------------------------------------------------------------------- */

int
database_init(void)
{
  current_path = xstrfmt("%s/%s", datadir(), CURRENT_TXT);
  current_work = xstrfmt("%s.tmp", current_path);
  current_back = xstrfmt("%s.bak", current_path);
  custom_path  = xstrfmt("%s/%s", datadir(), CUSTOM_INI);
  custom_work  = xstrfmt("%s.tmp", custom_path);
  custom_back  = xstrfmt("%s.bak", custom_path);

  if( access(datadir(), F_OK) != 0 && access(legacydir(), F_OK) == 0 )
  {
    migrate_configs(legacydir(), datadir());
  }
  xstrset(&database_previous, *database_builtins);
  xstrset(&database_current,  *database_builtins);

  database_static  = inifile_create();
  database_custom  = inifile_create();

  database_load();

  // Make sure that $HOME/.profiled/current is always available
  database_save_now();

  xstrset(&database_previous, database_current);

  // generate and discard initial changeset
  database_generate_changes();
  database_clear_changes();

  return 0;
}

/* ------------------------------------------------------------------------- *
 * database_quit
 * ------------------------------------------------------------------------- */

void
database_quit(void)
{
  // cancel pending retries
  database_save_cancel();

  // if there are unsaved changes to profile data,
  // the save will be triggered by server_quit()
  //database_save_now();

  xstrset(&database_previous, 0);
  xstrset(&database_current,  0);

  inifile_delete(database_static),  database_static  = 0;
  inifile_delete(database_custom),  database_custom  = 0;

  xstrset(&custom_path, 0);
  xstrset(&custom_work, 0);
  xstrset(&custom_back, 0);

  xstrset(&current_path, 0);
  xstrset(&current_work, 0);
  xstrset(&current_back, 0);
}

/* ------------------------------------------------------------------------- *
 * database_get_profiles
 * ------------------------------------------------------------------------- */

static int
database_get_profiles_cb(const inisec_t *s, void *aptr)
{
  unique_t *unique = aptr;
  if( !database_is_special(s->is_name) )
  {
    unique_add(unique, s->is_name);
  }
  return 0;
}

char **
database_get_profiles(int *pcount)
{
  /* Available profile names:
   * - default set of builtin profiles
   * - non special section names in static configuration files
   */

  char     **names = 0;
  size_t     count = 0;
  unique_t   unique;

  unique_ctor(&unique);

  inifile_scan_sections(database_static, database_get_profiles_cb, &unique);
#if 0 // no need to scan custom values
  inifile_scan_sections(database_custom, database_get_profiles_cb, &unique);
#endif

  for( int i = 0; database_builtins[i]; ++i )
  {
    unique_add(&unique, database_builtins[i]);
  }

  names = unique_steal(&unique, &count);

  unique_dtor(&unique);

  if( pcount ) *pcount = count;

  return names;
}

/* ------------------------------------------------------------------------- *
 * database_free_profiles
 * ------------------------------------------------------------------------- */

void
database_free_profiles(char **profiles)
{
  xfreev(profiles);
}

/* ------------------------------------------------------------------------- *
 * database_get_profile
 * ------------------------------------------------------------------------- */

const char *
database_get_profile(void)
{
  return database_current;
}

/* ------------------------------------------------------------------------- *
 * database_get_previous
 * ------------------------------------------------------------------------- */

const char *
database_get_previous(void)
{
  return database_previous;
}

/* ------------------------------------------------------------------------- *
 * database_set_profile
 * ------------------------------------------------------------------------- */

int
database_set_profile(const char *profile)
{
  /* Current profile can be set to only something that exists */
  int res = -1;

  database_check_profile(&profile);

  if( database_has_profile(profile) )
  {
    res = 0;

    if( strcmp(database_current, profile) )
    {
      xstrset(&database_current, profile);
      database_notify_changes();
    }
  }
  return res;
}

/* ------------------------------------------------------------------------- *
 * database_has_profile
 * ------------------------------------------------------------------------- */

int
database_has_profile(const char *profile)
{
  /* Available profile names:
   * - default set of builtin profiles
   * - non special section names in static configuration files
   */

  database_check_profile(&profile);

  if( database_is_builtin(profile) )
  {
    return 1;
  }

  if( database_is_special(profile) )
  {
    return 0;
  }

  if( inifile_has_section(database_static, profile) )
  {
    return 1;
  }

#if 0 // no need to scan custom values
  if( inifile_has_section(database_custom, profile) )
  {
    return 1;
  }
#endif

  return 0;
}

/* ------------------------------------------------------------------------- *
 * database_get_keys
 * ------------------------------------------------------------------------- */

static int
database_get_keys_cb(const inisec_t *s, const inival_t *v, void *aptr)
{
  unique_t *unique = aptr;

  if( strcmp(s->is_name, DATATYPE) )
  {
    if( fallback_(v->iv_key) != 0 )
    {
      // keys that have both datatype and fallback value
      unique_add(unique, v->iv_key);
    }
  }
  return 0;
}

char **
database_get_keys(int *pcount)
{
  /* Available key names:
   * - have both datatype and fallback value in static configuration files
   */

  char     **names = 0;
  size_t     count = 0;
  unique_t   unique;

  unique_ctor(&unique);

  inifile_scan_values(database_static, database_get_keys_cb, &unique);

  // no need to scan custom values
  //inifile_scan_values(database_custom, database_get_keys_cb, &unique);

  names = unique_steal(&unique, &count);

  unique_dtor(&unique);

  if( pcount ) *pcount = count;

  return names;
}

/* ------------------------------------------------------------------------- *
 * database_free_keys
 * ------------------------------------------------------------------------- */

void
database_free_keys(char **keys)
{
  xfreev(keys);
}

/* ------------------------------------------------------------------------- *
 * database_has_value
 * ------------------------------------------------------------------------- */

int
database_has_value(const char *key)
{
  /* Available key names:
   * - have both datatype and fallback value in static configuration files
   */
  return datatype_(key) != 0 && fallback_(key) != 0;
}

/* ------------------------------------------------------------------------- *
 * database_is_writable
 * ------------------------------------------------------------------------- */

int
database_is_writable(const char *key)
{
  /* Writable key: has datatype and fallback value but no override value */

  return override_(key) == 0 && datatype_(key) != 0 && fallback_(key) != 0;
}

/* ------------------------------------------------------------------------- *
 * database_get_value
 * ------------------------------------------------------------------------- */

const char *
database_get_value(const char *profile,
                   const char *key,
                   const char *val)
{
  /* Getting value for non-existing profile yields fallback value */

  const char *res = 0;
  database_check_profile(&profile);

  if( database_has_value(key) )
  {
    res = current_(profile, key);
  }

  return res ? res : val;
}

/* ------------------------------------------------------------------------- *
 * database_set_value
 * ------------------------------------------------------------------------- */

int
database_set_value(const char *profile,
                   const char *key,
                   const char *val)
{
  /* Value will be changed only if the profile and key exist.
   *
   * If the key is not writable, the value will be stored, but
   * will not take effect until the key becomes writeble. In
   * practice this means removal of configuration file that
   * declares [override] value for the key.
   */

  int res = -1;

  database_check_profile(&profile);

  if( database_has_profile(profile) )
  {
    if( database_has_value(key) )
    {
      const char *cus = custom_(profile, key);
      const char *cur = current_(profile, key);
      char       *use = xstrip(strdup(val ?: ""));

      /* differs from current value, or custom value not set */
      if( !xstrsame(cur, use) || xstrnull(cus) )
      {
        // set custom data
        inifile_set(database_custom,  profile, key, use);

        if( database_is_writable(key) )
        {
          // notify changes
          database_notify_changes();
        }
        else
        {
          // just make sure the changes get saved
          database_save_later(1);
        }
      }
      free(use);
      res = 0;
    }
  }

  return res;
}

/* ------------------------------------------------------------------------- *
 * database_get_type
 * ------------------------------------------------------------------------- */

const char *
database_get_type(const char *key, const char *def)
{
  /* Return configured data type or caller specified
   * default value if one does not exist */
  return datatype_(key) ?: def;
}

/* ------------------------------------------------------------------------- *
 * database_get_values
 * ------------------------------------------------------------------------- */

profileval_t *
database_get_values(const char *profile, int *pcount)
{
  /* Return (key, value, datatype) triplets for
   * all existing keys in the specified profile.
   * If the profile does not exist, fallback values
   * will be returned.
   */
  int           keys = 0;
  char        **key  = database_get_keys(&keys);

  int           cnt = 0;
  profileval_t *vec  = calloc(keys + 1, sizeof *vec);

  database_check_profile(&profile);

  for( int i = 0; i < keys; ++i )
  {
    const char *k = key[i];
    const char *v = current_(profile, k);
    const char *t = datatype_(k);

    if( k != 0 && v != 0 && t != 0 )
    {
      profileval_ctor_ex(&vec[cnt++], k, v, t);
    }
  }
  profileval_ctor(&vec[cnt]);

  database_free_keys(key);

  if( pcount ) *pcount = cnt;
  return vec;
}

/* ------------------------------------------------------------------------- *
 * database_free_values
 * ------------------------------------------------------------------------- */

void
database_free_values(profileval_t *values)
{
  profileval_free_vector(values);
}

static inifile_t *bc_state_prev = 0;
static inifile_t *bc_state_curr = 0;
static inifile_t *bc_state_diff = 0;

/* ------------------------------------------------------------------------- *
 * database_clear_changes
 * ------------------------------------------------------------------------- */

void
database_clear_changes(void)
{
  /* Clear changeset */
  xstrset(&database_previous, database_current);
  inifile_delete(bc_state_diff), bc_state_diff = 0;

  /* Save state information, if it fails it will be
   * periodically retried until it succeeds */
  database_save_request();
}

/* ------------------------------------------------------------------------- *
 * database_generate_changes
 * ------------------------------------------------------------------------- */

static void
database_generate_changes(void)
{
  char **prof = 0;
  char **key  = 0;

  if( bc_state_diff )
  {
    // already have delta set
    goto cleanup;
  }

  inifile_delete(bc_state_prev);
  bc_state_prev = bc_state_curr ?: inifile_create();
  bc_state_curr = inifile_create();
  bc_state_diff = inifile_create();

  /* Scan changeset for keys with changed values.
   *
   * Special case: if the active profile has changed,
   * values of all keys will be included
   *
   * Keys that are no longer available (due to removal of
   * configuration files for example) will be listed
   * with empty value and datatype.
   */

  // accumulate changed values
  prof = database_get_profiles(0);
  key  = database_get_keys(0);
  for( int p = 0; prof && prof[p]; ++p )
  {
    int force = (!xstrsame(database_current, database_previous) &&
		 xstrsame(database_current, prof[p]));

    for( int k = 0; key && key[k]; ++k )
    {
      const char *v_curr = database_get_value(prof[p], key[k], "");
      const char *v_prev = inifile_get(bc_state_prev, prof[p], key[k], "");
      inifile_set(bc_state_curr, prof[p], key[k], v_curr);
      if( force || !xstrsame(v_prev, v_curr) )
      {
	inifile_set(bc_state_diff, prof[p], key[k], v_curr);
      }
    }
  }
  database_free_keys(key), key = 0;
  database_free_profiles(prof), prof = 0;

  // accumulate dropped values
  prof = inifile_get_section_names(bc_state_prev, 0);
  key  = inifile_get_value_keys(bc_state_prev, 0);
  for( int p = 0; prof && prof[p]; ++p )
  {
    for( int k = 0; key && key[k]; ++k )
    {
      const char *v_prev = inifile_get(bc_state_prev, prof[p], key[k], 0);
      const char *v_curr = inifile_get(bc_state_curr, prof[p], key[k], 0);
      if( v_prev && !v_curr )
      {
	inifile_set(bc_state_diff, prof[p], key[k], "");
      }
    }
  }
  database_free_keys(key), key = 0;
  database_free_profiles(prof), prof = 0;

cleanup:
  return;
}

/* ------------------------------------------------------------------------- *
 * database_get_changed_profiles
 * ------------------------------------------------------------------------- */

char **
database_get_changed_profiles(int *pcount)
{
  size_t   cnt = 0;
  char   **res = 0;

  database_generate_changes();

  res = inifile_get_section_names(bc_state_diff, &cnt);

  if( pcount ) *pcount = (int)cnt;

  return res;
}

/* ------------------------------------------------------------------------- *
 * database_free_changed_profiles
 * ------------------------------------------------------------------------- */

void
database_free_changed_profiles(char **profiles)
{
  xfreev(profiles);
}

/* ------------------------------------------------------------------------- *
 * database_get_changed_values
 * ------------------------------------------------------------------------- */

profileval_t *
database_get_changed_values(const char *profile, int *pcount)
{
  profileval_t *res = 0;
  char        **key = 0;
  int           keys = 0;
  int           cnt = 0;

  database_generate_changes();

  key = inifile_get_section_keys(bc_state_diff, profile, &keys);
  res = calloc(keys + 1, sizeof *res);

  for( int i = 0; i < keys; ++i )
  {
    // key is transferred, rest is strdup:ed
    char       *k = key[i];
    const char *v = inifile_get(bc_state_diff, profile, k, "");
    const char *t = database_get_type(k, "");
    if( k != 0 && v != 0 && t != 0 )
    {
      profileval_ctor_ex(&res[cnt], 0, v, t);
      res[cnt++].pv_key = k, k = 0;
    }
    free(k);
  }
  profileval_ctor(&res[cnt]);

  free(key);
  if( pcount ) *pcount = cnt;
  return res;
}

/* ------------------------------------------------------------------------- *
 * database_free_changed_values
 * ------------------------------------------------------------------------- */

void
database_free_changed_values(profileval_t *values)
{
  profileval_free_vector(values);
}
