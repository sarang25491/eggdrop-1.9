EGG_MODULE_START(compress, [compression support], "no")

AC_CHECK_LIB(z, gzopen, ZLIB="-lz")
AC_CHECK_HEADER(zlib.h)

# Disable the module if either the header file or the library
# are missing.
if test "${ZLIB+set}" != set
then
  AC_MSG_WARN([

  Your system does not provide a working zlib compression library. The
  compress module will therefore be disabled.

])
  EGG_MOD_ENABLED=no
else
  if test "$ac_cv_header_zlib_h" != yes
  then
  AC_MSG_WARN([

  Your system does not provide the necessary zlib header files. The
  compress module will therefore be disabled.

])
    EGG_MOD_ENABLED=no
  else
    AC_FUNC_MMAP
    AC_SUBST(ZLIB)
  fi
fi

EGG_MODULE_END()
