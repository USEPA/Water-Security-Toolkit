#
# Script to be processed by autoconf to produce a configure script
# and config.h header file.
#
###############################################################
# boilerplate
###############################################################
#
AC_PREREQ(2.59)
AC_INIT([teva-spot], [2.1], [wehart@sandia.gov])
#
# Turn off features that only maintainers would use
AM_MAINTAINER_MODE
#
# A file that should be present in the source directory
AC_CONFIG_SRCDIR([packages/cat/tso2Impact/tso2Impact.cpp])
#
# The directory containing our M4 macros
AC_CONFIG_MACRO_DIR([config])
AC_CONFIG_AUX_DIR([config])

###############################################################
# Platform ID

AC_CANONICAL_BUILD
AC_CANONICAL_HOST
AC_CANONICAL_TARGET

ACRO_PLATFORM

#
###############################################################
# Create this header file which will help configure our Makefiles
# and source files at build time.
#
AC_CONFIG_HEADERS([config.h])
AX_PREFIX_CONFIG_H([teva_config.h])

#
# automake initialization
#
AM_INIT_AUTOMAKE([-Wall -Werror foreign tar-ustar])

###############################################################
# Configure command line options
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Compiler options

ACRO_COMPILER_OPTIONS

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# Build options

# TODO: determine when to turn on the -DDLL flag in tevalibs. 

ACRO_EXECUTABLES_OPTION

# We only build modules on 32 bit linux

AC_MSG_CHECKING(whether to build modules)

case ${build_cpu} in
  *64)
    cpu_64=yes
    ;;
  *)
    cpu_64=no
    ;; 
esac 

case ${build_os} in
  linux*)
    linux_os=yes
    ;;
  cygwin*)
    cygwin_os=yes
    ;;
  *)
    linux_os=no
    cygwin_os=no
    ;;
esac

if test X${cpu_64} = Xno && ((test X${linux_os} = Xyes) || (test X${cygwin_os} = Xyes))  ; then
  AM_CONDITIONAL(HAVE_LIBANLUTILS, true)
  ac_cv_have_anlutils=yes
  AC_MSG_RESULT(yes)  
  if test X${linux_os} = Xyes ; then
    ANL_LIBDIR=packages/tevaUtils/anl/libs/linux
  else
    ANL_LIBDIR=packages/tevaUtils/anl/libs/win32
  fi
else
  if test X${cpu_64} = Xyes && test X${linux_os} = Xyes  ; then
    AM_CONDITIONAL(HAVE_LIBANLUTILS, true)
    ac_cv_have_anlutils=yes
    AC_MSG_RESULT(yes)  
    ANL_LIBDIR=packages/tevaUtils/anl/libs/linux64
  else
    AM_CONDITIONAL(HAVE_LIBANLUTILS, false)
    ac_cv_have_anlutils=no
    AC_MSG_RESULT(no)  
    ANL_LIBDIR=none
  fi
fi

AC_SUBST([ANL_LIBDIR])

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# --with-compiler, only used for mingw

ACRO_COMPILER_OPTION

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #
# options specifying MPI
#
ACRO_MPI_OPTIONS
#
# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

# Default: No BUILD_TESTS, so that acro will build faster

TEVA_ENABLE_BUILD(acro-tests, ACRO_PICO_TESTS, no)

if test X$ENABLE_ACRO_PICO_TESTS = Xno ; then
  ac_configure_args="$ac_configure_args --disable-tests"
fi

# # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #

TEVA_ENABLE_BUILD(boostersim, BOOSTERSIM, yes)

TEVA_ENABLE_BUILD(boosterquality, BOOSTERQUALITY, yes)

TEVA_ENABLE_BUILD(boosterimpact, BOOSTERIMPACT, yes)

TEVA_ENABLE_BUILD(erdval, ERDVAL, yes)

TEVA_ENABLE_BUILD(inversionsim, INVERSIONSIM, yes)

TEVA_ENABLE_BUILD(measuregen, MEASUREGEN, yes)

TEVA_ENABLE_BUILD(samplelocation, SAMPLELOCATION, yes)

