# $Id: Makefile.am,v 1.3 2001/11/24 20:42:25 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(exec_prefix)/modules

# FIXME: test if there's dependency on gnu make. It shouldn't.
if EGG_PERLSCRIPT
  EGG_PRL = perlscript.la
else
  EGG_PRL =
endif

pkglib_LTLIBRARIES  = $(EGG_PRL)
perlscript_la_SOURCES = perlscript.c
perlscript_la_LDFLAGS = -module -avoid-version -no-undefined
perlscript_la_LIBADD  = @PERL_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la

MAINTAINERCLEANFILES    = Makefile.in

INCLUDES                = -I$(top_builddir) -I$(top_srcdir) @INCLTDL@ \
			-I$(top_builddir)/intl -I$(top_srcdir)/intl \
			-I$(top_builddir)/lib/eggdrop \
			-I$(top_srcdir)/lib/eggdrop @PERL_CCFLAGS@

DEFS = $(EGG_DEBUG) @DEFS@

