# -*- mode: sh -*-

# ----------------------------------------------------------------------------
# Global Flags
# ----------------------------------------------------------------------------

CPPFLAGS += -D_GNU_SOURCE

CFLAGS   += -Wall
CFLAGS   += -Wmissing-prototypes
CFLAGS   += -std=c99
CFLAGS   += -Os
CFLAGS   += -g

LDFLAGS  += -g

LDLIBS   += -Wl,--as-needed
LDLIBS   += -lrt

PKG_NAMES    := dbus-glib-1 dbus-1 glib-2.0 profile
PKG_CFLAGS   := $(shell pkg-config --cflags $(PKG_NAMES))
PKG_LDLIBS   := $(shell pkg-config --libs   $(PKG_NAMES))

PKG_CPPFLAGS := $(filter -D%,$(PKG_CFLAGS)) $(filter -I%,$(PKG_CFLAGS))
PKG_CFLAGS   := $(filter-out -I%, $(filter-out -D%, $(PKG_CFLAGS)))

CPPFLAGS += $(PKG_CPPFLAGS)
CFLAGS   += $(PKG_CFLAGS)
LDLIBS   += $(PKG_LDLIBS)

# ----------------------------------------------------------------------------
# Top Level Targets
# ----------------------------------------------------------------------------

TARGETS += delayed_session_bus
TARGETS += callbacks_without_mainloop
TARGETS += multiple_callbacks

.PHONY: build clean distclean mostlyclean install debclean

build:: $(TARGETS)

all:: build extra

clean:: mostlyclean
	$(RM) $(TARGETS) $(EXTRA)

distclean:: clean

mostlyclean::
	$(RM) *.o *~
