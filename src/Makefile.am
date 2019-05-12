lib_LTLIBRARIES = \
	libofono.la

libofono_la_CPPFLAGS = \
	$(GLIB_CFLAGS) \
	$(DBUS_CFLAGS) \
	$(ICD2_CFLAGS) \
	$(OFONO_CFLAGS)

libofono_la_SOURCES = \
	notifier.c \
	dbus-helpers.c \
	modem.c \
	ofono-net.c \
	ofono-sim.c \
	ofono-modem.c \
	ofono-manager.c

libofono_la_LIBADD = \
	$(GLIB_LIBS) \
	$(DBUS_LIBS) \
	$(ICD2_LIBS)

libofono_la_LDFLAGS = -Wl,--as-needed -Wl,--no-undefined

MAINTAINERCLEANFILES = \
	Makefile.in
