
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

/**
 * @brief C API to profile data
 *
 * @file libprofile.h
 *
 * This is the API for accessing data on profile daemon.
 *
 * <p>
 * Copyright (C) 2008 Nokia. All rights reserved.
 *
 * @author Simo Piiroinen <simo.piiroinen@nokia.com>
 */

#ifndef LIBPROFILE_H_
# define LIBPROFILE_H_

# include "profileval.h"

# include <dbus/dbus.h>

# ifdef __cplusplus
extern "C" {
# elif 0
} /* fool JED indentation ... */
# endif
#pragma GCC visibility push(default)

/** \brief Profile changed callback
 *
 * Callback function type used for signalling when the
 * currently active profile changes.
 *
 * @param profile profile name
 */
typedef void (*profile_track_profile_fn)(const char *profile);

/** \brief Profile changed callback with user data
 *
 * Callback function type used for signalling when the
 * currently active profile changes.
 *
 * @since 0.0.13
 *
 * @param profile   profile name
 * @param user_data pointer specified when the callback was added
 */
typedef void (*profile_track_profile_fn_data)(const char *profile,
                                              void *user_data);

/** \brief Profile value changed callback
 *
 * Callback function type used for signalling when the
 * values within a profile change.
 *
 * @param profile profile name
 * @param key     value name
 * @param val     value string
 * @param type    value type
 */
typedef void (*profile_track_value_fn)(const char *profile,
                                       const char *key,
                                       const char *val,
                                       const char *type);

/** \brief Profile value changed callback with user data
 *
 * Callback function type used for signalling when the
 * values within a profile change.
 *
 * @since 0.0.13
 *
 * @param profile   profile name
 * @param key       value name
 * @param val       value string
 * @param type      value type
 * @param user_data pointer specified when the callback was added
 */
typedef void (*profile_track_value_fn_data)(const char *profile,
                                            const char *key,
                                            const char *val,
                                            const char *type,
                                            void *user_data);

/** \brief Free user data callback
 *
 * Callback function type used for freeing user data.
 *
 * @since 0.0.15
 *
 * @param user_data data to be deallocated
 */
typedef void (*profile_user_data_free_fn)(void *user_data);

/** \name Query Functions
 */
/*@{*/

/** \brief Returns array of profile names
 *
 * Get names of currently available profiles.
 *
 * Use #profile_free_profiles() to free the resulting array.
 *
 * @return      Array of profile name strings, NULL on error
 */
char **       profile_get_profiles(void);

/** \brief Frees array of profile names
 *
 * Free profile name string array obtained
 * via #profile_get_profiles() call.
 *
 * @param profiles array of profile names
 *
 */
void          profile_free_profiles(char **profiles);

/** \brief Returns array of profile keys.
 *
 * Get names of currently available profile values.
 *
 * Use #profile_free_keys() to free the resulting array.
 *
 * @return      Array of profile value name strings, NULL on error
 */
char        **profile_get_keys     (void);

/** \brief Frees array of profile keys.
 *
 * Free profile value name string array obtained
 * via #profile_get_keys() call.
 *
 * @param keys Array of key names
 */
void          profile_free_keys    (char **keys);

/** \brief Returns array of profile values
 *
 * Get all values for given profile.
 *
 * Use #profile_free_values() to free the
 * resulting array.
 *
 * @param profile profile name or NULL for current
 *
 * @return      Array of profile values, NULL on error
 */
profileval_t *profile_get_values   (const char *profile);

/** \brief Frees array of profile values
 *
 * Free profile value array obtained
 * via profile_get_values() call.
 *
 * @param values Array of values
 */
void          profile_free_values  (profileval_t *values);

/** \brief Check if a profile exists
 *
 * Check if given profile exists.
 *
 * @param profile profile name
 *
 * @returns non-zero value if profile exists
 */
int           profile_has_profile  (const char *profile);

/** \brief Get name of the current profile
 *
 * Get name of the currently active profile.
 *
 * @returns profile name, NULL on error
 */
char         *profile_get_profile  (void);

/** \brief Set the active profile
 *
 * Set currently active  profil.
 *
 * @param profile profile name
 *
 * @returns 0 on success, -1 on error
 */
int           profile_set_profile  (const char *profile);

