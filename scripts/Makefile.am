# $Id: Makefile.am,v 1.2 2001/10/21 16:05:52 tothwolf Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(prefix)

SCRIPTS_DIST_EXTRA	= action.fix.tcl \
			cmd_resolve.tcl \
			getops.tcl \
			sentinel.tcl \
			alltools.tcl \
			compat.tcl \
			userinfo.tcl \
			autobotchk \
			klined.tcl \
			notes2.tcl \
			weed \
			botchk \
			ques5.tcl \
			CONTENTS \
			help/cmd_resolve.help \
			help/userinfo.help \
			help/msg/userinfo.help

SUBDIRS			= 
MAINTAINERCLEANFILES	= Makefile.in

dist-hook:
	(cd $(distdir) && mkdir scripts && mkdir scripts/help && \
		mkdir scripts/help/msg)
	for file in $(SCRIPTS_DIST_EXTRA); do \
	  ln $$file $(distdir)/scripts 2>/dev/null || \
	  cp -p $$file $(distdir)/scripts/$$file; \
	done

install-data-local:
	($(mkinstalldirs) $(pkglibdir)/scripts $(pkglibdir)/scripts/help \
		$(pkglibdir)/scripts/help/msg)
	for file in $(SCRIPTS_DIST_EXTRA); do \
	  $(INSTALL_DATA) $$file $(pkglibdir)/scripts/$$file; \
	done
