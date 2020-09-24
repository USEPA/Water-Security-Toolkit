SRCDIR=@top_srcdir@/packages/sim
INCDIR=@top_builddir@/include
LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u
EPANETDIR=@top_builddir@/tpl/epanet

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

libmerlion_la_SOURCES = \
merlion/src/merlion/TriSolve.cpp \
merlion/src/merlion/BlasWrapper.cpp \
merlion/src/merlion/DirectedGraph.cpp \
merlion/src/merlion/SparseMatrix.cpp \
merlion/src/merlion/Network.cpp \
merlion/src/merlion/WaterQualityModel.cpp \
merlion/src/merlion/Merlion.cpp

libmerlion_la_LDFLAGS=$(DEPENDENCY_FLAG)
libmerlion_la_CPPFLAGS= -Imerlion/src

libmerlionUtils_la_SOURCES = \
merlion/merlionUtils/src/merlionUtils/EpanetLinker.cpp \
merlion/merlionUtils/src/merlionUtils/ModelWriter.cpp \
merlion/merlionUtils/src/merlionUtils/TSG_Reader.cpp \
merlion/merlionUtils/src/merlionUtils/SimTools.cpp \
merlion/merlionUtils/src/merlionUtils/Scenarios.cpp \
merlion/merlionUtils/src/merlionUtils/NetworkSimulator.cpp

libmerlionUtils_la_LDFLAGS=$(DEPENDENCY_FLAG) 
libmerlionUtils_la_CPPFLAGS= -Imerlion/src \
			     -Imerlion/merlionUtils/src \
			     -I$(EPANETDIR)/EPANET_/include \
			     -I$(EPANETDIR)/EPANET_/src \
			     -I${top_srcdir}/tpl/acro/tpl/boost
libmerlionUtils_la_LIBADD = libmerlion.la $(EPANETDIR)/libepanet.la


if BUILD_SHARED
  lib_LTLIBRARIES=libmerlion.la libmerlionUtils.la
else
  lib_LTLIBRARIES=libmerlion.la libmerlionUtils.la
endif

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

MERLION_H= \
Merlion.hpp \
MerlionDefines.hpp \
TriSolve.hpp \
BlasWrapper.hpp \
DirectedGraph.hpp \
SparseMatrix.hpp \
Network.hpp \
WaterQualityModel.hpp

MERLION_H_SRC=$(MERLION_H:%.hpp=$(SRCDIR)/merlion/src/merlion/%.hpp)
MERLION_H_INS=$(MERLION_H:%.hpp=$(INCDIR)/tevalibs/merlion/%.hpp)

MERLION_UTILS_H= \
EpanetLinker.hpp \
ModelWriter.hpp \
TaskTimer.hpp \
TSG_Reader.hpp \
NetworkSimulator.hpp \
SimTools.hpp \
Scenarios.hpp

MERLION_UTILS_H_SRC=$(MERLION_UTILS_H:%.hpp=$(SRCDIR)/merlion/merlionUtils/src/merlionUtils/%.hpp)
MERLION_UTILS_H_INS=$(MERLION_UTILS_H:%.hpp=$(INCDIR)/tevalibs/merlionUtils/%.hpp)

TEVASIM_H=version.h

TEVASIM_H_SRC=$(TEVASIM_H:%.h=$(SRCDIR)/tevasim/%.h)
TEVASIM_H_INS=$(TEVASIM_H:%.h=$(INCDIR)/tevalibs/%.h)

ALL_H_SRC= \
  $(TEVASIM_H_SRC)

ALL_H_INS= \
  $(TEVASIM_H_INS) \
  $(MERLION_H_INS) \
  $(MERLION_UTILS_H_INS)


#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES 

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

endif

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
	@$(COPY_EPANET)
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/tevalibs ; then : ; else mkdir $(INCDIR)/tevalibs ; fi
	@$(COPY_UPDATE) $(ALL_H_SRC) $(INCDIR)/tevalibs
	@if test -d $(INCDIR)/tevalibs/merlion ; then : ; else mkdir $(INCDIR)/tevalibs/merlion ; fi
	@$(COPY_UPDATE) $(MERLION_H_SRC) $(INCDIR)/tevalibs/merlion
	@if test -d $(INCDIR)/tevalibs/merlionUtils ; then : ; else mkdir $(INCDIR)/tevalibs/merlionUtils ; fi
	@$(COPY_UPDATE) $(MERLION_UTILS_H_SRC) $(INCDIR)/tevalibs/merlionUtils

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
	@$(CLEAN_EPANET)
	@rm -f $(ALL_H_INS)
