# MinGW Makefile for EPANET

# Note: This makefile usese MinGW to produce native windows
# libraries and executables.  The dll/import library, and
# executable files will have the same names as those produced
# using other Windows compilers, e.g. MSVC++.  So be aware that
# there is no way to reliably distinguish between these two compiler
# sources based on output filenames.

# This will build EPANET as a native windows DLL and import
# library (epanet2.dll/libepanet2.dll.a) using MinGW/gcc,
# and a standalone executable (epanet2.exe).
# This makefile also creates a windows batch file
# (see MScmdname below) that can be run to produce an import
# library for linking the DLL using MSVC.
# This makefile assumes a minimum
# MinGW installation, and uses the MinGW gcc compiler from
# the MinGW install directory.
# The current release of MinGW can be obtained from
# http://www.mingw.org/wiki/Getting_Started -- the easiest
# approach is to download the latest MingW installer and
# accept all of the defaults.

# The following targets are defined (for execution in
# the build directory):
# make
#     -Builds epanet2.dll, epanet2.def, libepanet2.dll.a,
#      epanet2.exe
# make install
#     -Creates scripts runepanet2.sh and CreateEpanet2Lib.bat that
#      execute epanet2.exe, and MSVC 'lib' (to create an MSVC
#      import library).  The runepanet2.sh wrapper simply exports
#      environment variables so that the DLL is found at runtime,
#      allowing you to specify your own locations for installing
#      the DLL.
#     -Installs epanet2.dll, epanet2.exe, and runepanet2.sh
#      in <prefix>/bin, where <prefix> defaults to ~ (and can be set
#      below to something different - see "Install directories")
#     -Installs libepanet2.dll.a (import library), epanet2.def,
#      and CreateEpanet2Lib.bat in <prefix>/lib
#     -Installs epanet2.h in <prefix>/include.  This is the required
#      header file for the EPANET programmer's toolkit, and thus
#      <prefix>/include should be on your CPP include search path
#      for subsequent use of the toolkit and linking with the import
#      library libepanet2.dll.a or epanet2.lib.
# make clean
#     -Removes object and library files, returning the build directory
#      to its pristine state.

SHELL = /bin/sh

# C H A N G E  H E R E  A S  N E E D E D
# Change (as needed) the MinGW install directory below.
# Change (as needed) the location off the MS LIB tool.
# You may also wish to change the install path 'prefix',
# or the compiler flags, but these defaults should be fine.

# MinGW top level install directory, accessible from build environment
MinGWdir = /cygdrive/c/MinGW
# Microsoft lib tool directory (for creating an MS import library)
MSVClibexe = c:\\Program Files\\Microsoft Visual Studio 9.0\\VC\\bin\\lib

# Target filenames
# svnname.sh constructs a name: <prefix><root><Rev><postfix>
# where <Rev> is the atomic revision number of the svn repo.
epanetrootname := epanet2
dllname := $(epanetrootname).dll
defname := $(epanetrootname).def
implibname = lib$(dllname).a
exename := $(epanetrootname)
# Shell wrapper
runcmdtemplate = runepanet.sh.template
runcmdname = runepanet2.sh
# MSVC import lib batch file
MScmdtemplate = CreateMSLib.bat.template
MScmdname = CreateEpanet2Lib.bat
# Location of EPANET toolkit includes
epanetincludedir = ../../include
# Search path for sources
epanetsrcdir = ../../src
VPATH = $(epanetsrcdir):$(epanetincludedir)

# Install directories
winprefix = $(shell cygpath -w $$HOME)
prefix = ~
exec_prefix = $(prefix)
srcdir = .
libdir = $(exec_prefix)/lib
winlibdir = "$(winprefix)\lib"
bindir = $(exec_prefix)/bin
includedir = $(prefix)/include
winincludedir = "$(winprefix)\include"
datarootdir = $(prefix)/share
docdir = $(datarootdir)/doc/epanet

# Compiler and flags
# MinGW gcc
CC = $(MinGWdir)/bin/gcc
dlltool = $(MinGWdir)/bin/dlltool
CFLAGS = -g -O3
CPPFLAGS = -I $(srcdir) -I $(epanetincludedir)
LDFLAGS = -L . -W1,-rpath,$(libdir) -lm

# Installer
INSTALL = install
INSTALL_PROGRAM = $(INSTALL)
INSTALL_DATA = $(INSTALL) -m 644

# EPANET object files
epanet_objs=hash.o hydraul.o inpfile.o input1.o input2.o \
	    input3.o mempool.o output.o quality.o report.o \
	    rules.o smatrix.o
# Epanet header files
epanet_heads=enumstxt.h funcs.h hash.h mempool.h text.h toolkit.h types.h vars.h
epanet_main_heads=epanet2.h
# Epanet main program
epanet_main=epanet

.PHONY: all
all: $(dllname) $(exename)

$(dllname): $(epanet_objs)
#	$(dlltool) -z $(defname) --dllname $(dllname) --output-lib $(implibname) $^
#	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ $(defname) $^
	$(CC) $(CFLAGS) $(CPPFLAGS) -D DLL -c $(epanetsrcdir)/epanet.c
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ epanet.o $^ -Wl,--output-def,$(defname)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared -o $@ epanet.o $^ -Wl,--kill-at
	$(dlltool) -d $(defname) --dllname $@ --output-lib $(implibname) --kill-at

$(exename): $(epanet_objs)
	$(CC) $(CFLAGS) $(CPPFLAGS) -D CLE -c $(epanetsrcdir)/$(epanet_main).c
	$(CC) $(CFLAGS) -o $@ $(epanet_main).o $^ $(LDFLAGS)

$(epanet_objs): $(epanet_heads)

.PHONY: install
install:
	cat $(runcmdtemplate) | sed 's|libdir|$(bindir)|' \
                                | sed 's|exename|$(bindir)/$(exename)|' \
                                > $(runcmdname)
	cat $(MScmdtemplate) | sed 's|MSVClibexe|$(MSVClibexe)|' \
                                | sed 's|defname|$(defname)|' \
                                | sed 's|libdir|$(libdir)|' \
                                > $(MScmdname)
	$(INSTALL_PROGRAM) -D $(exename) $(bindir)/$(exename)
	$(INSTALL_PROGRAM) -D $(dllname) $(bindir)/$(dllname)
	$(INSTALL_PROGRAM) -D $(defname) $(libdir)/$(defname)
	$(INSTALL_PROGRAM) -D $(implibname) $(libdir)/$(implibname)
	$(INSTALL_DATA) -D $(epanetincludedir)/epanet2.h $(includedir)/epanet2.h
	$(INSTALL_PROGRAM) -D $(runcmdname) $(bindir)/$(runcmdname)
	$(INSTALL_PROGRAM) -D $(MScmdname) $(libdir)/$(MScmdname)

.PHONY: uninstall
uninstall:

.PHONY: check
check:

.PHONY: clean
clean:
	-/bin/rm *.o $(dllname) $(defname) $(implibname) $(exename).exe $(runcmdname) $(MScmdname)
