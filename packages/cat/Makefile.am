include ${top_srcdir}/config/MakeMacros.mk
SUFFIXES= .i
INCDIR=@top_builddir@/include
COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

SUBDIRS=. tso2Impact 

H_FILES=\
ExtentOfContaminationObjective.h \
Impact.h \
impacts.h \
MassConsumedObjective.h \
NumberFailedDetectionsObjective.h \
ObjectiveBase.h \
ObjectiveVisitor.h \
ObjectiveAggr.h \
EventImpacts.h \
scenario.h \
SPProblem.h \
TimeExtentOfContaminationObjective.h \
TimeToDetectionObjective.h \
VecTrie.h \
vlink.h \
VolumeContaminatedWaterConsumedObjective.h

SRC_H_FILES=${H_FILES:%=@srcdir@/tso2Impact/sp/%}

INSTALLED_H_FILES=${H_FILES:%=$(INCDIR)/sp/%}

include_HEADERS=$(SRC_H_FILES)

all-local:
	@if test -d $(INCDIR) ; then : ; else mkdir $(INCDIR) ; fi
	@if test -d $(INCDIR)/sp ; then : ; else mkdir $(INCDIR)/sp ; fi
	@$(COPY_UPDATE) $(SRC_H_FILES) $(INCDIR)/sp

clean-local:
	@rm -f $(INSTALLED_H_FILES)
