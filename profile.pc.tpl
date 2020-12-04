prefix=@PREFIX@
exec_prefix=@BINDIR@
libdir=@LIBDIR@
includedir=@INCDIR@

Name: profile
Description: profiled client library
Version: @VERS@
Requires: dbus-1 glib-2.0
Requires.private:
Cflags: -I@INCDIR@
Libs: -L@LIBDIR@ -lprofile
