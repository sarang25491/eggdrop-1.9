# $Id: Makefile.am,v 1.2 2003/12/14 08:51:02 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= ircparty.la
ircparty_la_SOURCES	= ircparty.c ircparty.h events.c
ircparty_la_LDFLAGS	= -module -avoid-version -no-undefined
ircparty_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
