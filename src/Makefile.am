CC = @CC@
CPP = @CPP@
AM_CPPFLAGS = $(TCUTILS_CFLAGS) -I$(top_srcdir)/include
AM_CPPLIBS = $(TCUTILS_LIBS) -lpthread
DEFS += $(SESSIONBUS)

lib_LTLIBRARIES = libtcutils.la
libtcutils_la_SOURCES = TCDBusRawAPI.c TCInput.c example.c TCLog.c
libtcutils_la_LIBADD = -lpthread
libtcutils_la_LDFLAGS = -version-info $(TCUTIL_VERSION_INFO)
