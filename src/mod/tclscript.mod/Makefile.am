# $Id: Makefile.am,v 1.2 2001/10/21 16:05:55 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= tclscript.la
tclscript_la_SOURCES	= tclscript.c
tclscript_la_LDFLAGS	= -module -avoid-version -no-undefined
tclscript_la_LIBADD	= @TCL_LIBS@ @LIBS@

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

DEFS			= $(EGG_DEBUG) @DEFS@
