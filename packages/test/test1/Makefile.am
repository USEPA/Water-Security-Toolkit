testdir = $(prefix)/test/test1

batFiles= \
    test1o.bat \
    test1p.bat \
    test1f.bat    \
    test1aa.bat   \
    test1q.bat \
    test1j.bat     \
    test1r.bat \
    test1a.bat    \
    test1ab.bat   \
    test1k.bat     \
    test1s.bat \
    test1g.bat    \
    test1l.bat     \
    test1h.bat    \
    test1t.bat \
    test1MakeImpact.bat  \
    test1a_m.bat  \
    test1m.bat     \
    test1i.bat   \
    test1mMakeImpact.bat \
    test1.bat    \
    test1n.bat

cshFiles= \
    bindirLocation    \
    pythonLocation    \
    test1.imperfectsc   \
    test1o \
    moddirLocation    \
    test1c       \
    test1p \
    test1a  \
    test1f       \
    test1aa \
    test1j         \
    test1r \
    test1ab \
    test1k         \
    test1s \
    test1g       \
    test1l         \
    test1.sh \
    test1h       \
    test1t \
    test1m         \
    test1a_m\
    test1i       \
    test1a_memcheck   \
    test1n

INSTALLED_CSH_FILES=${cshFiles:%=$(DESTDIR)/$(prefix)/test/test1/%}

dataFiles= \
    test1b.tsg    \
    test1-invalid-ids   \
    Net3.inp\
    test1c.tsg    \
    test1-invalid-junctions   \
    test1-invalid-locations   \
    test1i.qa       \
    test1q \
    test1-fixed-ids   \
    test1aa.qa    \
    test1-fixed-junctions   \
    test1-fixed-locations   \
    test1j.qa       \
    test1f.qa     \
    test1ab.qa    \
    test1k.qa       \
    test1-sensor-locations \
    test1a.glpk   \
    test1g.qa     \
    test1h.qa     \
    test1.tsg \
    test1-valid-locations \
    test1a.qa\
    test1-id-costs\
    test1.imperfectjc
    
if BUILD_CYGWIN

test_DATA=$(dataFiles) $(batFiles)

else

test_DATA=$(dataFiles) $(cshFiles)

install-data-hook:
	@chmod +x $(INSTALLED_CSH_FILES)


endif

COPY_UPDATE=${top_srcdir}/tpl/acro/bin/cp_u

SRCFILES=$(test_DATA:%=@srcdir@/%)

all-local:
if OUT_OF_SOURCE_BUILD
	@$(COPY_UPDATE) $(SRCFILES) .
endif

all-clean:
if OUT_OF_SOURCE_BUILD
	@rm -f $(test_DATA)
endif
