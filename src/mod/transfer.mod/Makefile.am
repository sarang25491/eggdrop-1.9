# $Id: Makefile.am,v 1.2 2001/10/13 21:37:01 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(exec_prefix)/modules

pkglib_LTLIBRARIES  = transfer.la
transfer_la_SOURCES = transfer.c
transfer_la_LDFLAGS = -module -avoid-version -no-undefined
transfer_la_LIBADD  = @TCL_LIBS@ @LIBS@

MAINTAINERCLEANFILES    = Makefile.in

INCLUDES                = -I$(top_builddir) -I$(top_srcdir)

DEFS = $(EGG_DEBUG) @DEFS@

