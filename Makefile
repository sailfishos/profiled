# -*- mode: sh -*-
# ----------------------------------------------------------------------------
# Configuration options
# ----------------------------------------------------------------------------

#USE_SYSTEM_BUS=y# Anything but 'y' will make profiled use session bus

# ----------------------------------------------------------------------------
# Basic Setup
# ----------------------------------------------------------------------------

VERS := $(shell head -1 debian/changelog  | sed -e 's:^.*(::' -e 's:).*::')
NAME ?= profiled
ROOT ?= /tmp/test-$(NAME)

PREFIX ?= /usr
ETCDIR ?= /etc/profiled
BINDIR ?= $(PREFIX)/bin
DLLDIR ?= $(PREFIX)/lib
LIBDIR ?= $(PREFIX)/lib
INCDIR ?= $(PREFIX)/include/$(NAME)

DOCDIR ?= $(PREFIX)/share/doc/$(NAME)
MANDIR ?= $(PREFIX)/share/man

DEVDOCDIR ?= $(PREFIX)/share/doc/libprofile-doc
PKGCFGDIR ?= $(PREFIX)/lib/pkgconfig

BACKUP_DIR ?= /etc/osso-backup
BACKUPFW_DIR ?= /usr/share/backup-framework

CUD_DIR      ?= /etc/osso-cud-scripts
RFS_DIR      ?= /etc/osso-rfs-scripts

DOXYDIR := doxydir

SO ?= .so.0

TEMPLATE_COPY = sed\
 -e 's:@NAME@:${NAME}:g'\
 -e 's:@VERS@:${VERS}:g'\
 -e 's:@ROOT@:${ROOT}:g'\
 -e 's:@PREFIX@:${PREFIX}:g'\
 -e 's:@BINDIR@:${BINDIR}:g'\
 -e 's:@LIBDIR@:${LIBDIR}:g'\
 -e 's:@DLLDIR@:${DLLDIR}:g'\
 -e 's:@INCDIR@:${INCDIR}:g'\
 -e 's:@DOCDIR@:${DOCDIR}:g'\
 -e 's:@MANDIR@:${MANDIR}:g'\
 -e 's:@DEVDOCDIR@:${DEVDOCDIR}:g'\
 -e 's:@PKGCFGDIR@:${PKGCFGDIR}:g'\
 -e 's:@BACKUPFW_DIR@:${BACKUPFW_DIR}:g'\
 -e 's:@BACKUP_DIR@:${BACKUP_DIR}:g'\
 < $< > $@

# ----------------------------------------------------------------------------
# Global Flags
# ----------------------------------------------------------------------------

CPPFLAGS += -D_GNU_SOURCE

CFLAGS   += -Wall
CFLAGS   += -Wmissing-prototypes
CFLAGS   += -Wunused-result
CFLAGS   += -W
CFLAGS   += -std=c99
CFLAGS   += -Os
CFLAGS   += -g

LDFLAGS  += -g

LDLIBS   += -Wl,--as-needed

# flags from pkgtool

PKG_NAMES    := dbus-glib-1 dbus-1 glib-2.0
PKG_CFLAGS   := $(shell pkg-config --cflags $(PKG_NAMES))
PKG_LDLIBS   := $(shell pkg-config --libs   $(PKG_NAMES))

PKG_CPPFLAGS := $(filter -D%,$(PKG_CFLAGS)) $(filter -I%,$(PKG_CFLAGS))
PKG_CFLAGS   := $(filter-out -I%, $(filter-out -D%, $(PKG_CFLAGS)))

CPPFLAGS += $(PKG_CPPFLAGS)
CFLAGS   += $(PKG_CFLAGS)
LDLIBS   += $(PKG_LDLIBS) -lrt

ifeq ($(USE_SYSTEM_BUS),y)
CPPFLAGS += -DUSE_SYSTEM_BUS
endif

CPPFLAGS += -D LOGGING_ENABLED # any logging at all?

## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=7 # debug
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=6 # info
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=5 # notice
CPPFLAGS += -D LOGGING_LEVEL=4 # warning
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=3 # err
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=2 # crit
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=1 # alert
## QUARANTINE CPPFLAGS += -D LOGGING_LEVEL=0 # emerg

