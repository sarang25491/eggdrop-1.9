# $Id: Makefile.am,v 1.9 2002/09/20 02:06:25 stdarg Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir) -I$(top_srcdir)/lib

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= botnetutil.c \
			botnetutil.h \
			common.h \
			eggdrop.c \
			eggdrop.h \
			eggdns.c eggdns.h \
			eggident.c eggident.h \
			eggnet.h eggnet.c \
			eggtimer.c eggtimer.h \
			fileutil.c \
			fileutil.h \
			linemode.c linemode.h \
			memutil.c \
			memutil.h \
			my_socket.c my_socket.h \
			script.c script.h \
			sockbuf.c sockbuf.h \
			stat.h

libeggdrop_la_LIBADD	= $(top_builddir)/lib/compat/libcompat.la \
		   	$(top_builddir)/lib/egglib/libegg.la \
			$(top_builddir)/lib/adns/libadns.la

