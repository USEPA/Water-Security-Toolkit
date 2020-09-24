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
liblzma_la_SOURCES = C/Alloc.c \
				     C/LzFind.c \
				     C/LzmaDec.c \
				     C/LzmaEnc.c \
				     C/LzmaLib.c

liblzma_la_LIBADD=
liblzma_la_LDFLAGS=$(DEPENDENCY_FLAG)
liblzma_la_CFLAGS=-c -O2 -Wall -D_7ZIP_ST

lib_LTLIBRARIES= liblzma.la

if HOST_MINGW
if BUILD_SHARED
  MINGW_LIBS=.libs/*.dll
endif
endif

LIB_ORIG=$(lib_LTLIBRARIES:%.la=.libs/%.*)
LIB_COPIES=$(lib_LTLIBRARIES:%.la=$(LIBDIR)/.libs/%.*)
LA_COPIES=$(lib_LTLIBRARIES:%=$(LIBDIR)/%)


#########################################################
#########################################################
#
# all-local target is built when "make all" is completed
#
all-local:
	@if test -d $(LIBDIR) ; then : ; else mkdir $(LIBDIR) ; fi
	@$(COPY_UPDATE) $(lib_LTLIBRARIES) $(LIBDIR)
	@if test -d $(LIBDIR)/.libs ; then : ; else mkdir $(LIBDIR)/.libs ; fi
	@$(COPY_UPDATE) $(LIB_ORIG) $(LIBDIR)/.libs
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/tevalibs ; then : ; else mkdir $(INCDIR)/tevalibs ; fi

clean-local:
	@rm -f $(LIB_COPIES) $(LA_COPIES)
	@rm -f $(ALL_H_INS)

EXTRA_DIST=
