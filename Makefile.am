# $Id: Makefile.am,v 1.8 2002/02/17 12:40:43 ite Exp $

AUX_DIST		= $(ac_aux_dir)/install-sh \
			$(ac_aux_dir)/ltmain.sh \
			$(ac_aux_dir)/missing \
			$(ac_aux_dir)/mkinstalldirs \
			$(ac_aux_dir)/depcomp

AUX_DIST_EXTRA		= $(ac_aux_dir)/codeset.m4 \
			$(ac_aux_dir)/gettext.m4 \
			$(ac_aux_dir)/glibc21.m4 \
			$(ac_aux_dir)/iconv.m4 \
			$(ac_aux_dir)/isc-posix.m4 \
			$(ac_aux_dir)/lcmessage.m4 \
			$(ac_aux_dir)/mpatrol.m4 \
			$(ac_aux_dir)/progtest.m4 \
			$(ac_aux_dir)/acinclude.m4 \
			$(ac_aux_dir)/config.guess \
			$(ac_aux_dir)/config.sub

EXTRA_DIST		= bootstrap

SUBDIRS			= intl libltdl lib modules src scripts nettype @POSUB@
DISTCLEANFILES		= lush.h
MAINTAINERCLEANFILES	= Makefile.in aclocal.m4 configure config-h.in \
			config.cache config.log config.status \
			config.h.in stamp-h.in $(AUX_DIST)
DISTCLEANLOCAL		= autom4te.cache
MAINTAINERCLEANLOCAL	= libltdl

ACLOCAL			= aclocal -I $(ac_aux_dir)

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir); \
	done

install-data-local:
	$(INSTALL_DATA) $(srcdir)/eggdrop.complete.conf $(prefix)/eggdrop.complete.conf
	$(INSTALL_DATA) $(srcdir)/eggdrop.simple.conf $(prefix)/eggdrop.simple.conf

distclean-local:
	-test -z "$(DISTCLEANLOCAL)" || rm -rf $(DISTCLEANLOCAL)

maintainer-clean-local:
	-test -z "$(MAINTAINERCLEANLOCAL)" || rm -rf $(MAINTAINERCLEANLOCAL)

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
