# $Id: Makefile.am,v 1.1 2007/06/03 23:43:45 sven Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES = botnet.la
botnet_la_SOURCES  = botnet.c botnet.h events.c
botnet_la_LDFLAGS  = -module -avoid-version -no-undefined
botnet_la_LIBADD   = @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la
