# $Id: Makefile.am,v 1.22 2002/03/22 16:01:20 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
bindir			= $(exec_prefix)

# stuff for Tcl
XREQS			= @TCL_REQS@
XLIBS			= @TCL_LIBS@ @LIBS@
TCLLIB			= @TCLLIB@
TCLLIBFN		= @TCLLIBFN@

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

localedir		= $(datadir)/locale
DEFS			= -DLOCALEDIR=\"\$(localedir)\" $(EGG_DEBUG) @DEFS@

bin_PROGRAMS		= eggdrop
eggdrop_SOURCES		= bg.c \
			bg.h \
			botcmd.c \
			botmsg.c \
			botnet.c \
			chan.h \
			chanprog.c \
			cmds.c \
			cmdt.h \
			core_binds.c \
			core_binds.h \
			dcc.c \
			dccutil.c \
			debug.h \
			dns.c \
			dns.h \
			egg_timer.c \
			egg_timer.h \
			egg.h \
			flags.c \
			flags.h \
			irccmp.c \
			irccmp.h \
			logfile.c \
			logfile.h \
			main.c \
			main.h \
			match.c \
			match.h \
			md5.c \
			md5.h \
			misc.c \
			misc.h \
			modules.c \
			modules.h \
			net.c \
			patch.h \
			proto.h \
			script.c \
			script.h \
			script_api.h \
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

eggdrop_LDFLAGS		= -no-undefined

# FIXME: test if there's dependency on gnu make. It shouldn't.
if EGG_PERLSCRIPT
  EGG_PRL = -dlopen ../modules/perlscript/perlscript.la
else
  EGG_PRL =
endif
if EGG_TCLSCRIPT
  EGG_TCL = -dlopen ../modules/tclscript/tclscript.la
else
  EGG_TCL =
endif
if EGG_COMPRESS
  EGG_CMP = -dlopen ../modules/compress/compress.la
else
  EGG_CMP = 
endif

# FIXME: dlopened/preopened module's list is going to be dynamically defined
#        by ./configure
eggdrop_LDADD		= @LIBLTDL@ \
			-dlopen ../modules/blowfish/blowfish.la \
			-dlopen ../modules/channels/channels.la \
			$(EGG_CMP) \
			-dlopen ../modules/console/console.la \
			-dlopen ../modules/ctcp/ctcp.la \
			-dlopen ../modules/filesys/filesys.la \
			-dlopen ../modules/irc/irc.la \
			-dlopen ../modules/notes/notes.la \
			$(EGG_PRL) \
			-dlopen ../modules/server/server.la \
			-dlopen ../modules/share/share.la \
			$(EGG_TCL) \
			-dlopen ../modules/transfer/transfer.la \
			-dlopen ../modules/uptime/uptime.la \
			-dlopen ../modules/woobie/woobie.la \
			$(top_builddir)/lib/eggdrop/libeggdrop.la \
			$(XLIBS)

libtcle.a: $(TCLLIB)/lib$(TCLLIBFN)
	echo "[ Fixing lib$(TCLLIBFN) -> libtcle.a ]"
	cp $(TCLLIB)/lib$(TCLLIBFN) libtcle.a
	chmod u+rw libtcle.a
	ar d libtcle.a tclMain.o
	$(RANLIB) libtcle.a
