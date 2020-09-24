SRCDIR=@top_srcdir@/packages
BINDIR=@top_builddir@/bin

GMPCONFIGURE=../$(SRCDIR)/@gmp@/configure

if HAVE_SYSTEM_32
  GMP_SPECIAL="ABI=32"
endif

if GMP_NOT_CONFIGURED
  BUILD_GMP=cd @gmp@ && @EXPORT_FLAGS@ && $(GMPCONFIGURE) $(GMP_SPECIAL) @ac_configure_args@ && $(MAKE) all
else
  BUILD_GMP=cd @gmp@ && $(MAKE) all
endif

SUBDIRS =@acro@ @lzma@ epanet

all-local: 
	@if test -d $(BINDIR) ; then : ; else mkdir $(BINDIR) ; fi
	@if test -x $(top_builddir)/../acro-pico/PICO ; then \
	cp -f $(top_builddir)/../acro-pico/bin/PICO* $(BINDIR) ; \
	cp -f $(top_builddir)/../acro-pico/bin/cbc* $(BINDIR) ; \
	cp -f $(top_builddir)/../acro-pico/bin/memmon* $(BINDIR) ; \
	cp -f $(top_builddir)/../acro-pico/bin/glpsol $(BINDIR) ; \
	fi 
	@cp -f ./ufl* $(BINDIR) 

clean-local:
	@if test -d $(top_builddir)/../acro-pico ; then \
	rm -f $(BINDIR)/PICO* ; \
	rm -f $(BINDIR)/cbc* ; \
	rm -f $(BINDIR)/memmon* ; \
	rm -f $(BINDIR)/glpsol* ; \
	fi 
	@rm -f ufl* VolVolume*

#
# We use "make install DESTDIR={release directory path}" to create a
# release directory to tar or zip for customers.
#
# The "make install" in acro installed many files we don't want
# in a release directory.  Add an install hook to remove those.
#
# The executables we want from acro-pico are PICO, memmon and ufl.
# And glpsol.
# The only library we want from acro is libutilib.

install-exec-hook:
	@for nm in $$(ls $(DESTDIR)@bindir@) ; do \
          basenm=$${nm##*/}; \
          if test -f $(top_build_dir)/../acro-pico/bin/$$basenm ; then \
            if test $$basenm == "PICO" || \
               test $$basenm == "glpsol" || \
               test $$basenm == "ufl" || \
               test $$basenm == "memmon" ; then : ; else \
               rm -rf $$nm; \
            fi \
          fi \
        done 
	@for nm in $$(ls $(DESTDIR)@libdir@) ; do \
          basenm=$${nm##*/}; \
          if test -f acro/lib/$$basenm ; then \
            newnm=$$(echo $$basenm | sed s/utilib/xxxxxx/); \
            if test "$$basenm" == "$$newnm" ; then \
              rm -rf $$nm; \
            fi \
          fi \
        done 
	@rm -f $(DESTDIR)@libdir@/*.la
	@rm -rf $(DESTDIR)@includedir@/*

#
# Install the python scripts and modules that are in acro/python.
# They were copied there at build time, so we can't just call them
# bin_SCRIPTS or py_DATA and have them installed at "make install" time.
#
#
#PY_DIR=$(DESTDIR)$(prefix)/python
#
#if DO_PYINSTALL
#install-data-hook:
#	@if test -d $(PY_DIR); then : ; else mkdir $(PY_DIR); fi;
#	@for dir in $$(ls acro/python/lib) ; do \
#           cp -r acro/python/lib/$$dir/site-packages/*.egg  $(PY_DIR) ; \
#        done
#endif

#
# build ufl executable

UFL_SRC = @srcdir@/sundry/src
UFL_INCLUDE = @srcdir@/sundry

bin_PROGRAMS = ufl

ufl_SOURCES = $(UFL_SRC)/ufl.cpp $(UFL_SRC)/VolVolume.cpp
ufl_CPPFLAGS = -I$(UFL_INCLUDE)




check-local: python

python.log: ../admin/vpy/python.zip
	rm -Rf ../python
	touch python.log

python:
	@if test -d ../python; then : ;\
	else\
	   $(top_srcdir)/tpl/vpykit/bin/install_python --src $(top_srcdir) --build $(abs_top_builddir) -z $(top_srcdir)/admin/vpy/python.zip -c $(top_srcdir)/admin/vpy/dev.ini --logfile $(abs_top_builddir)/tpl/python.log; \
	fi

update-zip:
	cd $(top_srcdir)/tpl/vpykit; \
	bin/create_zipfile -c ../../admin/vpy/dev.ini -d ../../admin/vpy

update-zip-trunk:
	cd $(top_srcdir)/tpl/vpykit; \
	bin/create_zipfile --trunk -c ../../admin/vpy/dev.ini -d ../../admin/vpy

