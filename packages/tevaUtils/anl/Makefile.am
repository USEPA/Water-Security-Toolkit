SRCDIR=@top_srcdir@/packages/tevaModule
INCDIR=@top_builddir@/include
LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
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

if HAVE_LIBANLUTILS
if BUILD_SHARED

  ANLLIBDIR=@top_srcdir@/@ANL_LIBDIR@
  ANLLIBFLAG=-L$(ANLLIBDIR)
  ANLLIB=-lANLUtils

endif
endif

ARCH_FLAGS=$(SPECIAL_CFLAGS) $(CYGWIN_FLAGS) $(LINUX_FLAGS) $(APPLE_FLAGS) $(MINGW_FLAGS)

if HAVE_ANSI_C_HEADERS
  ANSI_FLAGS=-DANSI
endif

if HAVE_NAMESPACES
  NS_FLAGS=-DANSI_NAMESPACES
endif

FEATURE_FLAGS=$(ANSI_FLAGS) $(NS_FLAGS)

AM_CFLAGS=$(ARCH_FLAGS) $(FEATURE_FLAGS) -I$(INCDIR)/tevalibs $(JDK_INC) $(CFLAGS)

if NO_STRICMP
if NO_STRCASECMP
HIA_FLAGS="-Dstricmp=_stricmp"
else
HIA_FLAGS="-Dstricmp=strcasecmp"
endif
endif

LIB_ORIG=$(lib_LTLIBRARIES:%.la=.libs/%.*) $(MINGW_LIBS)
LIB_COPIES=$(lib_LTLIBRARIES:%.la=$(LIBDIR)/.libs/%.*)
LA_COPIES=$(lib_LTLIBRARIES:%=$(LIBDIR)/%)

#########################################################
#########################################################

libAnalysisModule_la_SOURCES =\
  src/namedData.c\
  src/AggregationServerJNI.c  \
  src/IntAnalysisServerJNI.c \
  src/AnalysisModuleUtils.c \
  src/DiskCachedData.c

libAnalysisModule_la_CFLAGS=$(AM_CFLAGS) -I$(SRCDIR)/include -I$(SRCDIR)/src -I$(SRCDIR)/ANLUtils/include
libAnalysisModule_la_LDFLAGS=$(DEPENDENCY_FLAG) $(ANLLIBFLAG)
libAnalysisModule_la_LIBADD=$(ANLLIB)

lib_LTLIBRARIES= libAnalysisModule.la 

# Install headers where all builds can find them

SRC_H= IntAnalysisServer.h NamedDataJNI.h AggregationServer.h

SRC_H_SRC=$(SRC_H:%.h=$(SRCDIR)/src/%.h)
SRC_H_INS=$(SRC_H:%.h=$(INCDIR)/tevalibs/%.h)

ANLUTILS_H= loggingUtils.h jniUtils.h

ANLUTILS_H_SRC=$(ANLUTILS_H:%.h=$(SRCDIR)/ANLUtils/include/%.h)
ANLUTILS_H_INS=$(ANLUTILS_H:%.h=$(INCDIR)/tevalibs/%.h)

TEVAMODULE_H= NamedData.h ExternalAnalysis.h

TEVAMODULE_H_SRC=$(TEVAMODULE_H:%.h=$(SRCDIR)/include/%.h)
TEVAMODULE_H_INS=$(TEVAMODULE_H:%.h=$(INCDIR)/tevalibs/%.h)

JNI_H=$(SRCDIR)/include/jni.h

ALL_H_SRC= $(SRC_H_SRC) $(ANLUTILS_H_SRC) $(TEVAMODULE_H_SRC) 
ALL_H_INS= $(SRC_H_INS) $(ANLUTILS_H_INS) $(TEVAMODULE_H_INS)

#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#
all-local:
	if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	$(COPY_UPDATE) $(lib_LTLIBRARIES) $(LIBDIR)
	if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	$(COPY_UPDATE) $(LIB_ORIG) $(LIBDIR)/.libs
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/tevalibs ; then : ; else mkdir $(INCDIR)/tevalibs ; fi
	@$(COPY_UPDATE) $(ALL_H_SRC) $(INCDIR)/tevalibs
	@if test -f $(JNI_H) ; then $(COPY_UPDATE) $(JNI_H) $(INCDIR)/tevalibs; fi

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES)
	@rm -rf $(ALL_H_INS)
