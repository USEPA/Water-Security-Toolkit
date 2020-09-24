dnl TEVA enable/disable build of code
dnl
dnl  TEVA_ENABLE_BUILD(lc, UC, default)
dnl
dnl    lc   --enable-name, usually code name in lower case 
dnl    UC   ENABLE_NAME, BUILD_NAME, usually name in upper case
dnl    default    yes, build it or no, don't build it
dnl

AC_DEFUN([TEVA_ENABLE_BUILD],[
AC_ARG_ENABLE([$1],
AS_HELP_STRING([--enable-$1],[Build $1.  Default is $3.]),
[ENABLE_$2=$enableval],
[ENABLE_$2=$3]
)
AC_MSG_CHECKING([whether to build $1])
if test X${ENABLE_$2} = Xyes ; then
  AM_CONDITIONAL(BUILD_$2, true)
  AC_MSG_RESULT(yes)
else
  AM_CONDITIONAL(BUILD_$2, false)
  AC_MSG_RESULT(no)
fi

])
