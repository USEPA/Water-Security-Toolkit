
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

CXXTEST_INCLUDE=-I${top_srcdir}/tpl/acro/tpl/cxxtest
MERLION_INCLUDE=-I${top_srcdir}/packages/sim/merlion/src
MERLIONUTILS_INCLUDE=-I${top_srcdir}/packages/sim/merlion/merlionUtils/src
BOOST_INCLUDE=-I${top_srcdir}/tpl/acro/tpl/boost

AM_CPPFLAGS=$(CXXTEST_INCLUDE) $(MERLION_INCLUDE) $(MERLIONUTILS_INCLUDE) $(BOOST_INCLUDE)

#
# programs to build at "make check" time
#
check_PROGRAMS = runner


EXECS=$(check_PROGRAMS)

runner_CXXTEST_SRC=\
	TInjection.cpp \
	TTSG_Reader.cpp \
	TEpanetLinker.cpp \
	runner.cpp

runner_STATIC_SRC=
runner_LDADD= $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la $(BLAS_LIBS) $(FLIBS)
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

