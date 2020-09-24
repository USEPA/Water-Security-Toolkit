SRCDIR=@top_srcdir@/packages/tevaUtils
INCDIR=@top_builddir@/include
LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
LZMADIR=@top_builddir@/tpl/lzma
EPANETDIR=@top_builddir@/tpl/epanet
LZMAINC=$(LZMADIR)/C
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

AM_CPPFLAGS=$(ARCH_FLAGS) $(FEATURE_FLAGS) -I$(INCDIR)/tevalibs $(JDK_INC)

#########################################################
# LIBRARIES
#########################################################
#
libenl_la_SOURCES =\
erd/enl/src/enl.c erd/enl/include/enl.h

libenl_la_LIBADD=$(EPANETDIR)/libepanet.la
libenl_la_LDFLAGS=$(DEPENDENCY_FLAG)
libenl_la_CFLAGS=-Ierd/enl/include -Ierd/erd/include -I$(INCDIR)/tevalibs

libtsoutil_la_SOURCES = tso/src/tso.c tso/src/tevautil.h
libtsoutil_la_LDFLAGS=$(DEPENDENCY_FLAG) 
libtsoutil_la_CFLAGS=-D_FILE_OFFSET_BITS=64 -I$(SRCDIR)/erd/erd/include -I$(SRCDIR)/erd/enl/include

liberd_la_SOURCES = erd/erd/src/dpx.c \
				    erd/erd/src/erd.c \
				    erd/erd/src/erdinternal.c \
				    erd/erd/src/lzma_dec.c \
				    erd/erd/src/lzma_enc.c \
				    erd/erd/src/rle_dec.c \
				    erd/erd/src/rle_enc.c \
				    erd/erd/src/teva.c
liberd_la_LDFLAGS=$(DEPENDENCY_FLAG) 
liberd_la_CFLAGS=-D_FILE_OFFSET_BITS=64 -I$(SRCDIR)/erd/erd/include -I$(LZMAINC)

if HAVE_LIBANLUTILS
if BUILD_SHARED
libAnalysisModule_la_SOURCES =\
  anl/src/namedData.c\
  anl/src/AggregationServerJNI.c  \
  anl/src/IntAnalysisServerJNI.c \
  anl/src/AggregationServer.h \
  anl/src/IntAnalysisServer.h \
  anl/src/NamedDataJNI.h

libAnalysisModule_la_CFLAGS=$(AM_CFLAGS) \
         -I$(SRCDIR)/anl/include -I$(SRCDIR)/anl/src -I$(SRCDIR)/anl/ANLUtils/include

libAnalysisModule_la_LDFLAGS=$(DEPENDENCY_FLAG) $(ANLLIBFLAG)
libAnalysisModule_la_LIBADD=$(ANLLIB)

ANALYSISMODULE=libAnalysisModule.la 

libTSO_la_SOURCES =\
  tso_jni/tso_jni.c \
  tso_jni/anl_teva_db_TSOReader.h \
  tso_jni/tsodata.h

libTSO_la_CFLAGS=-I$(SRCDIR)/tso/src -I$(SRCDIR)/anl/ANLUtils/include -I$(SRCDIR)/erd/erd/include -I$(SRCDIR)/erd/enl/include -I$(SRCDIR)/erd_jni

libTSO_la_LDFLAGS=$(DEPENDENCY_FLAG) $(ANLLIBFLAG) -ltsoutil
libTSO_la_LIBADD=$(ANLLIB)
TSO_JNI=libTSO.la

endif
endif

lib_LTLIBRARIES= libtsoutil.la $(ANALYSISMODULE) libenl.la $(TSO_JNI) liberd.la

