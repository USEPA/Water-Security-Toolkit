SRCDIR=@top_srcdir@/packages/sim
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

ARCH_FLAGS=$(CYGWIN_FLAGS) $(LINUX_FLAGS) $(APPLE_FLAGS) $(MINGW_FLAGS)

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

#########################################################
# HEADERS
#########################################################
#
# Path to header files in source, and to where we'll copy
# them for subsequent build of executables

#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES 

if BUILD_BOOSTERSIM
  BOOSTERSIM = boostersim
  boostersim_SOURCES = \
		       booster/src/booster/BoosterNetworkSimulator.cpp \
		       booster/src/booster/BoosterDataWriter.cpp \
		       booster/src/booster/BoosterDataWriterAMPL.cpp \
		       booster/src/booster/BoosterDataWriterPYOMO.cpp \
		       booster/src/booster/BoosterDataWriterPYSP.cpp \
		       booster/src/booster/BoosterSimOptions.cpp \
		       booster/src/booster/BoosterOptions.cpp \
		       booster/src/booster/BoosterSim.cpp
  boostersim_CPPFLAGS = -I$(INCDIR)/tevalibs \
			-Ibooster/src \
			-I${top_srcdir}/tpl/acro/tpl/boost \
			-I${top_srcdir}/tpl/acro/packages/utilib/src
  boostersim_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_BOOSTERSIM = $(COPY_UPDATE) $(BOOSTERSIM)$(EXEEXT) $(BINDIR)
  CLEAN_BOOSTERSIM = rm -f $(BINDIR)/$(BOOSTERSIM)$(EXEEXT)
endif

if BUILD_BOOSTERQUALITY
  BOOSTERQUALITY = boosterquality
  boosterquality_SOURCES = \
		       booster/src/booster/BoosterNetworkSimulator.cpp \
		       booster/src/booster/BoosterDataWriter.cpp \
		       booster/src/booster/BoosterDataWriterAMPL.cpp \
		       booster/src/booster/BoosterDataWriterPYOMO.cpp \
		       booster/src/booster/BoosterQualityOptions.cpp \
		       booster/src/booster/BoosterOptions.cpp \
		       booster/src/booster/BoosterQuality.cpp
  boosterquality_CPPFLAGS = -I$(INCDIR)/tevalibs \
			-Ibooster/src \
			-I${top_srcdir}/tpl/acro/tpl/boost \
			-I${top_srcdir}/tpl/acro/packages/utilib/src
  boosterquality_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_BOOSTERQUALITY = $(COPY_UPDATE) $(BOOSTERQUALITY)$(EXEEXT) $(BINDIR)
  CLEAN_BOOSTERQUALITY = rm -f $(BINDIR)/$(BOOSTERQUALITY)$(EXEEXT)
endif

if BUILD_BOOSTERIMPACT
  BOOSTERIMPACT = boosterimpact
  boosterimpact_SOURCES = \
		       booster/src/booster/BoosterNetworkSimulator.cpp \
		       booster/src/booster/BoosterImpactOptions.cpp \
		       booster/src/booster/BoosterOptions.cpp \
		       booster/src/booster/BoosterImpact.cpp
  boosterimpact_CPPFLAGS = -I$(INCDIR)/tevalibs \
			-Ibooster/src \
			-I${top_srcdir}/tpl/acro/tpl/boost \
			-I${top_srcdir}/tpl/acro/packages/utilib/src
  boosterimpact_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_BOOSTERIMPACT = $(COPY_UPDATE) $(BOOSTERIMPACT)$(EXEEXT) $(BINDIR)
  CLEAN_BOOSTERIMPACT = rm -f $(BINDIR)/$(BOOSTERIMPACT)$(EXEEXT)
endif

