# $Id: Makefile.am,v 1.3 2001/11/24 20:42:24 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= console.la
console_la_SOURCES	= console.c
console_la_LDFLAGS	= -module -avoid-version -no-undefined
console_la_LIBADD	= @TCL_LIBS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib/eggdrop \
			-I$(top_srcdir)/lib/eggdrop

DEFS			= $(EGG_DEBUG) @DEFS@
