SHELL=/bin/bash
DEPdir = .deps/

# Subdirectory holding the figures.  This should be defined + accessed
# in the TeX using a custom \fig or \figDir command
FIGdir = figures

# How should we compile the TeX?  Normally this will be either latex or
# pdflatex.  We recommend overriding this in Makefile.local
LATEX = latex

# Ideally, the list of targets should be read in from Makefile.local so
# that we can re-use this master Makefile
TARGETS = 


BASE_OS:=$(shell uname | cut -d'-' -f1)
ifeq ($(BASE_OS),Darwin)
  RE = -E
  BACKREF=\\\\
  EOW=[[:>:]]/
else
  RE = -r
  BACKREF=\\\\
  EOW=(\\S)\\s/\\1
endif

.PHONY: docs
docs: default_target

#####################################################################
# Local customizations & special auto-generated targets:
ifneq ($(wildcard Makefile.local),)
  include Makefile.local
  MAKEDEPS=Makefile Makefile.local
else
  MAKEDEPS=Makefile
endif
#####################################################################

PS_TARGETS = lstanchors.out $(patsubst %,%.ps,$(TARGETS))
PDF_TARGETS = lstanchors.out $(patsubst %,%.pdf,$(TARGETS))
EXISTING_PDF_TARGETS = $(filter $(PDF_TARGETS),$(wildcard *.pdf))
BIB_TARGETS = $(shell egrep -H '\\bibliography\{ *[^\}]' *.tex | \
	egrep -v '^([^[:space:]]+:)?\s*\%' | cut -d':' -f1 | uniq | \
	sed 's/\.tex/\.bbl/' )

ifeq ($(LATEX),pdflatex)
	DEFAULT_TARGETS=$(PDF_TARGETS)
	TEX_OUT=pdf
else
	DEFAULT_TARGETS=$(PS_TARGETS) $(EXISTING_PDF_TARGETS)
	TEX_OUT=dvi
endif

.PHONY: default_target
default_target: $(DEFAULT_TARGETS)

.PHONY: pdf
pdf: $(PDF_TARGETS)

.PHONY: ps
pdf: $(PS_TARGETS)

.PHONY: all
all:	pdf 

.PHONY: dox
dox:
	doxygen Doxyfile

.PHONY: FORCE

%.ps: %.dvi
	@echo "(INFO) Regenerating Postscript: $@"
	@echo
	@dvips -D600 -Z -G0 -t letter -o $@ $^

# Run latex up to four times to get references right...
%.$(TEX_OUT): %.tex 
	@if [ ! -z $(filter $*.bbl,$(BIB_TARGETS)) ]; then $(MAKE) $*.bbl; fi
	@echo
	@echo "(INFO) Compiling LaTeX: $<"
	@echo
	@$(LATEX) $< 
	@echo
	@for((I=2; I<=4; I++)); do \
	   LOG="$(subst .$(TEX_OUT),.log,$@)"; \
	   if [ \( `grep 'were undefined ' $$LOG  | wc -l` -ne 0 \) \
		-o \( `grep 'Rerun to get ' $$LOG | wc -l` -ne 0 \) \
		-o \( ! -e $@ \) ]; then \
	     echo "(INFO) Compiling LaTeX: $< (pass $$I)"; \
	     $(LATEX) $< > /dev/null; \
	   fi; \
	 done
	@LOG="$(subst .$(TEX_OUT),.log,$@)"; \
	 if [ `grep 'were undefined references' $$LOG | wc -l`\
		 -ne 0 ]; then \
	  echo ;\
	  echo "(WARNING) Undefined references remain! "\
		"See $$LOG"; \
	fi
	@LOG="$(subst .$(TEX_OUT),.log,$@)"; \
	 if [ `grep 'were undefined citations' $$LOG | wc -l`\
		 -ne 0 ]; then \
	  echo ;\
	  echo "(WARNING) Undefined citations remain! "\
		"See $$LOG"; \
	fi
	@echo


ifneq ($(MODE),pdflatex)
%.pdf: %.ps
	@echo "(INFO) Regenerating PDF: $@"
	@echo
	@ps2pdf14 $<
	@if [ -d Doxygen ]; then \
	  cp -vp $@ Doxygen/$@ ;\
	fi
endif


%.bbl: %.tex
	@echo
	@echo "(INFO) Precompiling LaTeX for BiBTeX run: $<"
	@echo
	$(LATEX) $<
	@echo
	@echo "(INFO) Running BibTeX on: $<"
	@echo
	@rm -fv `echo $< | sed $(RE) 's/\.tex/.$(TEX_OUT)/'`
	BIBINPUTS="$BIBINPUTS:."; bibtex $*


VERSION = version.tex
SVNREV = $(shell if test -d .svn && type svnversion > /dev/null 2>&1; \
		then svnversion . -n | sed 's/^/r/'; \
	   elif test -f $(VERSION); \
		then cut $(VERSION) -d'(' -f2 | cut -d')' -f1; \
	   else echo "Unknown"; fi)
SVNOLDREV = $(shell if test -f $(VERSION); \
		then cut $(VERSION) -d'(' -f2 | cut -d')' -f1; \
	      else echo "DNE"; fi)
