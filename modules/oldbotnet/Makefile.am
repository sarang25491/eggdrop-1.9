# $Id: Makefile.am,v 1.1 2003/12/12 19:45:27 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= oldbotnet.la
oldbotnet_la_SOURCES	= oldbotnet.c events.c
oldbotnet_la_LDFLAGS	= -module -avoid-version -no-undefined
oldbotnet_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
