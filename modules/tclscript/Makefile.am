# $Id: Makefile.am,v 1.13 2004/06/17 13:32:44 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= tclscript.la 
tclscript_la_SOURCES	= tclscript.c
tclscript_la_LDFLAGS	= -module -avoid-version -no-undefined
tclscript_la_LIBADD	= @TCL_LIBS@ @TCL_LIB_SPEC@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
