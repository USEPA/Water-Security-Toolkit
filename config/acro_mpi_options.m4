dnl ACRO MPI options

AC_DEFUN([ACRO_MPI_OPTIONS],[
  dnl
  dnl Command-line options for setting up MPI
  dnl 
AC_ARG_ENABLE(mpi,
AS_HELP_STRING([--enable-mpi],[Enable MPI support]),
[ENABLE_MPI=$enableval],
[ENABLE_MPI=no]
)

AC_ARG_WITH(mpi-compilers,
AS_HELP_STRING([--with-mpi-compilers=DIR or --with-mpi-compilers=yes],
[use MPI compilers (mpicc, etc.) found in directory DIR, or in PATH if =yes]),
[
  ENABLE_MPI=yes
  if test X${withval} = Xyes; then
    # Look for MPI C++ compile script/program in user's path, set
    # MPI_CXX to name of program found, else set it to "none".  Could
    # add more paths to check as 4th argument if you wanted to.
    # Calls AC_SUBST for MPI_CXX, etc.

    AC_CHECK_PROGS(MPI_CXX, mpiCC mpicxx mpic++, none,)
    AC_CHECK_PROGS(MPI_CC, mpicc, none,)
    AC_CHECK_PROGS(MPI_F77, mpif77, none,)
  else
    MPI_CXX=none
    AC_MSG_CHECKING(MPI C++ compiler)
    if test -f ${withval}/mpiCC ; then
      MPI_CXX=${withval}/mpiCC
    fi
    if test "X${MPI_CXX}" = "Xnone" && test -f ${withval}/mpicxx ; then
      MPI_CXX=${withval}/mpicxx
    fi
    if test "X${MPI_CXX}" = "Xnone" && test -f ${withval}/mpic++ ; then
      MPI_CXX=${withval}/mpic++
    fi
    AC_MSG_RESULT([${MPI_CXX}])

    AC_MSG_CHECKING(MPI C compiler)
    if test -f ${withval}/mpicc ; then
      MPI_CC=${withval}/mpicc
    else
      MPI_CC=none
    fi
    AC_MSG_RESULT([${MPI_CC}])

    AC_MSG_CHECKING(MPI Fortran compiler)
    if test -f ${withval}/mpif77 ; then
      MPI_F77=${withval}/mpif77
    else
      MPI_F77=none
    fi
    AC_MSG_RESULT([${MPI_F77}])
  fi

  if test "X${MPI_CXX}" = "Xnone" ; then
    AC_MSG_ERROR([MPI C++ compiler script/program not found.])
    AM_CONDITIONAL(USE_MPI_CXX, false)
  else
    AM_CONDITIONAL(USE_MPI_CXX, true)
  fi

  if test "X${MPI_CC}" = "Xnone" ; then
    AC_MSG_ERROR([MPI C compiler script/program (mpicc) not found.])
    AM_CONDITIONAL(USE_MPI_CC, false)
  else
    AM_CONDITIONAL(USE_MPI_CC, true)
  fi

  if test "X${MPI_F77}" = "Xnone" ; then
    AC_MSG_ERROR([MPI Fortran compiler script/program (mpif77) not found.])
    AM_CONDITIONAL(USE_MPI_F77, false)
  else
    AM_CONDITIONAL(USE_MPI_F77, true)
  fi
],
[ MPI_CXX=none
  MPI_CC=none
  MPI_F77=none
  AM_CONDITIONAL(USE_MPI_CXX, false)
  AM_CONDITIONAL(USE_MPI_CC, false)
  AM_CONDITIONAL(USE_MPI_F77, false)
]
)


AC_ARG_WITH(mpi,
AS_HELP_STRING([--with-mpi=MPIROOT],[use MPI root directory. Automatically enables MPI.]),
[
  # This if statement is necessary (for now) to make this compatible with DAKOTA
  if test X${withval} = X; then :; else
     ENABLE_MPI=yes
     MPI_DIR=${withval}
     AC_MSG_CHECKING(MPI directory)
     AC_MSG_RESULT([${MPI_DIR}])
  fi
]
)

AC_ARG_WITH(mpi-libs,
AS_HELP_STRING([--with-mpi-libs="LIBS"],[MPI libraries @<:@default "-lmpi"@:>@]),
[
  ENABLE_MPI=yes
  MPI_LIBS=${withval}
  AC_MSG_CHECKING(user-defined MPI libraries)
  AC_MSG_RESULT([${MPI_LIBS}])
]
)

AC_ARG_WITH(mpe-libs,
AS_HELP_STRING([--with-mpe-libs="LIBS"],[MPE libraries @<:@default "-lampe -llmpe -ltmpe -lmpe"@:>@]),
[
  ENABLE_MPE=yes
  MPE_LIBS=${withval}
  AC_MSG_CHECKING(user-defined MPE libraries)
  AC_MSG_RESULT([${MPE_LIBS}])
],
[ ENABLE_MPE=no]
)

AC_ARG_WITH(mpi-incdir,
AS_HELP_STRING([--with-mpi-incdir=DIR],[MPI include directory @<:@default MPIROOT/include@:>@]),
[
  ENABLE_MPI=yes
  MPI_INC=${withval}
  AC_MSG_CHECKING(user-defined MPI includes)
  AC_MSG_RESULT([${MPI_INC}])
]
)

AC_ARG_WITH(mpi-libdir,
AS_HELP_STRING([--with-mpi-libdir=DIR],[MPI library directory @<:@default MPIROOT/lib@:>@]),
[
  ENABLE_MPI=yes
  MPI_LIBDIR=${withval}
  AC_MSG_CHECKING(user-defined MPI library directory)
  AC_MSG_RESULT([${MPI_LIBDIR}])
]
)

AC_ARG_WITH(mpi-cppflags,
AS_HELP_STRING([--with-mpi-cppflags=flags],[Extra preprocessor flags for MPI]),
[
  ENABLE_MPI=yes
  MPI_CPPFLAGS=${withval}
  AC_MSG_CHECKING(user-defined MPI preprocessor flags)
  AC_MSG_RESULT([${MPI_CPPFLAGS}])
]
)

AC_ARG_WITH(mpi-ldflags,
AS_HELP_STRING([--with-mpi-ldflags=flags],[Extra link flags for MPI]),
[
  ENABLE_MPI=yes
  MPI_LDFLAGS=${withval}
  AC_MSG_CHECKING(user-defined MPI link flags)
  AC_MSG_RESULT([${MPI_LDFLAGS}])
]
)

AC_MSG_CHECKING(whether we are using MPI)
AC_MSG_RESULT([${ENABLE_MPI}])

])
