# $Id: Makefile.am,v 1.4 2002/01/06 06:21:08 ite Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir) -I$(top_srcdir)/lib

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= botnetutil.c \
			botnetutil.h \
			common.h \
			eggdrop.h \
			memutil.c \
			memutil.h

libeggdrop_la_LIBADD	= $(top_builddir)/lib/compat/libcompat.la \
		   	$(top_builddir)/lib/egglib/libegg.la \
			$(top_builddir)/lib/adns/libadns.la

