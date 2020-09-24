INCDIR=@top_builddir@/include
LIBDIR=@top_builddir@/lib

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
#########################################################


lib_LTLIBRARIES=libdetect.la

libdetect_la_SOURCES = Detection.h Detection.cpp

libdetect_la_LDFLAGS =$(DEPENDENCY_FLAG) $(ARCH_FLAGS) $(FEATURE_FLAGS) 

libdetect_la_CXXFLAGS=-fexceptions

libdetect_la_CPPFLAGS=$(ARCH_FLAGS) $(FEATURE_FLAGS) -I$(INCDIR) -I$(INCDIR)/tevalibs

if !BUILD_SHARED
#
# A flag for libtool
#
AM_LDFLAGS=-all-static
endif

all-local:
	if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	cp .libs/libdetect.* $(LIBDIR)/.libs

clean-local:
	rm -f $(LIBDIR)/.libs/libdetect.*
