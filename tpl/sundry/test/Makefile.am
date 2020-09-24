
include ../../../config/MakeMacros.mk

if HAVE_ANSI_CPP_HEADERS
  ANSI_FLAG=-DANSI_HDRS 
else
  ANSI_FLAG=
endif

if BUILD_STATIC_EXECUTABLES
#
#A libtool flag
#
AM_LDFLAGS=@STATIC_FLAGS@
endif

TOPINC=$(top_builddir)/include
TOPLIB=$(top_builddir)/lib

LDADD=$(top_builddir)/packages/utilib/src/.libs/libutilib.a -lm

#
# Paths to: config.h, other configure headers, utilib headers, MPI
#
AM_CPPFLAGS=-I$(top_builddir) -I$(top_srcdir)/config  \
            -I$(TOPINC) $(MPI_FLAGS) $(ANSI_FLAG) \
            -DMPICH_IGNORE_CXX_SEEK

check_PROGRAMS = \
namespace

namespace_SOURCES=namespace.cpp

TESTS = \
namespace

MAINTAINERCLEANFILES = Makefile.in
