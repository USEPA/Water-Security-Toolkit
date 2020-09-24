dnl ACRO MPI tests
dnl
dnl   ACRO_MPI_TESTS(subpackage)
dnl
dnl     Use "subpackage" option to omit tests and just set
dnl     makefile conditionals and variables.  Also avoids
dnl     repeating MPI paths in CPPFLAGS and LDFLAGS.

AC_DEFUN([ACRO_MPI_TESTS],[
  dnl
  dnl Test to see if MPI compilers work properly
  dnl 

dnl
dnl Compile check
dnl
if test "X${ENABLE_MPI}" = "Xyes"; then

  if test -n "${MPI_DIR}" && test -z "${MPI_INC}"; then
    MPI_INC="${MPI_DIR}/include"
  fi

  if test X$1 = Xsubpackage ; then
      AC_DEFINE(HAVE_MPI,,[define that mpi is being used])
  else
    if test "X${MPI_CXX}" = "Xnone" || test "X${MPI_CC}" = "Xnone" ; then
      CPPFLAGS="${CPPFLAGS} -I${MPI_INC}"
    fi
  
    if test -n "${MPI_CPPFLAGS}" ; then
      CPPFLAGS="${CPPFLAGS} ${MPI_CPPFLAGS}"
    fi
  
    AC_LANG([C++])
    AC_MSG_CHECKING(for mpi.h)
    AC_PREPROC_IFELSE(
    [AC_LANG_SOURCE([[#include "mpi.h"]])],
    [
      AC_MSG_RESULT(yes)
      AC_DEFINE(HAVE_MPI,,[define that mpi is being used])
    ],[
      AC_MSG_RESULT(no)
      echo "---"
      echo "Cannot find header file mpi.h."
      echo "Either compile without mpi (no --enable-mpi or --with-mpi directives)"
      echo "or specify --with-mpi-incdir=<dir>."
      echo "---"
      AC_MSG_ERROR(cannot find mpi.h)
    ])
  fi

  AM_CONDITIONAL(BUILD_MPI, true)

  AC_SUBST([MPI_INC])
  AC_SUBST([MPI_CXX])
  AC_SUBST([MPI_CC])
  AC_SUBST([MPI_F77])

  if test "X${ENABLE_MPE}" = "Xyes"; then
    if test X$1 = Xsubpackage ; then
        AC_DEFINE(HAVE_MPE,1,[define that mpi's mpe is being used])
    else
      AC_LANG([C++])
      AC_MSG_CHECKING(for mpe.h)
      AC_PREPROC_IFELSE(
      [AC_LANG_SOURCE([[#include "mpe.h"]])],
      [
        AC_MSG_RESULT(yes)
        AC_DEFINE(HAVE_MPE,1,[define that mpi's mpe is being used])
      ],[
        AC_MSG_RESULT(no)
        echo "---"
        echo "Cannot find header file mpe.h."
        echo "---"
        AC_MSG_ERROR(cannot find mpe.h)
      ])
    fi

    AM_CONDITIONAL(BUILD_MPE, true)
    AC_SUBST([MPE_LIBS])
  else
    AM_CONDITIONAL(BUILD_MPE, false)
  fi

else

  AM_CONDITIONAL(BUILD_MPI, false)
  AM_CONDITIONAL(BUILD_MPE, false)

fi

dnl
dnl Link check
dnl
dnl TODO - do we want to test that the mpe libraries are there?
dnl this is probably not necessary, if mpi was there and mpe.h was there
dnl   if test X${ENABLE_MPE} = Xyes"; then
dnl   fi
dnl


if test "X${ENABLE_TESTS}" = "Xyes" && test "X${ENABLE_MPI}" = "Xyes"; then

  if test -n "${MPI_DIR}" && test -z "${MPI_LIBDIR}"; then
    MPI_LIBDIR="${MPI_DIR}/lib"
  fi

  if test -z "${MPI_LIBS}" && test -n "${MPI_LIBDIR}"; then
    MPI_LIBS="-lmpi"
  fi

  if test X$1 = Xsubpackage ; then :; else
    if test -n "${MPI_LDFLAGS}" ; then
      LDFLAGS="${LDFLAGS} ${MPI_LDFLAGS}"
    fi
  
    AC_LANG([C++])
    AC_MSG_CHECKING(whether MPI will link using C++ compiler)
  
    if test "X${MPI_CXX}" = "Xnone" || test "X${MPI_CC}" = "Xnone" ; then
      LIBS="${LIBS} ${MPI_LIBS}"
      LDFLAGS="${LDFLAGS} -L${MPI_LIBDIR}"
    fi
  
    AC_LINK_IFELSE(
    [AC_LANG_PROGRAM([[#include <mpi.h>]], [[int c; char** v; MPI_Init(&c,&v);]])],
    [AC_MSG_RESULT(yes)],
    [AC_MSG_RESULT(no)
     echo "-----"
     echo "Cannot link simple MPI program."
     echo "Try --with-mpi-cxx to specify MPI C++ compile script."
     echo "Or try --with-mpi-libs, --with-mpi-incdir, --with-mpi-libdir"
     echo "to specify all the specific MPI compile options."
     echo "-----"
     AC_MSG_ERROR(MPI cannot link)
    ])
  fi
fi

])
