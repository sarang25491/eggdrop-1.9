# $Id: Makefile.am,v 1.5 2004/06/17 13:32:43 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= javascript.la 
javascript_la_SOURCES	= javascript.c
javascript_la_LDFLAGS	= -module -avoid-version -no-undefined @JSLDFLAGS@
javascript_la_LIBADD	= @JSLIBS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
