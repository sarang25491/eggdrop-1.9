# $Id: Makefile.am,v 1.13 2002/11/03 23:16:09 stdarg Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir) -I$(top_srcdir)/lib

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= binds.c binds.h \
			botnetutil.c botnetutil.h \
			common.h \
			eggdrop.c \
			eggdrop.h \
			eggdns.c eggdns.h \
			eggident.c eggident.h \
			eggnet.h eggnet.c \
			eggtimer.c eggtimer.h \
			fileutil.c fileutil.h \
			flags.c flags.h \
			hash_table.c hash_table.h \
			ircmasks.c ircmasks.h \
			linemode.c linemode.h \
			match.c match.h \
			md5.c md5.h \
			memutil.c \
			memutil.h \
			my_socket.c my_socket.h \
			partyline.c partyline.h \
			script.c script.h \
			sockbuf.c sockbuf.h \
			stat.h \
			throttle.c throttle.h \
			users.c users.h \
			xml.c xmlread.c xmlwrite.c xml.h

libeggdrop_la_LIBADD	= $(top_builddir)/lib/compat/libcompat.la \
		   	$(top_builddir)/lib/egglib/libegg.la \
			$(top_builddir)/lib/adns/libadns.la

