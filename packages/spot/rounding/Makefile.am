BINDIR=${top_builddir}/bin
INCDIR=${top_builddir}/include
LIBDIR=${top_builddir}/lib
GMPDIR=${top_builddir}/packages/gmp-4.1.4
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

if BUILD_EXECUTABLES

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

FEATURE_FLAGS=$(ANSI_FLAGS) $(NS_FLAGS) -DMPICH_IGNORE_CXX_SEEK

if BUILD_SNLROUND
  EXECS=SNLRound
  SNLRound_SOURCES=SNLRound.cpp
  SNLRound_LDADD=$(GMPDIR)/libgmp.la -lm
  SNLRound_LDFLAGS=-L$(GMPDIR)
  SNLRound_CPPFLAGS=-I$(GMPDIR) $(ARCH_FLAGS) $(FEATURE_FLAGS)
endif

#
# We use "make install prefix={release dir}" to create release
# directory.  Since we don't want SNLRound in release directory,
# we make it "noinst_PROGRAMS" instead of "bin_PROGRAMS".
#
noinst_PROGRAMS = $(EXECS)

if !BUILD_SHARED
#
# A flag for libtool
#
AM_LDFLAGS=-all-static
endif


all-local:
if BUILD_SNLROUND
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@$(COPY_UPDATE) $(EXECS) $(BINDIR)
endif

endif
