# $Id: Makefile.am,v 1.30 2002/10/07 22:36:36 stdarg Exp $

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
			chan.h \
			chanprog.c chanprog.h \
			cmdt.h \
			core_binds.c core_binds.h \
			dcc.c dcc.h \
			dccutil.c dccutil.h \
			debug.h \
			dns.c dns.h \
			egg.h \
			egg_timer.c \
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
			net.h \
			patch.h \
			scriptnet.c \
			scriptuser.c \
			tcl.c \
			tcl.h \
			tcldcc.c \
			tcldcc.h \
			tclegg.h \
			tclhash.c \
			tclhash.h \
			tclmisc.c \
			tcluser.c \
			traffic.c \
			traffic.h \
			userent.c \
			userent.h \
			userrec.c \
			userrec.h \
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
			$(EGG_PRL) \
			-dlopen ../modules/server/server.la \
			$(EGG_TCL) \
			$(top_builddir)/lib/eggdrop/libeggdrop.la \
			$(XLIBS)

libtcle.a: $(TCLLIB)/lib$(TCLLIBFN)
	echo "[ Fixing lib$(TCLLIBFN) -> libtcle.a ]"
	cp $(TCLLIB)/lib$(TCLLIBFN) libtcle.a
	chmod u+rw libtcle.a
	ar d libtcle.a tclMain.o
	$(RANLIB) libtcle.a
