SRCDIR=@top_srcdir@/packages/
INCDIR=@top_builddir@/include
EPADIR=@top_builddir@/tpl/epanet
LIBDIR=@top_builddir@/lib
BINDIR=@top_builddir@/bin
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u
AUTOMAKE_OPTIONS=subdir-objects

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

libcsa_la_SOURCES = \
src/CSA.c
libcsa_la_LDFLAGS=$(DEPENDENCY_FLAG)

if BUILD_APPLE
  libcsa_la_CPPFLAGS= -Iinclude -I$(EPADIR)/EPANET_/include -I$(EPADIR)/EPANET_BTX_/include
else
  libcsa_la_CPPFLAGS= -Iinclude -I$(EPADIR)/EPANET_BTX_/include -I$(EPADIR)/EPANET_/include
endif
libcsa_la_LIBADD = $(EPADIR)/libepanet.la $(EPADIR)/libepanetbtx.la

#if BUILD_SHARED
#  lib_LTLIBRARIES=libepanet.la libcsa.la libepanetbtx.la 
#else
#  lib_LTLIBRARIES=libepanet.la libcsa.la libepanetbtx.la
lib_LTLIBRARIES= libcsa.la
#endif

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

CSA_I_H= \
  csa.h
#
#EPANET_BTX_I_H= \
#  epanetbtx.h

#EPANET_MSX_I_H_SRC=$(EPANET_MSX_I_H:%.h=$(SRCDIR)/EPANET_MSX_/include/%.h)
#EPANET_MSX_I_H_INS=$(EPANET_MSX_I_H:%.h=$(INCDIR)/tevalibs/%.h)
#
#EPANET_BTX_I_H_SRC=$(EPANET_BTX_I_H:%.h=$(SRCDIR)/EPANET_BTX_/include/%.h)
#EPANET_BTX_I_H_INS=$(EPANET_BTX_I_H:%.h=$(INCDIR)/tevalibs/%.h)
#
CSA_I_H_SRC=$(CSA_I_H:%.h=$(SRCDIR)/csa/include/%.h)
CSA_I_H_INS=$(CSA_I_H:%.h=$(INCDIR)/tevalibs/%.h)

#EPANET2_H= \
#  include/epanet2.h 

#EPANET2_H_SRC=$(EPANET2_H:%.h=$(SRCDIR)/EPANET_/%.h)
#EPANET2_H_INS=$(EPANET2_H:%.h=$(INCDIR)/tevalibs/%.h)
#
#EN2TOOLKIT_H=epanet2.h
##EN2TOOLKIT_H=epanet2.h epanet2ms.h
#
#
#ALL_H_SRC= \
#  $(EPANET2_H_SRC) \
#  $(EPANET_MSX_I_H_SRC) \
#  $(EPANET_BTX_I_H_SRC)
#
ALL_H_SRC= \
  $(CSA_I_H_SRC)

#ALL_H_INS= \
#  $(EPANET2_H_INS) \
#  $(EPANET_MSX_I_H_INS) \
#  $(EPANET_BTX_I_H_INS)
#
ALL_H_INS= \
  $(CSA_I_H_INS) 


#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES 

if BUILD_CSATEST
  CSATEST=csatest
  csatest_SOURCES=test/csatest.cpp
  csatest_CPPFLAGS=-I$(INCDIR) -Iinclude -I$(EPADIR)/EPANET_/include -I$(EPADIR)/EPANET_BTX_/include
  csatest_LDADD=$(EPADIR)/libepanet.la $(EPADIR)/libepanetbtx.la libcsa.la -lm
  COPY_CSATEST=$(COPY_UPDATE) csatest$(EXEEXT) $(BINDIR)
  CLEAN_CSATEST=rm -f $(BINDIR)/csatest$(EXEEXT)
endif

if BUILD_CSARUN
  CSARUN=csarun
  csarun_SOURCES=CSA_run/csarun.cpp \
		  CSA_run/CSArunOptions.cpp
  csarun_CPPFLAGS=-I$(INCDIR) -Iinclude -ICSA_run/ -I$(EPADIR)/EPANET_/include -I$(EPADIR)/EPANET_BTX_/include \
		  -I$(INCDIR)/tevalibs -I${top_srcdir}/tpl/acro/tpl/boost -I${top_srcdir}/tpl/acro/packages/utilib/src
  csarun_LDADD=$(EPADIR)/libepanet.la $(EPADIR)/libepanetbtx.la libcsa.la ${top_srcdir}/tpl/acro/lib/libutilib.la \
	       $(BLAS_LIBS) $(FLIBS) -lm
  COPY_CSARUN=$(COPY_UPDATE) csarun$(EXEEXT) $(BINDIR)
  CLEAN_CSARUN=rm -f $(BINDIR)/csarun$(EXEEXT)
endif

#
#if BUILD_EPANET_MSX
#  EPANET_MSX=epanet_msx
#  epanet_msx_SOURCES=EPANET_MSX_/src/msxmain.c
#  epanet_msx_CPPFLAGS=-IEPANET_MSX_/include -IEPANET_/include
#  epanet_msx_LDADD=libepanet.la libepanetmsx.la -lm -ldl
#  COPY_EPANETMSX=$(COPY_UPDATE) epanet_msx$(EXEEXT) $(BINDIR)
#  CLEAN_EPANETMSX=rm -f $(BINDIR)/epanet_msx$(EXEEXT)
#endif
#
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

bin_PROGRAMS=$(CSATEST) $(CSARUN)

#BIN_COPIES=$(noinst_PROGRAMS: %=$(BINDIR)/%)

endif

#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#

EXTRA_DIST = EPANET

all-local:

if BUILD_EXECUTABLES
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@cp $(bin_PROGRAMS) $(BINDIR)
endif

	@if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	@$(COPY_UPDATE) $(lib_LTLIBRARIES) $(LIBDIR)
	@if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	@$(COPY_UPDATE) $(LIB_ORIG) $(LIBDIR)/.libs
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@$(COPY_EPANET)
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/tevalibs ; then : ; else mkdir $(INCDIR)/tevalibs ; fi
	@$(COPY_UPDATE) $(ALL_H_SRC) $(INCDIR)/tevalibs

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
	@$(CLEAN_EPANET)
	@rm -f $(ALL_H_INS)
