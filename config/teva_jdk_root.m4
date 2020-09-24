dnl TEVA set jdk root directory
dnl
dnl  TEVA_WITH_JDKROOT
dnl
dnl    lc   --enable-name, usually code name in lower case 
dnl    UC   ENABLE_NAME, BUILD_NAME, usually name in upper case
dnl

AC_DEFUN([TEVA_WITH_JDKROOT],[
AC_ARG_WITH(jdk-root,
AS_HELP_STRING([--with-jdk-root="JDK_ROOT_DIR"],[Use the specified dir to locate JNI include files.]),
[JDK_ROOT_DIR=$withval],
[JDK_ROOT_DIR=notset]
)

if test "$JDK_ROOT_DIR" = "notset" ; then : ; else
  if test X${linux_os} = Xyes ; then 
    jdk_inc_root=$JDK_ROOT_DIR/include
    jdk_inc_os=$jdk_inc_root/linux
  else
    jroot=`cygpath -d $JDK_ROOT_DIR`
    jdk_inc_root=$JDK_ROOT_DIR/include
    jdk_inc_os=$jdk_inc_root/win32
  fi 
  if ! test -d $jdk_inc_root ; then
    AC_MSG_ERROR([JDK_ROOT: $jdk_inc_root does not exist])
  fi
  if ! test -d $jdk_inc_os ; then
    AC_MSG_ERROR([JDK_ROOT: $jdk_inc_os does not exist])
  fi
  JDK_INC="-I$jdk_inc_root -I$jdk_inc_os"
fi

])
