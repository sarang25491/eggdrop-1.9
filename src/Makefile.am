# $Id: Makefile.am,v 1.48 2004/06/14 14:45:53 wingman Exp $

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
			core_binds.c core_binds.h \
			core_config.c core_config.h \
			core_party.c \
			debug.h \
			logfile.c logfile.h \
			main.c main.h

eggdrop_LDFLAGS		= -no-undefined
eggdrop_LDADD		= @LIBLTDL@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la \
			@LIBEGGDROP_PRELOAD@
			$(LIBS)
