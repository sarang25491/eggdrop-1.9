# $Id: Makefile.am,v 1.16 2003/06/09 02:21:53 stdarg Exp $

AUX_DIST		= $(ac_aux_dir)/install-sh \
			$(ac_aux_dir)/ltmain.sh \
			$(ac_aux_dir)/missing \
			$(ac_aux_dir)/mkinstalldirs \
			$(ac_aux_dir)/depcomp

AUX_DIST_EXTRA		= $(ac_aux_dir)/acinclude.m4 \
			$(ac_aux_dir)/mpatrol.m4 \
			$(ac_aux_dir)/config.guess \
			$(ac_aux_dir)/config.sub \
			$(ac_aux_dir)/config.rpath

EXTRA_DIST		= bootstrap

SUBDIRS			= intl libltdl lib m4 modules src scripts nettype po
DISTCLEANFILES		=
MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure config-h.in \
			config.cache config.log config.status \
			config.h.in stamp-h.in $(AUX_DIST)
DISTCLEANLOCAL		= autom4te.cache
MAINTAINERCLEANLOCAL	= libltdl

ACLOCAL_AMFLAGS		= -I m4
ACLOCAL			= aclocal -I $(ac_aux_dir)

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir); \
	done

install-data-local:
	@if test ! -f $(prefix)/config.xml; then \
		$(INSTALL_DATA) $(srcdir)/config.xml $(prefix)/config.xml; \
	fi
	@if test ! -f $(prefix)/config.tcl; then \
		$(INSTALL_DATA) $(srcdir)/config.tcl $(prefix)/config.tcl; \
	fi

distclean-local:
	-test -z "$(DISTCLEANLOCAL)" || rm -rf $(DISTCLEANLOCAL)

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)

LIBTOOL_DEPS = @LIBTOOL_DEPS@
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
