# $Id: Makefile.am,v 1.4 2004/06/17 13:32:43 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= oldbotnet.la
oldbotnet_la_SOURCES	= oldbotnet.c oldbotnet.h events.c
oldbotnet_la_LDFLAGS	= -module -avoid-version -no-undefined
oldbotnet_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