/** \brief Check if profile key exists
 *
 * Check if given profile value exists.
 *
 * @param key profile value name
 *
 * @returns non-zero value if value exists
 */
int           profile_has_value    (const char *key);

/** \brief Get value of a profile key
 *
 * Get value from profile.
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 *
 * @returns profile value string, NULL on error
 */
char         *profile_get_value    (const char *profile, const char *key);

/** \brief Set value of a profile key
 *
 * Set value in profile.
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 * @param val     value
 *
 * @returns 0 on success, -1 on failure
 */
int           profile_set_value    (const char *profile, const char *key,
                                    const char *val);

/** \brief Check if a value can be modified
 *
 * Check if given profile value is writable.
 *
 * @param key profile value name
 *
 * @returns non-zero value if value can be modified
 */
int           profile_is_writable  (const char *key);

/** \brief Get type hint of profile value
 *
 * Get type description associated with profile value.
 *
 * @param key profile value name
 *
 * @returns type description string, NULL on error
 */
char         *profile_get_type     (const char *key);

/*@}*/

/** \name Tracking Functions
 */
/*@{*/

/** \brief Start change tracking
 *
 * Start listening to profile daemon signals over dbus.
 *
 * The callback functions can be assigned prior to calling
 * this function using
 *
 * profile_track_set_profile_cb()
 * profile_track_set_active_cb()
 * profile_track_set_change_cb()
 *
 * If session bus connection status changes due to calling
 * one of #profile_connection_disable_autoconnect(),
 * #profile_connection_enable_autoconnect(),
 * #profile_connection_set() or #profile_connection_get()
 * the tracking will be, depending on the new connection
 * state either termporarily disabled or restarted.
 *
 * @returns 0 = success, -1 = error
 */
int           profile_tracker_init(void);

/** \brief Stop change tracking
 *
 * Stop listening to profile daemon signals over dbus.
 */
void          profile_tracker_quit(void);

/** \brief Setup current profile chaged callback
 *
 * Adds callback function to be called when the currently
 * active profile changes.
 *
 * User data pointer is passed to the callback.
 *
 * Callbacks are called in the order that they were added.
 * The same callback can be added several times,
 * in which case it will be executed more than once.
 *
 * The callback can be removed by calling #profile_track_remove_profile_cb().
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 * @param free_cb   function for deallocating user_data
 */
void profile_track_add_profile_cb(profile_track_profile_fn_data cb,
                                  void *user_data,
                                  profile_user_data_free_fn free_cb);

/** \brief Remove current profile chaged callback
 *
 * Removes callback added via #profile_track_add_profile_cb().
 *
 * The most recently added callback that matches the parameters
 * will be removed.
 *
 * The free callback will be called if both user data and free
 * callback were set non-NULL when the callback was added.
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 *
 * @returns non-zero value if callback existed
 */
int profile_track_remove_profile_cb(profile_track_profile_fn_data cb,
                                     void *user_data);

/** \brief Setup value in current profile changed callback
 *
 * Adds callback function to be called when a value in the
 * currently active profile changes.
 *
 * User data pointer is passed to the callback.
 *
 * Callbacks are called in the order that they were added.
 * The same callback can be added several times,
 * in which case it will be executed more than once.
 *
 * The callback can be removed by calling #profile_track_remove_active_cb().
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 * @param free_cb   function for deallocating user_data
 */
void profile_track_add_active_cb(profile_track_value_fn_data cb,
                                 void *user_data,
                                 profile_user_data_free_fn free_cb);

/** \brief Remove value in current profile changed callback
 *
 * Removes callback added via #profile_track_add_active_cb().
 *
 * The most recently added callback that matches the parameters
 * will be removed.
 *
 * The free callback will be called if both user data and free
 * callback were set non-NULL when the callback was added.
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 *
 * @returns non-zero value if callback existed
 */
int profile_track_remove_active_cb(profile_track_value_fn_data cb,
                                    void *user_data);

/** \brief Setup value in non-current profile changed callback
 *
 * Adds callback function to be called when a value in other
 * than the currently active profile changes.
 *
 * User data pointer is passed to the callback.
 *
 * Callbacks are called in the order that they were added.
 * The same callback can be added several times,
 * in which case it will be executed more than once.
 *
 * The callback can be removed by calling #profile_track_remove_change_cb().
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 * @param free_cb   function for deallocating user_data
 *
 */