CPPFLAGS += -D LOGGING_CHECK1ST# level check before args eval
## QUARANTINE CPPFLAGS += -D LOGGING_EXTRA   # promote debug to warnings
## QUARANTINE CPPFLAGS += -D LOGGING_FNCALLS # calltree via ENTER/LEAVE

# ----------------------------------------------------------------------------
# Top Level Targets
# ----------------------------------------------------------------------------

TARGETS += libprofile.a
TARGETS += libprofile$(SO)
TARGETS += profiled
TARGETS += profileclient
TARGETS += profile-tracker

FLOW_GRAPHS = $(foreach e,.png .pdf .eps,\
		  $(addsuffix $e,$(addprefix $1,.fun .mod .api .top)))

EXTRA += $(call FLOW_GRAPHS, profileclient)
EXTRA += $(call FLOW_GRAPHS, profiled)

.PHONY: build clean distclean mostlyclean install debclean

build:: $(TARGETS)

extra:: $(EXTRA)

all:: build extra

clean:: mostlyclean
	$(RM) $(TARGETS) $(EXTRA)

distclean:: clean

debclean:: distclean
	fakeroot ./debian/rules clean

mostlyclean::
	$(RM) *.o *~

install:: $(addprefix install-,profiled profileclient libprofile libprofile-dev libprofile-doc)

doxygen.executed:
	doxygen
	touch doxygen.executed
clean::
	$(RM) doxygen.executed
distclean::
	$(RM) -r $(DOXYDIR)

# ----------------------------------------------------------------------------
# Pattern rules
# ----------------------------------------------------------------------------

install-%-bin:
	$(if $<, install -m755 -d $(ROOT)$(BINDIR))
	$(if $<, install -m755 $^ $(ROOT)$(BINDIR))

install-%-lib:
	$(if $<, install -m755 -d $(ROOT)$(LIBDIR))
	$(if $<, install -m755 $^ $(ROOT)$(LIBDIR))

install-%-dll:
	$(if $<, install -m755 -d $(ROOT)$(DLLDIR))
	$(if $<, install -m755 $^ $(ROOT)$(DLLDIR))

install-%-inc:
	$(if $<, install -m755 -d $(ROOT)$(INCDIR))
	$(if $<, install -m644 $^ $(ROOT)$(INCDIR))

install-%-backup-config:
	$(if $<, install -m755 -d $(ROOT)$(BACKUP_DIR)/applications)
	$(if $<, install -m644 $^ $(ROOT)$(BACKUP_DIR)/applications)

install-%-backup-restore:
	$(if $<, install -m755 -d $(ROOT)$(BACKUP_DIR)/restore.d/always)
	$(if $<, install -m755 $^ $(ROOT)$(BACKUP_DIR)/restore.d/always)

install-%-backupfw-config:
	$(if $<, install -m755 -d $(ROOT)$(BACKUPFW_DIR)/applications)
	$(if $<, install -m644 $^ $(ROOT)$(BACKUPFW_DIR)/applications)

install-%-backupfw-restore:
	$(if $<, install -m755 -d $(ROOT)$(BACKUPFW_DIR)/restore.d/always)
	$(if $<, install -m755 $^ $(ROOT)$(BACKUPFW_DIR)/restore.d/always)

install-%-cud-scripts:
	$(if $<, install -m755 -d $(ROOT)$(CUD_DIR))
	$(if $<, install -m755 $^ $(ROOT)$(CUD_DIR))

install-%-rfs-scripts:
	$(if $<, install -m755 -d $(ROOT)$(RFS_DIR))
	$(if $<, install -m755 $^ $(ROOT)$(RFS_DIR))

%.pc     : %.pc.tpl ; $(TEMPLATE_COPY)
%        : %.tpl    ; $(TEMPLATE_COPY)

% : %.o
	$(CC) -o $@ $^ $(LDFLAGS) $(LDLIBS)

%$(SO): LDFLAGS += -shared -Wl,-soname,$@

%$(SO):
	$(CC) -o $@  $^ $(LDFLAGS) $(LDLIBS)

%.a:
	$(AR) r $@ $^

