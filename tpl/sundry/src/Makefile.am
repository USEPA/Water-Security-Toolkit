
include ../../../config/MakeMacros.mk

if BUILD_STATIC_EXECUTABLES
#
#A libtool flag
#
AM_LDFLAGS=@STATIC_FLAGS@
endif

bin_PROGRAMS=ufl

ufl_SOURCES=ufl.cpp VolVolume.cpp

#lib_LIBRARIES=libsundry.a

SRCBIN=@top_srcdir@/bin
BLDBIN=@top_builddir@/bin
TOPLIB=@top_builddir@/lib
TOPINC=@top_builddir@/include

if HAVE_ANSI_CPP_HEADERS
  ANSI_FLAG=-DANSI_HDRS 
else
  ANSI_FLAG=
endif

#
# Paths to: config.h, other configure headers, sundry headers, MPI
#
AM_CPPFLAGS=-I${top_srcdir}/packages/sundry -I$(TOPINC) $(MPI_FLAG) $(ANSI_FLAG) \
            -DMPICH_IGNORE_CXX_SEEK -I${top_srcdir}/packages/sundry/src

#libsundry_a_SOURCES= \
#sundry.cpp

all-local: ufl$(EXEEXT)
#	@if test -d $(TOPLIB) ; then : ; else mkdir $(TOPLIB) ; fi
#	@$(SRCBIN)/cp_u libsundry.a $(TOPLIB)
	@if test -d $(BLDBIN) ; then : ; else mkdir $(BLDBIN) ; fi
	@$(SRCBIN)/cp_u ufl$(EXEEXT) $(BLDBIN)

clean-local:
#	@rm -f $(TOPLIB)/*sundry*
	@rm -f $(BLDBIN)/ufl*

MAINTAINERCLEANFILES = Makefile.in

