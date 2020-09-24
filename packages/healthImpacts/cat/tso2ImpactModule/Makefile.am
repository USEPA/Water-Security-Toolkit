INCDIR=@top_builddir@/include
HISRC=@top_srcdir@/packages/healthImpacts
LIBDIR=@top_builddir@/lib

ACRO=@top_builddir@/tpl/@acro@
UTILIBDIR=$(ACRO)/packages/utilib/src
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

if BUILD_CYGWIN
  CYGWIN_FLAGS=-D__CYGWIN__ -DCYGWIN
  SPECIAL_CFLAGS=-D_WINDOWS
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

if BUILD_SHARED
if BUILD_CYGWIN
  DEPENDENCY_FLAG=-no-undefined -avoid-version -Wl,--add-stdcall-alias
else
  DEPENDENCY_FLAG=-avoid-version
endif
endif

ARCH_FLAGS=$(SPECIAL_FLAGS) $(CYGWIN_FLAGS) $(LINUX_FLAGS) $(APPLE_FLAGS) $(MINGW_FLAGS)

if HAVE_ANSI_C_HEADERS
  ANSI_FLAGS=-DANSI
endif

if HAVE_NAMESPACES
  NS_FLAGS=-DANSI_NAMESPACES
endif

FEATURE_FLAGS=$(ANSI_FLAGS) $(NS_FLAGS)

AM_CPPFLAGS=$(ARCH_FLAGS) $(FEATURE_FLAGS)

if NO_STRICMP
if NO_STRCASECMP
HIA_FLAGS="-Dstricmp=_stricmp"
else
HIA_FLAGS="-Dstricmp=strcasecmp"
endif
endif

if HAVE_LIBANLUTILS
if BUILD_SHARED

  ANLLIBDIR=@top_srcdir@/@ANL_LIBDIR@
  ANLLIBFLAG=-L$(ANLLIBDIR)
  ANLLIB=-lANLUtils

endif
endif

#########################################################
# LIBRARIES
#########################################################
#
if BUILD_SHARED

libtso2ImpactModule_la_CPPFLAGS=-I$(INCDIR) -I$(INCDIR)/sp \
   -I$(INCDIR)/tevalibs -I$(ACRO)/include \
   -I@top_srcdir@/packages/tevaUtils/anl/include \
   -I@top_srcdir@/packages/tevaUtils/anl/ANLUtils/include \
   -I$(HISRC)/hia/TEVAHIAModule/include \
   -I$(HISRC)/hia/include \
   -I$(HISRC)/threatAssess \
   -I$(HISRC)/cat/include \
    $(JDK_INC) $(AM_CPPFLAGS)

libtso2ImpactModule_la_SOURCES= \
  tso2ImpactAnalysisIA.cpp \
  tso2ImpactAnalysisAggr.cpp \
  realpath_subst.c \
  tso2ImpactAnalysisJNI.cpp \
  tso2ImpactAnalysisUtil.cpp \
  ModuleExtentOfContaminationObjective.h \
  ModuleMassConsumedObjective.h \
  ModuleNumberFailedDetectionsObjective.h \
  ModulePopulationExposedObjective.h \
  ModulePopulationKilledObjective.h\
  ModuleTimeToDetectionObjective.h\
  ModuleVolumeContaminatedWaterConsumedObjective.h \
  tso2ImpactAnalysis.h

libtso2ImpactModule_la_LIBADD= \
             ../../libHIAModule.la \
             ../../../cat/tso2Impact/libsp.la \
             ../../../tevaUtils/libAnalysisModule.la \
             ../../libthreatassess.la  \
             $(ANLLIB) \
             $(UTILIBDIR)/libutilib.la 

libtso2ImpactModule_la_LDFLAGS=$(DEPENDENCY_FLAG) $(ANLLIBFLAG)

lib_LTLIBRARIES=libtso2ImpactModule.la

endif

LIB_ORIG=$(lib_LTLIBRARIES:%.la=.libs/%.*) $(MINGW_LIBS)
LIB_COPIES=$(lib_LTLIBRARIES:%.la=$(LIBDIR)/.libs/%.*)
LA_COPIES=$(lib_LTLIBRARIES:%=$(LIBDIR)/%)

#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#
if BUILD_SHARED
all-local:
	if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	$(COPY_UPDATE) $(lib_LTLIBRARIES) $(LIBDIR)
	if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	$(COPY_UPDATE) $(LIB_ORIG) $(LIBDIR)/.libs

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
endif