%.pic.o : CFLAGS += -fPIC
%.pic.o : CFLAGS += -fvisibility=hidden

%.pic.o : %.c
	@echo "Compile Dynamic: $<"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

%.o     : %.c
	@echo "Compile Static: $<"
	@$(CC) -o $@ -c $< $(CPPFLAGS) $(CFLAGS)

FLOWFLAGS += -Mtracker=libprofile

FLOWFLAGS += -Xlogging,xutil
## QUARANTINE FLOWFLAGS += -Xcodec
## QUARANTINE FLOWFLAGS += -Xsymtab
## QUARANTINE FLOWFLAGS += -Xunique
## QUARANTINE FLOWFLAGS += -Xprofileval
## QUARANTINE FLOWFLAGS += -Xsighnd
## QUARANTINE FLOWFLAGS += -Xinifile
## QUARANTINE FLOWFLAGS += -Xdbusif

CFLOW := cflow -v --omit-arguments

%.eps     : %.ps    ; ps2epsi $< $@
%.ps      : %.dot   ; dot -Tps  $< -o $@
%.pdf     : %.dot   ; dot -Tpdf $< -o $@
%.png     : %.dot   ; dot -Tpng $< -o $@
%.mod.dot : %.cflow ; ./flow.py -c1 $(FLOWFLAGS) < $< > $@
%.fun.dot : %.cflow ; ./flow.py -c0 $(FLOWFLAGS) < $< > $@
%.api.dot : %.cflow ; ./flow.py -i1 $(FLOWFLAGS) < $< > $@
%.top.dot : %.cflow ; ./flow.py -t1 $(FLOWFLAGS) < $< > $@
%.cflow   :         ; $(CFLOW) -o $@ $^

# ----------------------------------------------------------------------------
# profiled
# ----------------------------------------------------------------------------

profiled_src = \
  profiled.c\
  mainloop.c\
  logging.c\
  sighnd.c\
  server.c\
  database.c\
  confmon.c\
  inifile.c\
  unique.c\
  symtab.c\
  codec.c\
  xutil.c\
  profileval.c

profiled_obj = $(profiled_src:.c=.o)

profiled : $(profiled_obj)
profiled.cflow : $(profiled_src)

# ----------------------------------------------------------------------------
# libprofile
# ----------------------------------------------------------------------------

libprofile_src =\
 libprofile.c\
 connection.c\
 tracker.c\
 codec.c\
 profileval.c\
 logging_client.c

libprofile_obj = $(libprofile_src:.c=.o)

libprofile$(SO) : $(libprofile_obj:.o=.pic.o)

libprofile.a : $(libprofile_obj)

# ----------------------------------------------------------------------------
# profileclient
# ----------------------------------------------------------------------------

profileclient_src = profileclient.c
profileclient_obj = $(profileclient_src:.c=.o)

#profileclient : CFLAGS += -Wno-missing-prototypes
profileclient : $(profileclient_obj) libprofile$(SO)

profileclient.cflow : $(profileclient_src) $(libprofile_src)

# special case: no warnings from using deprecated functions
profileclient.o: CFLAGS += -Wno-deprecated-declarations

# ----------------------------------------------------------------------------
# profile-tracker  -- debug stuff
# ----------------------------------------------------------------------------

profiletracker_src = profile-tracker.c logging.c
profiletracker_obj = $(profiletracker_src:.c=.o)

profile-tracker : $(profiletracker_obj) libprofile$(SO)
profile-tracker.cflow : $(profiletracker_src) $(libprofile_src)

# ----------------------------------------------------------------------------
# camera-example1
# ----------------------------------------------------------------------------

camera-example1 : camera-example1.o libprofile$(SO)

# ----------------------------------------------------------------------------
# profiled.deb
# ----------------------------------------------------------------------------

install-profiled-bin: profiled

install-profiled-backup-config: osso-backup/profiled.conf
install-profiled-backupfw-config: backupfw/profiled.conf

install-profiled-cud-scripts: osso-cud-scripts/profiled.sh
install-profiled-rfs-scripts: osso-rfs-scripts/profiled.sh

install-profiled-appkiller: install-profiled-cud-scripts install-profiled-rfs-scripts