TEVA_ENABLE_BUILD(eventDetection, EVENTDETECTION, yes)

TEVA_ENABLE_BUILD(tso2Impact, TSO2IMPACT, yes)

TEVA_ENABLE_BUILD(evalsensor, EVALSENSOR, yes)

TEVA_ENABLE_BUILD(setupIPData, SETUPIPDATA, yes)

TEVA_ENABLE_BUILD(scenarioAggr, SCENARIOAGGR, yes)

TEVA_ENABLE_BUILD(SNLRound, SNLROUND, no)

TEVA_ENABLE_BUILD(epanet, EPANET, yes)

TEVA_ENABLE_BUILD(epanet_msx, EPANET_MSX, yes)

TEVA_ENABLE_BUILD(erd2text, ERD2TEXT, yes)

TEVA_ENABLE_BUILD(csatest, CSATEST, yes)

TEVA_ENABLE_BUILD(csarun, CSARUN, yes)

TEVA_ENABLE_BUILD(erddiff, ERDDIFF, yes)

TEVA_ENABLE_BUILD(erdinfo, ERDINFO, yes)

TEVA_ENABLE_BUILD(threat_assess, THREAT_ASSESS, yes)

TEVA_ENABLE_BUILD(tevasim, TEVASIM, yes)

TEVA_ENABLE_BUILD(grasp, GRASP, yes)

TEVA_ENABLE_BUILD(pmgen, PMGEN, yes)

TEVA_ENABLE_BUILD(impact2pmm, IMPACT2PMM, yes)

TEVA_ENABLE_BUILD(randomsample, RANDOMSAMPLE, yes)

TEVA_ENABLE_BUILD(sideconstraints, SIDECONSTRAINTS, yes)

TEVA_ENABLE_BUILD(imperfect, IMPERFECT, yes)

TEVA_ENABLE_BUILD(new_imperfect, NEW_IMPERFECT, yes)

TEVA_ENABLE_BUILD(new_randomsample, NEW_RANDOMSAMPLE, yes)

TEVA_WITH_JDKROOT

AC_SUBST([JDK_INC])

#TODO - implement this
#TEVA_ENABLE_BUILD(teva-tests, TEVA_TESTS, yes)

#
# TODO - should we warn for randomsample, sideconstraints and imperfect too?
# should we turn those off by default and turn new_* on instead?
#

