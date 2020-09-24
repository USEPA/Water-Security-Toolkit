SRCDIR=@top_srcdir@/tpl/epanet
INCDIR=@top_builddir@/include
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

libepanet_la_SOURCES = \
EPANET_/src/epanet.c\
EPANET_/src/hash.c\
EPANET_/src/hydraul.c\
EPANET_/src/inpfile.c\
EPANET_/src/input1.c\
EPANET_/src/input2.c\
EPANET_/src/input3.c\
EPANET_/src/mempool.c\
EPANET_/src/output.c\
EPANET_/src/quality.c\
EPANET_/src/report.c\
EPANET_/src/rules.c\
EPANET_/src/smatrix.c

libepanet_la_LDFLAGS=$(DEPENDENCY_FLAG)

libepanetmsx_la_SOURCES = \
    EPANET_MSX_/src/mathexpr.c \
    EPANET_MSX_/src/msxchem.c \
    EPANET_MSX_/src/msxcompiler.c \
    EPANET_MSX_/src/msxfile.c \
    EPANET_MSX_/src/msxfuncs.c \
    EPANET_MSX_/src/msxinp.c \
    EPANET_MSX_/src/msxout.c \
    EPANET_MSX_/src/msxproj.c \
    EPANET_MSX_/src/msxqual.c \
    EPANET_MSX_/src/msxrpt.c \
    EPANET_MSX_/src/msxtank.c \
    EPANET_MSX_/src/msxtoolkit.c \
    EPANET_MSX_/src/msxutils.c \
    EPANET_MSX_/src/newton.c \
    EPANET_MSX_/src/rk5.c \
    EPANET_MSX_/src/ros2.c
libepanetmsx_la_LDFLAGS=$(DEPENDENCY_FLAG) 

libepanetbtx_la_SOURCES = \
    EPANET_BTX_/src/backtrack.c \
    EPANET_BTX_/src/btxtoolkit.c \
    EPANET_BTX_/src/btxutil.c
libepanetbtx_la_LDFLAGS=$(DEPENDENCY_FLAG) 

#libcsa_la_SOURCES = \
#    CSA_/src/CSA.cpp
#libcsa_la_LDFLAGS=$(DEPENDENCY_FLAG) 
#
if BUILD_APPLE
  libepanetmsx_la_CPPFLAGS=-IEPANET_MSX_/include -IEPANET_/include -IEPANET_MSX_/build/Xcode/epanetmsx
  libepanetbtx_la_CPPFLAGS=-IEPANET_BTX_/include -IEPANET_/include -IEPANET_BTX_/build/Xcode/epanetbtx
#  libcsa_la_CPPFLAGS=-IEPANET_BTX_/include -IEPANET_/include -ICSA_/include -IEPANET_BTX_/build/Xcode/epanetbtx
else
  libepanetmsx_la_CPPFLAGS=-IEPANET_MSX_/include -IEPANET_/include
  libepanetbtx_la_CPPFLAGS=-IEPANET_BTX_/include -IEPANET_/include
 # libcsa_la_CPPFLAGS=-IEPANET_BTX_/include -IEPANET_/include -ICSA_/include
endif
libepanetmsx_la_LIBADD = libepanet.la
libepanetbtx_la_LIBADD = libepanet.la
#libcsa_la_LIBADD = libepanet.la libepanetbtx.la

if BUILD_SHARED
  lib_LTLIBRARIES=libepanet.la libepanetmsx.la libepanetbtx.la #libcsa.la
else
  lib_LTLIBRARIES=libepanet.la libepanetmsx.la libepanetbtx.la #libcsa.la
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

EPANET_MSX_I_H= \
  epanetmsx.h
#  mstext.h \
#  mstext.h \
#  msvars.h \
#  msfuncs.h \
#  mstypes.h \
#  msenumstxt.h \
#  newton.h \
#  odesolve.h \
#  mathexpr.h \
#  mstoolkit.h
#
EPANET_BTX_I_H= \
  epanetbtx.h

#CSA_I_H= \
  csa.h

EPANET_MSX_I_H_SRC=$(EPANET_MSX_I_H:%.h=$(SRCDIR)/EPANET_MSX_/include/%.h)
EPANET_MSX_I_H_INS=$(EPANET_MSX_I_H:%.h=$(INCDIR)/tevalibs/%.h)

EPANET_BTX_I_H_SRC=$(EPANET_BTX_I_H:%.h=$(SRCDIR)/EPANET_BTX_/include/%.h)
EPANET_BTX_I_H_INS=$(EPANET_BTX_I_H:%.h=$(INCDIR)/tevalibs/%.h)

#CSA_I_H_SRC=$(CSA_I_H:%.h=$(SRCDIR)/CSA_/include/%.h)
#CSA_I_H_INS=$(CSA_I_H:%.h=$(INCDIR)/tevalibs/%.h)

EPANET2_H= \
  include/epanet2.h \
  src/toolkit.h \
  src/hash.h \
  src/funcs.h \
  src/types.h \
  src/text.h \
  src/vars.h \
  src/enumstxt.h \
  src/mempool.h

EPANET2_H_SRC=$(EPANET2_H:%.h=$(SRCDIR)/EPANET_/%.h)
EPANET2_H_INS=$(EPANET2_H:%.h=$(INCDIR)/tevalibs/%.h)

EN2TOOLKIT_H=epanet2.h
#EN2TOOLKIT_H=epanet2.h epanet2ms.h


ALL_H_SRC= \
  $(EPANET2_H_SRC) \
  $(EPANET_MSX_I_H_SRC) \
  $(EPANET_BTX_I_H_SRC) #\
  $(CSA_I_H_SRC)
  
ALL_H_INS= \
  $(EPANET2_H_INS) \
  $(EPANET_MSX_I_H_INS) \
  $(EPANET_BTX_I_H_INS) #\
  $(CSA_I_H_INS)


#########################################################
# EXECUTABLES
#########################################################

if BUILD_EXECUTABLES 

if BUILD_EPANET
  EPANET=epanet
  epanet_SOURCES=EPANET_/src/epanet.c
  epanet_CPPFLAGS=-DCLE
  epanet_LDADD=libepanet.la -lm
  COPY_EPANET=$(COPY_UPDATE) epanet$(EXEEXT) $(BINDIR)
  CLEAN_EPANET=rm -f $(BINDIR)/epanet$(EXEEXT)
endif

if BUILD_EPANET_MSX
  EPANET_MSX=epanet_msx
  epanet_msx_SOURCES=EPANET_MSX_/src/msxmain.c
  epanet_msx_CPPFLAGS=-IEPANET_MSX_/include -IEPANET_/include
  epanet_msx_LDADD=libepanet.la libepanetmsx.la -lm -ldl
  COPY_EPANETMSX=$(COPY_UPDATE) epanet_msx$(EXEEXT) $(BINDIR)
  CLEAN_EPANETMSX=rm -f $(BINDIR)/epanet_msx$(EXEEXT)
endif

#if CSATEST
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

noinst_PROGRAMS=$(EPANET) $(EPANET_MSX)

BIN_COPIES=$(noinst_PROGRAMS: %=$(BINDIR)/%)

endif

#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#

EXTRA_DIST = EPANET

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

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES) $(BIN_COPIES)
	@$(CLEAN_EPANET)
	@rm -f $(ALL_H_INS)
