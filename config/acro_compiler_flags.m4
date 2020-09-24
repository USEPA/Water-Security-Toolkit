dnl ACRO compiler flags

AC_DEFUN([ACRO_COMPILER_FLAGS],[
  dnl
  dnl Define the compiler flags.
  dnl 

dnl
dnl If the user specified --with_*flags= on the command line, use those.
dnl Otherwise use any flags that were specified in the environment.
dnl Add the command line --with-debugging and/or --with-optimization flags.
dnl Add special flags that we may define for various platforms (but only
dnl   if there was no --with-*flags= on configure command line).
dnl TODO: There are more flags we could set: Fortran, AR, LDFLAGS, etc.
dnl
dnl TODO: Find copies of AX_CXXFLAGS_*_OPTION macros so we can
dnl test that the chosen flags work.
dnl (AX_CFLAGS_*_OPTION macros are in the autoconf macro archive)
dnl

dnl
dnl Flags specified on configure command line with --with-*flags=
dnl override environment variable settings
dnl
if test -n "${USERDEF_CPPFLAGS}" ; then
  CPPFLAGS=${USERDEF_CPPFLAGS}
fi
if test -n "${USERDEF_FFLAGS}" ; then
  FFLAGS="${USERDEF_FFLAGS}"
fi
if test -n "${USERDEF_CFLAGS}" ; then
  CFLAGS="${USERDEF_CFLAGS}"
fi
if test -n "${USERDEF_CXXFLAGS}" ; then
  CXXFLAGS="${USERDEF_CXXFLAGS}"
fi

dnl
dnl Extra configure command line options --with-debugging, --with-optimization
dnl

if test "X${WITH_DEBUGGING}" != "Xnotset" ; then
  FFLAGS="${WITH_DEBUGGING} ${FFLAGS}"
  CFLAGS="${WITH_DEBUGGING} ${CFLAGS}"
  CXXFLAGS="${WITH_DEBUGGING} ${CXXFLAGS}"
fi

if test "X${WITH_OPT}" != "Xnotset" ; then
  FFLAGS="${WITH_OPT} ${FFLAGS}"
  CFLAGS="${WITH_OPT} ${CFLAGS}"
  CXXFLAGS="${WITH_OPT} ${CXXFLAGS}"
fi


dnl
dnl Special flags: apply only if they did not specify --with-*flags=
dnl
if test -z "${USERDEF_CPPFLAGS}" ; then
  CPPFLAGS="${CPPFLAGS} ${SPECIAL_CPPFLAGS}"
fi
if test -z "${USERDEF_CFLAGS}" ; then
  CFLAGS="${CFLAGS} ${SPECIAL_CFLAGS}"
fi
if test -z "${USERDEF_CXXFLAGS}" ; then
  CXXFLAGS="${CXXFLAGS} ${SPECIAL_CXXFLAGS}"
fi

dnl
dnl Default optimization level - apply if they didn't specify any flags
dnl

if test -z ${FFLAGS} ; then
  FFLAGS=${optimization_default}
fi
if test -z ${CFLAGS} ; then
  CFLAGS=${optimization_default}
fi
if test -z ${CXXFLAGS} ; then
  CXXFLAGS=${optimization_default}
fi

dnl
dnl Special flags for gcov
dnl

if test "X${WITH_GCOV}" != "Xnotset" ; then
  FFLAGS="${WITH_GCOV} ${FFLAGS}"
  CFLAGS="${WITH_GCOV} ${CFLAGS}"
  CXXFLAGS="${WITH_GCOV} ${CXXFLAGS}"
fi


dnl
dnl Special mingw flag
dnl

if test X${WITH_COMPILER} = Xmingw ; then
  
  testflag=`echo ${CFLAGS} | grep cygwin`
  if test -z "${testflag}" ; then
    CFLAGS="-mno-cygwin -DNO_RUSAGE ${CFLAGS}"
  fi
  testflag=`echo ${CXXFLAGS} | grep cygwin`
  if test -z "${testflag}" ; then
    CXXFLAGS="-mno-cygwin ${CXXFLAGS}"
  fi
  testflag=`echo ${FFLAGS} | grep cygwin`
  if test -z "${testflag}" ; then
    FFLAGS="-mno-cygwin ${FFLAGS}"
  fi
  testflag=`echo ${CPPFLAGS} | grep cygwin`
  if test -z "${testflag}" ; then
    CPPFLAGS="-mno-cygwin ${CPPFLAGS}"
  fi
  testflag=`echo ${LDFLAGS} | grep cygwin`
  if test -z "${testflag}" ; then
    LDFLAGS="-mno-cygwin ${LDFLAGS}"
  fi

  AM_CONDITIONAL(HOST_MINGW, true)
  AM_CONDITIONAL(TARGET_MINGW, true)
  AC_DEFINE(HOST_MINGW,1,[software host will be mingw])
  AC_DEFINE(TARGET_MINGW,1,[software target will be mingw])
fi
  

])
