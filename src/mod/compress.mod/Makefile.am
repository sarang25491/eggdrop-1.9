# $Id: Makefile.am,v 1.3 2001/10/14 10:22:50 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(exec_prefix)/modules

# FIXME: dependency on gnu make
if EGG_COMPRESS
  EGG_CMP = compress.la
else
  EGG_CMP =
endif

pkglib_LTLIBRARIES  = $(EGG_CMP)
compress_la_SOURCES = compress.c
compress_la_LDFLAGS = -module -avoid-version -no-undefined
compress_la_LIBADD  = @TCL_LIBS@ @ZLIB@ @LIBS@

MAINTAINERCLEANFILES    = Makefile.in configure config.h.in stamp-h.in

INCLUDES                = -I$(top_builddir) -I$(top_srcdir)

DEFS = $(EGG_DEBUG) @DEFS@

