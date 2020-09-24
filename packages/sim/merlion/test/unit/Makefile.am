
include ${top_srcdir}/config/MakeMacros.mk
SUFFIXES=.i

LIBDIR=@top_builddir@/lib

CXXTEST= ${top_builddir}/python/bin/cxxtestgen

TOPINC=$(top_builddir)/include
TOPLIB=$(top_builddir)/lib
TOPBUILDBIN=${top_builddir}/bin
TOPSRCBIN=${top_srcdir}/bin

if HAVE_ANSI_CPP_HEADERS
  ANSI_FLAG=-DANSI_HDRS
else
  ANSI_FLAG=
endif

#if BUILD_AMPL
#AMPLLIB=$(top_builddir)/tpl/ampl/amplsolver.a
#endif

#UTILIB_INCLUDE=-I${top_srcdir}/packages/utilib/src \
#	-I${top_builddir}/packages/utilib/src
#COLIN_INCLUDE=-I${top_srcdir}/packages/colin/src
CXXTEST_INCLUDE=-I${top_srcdir}/tpl/acro/tpl/cxxtest
MERLION_INCLUDE=-I${top_srcdir}/packages/sim/merlion/src

#
# Paths to: config.h, other configure headers, colin headers
#
#LDADD=$(top_builddir)/packages/colin/src/libcolin.a \
#	$(top_builddir)/packages/utilib/src/libutilib.la \
#	$(AMPLLIB) -lm $(DLOPEN_LIBS)
#
# Paths to: config.h, other configure headers, utilib headers, MPI
#
AM_CPPFLAGS=$(CXXTEST_INCLUDE) $(MERLION_INCLUDE)
#            $(UTILIB_INCLUDE) $(COLIN_INCLUDE) \
#            -I$(TOPINC) $(MPI_FLAGS) $(ANSI_FLAG) $(BOOST_CPPFLAGS) \
#            $(TINYXML_INCLUDE) -DMPICH_IGNORE_CXX_SEEK -DIGNORE_BROKEN

#if BUILD_STATIC_EXECUTABLES
#
#A libtool flag
#
#AM_LDFLAGS=@STATIC_FLAGS@
#endif

#
# programs to build at "make check" time
#
check_PROGRAMS = runner


#if BUILD_SHARED
#EXECS=$(check_PROGRAMS:%=.libs/%)
#else
#EXECS=$(check_PROGRAMS)
#endif
EXECS=$(check_PROGRAMS)

#BIN_COPIES=$(check_PROGRAMS:%=$(TOPBIN)/%)

runner_CXXTEST_SRC=\
	TBlasWrapperTests.cpp \
	TTriSolve.cpp \
	TSparseMatrix.cpp \
	runner.cpp

runner_STATIC_SRC=
runner_LDADD= $(LIBDIR)/libmerlion.la $(BLAS_LIBS) $(FLIBS)
runner_SOURCES=$(runner_STATIC_SRC) $(runner_CXXTEST_SRC) 

runner.cpp:
	$(CXXTEST) --have-eh --have-std --root --xunit-printer \
		-o runner.cpp 

# Implicit rule to invoke cxxtest to autogenerate .cpp from .h test files
#  [test needed to prevent using cxxtest on non-test sources]
.h.cpp:
	@for x in $(runner_CXXTEST_SRC); do \
	   if [ "$$x" = "$@" ]; then \
	      echo $(CXXTEST) --have-eh --have-std --part -o $@ $<; \
	      $(CXXTEST) --have-eh --have-std --part -o $@ $<; \
	   fi \
	done

# prevent implicit rule for static sources
.PHONY:
$(runner_STATIC_SRC):
	@echo NO-OP

check-local:

clean-local:
	@for i in *.gcda *.gcno *.gcov ; \
        do\
                rm -f $$i ;\
        done


