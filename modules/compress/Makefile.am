# $Id: Makefile.am,v 1.4 2002/01/04 02:56:22 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

# FIXME: test if there's dependency on gnu make. It shouldn't.
if EGG_COMPRESS
  EGG_CMP = compress.la
else
  EGG_CMP =
endif

pkglib_LTLIBRARIES	= $(EGG_CMP)
compress_la_SOURCES	= compress.c
compress_la_LDFLAGS	= -module -avoid-version -no-undefined
compress_la_LIBADD	= @TCL_LIBS@ @ZLIB@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
