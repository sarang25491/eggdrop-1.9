# $Id: Makefile.am,v 1.30 2004/06/21 11:35:46 wingman Exp $

MAINTAINERCLEANFILES	= Makefile.in

lib_LTLIBRARIES 	= libeggdrop.la

# We need to overwrite the DEFAULT_INCLUDES which contains "-I ."
# since #include <string.h> would actually include
# <lib/eggdrop/string.h> if -I. is supplied. By override the default
# includes we can safely (and portable) use string.h or config.h for
# our filenames.
DEFAULT_INCLUDES	= 

# Do not use CPPFLAGS since it's not supported by automake-1.6
libeggdrop_la_CFLAGS	= -I$(top_srcdir) -I$(top_srcdir)/lib $(INCLTDL)
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
			help.c help.h \
			irccmp.c irccmp.h \
			ircmasks.c ircmasks.h \
			ircparse.c ircparse.h \
			linemode.c linemode.h \
			egglog.c egglog.h \
			match.c match.h \
			md5.c md5.h \
			memutil.c memutil.h \
			socket.c socket.h \
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
