# $Id: Makefile.am,v 1.9 2003/03/06 13:49:53 tothwolf Exp $

## libcompat is built as convenience library

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

noinst_LTLIBRARIES	= libcompat.la

# LIBOBJS files are automatically added to the 'make dist' tarball
libcompat_la_LIBADD	= @LTLIBOBJS@ $(SNPRINTF_LIBS)

libcompat_la_SOURCES	= compat.h \
			getopt.h \
			inet_ntop.h \
			inet_pton.h \
			memcpy.h \
			memset.h \
			poll.h \
			snprintf.h \
			strcasecmp.h \
			strdup.h \
			strerror.h \
			strlcat.h \
			strlcpy.h \
			strncasecmp.h \
			strftime.h
