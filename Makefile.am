# $Id: Makefile.am,v 1.3 2001/10/19 23:07:46 tothwolf Exp $

AUX_DIST                = $(ac_aux_dir)/install-sh \
			$(ac_aux_dir)/ltmain.sh \
			$(ac_aux_dir)/missing \
			$(ac_aux_dir)/mkinstalldirs \
			$(ac_aux_dir)/depcomp \
			$(ac_aux_dir)/config.guess \
			$(ac_aux_dir)/config.sub

AUX_DIST_EXTRA		= $(ac_aux_dir)/codeset.m4 \
			$(ac_aux_dir)/gettext.m4 \
			$(ac_aux_dir)/glibc21.m4 \
			$(ac_aux_dir)/iconv.m4 \
			$(ac_aux_dir)/isc-posix.m4 \
			$(ac_aux_dir)/lcmessage.m4 \
			$(ac_aux_dir)/progtest.m4 \
			$(ac_aux_dir)/acinclude.m4

EXTRA_DIST              = bootstrap

SUBDIRS                 = intl libltdl scripts src @POSUB@
DISTCLEANFILES		= lush.h
MAINTAINERCLEANFILES    = Makefile.in aclocal.m4 configure config-h.in \
			config.cache config.log config.status \
			config.h.in stamp-h.in $(AUX_DIST)
			 
ACLOCAL                 = aclocal -I $(ac_aux_dir)

dist-hook:
	(cd $(distdir) && mkdir $(ac_aux_dir))
	for file in $(AUX_DIST) $(AUX_DIST_EXTRA); do \
	  ln $$file $(distdir) 2>/dev/null || cp -p $$file $(distdir); \
	done

install-data-local:
	$(INSTALL_DATA) $(srcdir)/eggdrop.complete.conf $(prefix)/eggdrop.complete.conf
	$(INSTALL_DATA) $(srcdir)/eggdrop.simple.conf $(prefix)/eggdrop.simple.conf

$(OBJECTS): libtool
libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

