# $Id: Makefile.am,v 1.3 2006/01/05 20:42:42 sven Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= pythonscript.la

pythonscript_la_CFLAGS	= @PYTHON_CFLAGS@
pythonscript_la_SOURCES	= pythonscript.c pythonscript.h mycallable.c mydict.c mymodule.c mystdio.c myegguser.c
pythonscript_la_LDFLAGS	= -module -avoid-version -no-undefined
pythonscript_la_LIBADD	= @PYTHON_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
