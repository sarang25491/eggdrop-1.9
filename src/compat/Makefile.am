# $Id: Makefile.am,v 1.5 2001/10/21 16:05:52 tothwolf Exp $

## libcompat is built as convenience library

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

noinst_LTLIBRARIES	= libcompat.la

# LIBOBJS files are automatically added to the 'make dist' tarball
libcompat_la_LIBADD	= @LIBOBJS@ $(SNPRINTF_LIBS)

libcompat_la_SOURCES	= compat.h \
			inet_aton.h \
			inet_ntop.h \
			inet_pton.h \
			memcpy.h \
			memset.h \
			snprintf.h \
			strcasecmp.h \
			strncasecmp.h \
			strftime.h
