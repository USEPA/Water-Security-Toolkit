dnl ACRO compiler options
dnl
dnl   ACRO_COMPILER_OPTIONS(subpackage)
dnl
dnl   Use "subpackage" option to set flags to defaults and ignore 
dnl   command line arguments.  It leaves command line options unset
dnl   and uses the compiler flags in the environment, which were
dnl   computed by the parent package.
dnl

AC_DEFUN([ACRO_COMPILER_OPTIONS],[
  dnl
  dnl An explicit -with-*flags= overrides our default flags and any
  dnl flags that be defined in the environment.
  dnl --with-debugging adds debugging flag to flags
  dnl --with-optimization=flag replaces optimization flag with "flag"
  dnl --without-optimization or --with-optimization=no changes to -O0
  dnl 

optimization_default=-O2
debugging_default=""
WITH_DEBUGGING=notset
WITH_OPT=notset

if test X$1 = Xsubpackage; then :; else
  
  AC_ARG_WITH(cppflags,
  AS_HELP_STRING([--with-cppflags],[set preprocessor flags (e.g., "-Dflag -Idir")]),
  [USERDEF_CPPFLAGS=${withval}]
  )
  
  AC_ARG_WITH(fflags,
  AS_HELP_STRING([--with-fflags],[set Fortran compiler flags]),
  [USERDEF_FFLAGS=${withval}]
  )
  
  AC_ARG_WITH(cflags,
  AS_HELP_STRING([--with-cflags],[set C compiler flags]),
  [USERDEF_CFLAGS=${withval}]
  )
  
  AC_ARG_WITH(cxxflags,
  AS_HELP_STRING([--with-cxxflags],[set C++ compiler flags]),
  [USERDEF_CXXFLAGS=${withval}]
  )
  
  AC_ARG_WITH(ldflags,
  AS_HELP_STRING([--with-ldflags],[add extra linker (typically -L) flags]),
  [LDFLAGS="${LDFLAGS} ${withval}"]
  )
  
  AC_ARG_WITH(libs,
  AS_HELP_STRING([--with-libs],[add extra library (typically -l) flags]),
  [LIBS="${LIBS} ${withval}"]
  )
  
  AC_ARG_WITH(debugging,
  AS_HELP_STRING([--with-debugging],[Build with debugging information. Default is no.]),
  [WITH_DEBUGGING=$withval],
  [WITH_DEBUGGING=notset]
  )
  
  if test "X${WITH_DEBUGGING}" = "Xyes"; then
    WITH_DEBUGGING="-g"
  fi
  if test "X${WITH_DEBUGGING}" = "Xno"; then
    WITH_DEBUGGING=""
  fi
  
  AC_ARG_WITH(optimization,
  AS_HELP_STRING([--with-optimization=flag],[Build with the specified optimization flag (Default -O2).]),
  [WITH_OPT=$withval],
  [WITH_OPT=notset]
  )
  
  if test X${WITH_OPT} = Xno ; then
    WITH_OPT=-O0
  fi
  if test X${WITH_OPT} = Xyes ; then
    WITH_OPT=${optimization_default}
  fi

  AC_ARG_ENABLE(gcov,
  AS_HELP_STRING([--enable-gcov],[Build with flags that enable the use of gcov.]),
  [WITH_GCOV="-fprofile-arcs -ftest-coverage"],
  [WITH_GCOV=""]
  )
fi
])

