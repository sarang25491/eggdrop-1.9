# $Id: Makefile.am,v 1.3 2004/06/17 13:32:44 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= proxy.la
proxy_la_SOURCES	= proxy.c proxy.h http.c socks5.c
proxy_la_LDFLAGS	= -module -avoid-version -no-undefined
proxy_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
