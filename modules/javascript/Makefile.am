# $Id: Makefile.am,v 1.3 2003/12/18 07:10:03 wcc Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

# FIXME: test if there's dependency on gnu make. It shouldn't.
if EGG_JAVASCRIPT
  EGG_JS = javascript.la
else
  EGG_JS =
endif
    
pkglib_LTLIBRARIES	= $(EGG_JS)
javascript_la_SOURCES	= javascript.c
javascript_la_LDFLAGS	= -module -avoid-version -no-undefined @JSLDFLAGS@
javascript_la_LIBADD	= @JSLIBS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) \
			-I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl \
			-I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib @JSINCL@

DEFS			= $(EGG_DEBUG) @DEFS@
