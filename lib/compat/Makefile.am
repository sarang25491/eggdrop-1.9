# $Id: Makefile.am,v 1.10 2004/06/19 09:59:12 wingman Exp $

## libcompat is built as convenience library

MAINTAINERCLEANFILES	= Makefile.in

noinst_LTLIBRARIES	= libcompat.la

# LIBOBJS files are automatically added to the 'make dist' tarball
libcompat_la_LIBADD	= @LTLIBOBJS@ $(SNPRINTF_LIBS)
libcompat_la_CPPFLAGS	= -I$(top_builddir) -I$(top_srcdir)
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
