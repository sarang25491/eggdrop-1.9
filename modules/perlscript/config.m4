EGG_MODULE_START(perlscript, [perl support], "yes")

AC_PATH_PROG(perlcmd, perl)
PERL_LDFLAGS=`$perlcmd -MExtUtils::Embed -e ldopts 2>/dev/null`

if test "${PERL_LDFLAGS+set}" != set
then
  AC_MSG_WARN([

  Your system does not provide a working perl environment. The
  perlscript module will therefore be disabled.
  
])
  EGG_MOD_ENABLED=no
else
  PERL_CCFLAGS=`$perlcmd -MExtUtils::Embed -e ccopts 2>/dev/null`
  egg_perlscript=yes
  AC_SUBST(PERL_LDFLAGS)
  AC_SUBST(PERL_CCFLAGS)
fi

EGG_MODULE_END()