distclean:: ; $(RM) osso-backup/profiled.conf

install-profiled-backup-restore:

install-profiled-ini: ini/10.meego_default.ini
	install -m755 -d $(ROOT)$(ETCDIR)/
	install -m644 ini/10.meego_default.ini $(ROOT)$(ETCDIR)/10.meego_default.ini

install-profiled:: $(addprefix install-profiled-, bin ini)
ifeq ($(USE_SYSTEM_BUS),y)
	install -m755 -d $(ROOT)$(PREFIX)/share/dbus-1/system-services
	install -m644 profiled.system.service \
		$(ROOT)$(PREFIX)/share/dbus-1/system-services/profiled.service
	install -m755 -d $(ROOT)/etc/dbus-1/system.d
	install -m644 profiled.system.conf \
		$(ROOT)/etc/dbus-1/system.d/profiled.conf
else
	install -m755 -d $(ROOT)$(PREFIX)/share/dbus-1/services
	install -m644 profiled.service \
		$(ROOT)$(PREFIX)/share/dbus-1/services/profiled.service
endif
# ----------------------------------------------------------------------------
# profileclient.deb
# ----------------------------------------------------------------------------

install-profileclient-bin: profileclient #profile-tracker

install-profileclient:: $(addprefix install-profileclient-, bin)

# ----------------------------------------------------------------------------
# libprofile.deb
# ----------------------------------------------------------------------------

install-libprofile-dll: libprofile$(SO)

install-libprofile:: $(addprefix install-libprofile-, dll)

# ----------------------------------------------------------------------------
# libprofile-dev.deb
# ----------------------------------------------------------------------------

install-libprofile-dev-inc: libprofile.h profileval.h
install-libprofile-dev-lib: libprofile.a

install-libprofile-dev:: $(addprefix install-libprofile-dev-, lib inc) profile.pc
	ln -sf libprofile$(SO) $(ROOT)$(LIBDIR)/libprofile.so
	install -m755 -d $(ROOT)$(PKGCFGDIR)
	install -m644 profile.pc $(ROOT)$(PKGCFGDIR)/

clean::
	$(RM) profile.pc

# ----------------------------------------------------------------------------
# libprofile-doc.deb
# ----------------------------------------------------------------------------

install-libprofile-doc-html: doxygen.executed
	install -m755 -d $(ROOT)$(DEVDOCDIR)/html
	install -m644 $(DOXYDIR)/html/* $(ROOT)$(DEVDOCDIR)/html/

install-libprofile-doc-man: doxygen.executed
	install -m755 -d $(ROOT)$(MANDIR)/man3
	install -m644 $(DOXYDIR)/man/man3/* $(ROOT)$(MANDIR)/man3/

install-libprofile-doc:: $(addprefix install-libprofile-doc-, html man)

# ----------------------------------------------------------------------------
# Dependency Scanning
# ----------------------------------------------------------------------------

.PHONY: depend

depend::
	gcc -MM $(CPPFLAGS) *.c | ./depend_filter.py > .depend

ifneq ($(MAKECMDGOALS),depend)
include .depend
endif

# ----------------------------------------------------------------------------
# Prototype Scanning
# ----------------------------------------------------------------------------

.PHONY: %.proto

%.proto : %.c
	cproto $(CPPFLAGS) $< | prettyproto.py

# ----------------------------------------------------------------------------
# CTAG scanning
# ----------------------------------------------------------------------------

.PHONY: tags

tags:
	ctags *.c *.h *.inc

distclean::
	$(RM) tags

clean::
	$(RM) *.cflow *.mod.dot *.fun.dot

eps_files:\
  doc/profiled.top.eps\
  profileclient.api.eps\
  profileclient.top.eps\
  profiled.api.eps\
  profiled.top.eps\

# ----------------------------------------------------------------------------
# Normalize whitespace in source code
# ----------------------------------------------------------------------------

.PHONY: normalize

normalize:
	crlf -a *.[ch] debian/control
	crlf -M Makefile debian/rules
	crlf -t -e -k debian/changelog
	crlf -M examples/Makefile
	crlf -a examples/*.[ch]
