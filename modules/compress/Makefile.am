# $Id: Makefile.am,v 1.5 2004/06/14 14:45:53 wingman Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= compress.la 
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
