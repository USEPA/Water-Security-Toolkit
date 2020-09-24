
#
# SLB: Temporary flag to prevent parallel compilation by gmake
#
#.NOTPARALLEL:

include ../../../config/MakeMacros.mk

TOPINC=../../../include
TOPBIN=${top_srcdir}/bin

MAINTAINERCLEANFILES = Makefile.in

FULLPATH_SRC=${srcdir}

ALL_H_SRC=\
  $(FULLPATH_SRC)/sundry.h\
  $(FULLPATH_SRC)/SMVkernel.h\
  $(FULLPATH_SRC)/SNLRound.h\
  $(FULLPATH_SRC)/ufl.h\
  $(FULLPATH_SRC)/VolVolume.h

includesundrydir=$(includedir)/sundry
includesundry_HEADERS = $(ALL_H_SRC)

all-local:
	@if test -d $(TOPINC) ; then : ; else mkdir $(TOPINC) ; fi
	@if test -d $(TOPINC)/sundry ; then : ; else mkdir $(TOPINC)/sundry ; fi
	@$(TOPBIN)/cp_u $(ALL_H_SRC) $(TOPINC)/sundry

clean-local:
	@rm -rf $(TOPINC)/sundry