void profile_track_add_change_cb(profile_track_value_fn_data cb,
                                 void *user_data,
                                 profile_user_data_free_fn free_cb);
/** \brief Remove value in non-current profile changed callback
 *
 * Removes callback added via #profile_track_add_change_cb().
 *
 * The most recently added callback that matches the parameters
 * will be removed.
 *
 * The free callback will be called if both user data and free
 * callback were set non-NULL when the callback was added.
 *
 * @since 0.0.15
 *
 * @param cb        callback function
 * @param user_data pointer to user data
 *
 * @returns non-zero value if callback existed
 */
int profile_track_remove_change_cb(profile_track_value_fn_data cb,
                                    void *user_data);

/** \brief Special: deny libprofile from connecting to session bus
 *
 * Forbid libprofile from making session bus connection
 *
 * @since 0.0.10
 *
 * Normally you do not need to call this function. The library
 * will internally select and discover the correct bus to use.
 *
 * However, there are some special cases where the address of the
 * bus is not known at start time. In such cases you can use this
 * function to stop libprofile from initiating the connection,
 * setup tracking if needed, connect to the bus once you discover
 * it and pass the connection to the profile library using
 * #profile_connection_set().
 *
 *
 * If libprofile is already connected at the time this function is
 * called, the connection will be dropped and tracking disabled.
 *
 * No communication to profile daemon is possible until either
 * #profile_connection_set() or
 * #profile_connection_enable_autoconnect() is called.
 */
void profile_connection_disable_autoconnect(void);

/** \brief Special: allow libprofile to connect to session bus
 *
 * Allow libprofile to make session bus connection
 *
 * @since 0.0.10
 *
 * If libprofile is already connected, no changes
 * to connection state are made.
 *
 * If there is no connection and tracking has been
 * started via #profile_tracker_init(), libprofile
 * will try to autoconnect and resume tracking.
 *
 * See #profile_connection_disable_autoconnect() for details.
 */
void profile_connection_enable_autoconnect(void);

/** \brief Special: get session bus connection used by libprofile
 *
 * Set the session bus connection used for communicating with the profile daemon.
 *
 * @since 0.0.10
 *
 * If libprofile already had a valid session bus connection,
 * that will be released and the one provided by this function
 * will be used. Reference count for the connection will be increased,
 * so the caller is free to call dbus_connection_unref() afterwards.
 *
 * If #profile_tracker_init() has been called previously, the tracking
 * will continue using the provided connection.
 *
 * If NULL connection is passed, libprofile will release any existing
 * connection and stops the tracking until non-null connection
 * is provided again.
 *
 * See #profile_connection_disable_autoconnect() for details.
 */
void profile_connection_set(DBusConnection *con);

/** \brief Special: set session bus connection used by libprofile
 *
 * Get the session bus connection used for communicating with the profile daemon.
 *
 * @since 0.0.10
 *
 * Returns the session bus connection established by libprofile
 * or provided by the application via #profile_connection_set().
 *
 * If no connection exists at the time of the call, libprofile
 * will try to connect to session bus - unless blocked from
 * doing so by call to profile_connection_disable_autoconnect().
 *
 * If non NULL connection is returned, the caller must release
 * it using dbus_connection_unref() after it is no longer needed.
 *
 * @returns session bus connection used by libprofile, or NULL
 */
DBusConnection *profile_connection_get(void);

/*@}*/

/** \name Convenience Functions
 */
/*@{*/

/** \brief Get value of profile key as boolean
 *
 * Convenience function: get profile value and convert to boolean
 * using profile_parse_bool().
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 *
 * @returns 1=True, 0=False
 */
int           profile_get_value_as_bool  (const char *profile,
                                          const char *key);

/** \brief Set value of profile key from boolean
 *
 * Convenience function: set profile value from boolean.
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 * @param val     zero = False, nonzero = True
 *
 * @returns 0 on success, -1 on failure
 */
int           profile_set_value_as_bool  (const char *profile,
                                          const char *key, int val);

/** \brief Get value of profile key as integer
 *
 * Convenience function: get profile value and convert to integer
 * using profile_parse_int().
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 *
 * @returns integer value
 */
int           profile_get_value_as_int   (const char *profile,
                                          const char *key);

/** \brief Set value of profile key integer boolean
 *
 * Convenience function: set profile value from integer.
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 * @param val     integer value
 *
 * @returns 0 on success, -1 on failure
 */
