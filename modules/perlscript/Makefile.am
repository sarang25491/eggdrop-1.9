# $Id: Makefile.am,v 1.9 2004/06/14 14:45:53 wingman Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(exec_prefix)/modules

pkglib_LTLIBRARIES    = perlscript.la
perlscript_la_SOURCES = perlscript.c mod_iface.c
perlscript_la_LDFLAGS = -module -avoid-version -no-undefined
perlscript_la_LIBADD  = @PERL_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES    = Makefile.in

INCLUDES                = -I$(top_builddir) \
			-I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl \
			-I$(top_srcdir)/intl \
			-I$(top_builddir)/lib \
			-I$(top_srcdir)/lib @PERL_CCFLAGS@

DEFS = $(EGG_DEBUG) @DEFS@