if HOST_MINGW
if BUILD_SHARED
  MINGW_LIBS=.libs/*.dll
endif
endif

LIB_ORIG=$(lib_LTLIBRARIES:%.la=.libs/%.*) $(MINGW_LIBS)
LIB_COPIES=$(lib_LTLIBRARIES:%.la=$(LIBDIR)/.libs/%.*)
LA_COPIES=$(lib_LTLIBRARIES:%=$(LIBDIR)/%)

#########################################################
# HEADERS
#########################################################
#
# Path to header files in source, and to where we'll copy
# them for subsequent build of executables

TEVAUTIL_H= tevatypes.h tevautil.h tso.h

TEVAUTIL_H_SRC=$(TEVAUTIL_H:%.h=$(SRCDIR)/tso/src/%.h)
TEVAUTIL_H_INS=$(TEVAUTIL_H:%.h=$(INCDIR)/tevalibs/%.h)


ENL_H= enl.h 

ENL_H_SRC=$(ENL_H:%.h=$(SRCDIR)/erd/enl/include/%.h)
ENL_H_INS=$(ENL_H:%.h=$(INCDIR)/tevalibs/%.h)

ERD_H= erd.h 

ERD_H_SRC=$(ERD_H:%.h=$(SRCDIR)/erd/erd/include/%.h)
ERD_H_INS=$(ERD_H:%.h=$(INCDIR)/tevalibs/%.h)

ANL_H= IntAnalysisServer.h NamedDataJNI.h AggregationServer.h

ANL_H_SRC=$(ANL_H:%.h=$(SRCDIR)/anl/src/%.h)
ANL_H_INS=$(ANL_H:%.h=$(INCDIR)/tevalibs/%.h)

ANLUTILS_H= loggingUtils.h jniUtils.h

ANLUTILS_H_SRC=$(ANLUTILS_H:%.h=$(SRCDIR)/anl/ANLUtils/include/%.h)
ANLUTILS_H_INS=$(ANLUTILS_H:%.h=$(INCDIR)/tevalibs/%.h)

TEVAMODULE_H= NamedData.h ExternalAnalysis.h

TEVAMODULE_H_SRC=$(TEVAMODULE_H:%.h=$(SRCDIR)/anl/include/%.h)
TEVAMODULE_H_INS=$(TEVAMODULE_H:%.h=$(INCDIR)/tevalibs/%.h)

JNI_H=$(SRCDIR)/anl/include/jni.h

ALL_H_SRC= $(TEVAUTIL_H_SRC) $(ENL_H_SRC) $(ERD_H_SRC) $(ANL_H_SRC) $(ANLUTILS_H_SRC) $(TEVAMODULE_H_SRC) 
ALL_H_INS= $(TEVAUTIL_H_INS) $(ENL_H_INS) $(ERD_H_INS) $(ANL_H_INS) $(ANLUTILS_H_INS) $(TEVAMODULE_H_INS)

#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES 

if BUILD_ERD2TEXT
  ERD2TEXT=erd2text
  erd2text_SOURCES=tso/src/tso2text.c
  erd2text_CPPFLAGS=-Ierd/erd/src -Ierd/erd/include
  erd2text_LDADD=libtsoutil.la liberd.la $(LZMADIR)/liblzma.la -lm
  COPY_ERD2TEXT=$(COPY_UPDATE) erd2text$(EXEEXT) $(BINDIR)
  CLEAN_ERD2TEXT=rm -f $(BINDIR)/erd2text$(EXEEXT)
endif

if BUILD_ERDDIFF
  ERDDIFF=erddiff
  erddiff_SOURCES=tso/src/tsodiff.c
  erddiff_CPPFLAGS=-Ierd/erd/src -Ierd/erd/include
  erddiff_LDADD=libtsoutil.la liberd.la $(LZMADIR)/liblzma.la -lm
  COPY_ERDDIFF=$(COPY_UPDATE) erddiff$(EXEEXT) $(BINDIR)
  CLEAN_ERDDIFF=rm -f $(BINDIR)/erddiff$(EXEEXT)
endif

if BUILD_ERDINFO
  ERDINFO=erdinfo
  erdinfo_SOURCES=tso/src/tso_info.c
  erdinfo_CPPFLAGS=-Ierd/erd/src -Ierd/erd/include
  erdinfo_LDADD=libtsoutil.la liberd.la $(LZMADIR)/liblzma.la -lm
  COPY_ERDINFO=$(COPY_UPDATE) erdinfo$(EXEEXT) $(BINDIR)
  CLEAN_ERDINFO=rm -f $(BINDIR)/erdinfo$(EXEEXT)
endif

#
# We use "make install" to create release directories.  Divide
# programs into those we want in the release directory (bin_) and 
# those we do not (noinst_).
#

if !BUILD_SHARED
#
# A flag for libtool
#
AM_LDFLAGS=-all-static
endif

bin_PROGRAMS=$(ERD2TEXT) $(ERDDIFF) $(ERDINFO)

endif

#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#
all-local:
if BUILD_EXECUTABLES
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@cp $(bin_PROGRAMS) $(BINDIR)
endif
	@if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	@$(COPY_UPDATE) $(lib_LTLIBRARIES) $(LIBDIR)
	@if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	@$(COPY_UPDATE) $(LIB_ORIG) $(LIBDIR)/.libs
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/tevalibs ; then : ; else mkdir $(INCDIR)/tevalibs ; fi
	@$(COPY_UPDATE) $(ALL_H_SRC) $(INCDIR)/tevalibs
	@if test -f $(JNI_H) ; then $(COPY_UPDATE) $(JNI_H) $(INCDIR)/tevalibs; fi

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES)
	@rm -f $(ALL_H_INS)

EXTRA_DIST=anl/include anl/ANLUtils
