# $Id: Makefile.am,v 1.1 2001/10/27 16:34:54 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= woobie.la
woobie_la_SOURCES	= woobie.c
woobie_la_LDFLAGS	= -module -avoid-version -no-undefined
woobie_la_LIBADD	= @TCL_LIBS@ @LIBS@

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

DEFS			= $(EGG_DEBUG) @DEFS@
