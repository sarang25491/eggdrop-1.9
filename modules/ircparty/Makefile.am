# $Id: Makefile.am,v 1.4 2004/06/17 13:32:43 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= ircparty.la
ircparty_la_SOURCES	= ircparty.c ircparty.h events.c
ircparty_la_LDFLAGS	= -module -avoid-version -no-undefined
ircparty_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
