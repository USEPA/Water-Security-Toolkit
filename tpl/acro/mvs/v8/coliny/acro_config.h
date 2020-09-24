#ifndef _INCLUDE_ACRO_CONFIG_H
#define _INCLUDE_ACRO_CONFIG_H 1
 
/* include/acro_config.h. Generated automatically at end of configure. */
/* include/config.h.  Generated by configure.  */
/* include/config.h.in.  Generated from configure.ac by autoheader.  */

/* software build cpu is sparc */
/* #undef ACRO_BUILD_CPU_SPARC */

/* software build cpu is x86 */
#ifndef ACRO_BUILD_CPU_X86 
#define ACRO_BUILD_CPU_X86  1 
#endif

/* software build cpu is 64 bit x86 */
/* #undef ACRO_BUILD_CPU_X86_64 */

/* software build os is cygwin */
#ifndef ACRO_BUILD_CYGWIN 
#define ACRO_BUILD_CYGWIN  1 
#endif

/* software build os is linux */
/* #undef ACRO_BUILD_LINUX */

/* software build os is solaris */
/* #undef ACRO_BUILD_SOLARIS */

/* Define to dummy `main' function (if any) required to link to the Fortran
   libraries. */
/* #undef ACRO_F77_DUMMY_MAIN */

/* Define to a macro mangling the given C identifier (in lower and upper
   case), which must not contain underscores, for linking with Fortran. */
#ifndef ACRO_F77_FUNC 
#define ACRO_F77_FUNC (name,NAME) name ## _ 
#endif

/* As F77_FUNC, but for C identifiers containing underscores. */
#ifndef ACRO_F77_FUNC_ 
#define ACRO_F77_FUNC_ (name,NAME) name ## __ 
#endif

/* Define if F77 and FC dummy `main' functions are identical. */
/* #undef ACRO_FC_DUMMY_MAIN_EQ_F77 */

/* Define if you have a BLAS library. */
/* #undef ACRO_HAVE_BLAS */

/* Define if you have CPLEX library. */
/* #undef ACRO_HAVE_CPLEX */

/* Define if you have the dlopen function. */
#ifndef ACRO_HAVE_DLOPEN 
#define ACRO_HAVE_DLOPEN  1 
#endif

/* define if the compiler supports exceptions */
#ifndef ACRO_HAVE_EXCEPTIONS 
#define ACRO_HAVE_EXCEPTIONS   
#endif

/* define if the compiler supports the explicit keyword */
#ifndef ACRO_HAVE_EXPLICIT 
#define ACRO_HAVE_EXPLICIT   
#endif

/* Define to 1 if you have the `getrusage' function. */
/* #ifndef ACRO_HAVE_GETRUSAGE  */
/* #define ACRO_HAVE_GETRUSAGE  1  */
/* #endif */

/* Define to 1 if you have the <inttypes.h> header file. */
#ifndef ACRO_HAVE_INTTYPES_H 
#define ACRO_HAVE_INTTYPES_H  1 
#endif

/* Define if you have LAPACK library. */
/* #undef ACRO_HAVE_LAPACK */

/* define if the compiler supports member templates */
#ifndef ACRO_HAVE_MEMBER_TEMPLATES 
#define ACRO_HAVE_MEMBER_TEMPLATES   
#endif

/* Define to 1 if you have the <memory.h> header file. */
#ifndef ACRO_HAVE_MEMORY_H 
#define ACRO_HAVE_MEMORY_H  1 
#endif

/* define that mpi is being used */
/* #undef ACRO_HAVE_MPI */

/* define if the compiler implements namespaces */
#ifndef ACRO_HAVE_NAMESPACES 
#define ACRO_HAVE_NAMESPACES   
#endif

/* Define if you have NPSOL library. */
/* #undef ACRO_HAVE_NPSOL */

/* define if the compiler has stringstream */
#ifndef ACRO_HAVE_SSTREAM 
#define ACRO_HAVE_SSTREAM   
#endif

/* define if the compiler supports ISO C++ standard library */
#ifndef ACRO_HAVE_STD 
#define ACRO_HAVE_STD   
#endif

/* Define to 1 if you have the <stdint.h> header file. */
#ifndef ACRO_HAVE_STDINT_H 
#define ACRO_HAVE_STDINT_H  1 
#endif

/* Define to 1 if you have the <stdlib.h> header file. */
#ifndef ACRO_HAVE_STDLIB_H 
#define ACRO_HAVE_STDLIB_H  1 
#endif

/* Define to 1 if you have the `strerror' function. */
#ifndef ACRO_HAVE_STRERROR 
#define ACRO_HAVE_STRERROR  1 
#endif

/* Define to 1 if you have the <strings.h> header file. */
/* #ifndef ACRO_HAVE_STRINGS_H  */
/* #define ACRO_HAVE_STRINGS_H  1  */
/* #endif */

/* Define to 1 if you have the <string.h> header file. */
#ifndef ACRO_HAVE_STRING_H 
#define ACRO_HAVE_STRING_H  1 
#endif

/* Define to 1 if you have the <sys/stat.h> header file. */
/* #ifndef ACRO_HAVE_SYS_STAT_H  */
/* #define ACRO_HAVE_SYS_STAT_H  1  */
/* #endif */

/* Define to 1 if you have the <sys/types.h> header file. */
/* #ifndef ACRO_HAVE_SYS_TYPES_H  */
/* #define ACRO_HAVE_SYS_TYPES_H  1  */
/* #endif */

/* Define to 1 if you have the <unistd.h> header file. */
/* #ifndef ACRO_HAVE_UNISTD_H  */
/* #define ACRO_HAVE_UNISTD_H  1  */
/* #endif */

