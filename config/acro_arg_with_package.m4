dnl @synopsis ACRO_ARG_WITH_PACKAGE(LC_PACKAGE, UC_PACKAGE,
dnl               PATH_TO_PACKAGE_DIR
dnl               PATH_TO_FILE_IN_PACKAGE
dnl               DEFAULT)
dnl  
dnl LC_PACKAGE, UC_PACKAGE - package name in lower and upper case, alphanumeric
dnl                          and underscores only
dnl PATH_TO_PACKAGE_DIR - path to package RELATIVE to Makefile that will list
dnl                       it as a SRCDIR
dnl PATH_TO_FILE_IN_PACKAGE - absolute path to a file in the package in the
dnl                           source directory
dnl DEFAULT - yes or no, default --with-package value
dnl
AC_DEFUN([ACRO_ARG_WITH_PACKAGE],
[
if test -f $4 ; then 
  AC_ARG_WITH([$1],
  AS_HELP_STRING([--with-$1],[build acro with $2 (default is $5)]),
  ac_cv_with_$1=$withval,
  ac_cv_with_$1=$5
  )
else
  ac_cv_with_$1=no
fi
  
AC_MSG_CHECKING(whether to include [$1])
  
AM_CONDITIONAL([BUILD_$2], test "X$ac_cv_with_$1" = "Xyes")
if test "X$ac_cv_with_$1" = "Xyes" ; then
  AC_DEFINE([USING_$2],,[Define if want to build with $1 enabled])
  AC_MSG_RESULT(yes)  
  $2_DIR=$3
else
  AC_MSG_RESULT(no)
fi

AC_SUBST([$2_DIR])
])
