
##
## TODO - 
## how to wildcard the target to anything
## makefile-hia depends on makefile-sim being successful
##

## to build everything (both static and shared), use the command
##    nmake JDK_ROOT="PATH_TO_YOUR_JDK_INSTALL" everything
##

!IFDEF DEBUGGING
DBG=DEBUGFLAG=/Z7
LDBG=LINKDBG=/DEBUG
!ELSE
DBG=DEBUGFLAG=
LDBG=LINKDBG=
!ENDIF
!IFDEF USE_SHARED_FLAGS
SHARED_INFO=USE_SHARED_FLAGS=$(USE_SHARED_FLAGS)
!ELSE
SHARED_INFO=USE_SHARED_FLAGS=yes
!ENDIF
!IFDEF JDK_ROOT
SHARED_INFO=JDK_ROOT="$(JDK_ROOT)" $(SHARED_INFO)
!ELSE
SHARED_INFO=JDK_ROOT=C:\Java\jdk1.6.0 $(SHARED_INFO)
!ENDIF
# bits.bat returns an error code of 0 if the code is being compiled in a 64-bit
# MS Visual Studio environment, and non-zero otherwise
# See the comment in bits.bat for an explanation of why it (apparently) had to be in
# a batch file and not inline.
!if [@bits.bat] != 0
BITS=32
!else
BITS=64
!endif


everything: all dlls

bits:
	@echo "BITS=$(BITS)"

all: sim merlionapps hia cat iia spot ufl network csa
    @-xcopy static\*.exe ..\..\bin /e /q /i /y

ufl:
    $(MAKE) /F makefile-ufl

merlionapps:
    $(MAKE) /F makefile-merlionapps $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-merlionapps $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

sim:
    $(MAKE) /F makefile-sim $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-sim $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

hia: sim
    $(MAKE) /F makefile-hia $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-hia $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

iia:
    $(MAKE) /F makefile-iia $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-iia $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

cat:
    $(MAKE) /F makefile-cat $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-cat $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

spot:
    $(MAKE) /F makefile-spot $(MAKEFLAGS) clean-objs
    $(MAKE) /F makefile-spot $(MAKEFLAGS) $(DBG) $(LDBG) all

network:
    $(MAKE) /F makefile-network $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-network $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

csa:
    $(MAKE) /F makefile-csa $(MAKEFLAGS) BITS=$(BITS) clean-objs
    $(MAKE) /F makefile-csa $(MAKEFLAGS) BITS=$(BITS) $(DBG) $(LDBG) all

dll: dlls

dlls:
    $(MAKE) /F makefile-sim $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) clean-objs
    $(MAKE) /F makefile-sim $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) dlls
    $(MAKE) /F makefile-hia $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) clean-objs
    $(MAKE) /F makefile-hia $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) dlls
    $(MAKE) /F makefile-iia $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) clean-objs
    $(MAKE) /F makefile-iia $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) dlls
    $(MAKE) /F makefile-cat $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) clean-objs
    $(MAKE) /F makefile-cat $(MAKEFLAGS) BITS=$(BITS) $(SHARED_INFO) dlls

clean:
    $(MAKE) /F makefile-merlionapps BITS=$(BITS) $(MAKEFLAGS) $@
    $(MAKE) /F makefile-sim BITS=$(BITS) $(MAKEFLAGS) $@
    $(MAKE) /F makefile-hia BITS=$(BITS) $(MAKEFLAGS) $@
    $(MAKE) /F makefile-iia BITS=$(BITS) $(MAKEFLAGS) $@
    $(MAKE) /F makefile-cat BITS=$(BITS) $(MAKEFLAGS) $@
    $(MAKE) /F makefile-spot $(MAKEFLAGS) $@
    $(MAKE) /F makefile-ufl $(MAKEFLAGS) $@
    $(MAKE) /F makefile-csa $(MAKEFLAGS) $@

zip:
    @-rd /S /Q bin
    md bin
    @-xcopy static\*.exe bin /e /q /i /y
    python zip.py pywst_bin.zip bin
    @-rd /S /Q bin

