# $Id: Makefile.am,v 1.3 2001/10/14 10:22:50 tothwolf Exp $

## libcompat is built as convenience library

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

noinst_LTLIBRARIES = libcompat.la
libcompat_la_SOURCES = compat.h \
		gnu_strftime.c \
		inet_aton.c \
		inet_aton.h \
		inet_ntop.c \
		inet_ntop.h \
		inet_pton.c \
		inet_pton.h \
		snprintf.c \
		snprintf.h \
		strcasecmp.c \
		strcasecmp.h \
		strftime.c \
		strftime.h

