dnl Process this file with autoconf to produce a configure script.
AC_INIT(Eggdrop, 1.7, bugs@eggheads.org)
AC_PREREQ(2.53)
AC_CONFIG_AUX_DIR(acconfig)
AM_CONFIG_HEADER(config.h)

# FIXME: make module Makefile list dynamic
AC_CONFIG_FILES([Makefile lib/Makefile lib/compat/Makefile lib/eggdrop/Makefile lib/egglib/Makefile m4/Makefile modules/Makefile modules/blowfish/Makefile modules/dccparty/Makefile modules/ircparty/Makefile modules/javascript/Makefile modules/oldbotnet/Makefile modules/perlscript/Makefile modules/proxy/Makefile modules/script/Makefile modules/server/Makefile modules/tclscript/Makefile modules/telnetparty/Makefile po/Makefile.in src/Makefile])

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

EGG_DEBUG_OPTIONS

EGG_DEFINE_VERSION_NUM

EGG_COMPRESS_MODULE
EGG_PERLSCRIPT_MODULE
EGG_TCLSCRIPT_MODULE
EGG_JAVASCRIPT_MODULE

AC_CONFIG_SUBDIRS(libltdl intl)

AC_SUBST(ac_aux_dir)

AC_OUTPUT

EGG_MSG_CONFIGURE_END
