# $Id: Makefile.am,v 1.9 2003/06/11 08:37:38 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= server.la
server_la_SOURCES	= server.c binds.c input.c output.c serverlist.c channels.c nicklist.c scriptcmds.c servsock.c dcc_commands.c dcc.c
server_la_LDFLAGS	= -module -avoid-version -no-undefined
server_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