/* Define to 1 if you have the <values.h> header file. */
/* #undef ACRO_HAVE_VALUES_H */

/* software host will be cygwin */
#ifndef ACRO_HOST_CYGWIN 
#define ACRO_HOST_CYGWIN  1 
#endif

/* software host is GNU */
/* #undef ACRO_HOST_GNU */

/* software host will be linux */
/* #undef ACRO_HOST_LINUX */

/* software host will be mingw */
/* #undef ACRO_HOST_MINGW */

/* software host will be solaris */
/* #undef ACRO_HOST_SOLARIS */

/* Name of package */
#ifndef ACRO_PACKAGE 
#define ACRO_PACKAGE  "acro" 
#endif

/* Define to the address where bug reports for this package should be sent. */
#ifndef ACRO_PACKAGE_BUGREPORT 
#define ACRO_PACKAGE_BUGREPORT  "acro-help@software.sandia.gov" 
#endif

/* Define to the full name of this package. */
#ifndef ACRO_PACKAGE_NAME 
#define ACRO_PACKAGE_NAME  "acro" 
#endif

/* Define to the full name and version of this package. */
#ifndef ACRO_PACKAGE_STRING 
#define ACRO_PACKAGE_STRING  "acro VOTD" 
#endif

/* Define to the one symbol short name of this package. */
#ifndef ACRO_PACKAGE_TARNAME 
#define ACRO_PACKAGE_TARNAME  "acro" 
#endif

/* Define to the version of this package. */
#ifndef ACRO_PACKAGE_VERSION 
#define ACRO_PACKAGE_VERSION  "VOTD" 
#endif

/* Define to 1 if you have the ANSI C header files. */
/* #ifndef ACRO_STDC_HEADERS  */
/* #define ACRO_STDC_HEADERS  1  */
/* #endif */

/* software target will be cygwin */
#ifndef ACRO_TARGET_CYGWIN 
#define ACRO_TARGET_CYGWIN  1 
#endif

/* software target will be linux */
/* #undef ACRO_TARGET_LINUX */

/* software target will be mingw */
/* #undef ACRO_TARGET_MINGW */

/* software target will be solaris */
/* #undef ACRO_TARGET_SOLARIS */

/* Define if want to build with ampl enabled */
#ifndef ACRO_USING_AMPL
#define ACRO_USING_AMPL
#endif

/* Define if want to build with appspack enabled */
/* #undef ACRO_USING_APPSPACK */

/* define that clp is being used */
#ifndef ACRO_USING_CLP 
#define ACRO_USING_CLP  1 
#endif

/* define that cobyla is being used */
/* #ifndef ACRO_USING_COBYLA */
/* #define ACRO_USING_COBYLA  1 */
/* #endif */

/* Define if want to build with coin enabled */
/* #undef ACRO_USING_COIN */

/* Define if want to build with colin enabled */
/* #undef ACRO_USING_COLIN */

/* Define if want to build with coliny enabled */
/* #undef ACRO_USING_COLINY */

/* Define if want to build with dscpack enabled */
/* #undef ACRO_USING_DSCPACK */

/* Define if want to build with exact enabled */
#ifndef ACRO_USING_EXACT 
#define ACRO_USING_EXACT   
#endif

/* Define if want to build with filib enabled */
/* #undef ACRO_USING_FILIB */

/* Define if want to build with glpk enabled */
/* #undef ACRO_USING_GLPK */

/* Define if want to build with gnlp enabled */
/* #undef ACRO_USING_GNLP */

/* Define if want to build with ipopt enabled */
/* #undef ACRO_USING_IPOPT */

/* Define if want to build with mtl enabled */
/* #undef ACRO_USING_MTL */

/* Define if want to build with optpp enabled */
/* #undef ACRO_USING_OPTPP */

/* Define if want to build with parpcx enabled */
/* #undef ACRO_USING_PARPCX */

/* Define if want to build with pebbl enabled */
/* #undef ACRO_USING_PEBBL */

/* Define if want to build with pico enabled */
/* #undef ACRO_USING_PICO */

/* define that plgo is being used */
#ifndef ACRO_USING_PLGO 
#define ACRO_USING_PLGO  1 
#endif

/* Define if want to build with soplex enabled */
/* #undef ACRO_USING_SOPLEX */

/* Define if want to build with 3po enabled */
/* #undef ACRO_USING_THREEPO */

/* Define if want to build with tmf enabled */
/* #undef ACRO_USING_TMF */

/* Define if want to build with tracecache enabled */
/* #undef ACRO_USING_TRACECACHE */

/* Define if want to build with trilinos enabled */
/* #undef ACRO_USING_TRILINOS */

/* Define if want to build with utilib enabled */
#ifndef ACRO_USING_UTILIB 
#define ACRO_USING_UTILIB   
#endif

/* turn on code validation tests */
/* #undef ACRO_VALIDATING */

/* Version number of package */
#ifndef ACRO_VERSION 
#define ACRO_VERSION  "VOTD" 
#endif

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef ACRO_WORDS_BIGENDIAN */

/* define whether checksum function is included in utilib */
#ifndef ACRO_YES_CHECKSUM 
#define ACRO_YES_CHECKSUM   
#endif

/* define whether CommonIO is included in utilib */
#ifndef ACRO_YES_COMMONIO 
#define ACRO_YES_COMMONIO   
#endif

/* define whether memdebug is included in utilib */
/* #undef ACRO_YES_MEMDEBUG */

/* Define to `unsigned' if <sys/types.h> does not define. */
/* #undef _acro_size_t */

#ifdef ACRO_USING_UTILIB
#include <utilib/utilib_config.h>
#endif

 
/* once: _INCLUDE_ACRO_CONFIG_H */
#endif
