dnl configure.ac: this file is processed by autoconf to produce ./configure.
dnl
dnl $Id: configure.ac,v 1.52 2004/06/17 13:32:43 wingman Exp $

AC_PREREQ(2.57)
AC_INIT([Eggdrop], [1.7.0], [bugs@eggheads.org])
AC_COPYRIGHT([Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Eggheads Development Team])
AC_REVISION($Revision: 1.52 $)
AC_CONFIG_AUX_DIR(acconfig)
AM_CONFIG_HEADER(config.h)
AC_CONFIG_FILES([Makefile lib/Makefile lib/compat/Makefile lib/eggdrop/Makefile m4/Makefile po/Makefile.in src/Makefile modules/Makefile acconfig/module.mk])


# Print start message
EGG_MSG_CONFIGURE_START
AM_INIT_AUTOMAKE(eggdrop, 1.7.0)


# FIXME: optionally allow a system wide install by bypassing the macro below.
AC_PREFIX_DEFAULT([\${HOME}/eggdrop])


# config.h stuff
AH_TOP([#ifndef _EGG_CONFIG_H
#define _EGG_CONFIG_H])
AH_BOTTOM([#endif /* !_EGG_CONFIG_H */])


# Check for a working C compiler
AC_PROG_CC
EGG_CHECK_CC


# Checks for odd operating systems
AC_AIX
AC_ISC_POSIX
AC_MINIX


# Checks for compiler characteristics
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
AC_PROG_LIBTOOL
EGG_PROG_AWK
EGG_PROG_BASENAME
AC_CHECK_PROG(EGG_UNAME,uname,uname)
AC_CHECK_PROG(AR,ar,ar)


# Test the os and set the module linking settings
EGG_CHECK_OS


# Checks for system libraries
EGG_CHECK_LIBS


# Checks for header files
AC_HEADER_DIRENT
AC_HEADER_STAT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_HEADER_TIME
AC_CHECK_HEADERS(string.h strings.h ctype.h stdlib.h limits.h unistd.h sys/rusage.h sys/select.h sys/poll.h stdarg.h std_args.h sys/time.h)


# Checks for types and structures
AC_TYPE_PID_T
AC_TYPE_SIZE_T
EGG_TYPE_32BIT
EGG_TYPE_SOCKLEN_T
EGG_INADDR_LOOPBACK


# Checks for library variables
EGG_VAR_SYS_ERRLIST


# Checks for library functions
AC_CHECK_FUNCS(clock dprintf fsync getdtablesize getrusage isascii random srandom rename setpgid sigaction sigemptyset uname)


# Check for these functions, and replace those not found
AC_REPLACE_FUNCS(memcpy memset poll strcasecmp strncasecmp strftime strdup inet_ntop inet_pton strerror strlcat strlcpy)


# These all share a common file: snprintf.c
AC_CHECK_FUNCS(vsnprintf snprintf asprintf vasprintf, [], [egg_replace_snprintf="yes"])
EGG_FUNC_C99_VSNPRINTF
EGG_REPLACE_SNPRINTF


# Check if getopt_long is avalaibale
EGG_FUNC_GETOPT_LONG


# Checks for IPV6
EGG_IPV6_SUPPORTED
EGG_IPV6_OPTIONS


# Checks for debug options
EGG_DEBUG_OPTIONS


# Defines version num
EGG_DEFINE_VERSION_NUM


# Save now since modules may fail due to a missing requirement
AC_CACHE_SAVE


# Check for modules
EGG_CHECK_MODULES


# Export list of enabled modules
AC_SUBST(EGG_MODULES_ENABLED)


# Export auxiliary dir (./acconfig/) 

AC_SUBST(ac_aux_dir)

# Export list of preloaded shared objects
AC_SUBST(LIBEGGDROP_PRELOAD)


# ldl support
AC_CONFIG_SUBDIRS(libltdl intl)


AC_OUTPUT
EGG_MSG_CONFIGURE_END
