# $Id: Makefile.am,v 1.2 2002/01/16 22:09:40 ite Exp $

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
			strdup.h \
			strncasecmp.h \
			strftime.h
