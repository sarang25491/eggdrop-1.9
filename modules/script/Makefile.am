# $Id: Makefile.am,v 1.3 2003/02/14 07:06:19 stdarg Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir		= $(exec_prefix)/modules

pkglib_LTLIBRARIES	= script.la
script_la_SOURCES	= scriptconfig.c scriptlog.c scriptmod.c scriptbind.c scriptnet.c scriptuser.c scriptparty.c scripttimer.c
script_la_LDFLAGS	= -module -avoid-version -no-undefined
script_la_LIBADD	= @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib

DEFS			= $(EGG_DEBUG) @DEFS@
