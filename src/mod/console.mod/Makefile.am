# $Id: Makefile.am,v 1.1 2001/10/10 01:20:12 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(exec_prefix)/modules

pkglib_LTLIBRARIES = console.la
console_la_SOURCES = console.c
console_la_LDFLAGS = -module -avoid-version -no-undefined
console_la_LIBADD  = @TCL_LIBS@ @LIBS@

MAINTAINERCLEANFILES    = Makefile.in

INCLUDES                = -I$(top_builddir) -I$(top_srcdir)

DEFS = @EGG_DEBUG@ @DEFS@

