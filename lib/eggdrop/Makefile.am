# $Id: Makefile.am,v 1.24 2003/08/19 18:20:04 stdarg Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir) -I$(top_srcdir)/lib $(INCLTDL)

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= base64.c base64.h \
			binds.c binds.h \
			common.h \
			eggconfig.c eggconfig.h \
			eggdrop.c eggdrop.h \
			eggdns.c eggdns.h \
			eggident.c eggident.h \
			eggmod.c eggmod.h \
			eggnet.c eggnet.h \
			eggowner.c eggowner.h \
			eggstring.c eggstring.h \
			eggtimer.c eggtimer.h \
			fileutil.c fileutil.h \
			flags.c flags.h \
			garbage.c garbage.h \
			hash_table.c hash_table.h \
			irccmp.c irccmp.h \
			ircmasks.c ircmasks.h \
			ircparse.c ircparse.h \
			linemode.c linemode.h \
			egglog.c egglog.h \
			match.c match.h \
			md5.c md5.h \
			memutil.c memutil.h \
			my_socket.c my_socket.h \
			partyline.c partyline.h \
			partychan.c partymember.c \
			script.c script.h \
			sockbuf.c sockbuf.h \
			stat.h \
			throttle.c throttle.h \
			users.c users.h \
			xml.c xmlread.c xmlwrite.c xml.h

libeggdrop_la_LIBADD	= @LIBLTDL@ \
			$(top_builddir)/lib/compat/libcompat.la