if BUILD_ERDVAL
  ERDVAL = erdval
  erdval_SOURCES = erdvalidate/src/erdvalidate/erdvalidate.cpp
  erdval_CPPFLAGS = -I$(INCDIR)/tevalibs
  erdval_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la $(LIBDIR)/liberd.la \
	$(LIBDIR)/libtsoutil.la $(LIBDIR)/liblzma.la $(BLAS_LIBS) $(FLIBS)
  COPY_ERDVAL = $(COPY_UPDATE) $(ERDVAL)$(EXEEXT) $(BINDIR)
  CLEAN_ERDVAL = rm -f $(BINDIR)/$(ERDVAL)$(EXEEXT)
endif

if BUILD_INVERSIONSIM
  INVERSIONSIM = inversionsim
  inversionsim_SOURCES = \
		       source_inversion/src/source_inversion/InversionDataWriter.cpp \
		       source_inversion/src/source_inversion/InversionSimOptions.cpp \
		       source_inversion/src/source_inversion/InversionNetworkSimulator.cpp \
		       source_inversion/src/source_inversion/InversionSim.cpp
  inversionsim_CPPFLAGS = -I$(INCDIR)/tevalibs \
			  -Isource_inversion/src \
			  -I${top_srcdir}/tpl/acro/tpl/boost \
			  -I${top_srcdir}/tpl/acro/packages/utilib/src
  inversionsim_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
        $(LIBDIR)/libenl.la $(LIBDIR)/liberd.la $(LIBDIR)/libepanet.la $(LIBDIR)/libepanetmsx.la -ldl \
        $(LIBDIR)/liblzma.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_INVERSIONSIM = $(COPY_UPDATE) $(INVERSIONSIM)$(EXEEXT) $(BINDIR)
  CLEAN_INVERSIONSIM = rm -f $(BINDIR)/$(INVERSIONSIM)$(EXEEXT)
endif

if BUILD_MEASUREGEN
  MEASUREGEN = measuregen
  measuregen_SOURCES = \
		       measure_gen/src/measure_gen/MeasureDataWriter.cpp \
		       measure_gen/src/measure_gen/MeasureSimOptions.cpp \
		       measure_gen/src/measure_gen/MeasureNetworkSimulator.cpp \
		       measure_gen/src/measure_gen/MeasureSim.cpp
  measuregen_CPPFLAGS = -I$(INCDIR)/tevalibs -Imeasure_gen/src -I${top_srcdir}/tpl/acro/tpl/boost \
			-I${top_srcdir}/tpl/acro/packages/utilib/src
  measuregen_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_MEASUREGEN = $(COPY_UPDATE) $(MEASUREGEN)$(EXEEXT) $(BINDIR)
  CLEAN_MEASUREGEN = rm -f $(BINDIR)/$(MEASUREGEN)$(EXEEXT)
endif

if BUILD_SAMPLELOCATION
  SAMPLELOCATION = samplelocation
  samplelocation_SOURCES = \
			   grab_sample_location/src/grab_sample_location/samplelocation.cpp \
		           grab_sample_location/src/grab_sample_location/SampleLocationOptions.cpp \
			   grab_sample_location/src/grab_sample_location/SampleNetworkSimulator.cpp \
			   grab_sample_location/src/grab_sample_location/SampleLocationDataWriter.cpp
  samplelocation_CPPFLAGS = -I$(INCDIR)/tevalibs -Igrab_sample_location/src -I${top_srcdir}/tpl/acro/tpl/boost \
                            -I${top_srcdir}/tpl/acro/packages/utilib/src
  samplelocation_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
        $(LIBDIR)/libenl.la $(LIBDIR)/liberd.la $(LIBDIR)/libepanet.la $(LIBDIR)/libepanetmsx.la -ldl \
        $(LIBDIR)/liblzma.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_SAMPLELOCATION = $(COPY_UPDATE) $(SAMPLELOCATION)$(EXEEXT) $(BINDIR)
  CLEAN_SAMPLELOCATION = rm -f $(BINDIR)/$(SAMPLELOCATION)$(EXEEXT)
endif 

