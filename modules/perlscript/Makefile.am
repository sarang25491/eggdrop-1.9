# $Id: Makefile.am,v 1.11 2004/06/20 09:46:41 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= perlscript.la

# Do not use CPPFLAGS, it's not supported by automake-1.6
perlscript_la_CFLAGS	= -I@PERL_CCFLAGS@
perlscript_la_SOURCES	= perlscript.c mod_iface.c
perlscript_la_LDFLAGS	= -module -avoid-version -no-undefined
perlscript_la_LIBADD	= @PERL_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
