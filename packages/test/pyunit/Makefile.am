#
# Nothing is built in this Makefile.  It is here so that tests get
# installed at "make install DESTDIR=" and so that tests can be
# copied into a build directory during an out-of-source build
#
testdir = $(prefix)/test/acceptance

test_DATA=README.txt

nobase_test_DATA = \
    evalsensor/OneSensor.txt \
    evalsensor/test1.qa \
    evalsensor/test2.qa \
    evalsensor/test_evalsensor.py \
    pico/__init__.py \
    pico/input/test_mc_fixed_locations.mps \
    pico/input/test_mc_valid_locations.mps \
    pico/input/test_mc_worst.mps \
    pico/input/test_mc.mps \
    pico/input/test_mc_sensor_locations.mps \
    pico/input/test_ec_threshold.mps \
    pico/input/test_mc_cvar.mps \
    pico/input/test_mc_costs.mps \
    pico/output/test_ec_threshold.solution \
    pico/output/test_mc.solution \
    pico/output/test_mc_costs.solution \
    pico/output/test_mc_fixed_locations.solution \
    pico/output/test_mc_sensor_locations.solution \
    pico/output/test_mc_valid_locations.solution \
    pico/test_pico.py \
    tevasim/__init__.py \
    tevasim/output/Net3.out \
    tevasim/output/linux32/test1.sdx \
    tevasim/output/linux32/test1.tsi \
    tevasim/output/linux32/test1.tso \
    tevasim/output/linux32/test1b.sdx \
    tevasim/output/linux32/test1b.tsi \
    tevasim/output/linux32/test1b.tso \
    tevasim/output/linux32/test1c.sdx \
    tevasim/output/linux32/test1c.tsi \
    tevasim/output/linux32/test1c.tso \
    tevasim/output/linux64/test1.sdx \
    tevasim/output/linux64/test1.tsi \
    tevasim/output/linux64/test1.tso \
    tevasim/output/linux64/test1b.sdx \
    tevasim/output/linux64/test1b.tsi \
    tevasim/output/linux64/test1b.tso \
    tevasim/output/linux64/test1c.sdx \
    tevasim/output/linux64/test1c.tsi \
    tevasim/output/linux64/test1c.tso \
    tevasim/output/win32/test1.sdx \
    tevasim/output/win32/test1.tsi \
    tevasim/output/win32/test1.tso \
    tevasim/output/win32/test1b.sdx \
    tevasim/output/win32/test1b.tsi \
    tevasim/output/win32/test1b.tso \
    tevasim/output/win32/test1c.sdx \
    tevasim/output/win32/test1c.tsi \
    tevasim/output/win32/test1c.tso \
    tevasim/test_tevasim.py \
    tso2Impact/__init__.py \
    tso2Impact/largeQuality_ec.impact \
    tso2Impact/largeQuality_mc.impact \
    tso2Impact/multi_quality.impact \
    tso2Impact/responseTime1.impact \
    tso2Impact/test_tso2Impact.py 

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
