# $Id: Makefile.am,v 1.2 2001/10/29 23:01:34 ite Exp $

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

INCLUDES                = -I$(top_builddir) -I$(top_srcdir) @PERL_CCFLAGS@

DEFS = $(EGG_DEBUG) @DEFS@

