# $Id: Makefile.am,v 1.6 2002/03/22 16:01:16 ite Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir) -I$(top_srcdir)/lib

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= botnetutil.c \
			botnetutil.h \
			common.h \
			eggdrop.c \
			eggdrop.h \
			fileutil.c \
			fileutil.h \
			memutil.c \
			memutil.h \
			registry.c \
			registry.h \
			stat.h

libeggdrop_la_LIBADD	= $(top_builddir)/lib/compat/libcompat.la \
		   	$(top_builddir)/lib/egglib/libegg.la \
			$(top_builddir)/lib/adns/libadns.la

