# $Id: Makefile.am,v 1.1 2004/12/13 15:02:29 stdarg Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= chanserv.la
chanserv_la_SOURCES	= chanserv.c chanserv.h events.c
chanserv_la_LDFLAGS	= -module -avoid-version -no-undefined
chanserv_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