if test $ENABLE_GRASP = yes ; then
 AC_MSG_WARN([                     - - - -                             ])
 AC_MSG_WARN([You have chosen to build GRASP.  The software can be used])
 AC_MSG_WARN([freely for research purposes but may require a license for])
 AC_MSG_WARN([commercial use.  Check http://www.research.att.com/~mgcr/])
 AC_MSG_WARN([                     - - - -                             ])
 AC_MSG_WARN([You may wish to use "new_imperfect" or "new_randomsample" instead.])
 AC_MSG_WARN([which have no such restrictions.])
 AC_MSG_WARN([                     - - - -                             ])
fi





# ------------------------------------------------------------------------
# Checks for programs
# ------------------------------------------------------------------------

temp_fflags=${FFLAGS}
temp_cflags=${CFLAGS}
temp_cxxflags=${CXXFLAGS}

#
# Even if we have decided we are going to use mpicc, etc, we
# still need to run AC_PROG_CC, etc.  They set up variables
# (like $OBJEXT) that are used by subsequent macros.
#

# AC_PROG_CC: If environment variable CC is set, that becomes
#  the compiler to use, otherwise search for compilers in list.
#  (There are sensible defaults to search for if the list is
#  empty.)  This macro sets the variable $ac_cv_prog_cc_stdc to
#  'yes' or 'no' if the compiler is ANSI or not.  If compiler is
#  GNU C, shell variable GCC is set to 'yes'.  CFLAGS is set
#  to "-g -O2" if not already set.

AC_PROG_CC(cc gcc)

if test "X${MPI_CC}" = "Xnone" ; then : ; else
  CC=${MPI_CC}
fi

#
# AC_PROG_CXX: If environment variable CXX or CCC is set, this
#  becomes the compiler to use, otherwise this macro searches
#  the list of compilers provided to it.  If compiler is
#  GNU C++, this macro sets the shell variable GXX to 'yes'.
#  This macro sets CXXFLAGS to "-g -O2" if it was not already set.
#

if test X${build_os} = Xcygwin ; then
  cpp_compilers="g++ gcc"
else
  cpp_compilers="g++ CC cxx xlC c++ gcc"
fi

AC_PROG_CXX(${cpp_compilers})

if test "X${MPI_CXX}" = "Xnone" ; then : ; else
  CXX=${MPI_CXX}
fi

AC_PROG_F77(gfortran f77 g77 f90 xlf90 ifort)

if test "X${MPI_F77}" = "Xnone" ; then : ; else
  F77=${MPI_F77}
fi

AC_PROG_CXXCPP
AC_PROG_LN_S
AM_PROG_AS
m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
AM_PROG_CC_C_O

CFLAGS=${temp_cflags}
CXXFLAGS=${temp_cxxflags}
FFLAGS=${temp_fflags}


###############################################################
# Platform-specific compiler options

ACRO_PLATFORM_FLAGS


###############################################################
# Setup compiler flags
#    this macro depends on values previously set in
#       acro_compiler_options
#       acro_platform_flags

ACRO_COMPILER_FLAGS


###############################################################
# Test for BLAS, LAPACK and setup libraries
#
if test "X${FC}" == "X" ; then
    # Find F77 link flags, create FLIBS
    AC_F77_LIBRARY_LDFLAGS
else
    # Find F90  link flags, create FCLIBS
    AC_FC_LIBRARY_LDFLAGS
fi

# Unfortunately when using GCC compilers, -lgcc_s (a 
# shared-only library) is always added to the link line.
if test "X$enable_shared" != "Xyes"; then
    FLIBS=`echo $FLIBS | sed 's/-lgcc_s//g'`
    FCLIBS=`echo $FCLIBS | sed 's/-lgcc_s//g'`
fi

if test "X${F77}" == "X${FC}" ; then
    FLIBS=${FCLIBS}
fi

ACX_BLAS(,[AC_MSG_WARN([No BLAS --- either because it is disabled or could not be found])])

ACX_LAPACK(,[AC_MSG_WARN([No LAPACK --- either because it is disabled or could not be found])])

# Define F77_FUNC - used for correctly munging Fortran function calls
AC_F77_WRAPPERS


###############################################################
# A machine with a 64-bit CPU may be using a 32-bit OS and
# compiler.  GMP configure, on certain hosts, needs to know
# it's using a 32-bit system in this case.  If we upgrade to
# GMP 4.2.1, we may no longer need this test.

AC_COMPILE_CHECK_SIZEOF(unsigned char *)

if test "X${ac_cv_sizeof_unsigned_char_p}" = "X8" ; then
  AM_CONDITIONAL(HAVE_SYSTEM_32, false)
else
  # GMP only allows "ABI=32" option on these hosts
  case ${host} in
    x86_64-*-*)
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;;
    *-*-aix*)
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;;
    *-*-solaris2.[7-9])
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;;
    *)
      AM_CONDITIONAL(HAVE_SYSTEM_32, false)
      ;;
  esac
fi

###############################################################
# Validate MPI builds

ACRO_MPI_TESTS

###############################################################
#
# TODO: We need to consider SPECIAL_AR

EXPORT_FLAGS="export CC=\"${CC}\" CXX=\"${CXX}\" F77=\"${F77}\" CPPFLAGS=\"${CPPFLAGS}\" CFLAGS=\"${CFLAGS}\" CXXFLAGS=\"${CXXFLAGS}\" FFLAGS=\"${FFLAGS}\" LDFLAGS=\"${LDFLAGS}\" LIBS=\"${LIBS}\""

AC_SUBST([EXPORT_FLAGS])

