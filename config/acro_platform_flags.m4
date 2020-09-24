dnl ACRO platform flags

AC_DEFUN([ACRO_PLATFORM_FLAGS],[
  dnl
  dnl Determine special platform preprocessor and compiler flags
  dnl TODO: Create tests for NO_MEMBER_TEMPLATE_FRIENDS

echo "OS $host_os"

case $host_os in

  # DEC OSF
  *osf*)

    case $CXX in
      cxx* ) SPECIAL_CXXFLAGS="-DOSF_CC";;
    esac
  ;;

  # IBM test
  *aix*)

    case $CXX in
      xlC* ) SPECIAL_CXXFLAGS="-DAIX_CC -qnolm -qrtti";;
    esac
  ;;

  # SGI/IRIX test
  irix* )

    if test "${CXX}" = "CC"; then
     dnl SPECIAL_CFLAGS="-DSGI_CC -LANG:std -LANG:ansi-for-init-scope=ON -64 -DSGI64"
     dnl SPECIAL_CXXFLAGS="-DSGI_CC -LANG:std -LANG:ansi-for-init-scope=ON -64 -DSGI64"
     SPECIAL_CFLAGS="-DSGI_CC -LANG:std -LANG:ansi-for-init-scope=ON"
     SPECIAL_CXXFLAGS="-DSGI_CC -LANG:std -LANG:ansi-for-init-scope=ON"
     F77_MISC=f77
     FFLAGS_MISC="-64"
     SPECIAL_AR="CC -ar -W,l -W,s -o"
    fi

  ;;

  solaris* )

    if test "${CXX}" = "CC"; then
       SPECIAL_AR="CC -xar -o";
       SPECIAL_CXXFLAGS="-DSOLARIS_CC -DACRO_NO_MEMBER_TEMPLATE_FRIENDS -DUTILIB_NO_MEMBER_TEMPLATE_FRIENDS"
       WITH_OPT=`echo "${WITH_OPT}" | sed 's/-O\([[0-9]]*\)/-xO\1/g'`;
       optimization_default=`echo "${optimization_default}" | sed 's/-O\([[0-9]]*\)/-xO\1/g'`;
    fi

  ;;

  cygwin*)

    if test X${WITH_MINGW} = Xyes ; then
       SPECIAL_CFLAGS="-mno-cygwin"
       SPECIAL_CPPFLAGS="-mno-cygwin"
       SPECIAL_CXXFLAGS="-mno-cygwin"
       SPECIAL_LDFLAGS="-mno-cygwin"
    fi

  ;;

esac

])
