# $Id: Makefile.am,v 1.2 2001/10/29 23:01:32 ite Exp $

MAINTAINERCLEANFILES	= Makefile.in

INCLUDES		= -I$(top_srcdir)

lib_LTLIBRARIES 	= libeggdrop.la

libeggdrop_la_SOURCES	= 
libeggdrop_la_LIBADD	= $(top_builddir)/lib/compat/libcompat.la \
		   	$(top_builddir)/lib/egglib/libegg.la \
			$(top_builddir)/lib/adns/libadns.la

