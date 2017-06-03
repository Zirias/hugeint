POCASTEST?= pocastest
PREPROC_POCASTEST_suffix := test
PREPROC_POCASTEST_intype := c
PREPROC_POCASTEST_outtype := c
PREPROC_POCASTEST_preproc = $(POCASTEST) -p

hugeint-tests_MODULES:= hugeint
hugeint-tests_PREPROCMODULES:= hugeint
hugeint-tests_LIBTYPE:= test
hugeint-tests_DEPS:= hugeint
hugeint-tests_LIBS:= hugeint
hugeint-tests_win32_LIBS:= pocastest
hugeint-tests_PREPROC:= POCASTEST
$(call librules,hugeint-tests)