export CC=${CC}
export CXX=${CXX}
export F77=${F77}
export CPPFLAGS=${CPPFLAGS}
export CFLAGS=${CFLAGS}
export CXXFLAGS=${CXXFLAGS}
export FFLAGS=${FFLAGS}
export LDFLAGS=${LDFLAGS}
export LIBS=${LIBS}

AC_MSG_NOTICE(CC=${CC})
AC_MSG_NOTICE(CXX=${CXX})
AC_MSG_NOTICE(F77=${F77})
AC_MSG_NOTICE(CPPFLAGS=${CPPFLAGS})
AC_MSG_NOTICE(CFLAGS=${CFLAGS})
AC_MSG_NOTICE(CXXFLAGS=${CXXFLAGS})
AC_MSG_NOTICE(FFLAGS=${FFLAGS})
AC_MSG_NOTICE(LDFLAGS=${LDFLAGS})
AC_MSG_NOTICE(LIBS=${LIBS})

# TODO: Find copies of AX_CXXFLAGS_*_OPTION macros so we can
# test that the chosen flags work.
# (AX_CFLAGS_*_OPTION macros are in the autoconf macro archive)

# ------------------------------------------------------------------------
# Libtool (shared library tool) initialization
# Place this after definition of CC, CFLAGS, etc. because it
# uses those.
#
# AM_PROG_LIBTOOL defines --enable-shared, --disable-shared,
# --enable-static and --disable-static.
#
# Shared libraries are enabled by default if the platform supports
# them.  To change this default, include macro AC_DISABLE_SHARED.
# There is also an AC_DISABLE_STATIC.
# Some people like to disable shared while they are developing, to
# speed up builds.
# ------------------------------------------------------------------------

AC_LIBTOOL_DLOPEN
AC_LIBTOOL_WIN32_DLL
AC_DISABLE_SHARED
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

if test "X$enable_shared" = "Xyes"; then
  AM_CONDITIONAL(BUILD_SHARED, true)
  if test X${ac_cv_have_anlutils} = Xno ; then
    AC_MSG_ERROR([You requested shared libraries but we can only build those on 32 bit linux or cygwin.])
  fi
else
  #
  # Add a configure option for acro-pico to ensure that PICO and ufl
  # are built with static libraries only
  #
  #ac_configure_args="$ac_configure_args --enable-static-executables"
  AM_CONDITIONAL(BUILD_SHARED, false)
fi

if test "X$enable_static" = "Xyes"; then
  AM_CONDITIONAL(BUILD_STATIC, true)
else
  AM_CONDITIONAL(BUILD_STATIC, false)
fi


###############################################################
# A machine with a 64-bit CPU may be using a 32-bit OS and
# compiler.  GMP configure, on certain hosts, needs to know 
# it's using a 32-bit system in this case.  If we upgrade to 
# GMP 4.2.1, we may no longer need this test.

AC_COMPILE_CHECK_SIZEOF(unsigned char *)

if test "X${ac_cv_sizeof_unsigned_char_p}" = "X8" ; then
  AM_CONDITIONAL(HAVE_SYSTEM_32, false)
else
  # GMP only allows "ABI=32" option on these hosts
  case ${host} in
    x86_64-*-*)
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;; 
    *-*-aix*)
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;; 
    *-*-solaris2.[7-9])
      AM_CONDITIONAL(HAVE_SYSTEM_32, true)
      ;; 
    *)
      AM_CONDITIONAL(HAVE_SYSTEM_32, false)
      ;; 
  esac 
fi

###############################################################
# Checks for libraries
#

###############################################################
# checks for headers
#

AC_HEADER_STDC

if test "X$ac_cv_header_stdc" = "Xyes"; then
  AM_CONDITIONAL(HAVE_ANSI_C_HEADERS, true)
else
  AM_CONDITIONAL(HAVE_ANSI_C_HEADERS, false)
fi

