include ${top_srcdir}/config/MakeMacros.mk
SUFFIXES=.i

LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
INCDIR=@top_builddir@/include
LIBBUILD=@top_builddir@/packages
SRCPACKAGES=@top_srcdir@/packages

EPANETDIR=@top_builddir@/tpl/epanet
ACRO=@top_builddir@/tpl/@acro@
UTILIBDIR=$(ACRO)/packages/utilib/src

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

if BUILD_SHARED
  EXEC_DIR=.libs/
endif

if HAVE_ANSI_C_HEADERS
  ANSI_FLAGS=-DANSI
endif

if HAVE_NAMESPACES
  NS_FLAGS=-DANSI_NAMESPACES
endif

if BUILD_SHARED
if BUILD_CYGWIN
  DEPENDENCY_FLAG=-no-undefined -avoid-version -Wl,--add-stdcall-alias
else
  DEPENDENCY_FLAG=-avoid-version
endif
endif

FEATURE_FLAGS=$(ANSI_FLAGS) $(NS_FLAGS) -DMPICH_IGNORE_CXX_SEEK

if HAVE_REALPATH
  REALPATH=
else
  REALPATH=../tso2ImpactModule/realpath_subst.c
endif

#########################################################
# Access to restricted codes
#
# Ensure we get restricted headers if available, or
# empty stubs if they are not available.
#########################################################

OUR_INCLUDES=-I$(INCDIR) -I$(INCDIR)/tevalibs -I$(SRCPACKAGES)/detect -I$(ACRO)/include 

ALL_INC_DIRS=-I@top_srcdir@/packages/healthImpacts/cat/include \
             -I@top_srcdir@/packages/healthImpacts/dcdfLib \
             -I@top_srcdir@/packages/healthImpacts/threatAssess $(OUR_INCLUDES)

THREATASSESS=@top_builddir@/packages/healthImpacts/libthreatassess.la

AM_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
#########################################################
# LIBRARIES
#########################################################

DETECT=$(LIBBUILD)/detect/libdetect.la
UTILIB=$(UTILIBDIR)/libutilib.la 

lib_LTLIBRARIES=libsp.la

libsp_la_SOURCES = $(SO_INTERFACE) \
  impacts.cpp \
  scenario.cpp \
  SPProblem.cpp \
  tso2ImpactAPI.cpp  \
  EventImpacts.cpp \
  EventImpacts.h \
  ExtentOfContaminationObjective.h\
  TimeExtentOfContaminationObjective.h\
  Impact.h\
  impacts.h\
  MassConsumedObjective.h\
  NumberFailedDetectionsObjective.h\
  ObjectiveBase.h\
  scenario.h\
  SPProblem.h\
  TimeToDetectionObjective.h\
  VecTrie.h\
  vlink.h\
  VolumeContaminatedWaterConsumedObjective.h

libsp_la_LIBADD = $(UTILIB) $(DETECT)

libsp_la_LDFLAGS =$(DEPENDENCY_FLAG) \
                   $(ARCH_FLAGS) $(FEATURE_FLAGS) 

libsp_la_CXXFLAGS=-fexceptions

libsp_la_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES
# if we are building shared, the msx shared lib also needs to be here
# so libenl won't complain about undefined references
if BUILD_SHARED
  ADDMSX=$(EPANETDIR)/libepanetmsx.la -ldl
endif

TEVAUTIL=$(LIBBUILD)/tevaUtils/libenl.la $(LIBBUILD)/tevaUtils/liberd.la @top_srcdir@/tpl/lzma/liblzma.la $(ADDMSX)
EPANET=$(EPANETDIR)/libepanet.la

TEVALIBS=$(THREATASSESS) $(TEVAUTIL) $(EPANET)

if BUILD_TSO2IMPACT
  TSO2IMPACT=tso2Impact
  tso2Impact_SOURCES=tso2Impact.cpp
  tso2Impact_LDADD=libsp.la $(DETECT) $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  tso2Impact_LDFLAGS=-L$(LIBDIR) 
  tso2Impact_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)  -fexceptions
  tso2Impact_CXXFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS) -fexceptions

tso2Impact.i:
	$(CXXCOMPILE) $(tso2Impact_CXXFLAGS) -E tso2Impact.cpp > $*.i
endif

if BUILD_EVALSENSOR
  EVALSENSOR=evalsensor
  evalsensor_SOURCES=evalsensor.cpp
  evalsensor_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  evalsensor_LDFLAGS=-L$(LIBDIR)
  evalsensor_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  evalsensor_CXXFLAGS=-fexceptions
endif

if BUILD_SETUPIPDATA
  SETUPIPDATA=createIPData
  createIPData_SOURCES=createIPData.cpp
  createIPData_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  createIPData_LDFLAGS=-L$(LIBDIR)
  createIPData_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  createIPData_CXXFLAGS=-fexceptions

  SETUPLAGDATA=createLagData
  createLagData_SOURCES=createLagData.cpp
  createLagData_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  createLagData_LDFLAGS=-L$(LIBDIR)
  createLagData_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  createLagData_CXXFLAGS=-fexceptions

  AGGREGATE_IMPACT=aggregateImpacts
  aggregateImpacts_SOURCES=aggregateImpacts.cpp
  aggregateImpacts_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  aggregateImpacts_LDFLAGS=-L$(LIBDIR)
  aggregateImpacts_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  aggregateImpacts_CXXFLAGS=-fexceptions

  PREPROCESS_IMPACT=preprocessImpacts
  preprocessImpacts_SOURCES=preprocessImpacts.cpp
  preprocessImpacts_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  preprocessImpacts_LDFLAGS=-L$(LIBDIR)
  preprocessImpacts_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  preprocessImpacts_CXXFLAGS=-fexceptions
endif

if BUILD_SCENARIOAGGR
  SCENARIOAGGR=scenarioAggr
  scenarioAggr_SOURCES=scenarioAggr.cpp
  scenarioAggr_LDADD=libsp.la $(TEVALIBS) $(UTILIB) -L$(ACRO)/lib -lm
  scenarioAggr_LDFLAGS=-L$(LIBDIR)
  scenarioAggr_CPPFLAGS=$(ALL_INC_DIRS) $(ARCH_FLAGS) $(FEATURE_FLAGS)
  scenarioAggr_CXXFLAGS=-fexceptions
endif

EXECS=$(TSO2IMPACT) $(EVALSENSOR) $(SETUPIPDATA) $(SETUPLAGDATA) $(AGGREGATE_IMPACT) $(PREPROCESS_IMPACT) $(SCENARIOAGGR)

bin_PROGRAMS = $(EXECS)

if !BUILD_SHARED
#
# A flag for libtool
#
AM_LDFLAGS=-all-static
endif

endif

all-local:
if BUILD_EXECUTABLES
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@cp $(bin_PROGRAMS) $(BINDIR)
endif
	if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	cp .libs/libsp.* $(LIBDIR)/.libs

clean-local:
if BUILD_EXECUTABLES
	@rm -f $(BINDIR)/tso2Impact $(BINDIR)/createIPData
	@rm -f  $(BINDIR)/scenarioAggr $(BINDIR)/evalsensor
	@rm -f $(BINDIR)/aggregateImpacts $(BINDIR)/preprocessImpacts
endif
	rm -f $(LIBDIR)/.libs/libsp.*
