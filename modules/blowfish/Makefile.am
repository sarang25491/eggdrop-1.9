# $Id: Makefile.am,v 1.2 2001/10/29 23:01:33 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= blowfish.la
blowfish_la_SOURCES	= blowfish.c
blowfish_la_LDFLAGS	= -module -avoid-version -no-undefined
blowfish_la_LIBADD	= @TCL_LIBS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

DEFS			= $(EGG_DEBUG) @DEFS@
