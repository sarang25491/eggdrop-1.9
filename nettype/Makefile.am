# $Id: Makefile.am,v 1.1 2002/02/17 12:44:54 ite Exp $

# FIXME: optionally allow a system wide install by ignoring the line below.
pkglibdir = $(prefix)

CONF_DIST_EXTRA	= custom.irc.conf \
		custom.server.conf \
		dalnet.irc.conf \
		dalnet.server.conf \
		efnet.irc.conf \
		efnet.server.conf \
		hybridefnet.irc.conf \
		hybridefnet.server.conf \
		ircnet.irc.conf \
		ircnet.server.conf \
		undernet.irc.conf \
		undernet.server.conf

SUBDIRS			= 
MAINTAINERCLEANFILES	= Makefile.in

dist-hook:
	(cd $(distdir) && mkdir nettype)
	for file in $(CONF_DIST_EXTRA); do \
	  ln $$file $(distdir)/nettype 2>/dev/null || \
	  cp -p $$file $(distdir)/nettype/$$file; \
	done

install-data-local:
	($(mkinstalldirs) $(pkglibdir)/nettype)
	for file in $(CONF_DIST_EXTRA); do \
	  $(INSTALL_DATA) $$file $(pkglibdir)/nettype/$$file; \
	done
