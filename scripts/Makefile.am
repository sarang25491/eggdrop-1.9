# $Id: Makefile.am,v 1.3 2003/12/17 00:58:02 wcc Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(prefix)

SCRIPTS_DIST_EXTRA	= alltools.tcl autobotchk botchk compat.tcl CONTENTS
SUBDIRS			= 
MAINTAINERCLEANFILES	= Makefile.in

dist-hook:
	(cd $(distdir) && mkdir scripts)
	for file in $(SCRIPTS_DIST_EXTRA); do \
	  ln $$file $(distdir)/scripts 2>/dev/null || \
	  cp -p $$file $(distdir)/scripts/$$file; \
	done

install-data-local:
	($(mkinstalldirs) $(pkglibdir)/scripts)
	for file in $(SCRIPTS_DIST_EXTRA); do \
	  $(INSTALL_DATA) $$file $(pkglibdir)/scripts/$$file; \
	done
