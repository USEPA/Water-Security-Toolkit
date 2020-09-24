#
# Nothing is built in this Makefile.  It is here so that tests get
# installed at "make install DESTDIR=" and so that tests can be
# copied into a build directory during an out-of-source build
#
# TODO - I think some of these files duplicate files in data/Robs
# and should be removed.  Script may need to be changed as well.
# The idea was to keep data centralized, since many tests use the
# same data.
#
testdir = $(prefix)/test/Robs

test_DATA=compare.py

nobase_test_DATA = \
    testfiles/README.txt \
    testfiles/TestNet3.inp \
    testfiles/TestNet3.nodemap \
    testfiles/TestNet3.out \
    testfiles/TestNet3.scenariomap \
    testfiles/TestNet3_ec.impact \
    testfiles/TestNet3_mc.impact \
    testfiles/TestNet3_nfd.impact \
    testfiles/TestNet3_pe.impact \
    testfiles/TestNet3_td.impact \
    testfiles/TestNet3_vc.impact \
    testfiles/bio.sdx \
    testfiles/bio.tai \
    testfiles/bio.tsg \
    testfiles/bio.tsi \
    testfiles/bio.tso \
    testfiles/demand.txt \
    testfiles/hyddata.tmp

COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

all-local:
if OUT_OF_SOURCE_BUILD
	@for fname in $(nobase_test_DATA) ; do  \
          dname=$${fname%/*};  \
          if test -d $$dname ; then : ; else mkdir $$dname ; fi ;  \
          $(COPY_UPDATE) ${srcdir}/$$fname $$dname ;  \
        done
	@for fname in $(test_DATA) ; do  \
          $(COPY_UPDATE) ${srcdir}/$$fname . ; \
        done
endif

all-clean:
if OUT_OF_SOURCE_BUILD
	@rm -f $(test_DATA) $(nobase_test_DATA)
endif


