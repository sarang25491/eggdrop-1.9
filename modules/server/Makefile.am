# $Id: Makefile.am,v 1.10 2003/12/14 08:51:02 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= server.la
server_la_SOURCES	= server.c server.h binds.c binds.h input.c input.h output.c output.h serverlist.c serverlist.h channels.c channels.h nicklist.c nicklist.h scriptcmds.c servsock.c servsock.h dcc_commands.c dcc.c dcc.h
server_la_LDFLAGS	= -module -avoid-version -no-undefined
server_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
