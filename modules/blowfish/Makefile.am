# $Id: Makefile.am,v 1.9 2004/06/17 13:32:43 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= blowfish.la
blowfish_la_SOURCES	= blowfish.c blowfish.h bf_tab.h
blowfish_la_LDFLAGS	= -module -avoid-version -no-undefined
blowfish_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