int           profile_set_value_as_int   (const char *profile,
                                          const char *key, int val);

/** \brief Get value of profile key as double
 *
 * Convenience function: get profile value and convert to double
 * using profile_parse_double().
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 *
 * @returns double value
 */
int           profile_get_value_as_double(const char *profile,
                                          const char *key);

/** \brief Set value of profile key integer double
 *
 * Convenience function: set profile value from double.
 *
 * @param profile profile name or NULL for current
 * @param key     value name
 * @param val     double value
 *
 * @returns 0 on success, -1 on failure
 */
int           profile_set_value_as_double(const char *profile,
                                          const char *key, double val);

/*@}*/

/** \name Utility Functions
 */
/*@{*/

/** \brief Text to boolean value utility
 *
 * Utility function for parsing boolean values from profile strings.
 *
 * @param text profile value content
 *
 * @returns 1=True, 0=False
 */
int           profile_parse_bool         (const char *text);

/** \brief Text to integer value utility
 *
 * Utility function for parsing integer values from profile strings.
 *
 * @param text profile value content
 *
 * @returns integer value as from strtol()
 */
int           profile_parse_int          (const char *text);

/** \brief Text to double value utility
 *
 * Utility function for parsing double values from profile strings.
 *
 * @param text profile value content
 *
 * @returns double value as from strtod()
 */
double        profile_parse_double       (const char *text);

/*@}*/

/** \name Deprecated Functions
 */
/*@{*/

/** \brief Deprecated
 *
 * \deprecated New code should use #profile_track_add_profile_cb()
 *             and #profile_track_remove_profile_cb().
 *
 * Assign callback function to be called when active
 * profile changes.
 *
 * @param cb callback function
 */
void profile_track_set_profile_cb(profile_track_profile_fn cb)
        __attribute__((deprecated));

/** \brief Deprecated
 *
 * \deprecated New code should use #profile_track_add_profile_cb()
 *             and #profile_track_remove_profile_cb().
 *
 * Assign callback function to be called when active
 * profile changes. Attaches user data to the callback.
 *
 * @since 0.0.13
 *
 * @param cb callback function
 * @param user_data data pointer to pass to the callback
 */
void profile_track_set_profile_cb_with_data(profile_track_profile_fn_data cb,
                                            void *user_data)
        __attribute__((deprecated));

/** \brief Deprecated
 *
 * \deprecated New code should use
 *             #profile_track_add_active_cb() and
 *             #profile_track_remove_active_cb().
 *
 * Assign callback function to be called when value
 * in active profile changes.
 *
 * @param cb callback function
 */
void profile_track_set_active_cb (profile_track_value_fn cb)
        __attribute__((deprecated));

/** \brief Deprecated
 *
 * \deprecated New code should use
 *             #profile_track_add_active_cb() and
 *             #profile_track_remove_active_cb().
 *
 * Assign callback function to be called when value
 * in active profile changes. Attaches user data to the callback.
 *
 * @since 0.0.13
 *
 * @param cb callback function
 * @param user_data data pointer to pass to the callback
 */
void profile_track_set_active_cb_with_data (profile_track_value_fn_data cb,
                                            void *user_data)
        __attribute__((deprecated));

/** \brief Deprecated
 *
 * \deprecated New code should use
 *             #profile_track_add_change_cb() and
 *             #profile_track_remove_change_cb().
 *
 * Assign callback function to be called when value
 * in non-active profile changes.
 *
 * @param cb callback function
 */
void profile_track_set_change_cb (profile_track_value_fn cb)
        __attribute__((deprecated));

/** \brief Deprecated
 *
 * \deprecated New code should use
 *             #profile_track_add_change_cb() and
 *             #profile_track_remove_change_cb().
 *
 * Assign callback function to be called when value
 * in non-active profile changes. Attaches user data to the callback.
 *
 * @since 0.0.13
 *
 * @param cb callback function
 * @param user_data data pointer to pass to the callback
 */
void profile_track_set_change_cb_with_data (profile_track_value_fn_data cb,
                                            void *user_data)
        __attribute__((deprecated));

/*@}*/

#pragma GCC visibility pop

# ifdef __cplusplus
};
# endif

#endif /* LIBPROFILE_H_ */
