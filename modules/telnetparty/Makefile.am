# $Id: Makefile.am,v 1.4 2004/06/17 13:32:44 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= telnetparty.la
telnetparty_la_SOURCES	= telnetparty.c telnetparty.h events.c
telnetparty_la_LDFLAGS	= -module -avoid-version -no-undefined
telnetparty_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
