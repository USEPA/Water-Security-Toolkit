include_directories(${CMAKE_CURRENT_SOURCE_DIR}/..)
add_definitions(-DMPICH_IGNORE_CXX_SEEK)
add_executable(ufl ufl.cpp VolVolume.cpp)
#
#SRCBIN=@top_srcdir@/bin
#BLDBIN=@top_builddir@/bin
#TOPLIB=@top_builddir@/lib
#TOPINC=@top_builddir@/include
#
#if HAVE_ANSI_CPP_HEADERS
#  ANSI_FLAG=-DANSI_HDRS 
#else
#  ANSI_FLAG=
#endif
#
##
## Paths to: config.h, other configure headers, sundry headers, MPI
##
#AM_CPPFLAGS=-I${top_srcdir}/packages/sundry -I$(TOPINC) $(MPI_FLAG) $(ANSI_FLAG) \
#            -DMPICH_IGNORE_CXX_SEEK -I${top_srcdir}/packages/sundry/src
