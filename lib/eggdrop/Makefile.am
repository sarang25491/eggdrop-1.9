# $Id: Makefile.am,v 1.44 2007/11/06 00:05:39 sven Exp $

MAINTAINERCLEANFILES	= Makefile.in

lib_LTLIBRARIES 	= libeggdrop.la

DEFS			= $(EGG_DEBUG) @DEFS@

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
			config.c config.h \
			eggdrop.c eggdrop.h \
			date.c date.h \
			dns.c dns.h \
			help.c help.h \
			ident.c ident.h \
			module.c module.h \
			net.c net.h \
			owner.c owner.h \
			string.c string.h \
			timer.c timer.h \
			fileutil.c fileutil.h \
			flags.c flags.h \
			garbage.c garbage.h \
			hash_table.c hash_table.h \
			irccmp.c irccmp.h \
			ircmasks.c ircmasks.h \
			ircparse.c ircparse.h \
			linemode.c linemode.h \
			netstring.c netstring.h \
			logging.c logging.h \
			match.c match.h \
			md5.c md5.h \
			memutil.c memutil.h \
			socket.c socket.h \
			botnet.c botnet.h \
			partyline.c partyline.h \
			partychan.c partychan.h \
			partymember.c partymember.h \
			script.c script.h \
			sockbuf.c sockbuf.h \
			socktimer.c socktimer.h \
			stat.h \
			throttle.c throttle.h \
			users.c users.h \
			xml.c xml.h \
			xmlread.c xmlwrite.c \
			timeutil.c timeutil.h

libeggdrop_la_LIBADD	= @LIBLTDL@ \
			$(top_builddir)/lib/compat/libcompat.la
