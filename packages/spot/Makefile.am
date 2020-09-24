SUBDIRS=. rounding grasp newgrasp

GMP_BUILD_DIR=../@gmp@
GMP_SOURCE_DIR=@top_srcdir@/packages/@gmp@

GMPCONFIGURE=$(GMP_SOURCE_DIR)/configure

if HAVE_SYSTEM_32
  GMP_SPECIAL="ABI=32"
endif

if GMP_NOT_CONFIGURED
  BUILD_GMP=cd $(GMP_BUILD_DIR) && @EXPORT_FLAGS@ && $(GMPCONFIGURE) $(GMP_SPECIAL) @ac_configure_args@ && $(MAKE) all
else
  BUILD_GMP=cd $(GMP_BUILD_DIR) && $(MAKE) all
endif

all-local:
if BUILD_SNLROUND
	@if test -d $(GMP_BUILD_DIR) ; then : ; else mkdir $(GMP_BUILD_DIR) ; fi
	@echo Entering directory $(GMP_BUILD_DIR)
	@$(BUILD_GMP)
endif

clean-local:
if BUILD_SNLROUND
	@if test -d $(GMP_BUILD_DIR) ; then cd $(GMP_BUILD_DIR) && $(MAKE) clean ; fi
endif


