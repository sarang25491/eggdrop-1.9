# $Id: Makefile.am,v 1.11 2004/06/13 23:31:37 takeda Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

# FIXME: test if there's dependency on gnu make. It shouldn't.
if EGG_TCLSCRIPT
  EGG_TCL = tclscript.la
else
  EGG_TCL =
endif
    
pkglib_LTLIBRARIES	= $(EGG_TCL)
tclscript_la_SOURCES	= tclscript.c
tclscript_la_LDFLAGS	= -module -avoid-version -no-undefined
tclscript_la_LIBADD	= @TCL_LIBS@ @TCL_LIB_SPEC@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) \
			-I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl \
			-I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib \
			@TCL_INCLUDE_SPEC@

DEFS			= $(EGG_DEBUG) @DEFS@