#
# Files in tevalibs include jni.h - a java header file.
# (This is only if --enable-shared was requested.)
# It's hard to find on some systems.
#
#if test "X$enable_shared" = "Xyes"; then
#  if test -f ${srcdir}/packages/tevalibs/TEVAModule/include/jni.h ; then 
#    AC_MSG_NOTICE(
#      [Found a copy of jni.h in ${srcdir}/packages/tevalibs/TEVAModule/include]
#    )
#  else
#    AC_CHECK_HEADER(
#      [jni.h],,
#      [AC_MSG_ERROR([Find Java's jni.h, include path to it in CPPFLAGS, or copy it into packages/tevalibs/TEVAModule/include])]
#    )
#  fi
#fi
#
###############################################################
# checks for typedefs, structures and compiler behavior
#

#
###############################################################
# checks for compiler behavior
#
# AC_CXX_* are checks for C++ behavior
#
  
# Defines HAVE_NAMESPACES if we can use them
#   TODO - find out if the means std namespace only, or any namespace
#          is it possible we can declare namespaces, but there's no std?
#          Modifying sources to use HAVE_NAMESPACES is not complete
AC_CXX_NAMESPACES

if test X${ac_cv_cxx_namespaces} = Xyes ; then
  AM_CONDITIONAL(HAVE_NAMESPACES, true)
else
  AM_CONDITIONAL(HAVE_NAMESPACES, false)
fi
  
# Defines HAVE_EXCEPTIONS if we can use throw/catch
AC_CXX_EXCEPTIONS
  
# Defines HAVE_STD if we can include ISO C++ headers
# (iostream, map, iomanip and cmath)
AC_CXX_HAVE_STD

if test X${ac_cv_cxx_have_std} = Xyes ; then
  AM_CONDITIONAL(HAVE_ANSI_CPP_HEADERS, true)
else
  AM_CONDITIONAL(HAVE_ANSI_CPP_HEADERS, false)
fi
  
# Define HAVE_SSTREAM if we have stringstreams
AC_CXX_HAVE_SSTREAM
  
# Define HAVE_EXPLICIT if we can use the explicit keyword
AC_CXX_EXPLICIT
  
# Define HAVE_MEMBER_TEMPLATES if class member functions can be templated
AC_CXX_MEMBER_TEMPLATES

AC_CXX_HAVE_SSTREAM

###############################################################
# checks for functions

AC_CHECK_FUNCS([stricmp],[ac_cv_have_stricmp=yes],[ac_cv_have_stricmp=no])

AC_CHECK_FUNCS([strcasecmp],[ac_cv_have_strcasecmp=yes],[ac_cv_have_strcasecmp=no])

AM_CONDITIONAL(NO_STRICMP, test X${ac_cv_have_stricmp} = Xno)
AM_CONDITIONAL(NO_STRCASECMP, test X${ac_cv_have_strcasecmp} = Xno)

AC_CHECK_FUNCS([realpath],[ac_cv_have_realpath=yes],[ac_cv_have_realpath=no])

if test X${ac_cv_have_realpath} = Xyes ; then
  AM_CONDITIONAL(HAVE_REALPATH, true)
  AC_DEFINE([HAVE_REALPATH],1,[define if system defines realpath function])
else
  AM_CONDITIONAL(HAVE_REALPATH, false)
fi

AC_CHECK_FUNCS([rint],[ac_cv_have_rint=yes],[ac_cv_have_rint=no])
AC_CHECK_FUNCS([srandom],[ac_cv_have_srandom=yes],[ac_cv_have_srandom=no])
AC_CHECK_FUNCS([log2],[ac_cv_have_log2=yes],[ac_cv_have_log2=no])

###############################################################
# more checks

