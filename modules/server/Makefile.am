# $Id: Makefile.am,v 1.17 2004/10/01 15:31:18 stdarg Exp $

include $(top_srcdir)/$(ac_aux_dir)/module.mk

pkglib_LTLIBRARIES	= server.la
server_la_SOURCES	= server.c server.h binds.c binds.h channels.c channels.h input.c input.h output.c output.h serverlist.c serverlist.h nicklist.c nicklist.h schan.c schan.h scriptcmds.c servsock.c servsock.h party_commands.c dcc.c dcc.h egg_server_api.c egg_server_api.h
server_la_LDFLAGS	= -module -avoid-version -no-undefined
server_la_LIBADD	= @LIBS@ $(top_builddir)/lib/eggdrop/libeggdrop.la

