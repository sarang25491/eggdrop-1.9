# $Id: Makefile.am,v 1.3 2001/10/21 16:05:54 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= irc.la
irc_la_SOURCES		= irc.c
irc_la_LDFLAGS		= -module -avoid-version -no-undefined
irc_la_LIBADD		= @TCL_LIBS@ @LIBS@

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir)

DEFS			= $(EGG_DEBUG) @DEFS@
