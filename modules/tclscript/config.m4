EGG_MODULE_START(tclscript, [tcl script support], "yes")

# XXX: maybe someone find a way how to remove the modules/<mod>/
# XXX: prefix. According to m4 man page there's an environment
# XXX: variable named M4PATH but this doesn't seem to work.
sinclude(modules/tclscript/tcl.m4)

SC_PATH_TCLCONFIG
SC_LOAD_TCLCONFIG
                                                                                
# We only support version 8
if test "$TCL_MAJOR_VERSION" = 8; then
  AC_SUBST(TCL_PREFIX)
  AC_SUBST(TCL_LIBS)
  EGG_MOD_ENABLED="yes"
else
  EGG_MOD_ENABLED="no"
  AC_MSG_WARN([
                                                                                
  Your system does not seem to provide tcl 8.0 or later. The
  tclscript module will therefore be disabled.
                                                                                
  To manually specify tcl's location, re-run configure using
    --with-tcl=/path/to/tcl
  where the path is the directory containing tclConfig.sh.
                                                                                
  ])
fi

EGG_MODULE_END()
