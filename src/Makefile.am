# $Id: Makefile.am,v 1.45 2003/08/06 18:01:00 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
bindir			= $(exec_prefix)

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

localedir		= $(datadir)/locale
DEFS			= -DLOCALEDIR=\"\$(localedir)\" $(EGG_DEBUG) @DEFS@

bin_PROGRAMS		= eggdrop
eggdrop_SOURCES		= bg.c bg.h \
			cmdt.h \
			core_binds.c core_binds.h \
			core_config.c core_config.h \
			core_party.c \
			debug.h \
			egg.h \
			logfile.c logfile.h \
			main.c main.h

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
			$(LIBS)
