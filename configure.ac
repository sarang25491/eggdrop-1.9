dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/egg.h)
AC_PREREQ(2.52)
AC_CONFIG_AUX_DIR(acconfig)
AM_CONFIG_HEADER(config.h)
EGG_MSG_CONFIGURE_START
AM_INIT_AUTOMAKE(eggdrop, 1.7.0)

# FIXME: optionally allow a system wide install by bypassing the macro below.
AC_PREFIX_DEFAULT([\${HOME}/eggdrop])

# config.h stuff
AH_TOP([#ifndef _EGG_CONFIG_H
#define _EGG_CONFIG_H])
AH_BOTTOM([#endif /* !_EGG_CONFIG_H */])

# Setup build environment
AC_PROG_CC
EGG_CHECK_CC

# Crazy machines
AC_AIX
AC_ISC_POSIX
AC_MINIX

# Checks for compiler characteristics
AC_PROG_CC_STDC
AC_C_BIGENDIAN
AC_C_CONST
AC_C_VOLATILE
AC_C_INLINE
AC_C_LONG_DOUBLE
EGG_C_LONG_LONG

# Gettext support
EGG_GNU_GETTEXT

# Libtool support
EGG_LIBTOOL

# Speedup compile
EGG_CHECK_CCPIPE

# Compile Warnings
EGG_CHECK_CFLAGS_WALL

# Checks for programs
AC_PROG_LN_S
EGG_PROG_AWK
EGG_PROG_BASENAME
AC_CHECK_PROG(EGG_UNAME,uname,uname)
AC_CHECK_PROG(AR,ar,ar)

# Should -O2 CFLAG be removed? 
EGG_DISABLE_CC_OPTIMIZATION

# Test the os and set the module linking settings
EGG_CHECK_OS

# Checks for system libraries
EGG_CHECK_LIBS

# Checks for header files
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS(sys/time.h)
AC_HEADER_TIME
AC_CHECK_HEADERS(string.h strings.h ctype.h stdlib.h limits.h unistd.h)
AC_CHECK_HEADERS(sys/rusage.h sys/select.h sys/poll.h stdarg.h std_args.h)

# Checks for types and structures
AC_TYPE_PID_T
AC_TYPE_SIZE_T
EGG_TYPE_32BIT
EGG_TYPE_SOCKLEN_T
EGG_INADDR_LOOPBACK

# Checks for library variables
EGG_VAR_SYS_ERRLIST

# Checks for library functions
AC_CHECK_FUNCS(clock dprintf fsync getdtablesize getrusage isascii)
AC_CHECK_FUNCS(random srandom rename setpgid sigaction sigemptyset uname)

# Check for these functions, and replace those not found
AC_REPLACE_FUNCS(memcpy memset poll strcasecmp strncasecmp strftime strdup)
AC_REPLACE_FUNCS(inet_ntop inet_pton strerror strlcat strlcpy)

# These all share a common file: snprintf.c
AC_CHECK_FUNCS(vsnprintf snprintf asprintf vasprintf,,egg_replace_snprintf="yes")

# Check if vsnprintf is C99 compliant
EGG_FUNC_C99_VSNPRINTF

# Check if getopt_long is avalaibale
EGG_FUNC_GETOPT_LONG

# Check if we need to compile all or part of snprintf.c
EGG_REPLACE_SNPRINTF

# Checks for IPV6
EGG_IPV6_SUPPORTED
EGG_IPV6_OPTIONS

# Where is tcl?  Is it here?
# ---------- begin robey's tcl thingies
# (well, what used to be robey's tcl thingies...)

# Latest tested Tcl version to recommend if Tcl isn't found
tclrecommendver="8.3.4"

# Site recommended to download Tcl from
tclrecommendsite="ftp://tcl.activestate.com/pub/tcl/"

# Tcl library filename prefixes (also used for Tcl header dir on FreeBSD)
tcllibnames="tcl tcl8.4 tcl84 tcl8.3 tcl83 tcl8.2 tcl82 tcl8.1 tcl81 \
	tcl8.0 tcl80 tcl7.6 tcl76 tcl7.5 tcl75 tcl7.4 tcl74 \
	tcl7.3 tcl73 tcl7.2 tcl72 tcl7.1 tcl71 tcl7.0 tcl70"

# Tcl library filename suffixes
tcllibextensions=".so .so.1 .so.1.0 .so.1.2 .a .sl .dll .dynlib"

# Tcl library search paths
tcllibpaths="/usr/local/lib /usr/local/pkgs/tcl/lib \
	/usr/lib /lib /usr/i486-linuxaout/lib \
	/beos/system/lib /sys/lib \
	$HOME/lib $HOME/tcl/lib $HOME"

# Tcl header filenames
tclheadernames="tcl.h"

# Tcl header search paths
tclheaderpaths="/usr/local/include /usr/local/pkgs/tcl/include \
	/usr/include /beos/system/include /beos/devel/include \
	/sys/include $HOME/include $HOME/tcl/include $HOME"

EGG_TCL_WITH_OPTIONS
EGG_TCL_ENV
EGG_TCL_WITH_TCLLIB
EGG_TCL_WITH_TCLINC
EGG_TCL_FIND_LIBRARY
EGG_TCL_FIND_HEADER
EGG_TCL_CHECK_LIBRARY
EGG_TCL_CHECK_HEADER
EGG_TCL_DETECT_CHANGE

EGG_TCL_CHECK_VERSION
EGG_TCL_CHECK_PRE70
EGG_TCL_CHECK_PRE75
EGG_TCL_TESTLIBS
EGG_TCL_CHECK_FREE
EGG_TCL_CHECK_THREADS
EGG_TCL_LIB_REQS
EGG_TCL_LUSH
# ---------- end of (what used to be) robey's tcl thingies

EGG_DEBUG_OPTIONS

EGG_DEFINE_VERSION_NUM

EGG_COMPRESS_MODULE
EGG_PERLSCRIPT_MODULE
EGG_TCLSCRIPT_MODULE
EGG_JAVASCRIPT_MODULE

AC_CONFIG_SUBDIRS(libltdl)

AC_SUBST(ac_aux_dir)

EGG_LTLIBOBJS

# FIXME: module Makefile list will HOPEFULLY become dynamic
AC_OUTPUT([Makefile doc/Makefile intl/Makefile lib/Makefile lib/compat/Makefile lib/eggdrop/Makefile lib/egglib/Makefile modules/Makefile modules/blowfish/Makefile modules/javascript/Makefile modules/perlscript/Makefile modules/script/Makefile modules/server/Makefile modules/tclscript/Makefile po/Makefile.in scripts/Makefile nettype/Makefile src/Makefile])

EGG_MSG_CONFIGURE_END
