# $Id: Makefile.am,v 1.10 2004/06/17 13:32:44 wingman Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES    = perlscript.la

perlscript_la_CPPFLAGS   = -I@PERL_CCFLAGS@
perlscript_la_SOURCES = perlscript.c mod_iface.c
perlscript_la_LDFLAGS = -module -avoid-version -no-undefined
perlscript_la_LIBADD  = @PERL_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
