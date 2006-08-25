# $Id: Makefile.am,v 1.7 2006/08/25 17:22:50 sven Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= script.la
script_la_SOURCES	= scriptconfig.c scriptlog.c scriptmod.c scriptbind.c scriptnet.c scriptdns.c scriptuser.c scriptparty.c scripttimer.c scriptmisc.c
script_la_LDFLAGS	= -module -avoid-version -no-undefined
script_la_LIBADD	= @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
