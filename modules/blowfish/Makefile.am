# $Id: Makefile.am,v 1.5 2002/05/05 04:12:18 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= blowfish.la
blowfish_la_SOURCES	= blowfish.c
blowfish_la_LDFLAGS	= -module -avoid-version -no-undefined
blowfish_la_LIBADD	= @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