#
# We have the usual problems treating gmp as a nested
# subpackage with AC_CONFIG_SUBDIRS, so, if it appears
# it has not been configured, we set a variable causing
# the makefile to descend into the directory and configure it.
#
if test -f tpl/gmp-4.1.4/config.status ; then
  AC_MSG_WARN([Skipping configuration of gmp because it's already configured.])
  AC_MSG_WARN([Delete config.status in the gmp directory if you want to force a reconfigure.])
  AM_CONDITIONAL(GMP_NOT_CONFIGURED, false)
else
  AM_CONDITIONAL(GMP_NOT_CONFIGURED, true)
fi

if test -f configure.ac ; then
  AM_CONDITIONAL(OUT_OF_SOURCE_BUILD, false)
else
  AM_CONDITIONAL(OUT_OF_SOURCE_BUILD, true)
fi

AC_SUBST([abs_top_builddir])
AC_SUBST([abs_top_srcdir])
#
###############################################################
# Output of autoconf/automake
#

acro=acro
gmp=gmp-4.1.4
lzma=lzma

AC_SUBST([acro])
AC_SUBST([gmp])
AC_SUBST([lzma])

# If configure has not been run in the acro directory,
# add it to the list of subpackages to be configured.

#if test -f packages/$acro/config.status ; then
#  AC_MSG_WARN([Skipping configuration of acro because it's already configured.])
#  AC_MSG_WARN([Delete config.status in the packages/$acro directory if you want to force a reconfigure.])
#else
  AC_CONFIG_SUBDIRS([tpl/acro])
#fi

##
## The python stuff doesn't work on the windows machine I'm
## working on, so I just want to build the executables.
##
AC_ARG_WITH(python, 
AS_HELP_STRING([--without-python],[Skip creation of python environment]),
[DO_PYINSTALL=$withval],
[DO_PYINSTALL=yes])

if test X${DO_PYINSTALL} = Xyes; then
  AM_CONDITIONAL(DO_PYINSTALL, true)
else
  AM_CONDITIONAL(DO_PYINSTALL, false)
fi

ac_configure_args="$ac_configure_args --disable-mpi --without-soplex --without-tinyxml --without-glpk-in-pico"

AC_SUBST([ac_configure_args])

AC_CONFIG_FILES([Makefile])
AC_CONFIG_FILES([examples/Makefile])
AC_CONFIG_FILES([tpl/Makefile])
AC_CONFIG_FILES([tpl/lzma/Makefile])
AC_CONFIG_FILES([tpl/epanet/Makefile])
AC_CONFIG_FILES([packages/Makefile])
AC_CONFIG_FILES([packages/cat/Makefile])
AC_CONFIG_FILES([packages/csa/Makefile])
AC_CONFIG_FILES([packages/cat/tso2Impact/Makefile])
AC_CONFIG_FILES([packages/detect/Makefile])
AC_CONFIG_FILES([packages/test/Makefile])
AC_CONFIG_FILES([packages/test/unit/Makefile])
#AC_CONFIG_FILES([packages/test/acceptance/Makefile])
AC_CONFIG_FILES([packages/test/Robs/Makefile])
AC_CONFIG_FILES([packages/test/test1/Makefile])
#AC_CONFIG_FILES([packages/test/data/Makefile])
#AC_CONFIG_FILES([packages/test/sp/Makefile])
AC_CONFIG_FILES([packages/tevaUtils/Makefile])
AC_CONFIG_FILES([packages/sim/Makefile])
AC_CONFIG_FILES([packages/sim/tevasim/Makefile])
AC_CONFIG_FILES([packages/spot/Makefile])
AC_CONFIG_FILES([packages/spot/grasp/Makefile])
AC_CONFIG_FILES([packages/spot/newgrasp/Makefile])
AC_CONFIG_FILES([packages/spot/rounding/Makefile])
AC_CONFIG_FILES([packages/healthImpacts/Makefile])
AC_CONFIG_FILES([packages/healthImpacts/cat/tso2ImpactModule/Makefile])
AC_CONFIG_FILES([packages/sim/TEVAEpanetJNI/Makefile])
AC_CONFIG_FILES([packages/sim/merlion/applications/Makefile])
AC_CONFIG_FILES([packages/sim/merlion/test/Makefile])
AC_CONFIG_FILES([packages/sim/merlion/test/unit/Makefile])
AC_CONFIG_FILES([packages/sim/merlion/merlionUtils/test/Makefile])
AC_CONFIG_FILES([packages/sim/merlion/merlionUtils/test/unit/Makefile])
AC_CONFIG_FILES([packages/network/Makefile])

AC_OUTPUT
