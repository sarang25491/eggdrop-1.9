# $Id: Makefile.am,v 1.4 2004/06/17 13:32:43 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= dccparty.la
dccparty_la_SOURCES	= dccparty.c events.c dccparty.h
dccparty_la_LDFLAGS	= -module -avoid-version -no-undefined
dccparty_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
