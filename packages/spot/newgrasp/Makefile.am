INCDIR=@top_builddir@/include
LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
ACRO=@top_builddir@/tpl/@acro@
UTILIBDIR=$(ACRO)/packages/utilib/src

TSO2IMPACTDIR=@top_builddir@/packages/cat/tso2Impact
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

if BUILD_EXECUTABLES

if BUILD_CYGWIN
  CYGWIN_FLAGS=-D__CYGWIN__ -DCYGWIN 
endif

if BUILD_LINUX
  LINUX_FLAGS=-D__LINUX__ -D__linux__
endif

if BUILD_APPLE
  APPLE_FLAGS=-D__APPLE__
endif

if HOST_MINGW
  MINGW_FLAGS=-DMINGW 
endif

ARCH_FLAGS=$(CYGWIN_FLAGS) $(LINUX_FLAGS) $(APPLE_FLAGS) $(MINGW_FLAGS)

if HAVE_ANSI_C_HEADERS
  ANSI_FLAGS=-DANSI
endif

if HAVE_NAMESPACES
  NS_FLAGS=-DANSI_NAMESPACES
endif

FEATURE_FLAGS=$(ANSI_FLAGS) $(NS_FLAGS)

if BUILD_SHARED
  EXEC_DIR=.libs/
endif

AM_CPPFLAGS=-DNDEBUG -DBOSSA_RUSAGE \
            -I$(INCDIR) -I$(INCDIR)/sp -I$(ACRO)/include \
             $(ARCH_FLAGS) $(FEATURE_FLAGS) -DMPICH_IGNORE_CXX_SEEK

TEVALIBS=$(TSO2IMPACTDIR)/libsp.la $(UTILIBDIR)/libutilib.la -L$(ACRO)/lib -lm

bin_PROGRAMS=new_imperfect new_randomsample

if BUILD_NEW_IMPERFECT
  new_imperfect_SOURCES= new_imperfect.cpp $(BASIC)
  new_imperfect_LDADD= $(TEVALIBS)
  CLEAN_NEW_IMPERFECT=rm -f $(BINDIR)/new_imperfect$(EXEEXT)
endif

if BUILD_NEW_RANDOMSAMPLE
  new_randomsample_SOURCES= new_randomsample.cpp $(BASIC)
  new_randomsample_LDADD= $(TEVALIBS)
  CLEAN_NEW_RANDOMSAMPLE=rm -f $(BINDIR)/new_randomsample$(EXEEXT)
endif

if !BUILD_SHARED
#
# A flag for libtool
#
AM_LDFLAGS=-all-static
endif


BASIC=\
    ImpactCache.cpp\
    ProblemInstance.cpp\
    SideConstraintInstance.cpp\
    ImperfectInstance.cpp\
    ProblemSolver.cpp\
    solution.cpp\
    InitialSolution.cpp\
    MeanSolution.cpp\
    WorstSolution.cpp\
    VarSolution.cpp\
    TceSolution.cpp\
    PreProcess.cpp\
    MeanSolution.h\
    redefs.h\
    PreProcess.h\
    WorstSolution.h\
    VarSolution.h\
    TceSolution.h\
    solution.h\
    InitialSolution.h\
    ImpactCache.h\
    ProblemSolver.h\
    SideConstraintInstance.h\
    ImperfectInstance.h\
    ProblemInstance.h

all-local:
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@$(COPY_UPDATE) $(bin_PROGRAMS) $(BINDIR)


clean-local:
	@$(CLEAN_NEW_RANDOMSAMPLE)
	@$(CLEAN_NEW_IMPERFECT)

endif
