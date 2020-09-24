BINDIR=@top_builddir@/bin
LIBDIR=@top_builddir@/lib
INCDIR=@top_builddir@/include
EPANETDIR=@top_builddir@/tpl/epanet
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
libthreatassess_la_SOURCES =\
threatAssess/hia.c\
threatAssess/nrutil.c\
threatAssess/sss.c\
threatAssess/tai.c\
threatAssess/ta_input.c\
threatAssess/ta_utils.c\
threatAssess/dose.c\
threatAssess/ingest.c\
threatAssess/response.c\
threatAssess/runif_if.c\
threatAssess/spline_if.c\
threatAssess/splines.c\
threatAssess/mtwist-0.6/mtwist.c\
dcdfLib/dcdflib.c

libthreatassess_la_CPPFLAGS=-I@top_srcdir@/include/tevalibs -I@srcdir@/dcdfLib \
                            -I@srcdir@/threatAssess/mtwist-0.6 $(AM_CPPFLAGS)
libthreatassess_la_CFLAGS=-DDPMPAR_OPT
libthreatassess_la_LIBADD=../tevaUtils/libtsoutil.la
libthreatassess_la_LDFLAGS=$(DEPENDENCY_FLAG)

if BUILD_SHARED

libHIAModule_la_SOURCES =\
 hia/src/HealthImpactsAggr.c  \
 hia/src/HealthImpactsIA.c  \
 hia/src/HealthImpactsJNI.c \
 hia/src/HealthImpactsUtil.c \
 hia/include/HealthImpacts.h

libHIAModule_la_CFLAGS=$(SPECIAL_CFLAGS) $(HIA_FLAGS) -D_FILE_OFFSET_BITS=64

libHIAModule_la_CPPFLAGS=-I$(INCDIR) -I@srcdir@/hia/include \
  -I@top_srcdir@/include/tevalibs \
  -I@srcdir@/threatAssess \
  -I@srcdir@/threatAssess/mtwist-0.6 \
  $(JDK_INC) $(AM_CPPFLAGS)

libHIAModule_la_LIBADD=../tevaUtils/libAnalysisModule.la \
                       libthreatassess.la  \
                       $(ANLLIB)

libHIAModule_la_LDFLAGS= $(DEPENDENCY_FLAG) $(ANLLIBFLAG)

lib_LTLIBRARIES=libthreatassess.la libHIAModule.la 

else

lib_LTLIBRARIES=libthreatassess.la

endif

LIB_ORIG=$(lib_LTLIBRARIES:%.la=.libs/%.*) $(MINGW_LIBS)
LIB_COPIES=$(lib_LTLIBRARIES:%.la=$(LIBDIR)/.libs/%.*)
LA_COPIES=$(lib_LTLIBRARIES:%=$(LIBDIR)/%)

#########################################################
# EXECUTABLES
#########################################################
#
if BUILD_THREAT_ASSESS

  THREAT_ASSESS=threat_assess
  threat_assess_SOURCES=threatAssess/teva-assess.c
  threat_assess_CPPFLAGS=-I@top_srcdir@/include/tevalibs \
                         -I@srcdir@/dcdfLib \
                         -I@srcdir@/threatAssess/mtwist-0.6 $(AM_CPPFLAGS)
# if we are building shared, the msx shared lib also needs to be here
# so libenl won't complain about undefined references
if BUILD_SHARED
  ADDMSX=$(EPANETDIR)/libepanetmsx.la -ldl
endif
  threat_assess_LDADD=libthreatassess.la \
          ../tevaUtils/libtsoutil.la ../tevaUtils/libenl.la \
          ../tevaUtils/liberd.la @top_srcdir@/tpl/lzma/liblzma.la $(EPANETDIR)/libepanet.la $(ADDMSX) -lm
  COPY_THREAT_ASSESS=$(COPY_UPDATE) threat_assess$(EXEEXT) $(BINDIR)
  CLEAN_THREAT_ASSESS=rm -f $(BINDIR)/threat_assess$(EXEEXT)
endif


EXTRA_DIST=threatAssess hia dcdfLib

noinst_PROGRAMS=$(THREAT_ASSESS)

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
	if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@$(COPY_THREAT_ASSESS)

clean-local:
	@$(CLEAN_THREAT_ASSESS)
	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
	

