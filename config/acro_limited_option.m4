dnl ACRO test options

AC_DEFUN([ACRO_LIMITED_OPTION],[
  dnl
  dnl do we have restricted codes available to include in build
  dnl 

LIMITED_DIR=""
AC_MSG_CHECKING(include teva-spot-limited code)

AC_ARG_WITH(limited,
AS_HELP_STRING([--with-limited],[compile with limited (restricted) code if available]),
[WITH_LIMITED=$withval],
[WITH_LIMITED=notset]
)

if test $WITH_LIMITED == no ; then
  LIMITED_DIR=notset

elif test $WITH_LIMITED == yes ; then
  if test -d ${srcdir}/packages/healthImpacts/hia ; then
    LIMITED_DIR=packages/healthImpacts
  else
    LIMITED_DIR=failed
  fi

elif test $WITH_LIMITED == notset ; then
  if test -d ${srcdir}/packages/healthImpacts/hia ; then
    LIMITED_DIR=packages/healthImpacts
  else
    LIMITED_DIR=notset
  fi
fi

if test $LIMITED_DIR == failed ; then
  AC_MSG_ERROR([Can not find teva-spot limited code.])
fi

if test $LIMITED_DIR == notset ; then
  AC_DEFINE([LIMITED],0,[define if limited codes are available])
  AM_CONDITIONAL(HAVE_LIMITED, false)
  AC_MSG_RESULT(no)
else
  AC_DEFINE([LIMITED],1,[define if limited codes are available])
  AM_CONDITIONAL(HAVE_LIMITED, true)
  AC_MSG_RESULT(yes)
fi

AC_SUBST([LIMITED_DIR])


])