VER_H = ../src/svninfo.h
VER    = $(shell if test -f $(VER_H); \
	     then grep version\(\) -A1 $(VER_H) | tail -1 | cut -d'"' -f2 \
                  | xargs echo Version ; \
	   else echo "Unknown Version"; fi)
OLDVER  = $(shell if test -s $(VERSION); \
	     then cut $(VERSION) -d' ' -f1-2; \
	   else echo "DNE"; fi)
$(VERSION): FORCE
	@if test \( "$(SVNREV)" != "$(SVNOLDREV)" \) -o \
	         \( "$(VER)" != "$(OLDVER)" \); then \
	   echo "$(VER) ($(SVNREV))" > $@;\
	   echo "Creating doxygen version information file."; \
	fi


.PHONY: clean
clean: $(LOCAL_CLEAN)
	@rm -fv *\~
	@rm -fv core.[0-9]*
	@rm -fv *.aux 
	@rm -fv *.bbl
	@rm -fv *.blg
	@rm -fv *.loc
	@rm -fv *.lof
	@rm -fv *.log
	@rm -fv *.lot
	@rm -fv *.spl
	@rm -fv *.dvi
	@rm -fv *.out
	@rm -fv *.toc
	@if [ ! -z $(DEPdir) ]; then rm -frv $(DEPdir); fi

.PHONY: distclean realclean clobber Clean
distclean realclean clobber Clean: clean $(LOCAL_DISTCLEAN)
	@rm -fv $(PS_TARGETS)
	@rm -fv $(PDF_TARGETS)

# Include dependency makefiles, provided that they are needed
ifneq ($(strip $(TARGETS)),)
  include $(patsubst %,$(DEPdir)%.d,$(TARGETS))
endif
ifneq ($(strip $(BIB_TARGETS)),)
  include $(patsubst %,$(DEPdir)%_d,$(BIB_TARGETS))
endif

# A target DVI file depends on:
#   1) the corresponding tex file (handled by the implicit rule)
#   2) any .tex files it imports through '\input{}'
# A target DVI file also depends on:
#   3) any .eps/png/pdf files it OR any of the .tex found by in (2) include
$(DEPdir)%.d: %.tex Makefile
	@echo Rebuilding dependencies: $<
	@if test ! -z $(DEPdir); then mkdir -p $(DEPdir); fi
	@echo "# Automatic dependencies for $<" > $@; \
         function process { \
	 echo "Processing $$1";\
         echo "# source files for $$1" >> $@; \
	 echo -n `echo "$$2 " | sed $(RE) 's/\.tex/.bbl/'` "" >> $@; \
	 echo -n `echo "$$2 " | sed $(RE) 's/\.tex/.$(TEX_OUT)/'` "" >> $@; \
	 echo $@: \\ >> $@; \
	 TEX=`grep '$(BACKREF)input{' $$1 | egrep -v '^\s*\%' | \
		sed $(RE) 's/.*input\{([^}]*).*/ \1 /g' | \
		sed $(RE) 's/ ([^\.]+) / \1.tex /g'`;\
	 echo "   "$$TEX >> $@; \
	 echo "# image files for $$1" >> $@; \
	 echo "$$2: \\" | sed $(RE) 's/\.tex/.$(TEX_OUT)/' >> $@; \
	 echo -n "   " >> $@; \
	 for f in $$1; do \
	   for ext in eps pdf png jpg jpeg yml yaml txt; do \
	     grep "\.$$ext" $$f | egrep -v '^\s*\%' | egrep '{[^}]*\.$$ext' | \
		egrep -v '://' | egrep -v '\\code' | \
		sed $(RE) "s/.*[{}= ]([^{}= ]+\.$$ext).*/\1/" | \
		sed $(RE) "s/\\\\fig(dir)?/$(FIGdir)/" | \
		xargs echo -n >> $@; \
           echo -n " " >> $@; \
           done; \
	 done; \
	 echo >> $@; \
	 if test -n "$$TEX"; then \
	   echo "# Recursive dependencies ($$1)" >> $@; \
	   for f in $$TEX; do process $$f $$2; done; \
	 fi; \
	 }; process $< $<

# A target BBL file depends on:
#   1) the corresponding tex file (handled by the implicit rule)
#   2) any .bib files it imports through '\bibliography{}'
$(DEPdir)%.bbl_d: %.tex Makefile
	@echo Rebuilding bibliography dependencies: $<
	@if test ! -z $(DEPdir); then mkdir -p $(DEPdir); fi
	@echo -n `echo "$< " | sed $(RE) 's/\.tex/.bbl/'` "" > $@; 
	@echo -n `echo "$< " | sed $(RE) 's/\.tex/.$(TEX_OUT)/'` "" >> $@; 
	@echo $@: \\ >> $@; 
	@BIB=`grep '$(BACKREF)bibliography{' $< | egrep -v '^\s*\%' | \
		sed $(RE) 's/.*bibliography\{([^{}]+)\}.*/\1 /g' | \
		sed $(RE) 's/,/ /g' | sed $(RE) 's/$(EOW).bib /g' | \
		sed $(RE) 's/(\.bib)+/\.bib/g'`; \
	 echo "   "$$BIB >> $@; 

#
# Update anchors
#
lstanchors.out:
	@echo Updating anchors
	@./lstanchors.pl *.tex > ./lstanchors.out

