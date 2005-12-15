# $Id: Makefile.am,v 1.1 2005/12/15 15:26:12 sven Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= pythonscript.la

pythonscript_la_CFLAGS	= -I@PYTHON_INCLUDE@
pythonscript_la_SOURCES	= pythonscript.c pythonscript.h mycallable.c mydict.c mymodule.c
pythonscript_la_LDFLAGS	= -module -avoid-version -no-undefined
pythonscript_la_LIBADD	= @PYTHON_LDFLAGS@ @LIBS@ \
			$(top_builddir)/lib/eggdrop/libeggdrop.la
