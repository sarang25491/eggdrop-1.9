# $Id: Makefile.am,v 1.7 2001/10/13 21:36:59 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
bindir    = $(exec_prefix)

# stuff for Tcl
XREQS = @TCL_REQS@
XLIBS = @TCL_LIBS@ @LIBS@
TCLLIB = @TCLLIB@
TCLLIBFN = @TCLLIBFN@

SUBDIRS			= compat egglib adns mod
MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl

localedir		= $(datadir)/locale
DEFS			= -DLOCALEDIR=\"\$(localedir)\" $(EGG_DEBUG) @DEFS@

bin_PROGRAMS	= eggdrop
eggdrop_SOURCES = bg.c \
		bg.h \
		botcmd.c \
		botmsg.c \
		botnet.c \
		chan.h \
		chanprog.c \
		cmds.c \
		cmdt.h \
		dcc.c \
		dccutil.c \
		debug.h \
		dns.c \
		dns.h \
		egg_timer.c \
		egg_timer.h \
		eggdrop.h \
		flags.c \
		flags.h \
		irccmp.c \
		irccmp.h \
		main.c \
		main.h \
		match.c \
		match.h \
		md5.c \
		md5.h \
		misc.c \
		misc_file.c \
		misc_file.h \
		modules.c \
		modules.h \
		net.c \
		patch.h \
		proto.h \
		stat.h \
		tandem.h \
		tcl.c \
		tcldcc.c \
		tclegg.h \
		tclhash.c \
		tclhash.h \
		tclmisc.c \
		tcluser.c \
		userent.c \
		userrec.c \
		users.c \
		users.h

eggdrop_LDFLAGS = -no-undefined
# FIXME: dlopened/preopened module's list is going to be dynamically defined
#        by ./configure
eggdrop_LDADD 	= @LIBLTDL@ \
		"-dlopen" self \
		"-dlopen" mod/assoc.mod/assoc.la \
		"-dlopen" mod/blowfish.mod/blowfish.la \
		"-dlopen" mod/compress.mod/compress.la \
		"-dlopen" mod/channels.mod/channels.la \
		"-dlopen" mod/console.mod/console.la \
		"-dlopen" mod/ctcp.mod/ctcp.la \
		"-dlopen" mod/filesys.mod/filesys.la \
		"-dlopen" mod/irc.mod/irc.la \
		"-dlopen" mod/notes.mod/notes.la \
		"-dlopen" mod/server.mod/server.la \
		"-dlopen" mod/share.mod/share.la \
		"-dlopen" mod/transfer.mod/transfer.la \
		"-dlopen" mod/uptime.mod/uptime.la \
		"-dlopen" mod/woobie.mod/woobie.la \
                compat/libcompat.la \
                adns/libadns.la \
                egglib/libegg.la \
                $(XLIBS)
eggdrop_DEPENDENCIES= $(XREQS)
			src/mod/assoc.mod/assoc.la \
			src/mod/blowflish/blowfish.la \
			src/mod/channels.la \
			src/mod/console.la \
			src/mod/ctcp.la \
			src/mod/filesys.la \
			src/mod/irc.la \
			src/mod/notes.la \
			src/mod/server.la \
			src/mod/share.la \
			src/mod/transfer.la \
			src/mod/uptime.la \
			src/mod/woobie.la

libtcle.a: $(TCLLIB)/lib$(TCLLIBFN)
	echo "[ Fixing lib$(TCLLIBFN) -> libtcle.a ]"
	cp $(TCLLIB)/lib$(TCLLIBFN) libtcle.a
	chmod u+rw libtcle.a
	ar d libtcle.a tclMain.o
	$(RANLIB) libtcle.a