if BUILD_EVENTDETECTION
  EVENTDETECTION = eventDetection
  eventDetection_SOURCES = \
		       event_detection/src/event_detection/EventDetectionOptions.cpp \
		       event_detection/src/event_detection/EventDetection.cpp
  eventDetection_CPPFLAGS = -I$(INCDIR)/tevalibs \
			    -Ievent_detection/src \
			    -I${top_srcdir}/tpl/acro/tpl/boost \
			    -I${top_srcdir}/tpl/acro/packages/utilib/src
  eventDetection_LDADD = $(LIBDIR)/libmerlionUtils.la $(LIBDIR)/libmerlion.la \
	$(LIBDIR)/libepanet.la ${top_srcdir}/tpl/acro/lib/libutilib.la $(BLAS_LIBS) $(FLIBS)
  COPY_EVENTDETECTION = $(COPY_UPDATE) $(EVENTDETECTION)$(EXEEXT) $(BINDIR)
  CLEAN_EVENTDETECTION = rm -f $(BINDIR)/$(EVENTDETECTION)$(EXEEXT)
endif

if TRUE # BUILD_JSONWRITER - where should this be defined?
  JSONWRITER = jsonwriter
  jsonwriter_SOURCES = \
		       json_writer/src/json_writer/json_main.cpp
  jsonwriter_CPPFLAGS = -I$(INCDIR)/tevalibs -Ijson_writer/src
  jsonwriter_LDADD = $(LIBDIR)/libepanet.la
   COPY_JSONWRITER = $(COPY_UPDATE) $(JSONWRITER)$(EXEEXT) $(BINDIR)
   CLEAN_JSONWRITER = rm -f $(BINDIR)/$(JSONWRITER)$(EXEEXT)
endif

if TRUE # BUILD_JSONWRITER2 - where should this be defined?
  JSONWRITER2 = jsonwriter2
  jsonwriter2_SOURCES = \
		       json_writer2/src/json_writer2/json2_main.cpp
  jsonwriter2_CPPFLAGS = -I$(INCDIR)/tevalibs -Ijson_writer2/src
  jsonwriter2_LDADD = $(LIBDIR)/libtsoutil.la $(LIBDIR)/liberd.la $(LIBDIR)/liblzma.la
   COPY_JSONWRITER2 = $(COPY_UPDATE) $(JSONWRITER2)$(EXEEXT) $(BINDIR)
   CLEAN_JSONWRITER2 = rm -f $(BINDIR)/$(JSONWRITER2)$(EXEEXT)
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

noinst_PROGRAMS=$(BOOSTERSIM) $(BOOSTERQUALITY) $(BOOSTERIMPACT) $(ERDVAL) $(INVERSIONSIM) $(EVENTDETECTION) $(MEASUREGEN) $(SAMPLELOCATION) $(JSONWRITER) $(JSONWRITER2)

BIN_COPIES=$(noinst_PROGRAMS: %=$(BINDIR)/%)

endif


#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#

all-local:
	if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@$(COPY_BOOSTERSIM)
	@$(COPY_BOOSTERQUALITY)
	@$(COPY_BOOSTERIMPACT)
	@$(COPY_ERDVAL)
	@$(COPY_INVERSIONSIM)
	@$(COPY_EVENTDETECTION)
	@$(COPY_MEASUREGEN)
	@$(COPY_SAMPLELOCATION)
	@$(COPY_JSONWRITER)
	@$(COPY_JSONWRITER2)
clean-local:

	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
	@$(CLEAN_BOOSTERSIM)
	@$(CLEAN_BOOSTERQUALITY)
	@$(CLEAN_BOOSTERIMPACT)
	@$(CLEAN_ERDVAL)
	@$(CLEAN_INVERSIONSIM)
	@$(CLEAN_EVENTDETECTION)
	@$(CLEAN_MEASUREGEN)
	@$(CLEAN_SAMPLELOCATION)
	@$(CLEAN_JSONWRITER)
	@$(CLEAN_JSONWRITER2)
