########################################################################
#  Copyright(c) 2011-2015 Intel Corporation All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
########################################################################


# Makefile include for optimized libraries
#  make targets:
#	lib  - build library of optimized functions
#	slib - build shared library
#	test - run unit tests of functions
#	perf - run performance tests
#	install - install headers and libs to system location
#	sim  - run on simulator
#	trace - get simulator trace
#	clean - remove object files

version ?= 2.23.0
host_cpu ?= $(shell uname -m | sed -e 's/amd/x86_/')
arch ?= $(shell uname | grep -v -e Linux -e BSD )

CC  = gcc
AS  = yasm
SIM = sde $(SIMFLAGS) --
AWK = awk

DEBUG      = -g
DEBUG_yasm = -g dwarf2
DEBUG_nasm = -g

# Default arch= build options
CFLAGS_    = -Wall
ASFLAGS_   = -f elf64
ARFLAGS_   = cr $@
STRIP_gcc  = strip -d -R .comment $@

# arch=32 build options
ASFLAGS_32 = -f elf32
CFLAGS_32  = -m32
ARFLAGS_32 = cr $@

# arch=win64 build options
ASFLAGS_win64 = -f win64
CFLAGS_icl    = -Qstd=c99
ARFLAGS_win64 = -out:$@

# arch=mingw build options
ASFLAGS_mingw = -f win64
ARFLAGS_mingw = cr $@
LDFLAGS_mingw = -Wl,--force-exe-suffix

LDFLAGS_so = -Wl,-soname,$(soname)

ifeq ($(arch),mingw)
  CC=x86_64-w64-mingw32-gcc
  AR=x86_64-w64-mingw32-ar
endif

# arch=noarch build options
ARFLAGS_noarch = cr $@
ifeq ($(arch),noarch)
  host_cpu=base_aliases
endif

ASFLAGS_Darwin = -f macho64 --prefix=_
ARFLAGS_Darwin = -r $@
ifeq ($(shell uname),Darwin)
  LDFLAGS_so =
  STRIP_gcc  =
endif

INCLUDE   = $(patsubst %,-I%/,$(subst :, ,$(VPATH)))
CFLAGS   = $(CFLAGS_$(arch)) $(CFLAGS_$(CC)) $(DEBUG) -O2 $(DEFINES) $(INCLUDE)
ASFLAGS  = $(ASFLAGS_$(arch)) $(ASFLAGS_$(CC)) $(DEBUG_$(AS)) $(DEFINES) $(INCLUDE)
ARFLAGS  = $(ARFLAGS_$(arch))
DEFINES += $(addprefix -D , $D)

lsrc += $(lsrc_$(host_cpu))
O = bin
lobj  += $(patsubst %.c,%.o,$(patsubst %.asm,%.o,$(lsrc) $(lsrc_intrinsic)))
objs  = $(addprefix $(O)/,$(notdir $(lobj)))


lib_name ?= isa-l.a
default: lib slib

# Defaults for windows build
ifeq ($(arch),win64)
  AR=lib
  CC=cl
  OUTPUT_OPTION = -Fo$@
  DEBUG=
  lib_name := $(basename $(lib_name)).lib
endif
lsrcwin64 = $(lsrc)
unit_testswin64 = $(unit_tests)
exampleswin64 = $(examples)
perf_testswin64 = $(perf_tests)

# Build and run unit tests, performance tests, etc.
all_tests = $(notdir $(sort $(perf_tests) $(check_tests) $(unit_tests) $(examples) $(other_tests)))
all_unit_tests = $(notdir $(sort $(check_tests) $(unit_tests)))
all_perf_tests = $(notdir $(sort $(perf_tests)))
all_check_tests = $(notdir $(sort $(check_tests)))
all_llvm_fuzz_tests = $(notdir $(sort $(llvm_fuzz_tests)))

$(all_unit_tests): % : %.c $(lib_name)
$(all_perf_tests): % : %.c $(lib_name)
$(sort $(notdir $(examples))): % : %.c $(lib_name)
$(sort $(notdir $(other_tests))): % : %.c $(lib_name)

$(all_llvm_fuzz_tests): FUZZLINK = -lFuzzer
$(all_llvm_fuzz_tests): CFLAGS += -fsanitize-coverage=trace-pc-guard -fsanitize=address
$(all_llvm_fuzz_tests): CXXFLAGS += -fsanitize-coverage=trace-pc-guard -fsanitize=address
$(all_llvm_fuzz_tests): % : %.o $(lib_name)
	$(CXX) $(CXXFLAGS)  $^ $(LDLIBS) $(FUZZLINK) -o $@


sim test trace: $(addsuffix .run,$(all_unit_tests))
perf: $(addsuffix .run,$(all_perf_tests))
check: $(addsuffix .run,$(all_check_tests))
ex:   $(notdir $(examples))
all: lib $(all_tests)
other: $(notdir $(other_tests))
llvm_fuzz_tests: $(all_llvm_fuzz_tests)
tests: $(all_unit_tests)
perfs: $(all_perf_tests)
checks: $(all_check_tests)
check test perf: SIM=
trace: SIMFLAGS = -debugtrace
check test sim:
	@echo Finished running $@

$(objs): | $(O)
$(O): ; mkdir -p $(O)

# Build rule to run tests
$(addsuffix .run,$(all_tests)): %.run : %
	$(SIM) ./$<
	@echo Completed run: $<

# Other build rules
msg = $(if $(DEBUG),DEBUG) $(patsubst 32,32-bit,$(host_cpu)) $D

$(O)/%.o: %.asm
	@echo "  ---> Building $< $(msg)"
	@$(AS) $(ASFLAGS) -o $@ $<

$(O)/%.o %.o: %.c
	@echo "  ---> Building $< $(msg)"
	@$(COMPILE.c) $(OUTPUT_OPTION) $<

$(all_tests):
	@echo "  ---> Building Test $@ $(msg)"
	@$(LINK.o) $(CFLAGS)  $^ $(LDLIBS) -o $@


# Target to build lib files
lib: $(lib_name)
ifneq ($(lib_debug),1)
 $(lib_name): DEBUG_$(AS)=	# Don't put debug symbols in the lib
 $(lib_name): DEBUG=
 $(lib_name): DEFINES+=-D NDEBUG
endif
ifeq ($(lib_debug),1)
 DEBUG+=-D DEBUG   # Define DEBUG for macros
endif

#lib $(lib_name): $(lib_name)(${objs})
$(lib_name): $(objs)
	@echo "  ---> Creating Lib $@"
	@$(AR) $(ARFLAGS) $^
	@$(STRIP_$(CC))


# Target for shared lib
so_lib_name = bin/libisal.so
so_lib_inst = $(notdir $(so_lib_name))
so_lib_ver  = $(so_lib_inst).$(version)
soname      = $(so_lib_inst).$(word 1, $(subst ., ,$(version)))

slib: $(so_lib_name)
aobjs  += $(addprefix $(O)/,$(patsubst %.asm,%.o,$(filter %.asm,$(notdir $(lsrc) $(lsrc_intrinsic)))))
shared_objs  += $(addprefix $(O)/shared_ver_,$(patsubst %.c,%.o,$(filter %.c,$(notdir $(lsrc) $(lsrc_intrinsic)))))

$(O)/shared_ver_%.o: %.c
	@echo "  ---> Building shared $< $(msg)"
	@$(COMPILE.c) $(OUTPUT_OPTION) $<

ifneq ($(lib_debug),1)
 $(so_lib_name): DEBUG_$(AS)=
 $(so_lib_name): DEBUG=
 $(so_lib_name): DEFINES+=-D NDEBUG
endif

$(shared_objs): CFLAGS += -fPIC
$(shared_objs) $(aobjs): | $(O)
$(so_lib_name): LDFLAGS+=$(LDFLAGS_so)
$(so_lib_name): $(shared_objs) $(aobjs)
	@echo "  ---> Creating Shared Lib $@"
	@$(CC) $(CFLAGS) --shared  $(LDFLAGS) -o $@ $^
	@(cd $(@D); ln -f -s $(so_lib_inst) $(soname))


isa-l.h:
	@echo 'Building $@'
	@echo ''			>> $@
	@echo '/**'                     >> $@
	@echo ' *  @file isa-l.h'       >> $@
	@echo ' *  @brief Include for ISA-L library' >> $@
	@echo ' */'                     >> $@
	@echo ''                        >> $@
	@echo '#ifndef _ISAL_H_'	>> $@
	@echo '#define _ISAL_H_'	>> $@
	@echo ''			>> $@
	@echo '#define.ISAL_MAJOR_VERSION.${version}' |  ${AWK} -F . '{print $$1, $$2, $$3}' >> $@
	@echo '#define.ISAL_MINOR_VERSION.${version}' |  ${AWK} -F . '{print $$1, $$2, $$4}' >> $@
	@echo '#define.ISAL_PATCH_VERSION.${version}' |  ${AWK} -F . '{print $$1, $$2, $$5}' >> $@
	@echo '#define ISAL_MAKE_VERSION(maj, min, patch)  ((maj) * 0x10000 + (min) * 0x100 + (patch))' >> $@
	@echo '#define ISAL_VERSION ISAL_MAKE_VERSION(ISAL_MAJOR_VERSION, ISAL_MINOR_VERSION, ISAL_PATCH_VERSION)' >> $@
	@echo ''			>> $@
	@for unit in $(sort $(extern_hdrs)); do echo "#include <isa-l/$$unit>" | sed -e 's;include/;;' >> $@; done
	@echo '#endif //_ISAL_H_'	>> $@


# Target for install
prefix = /usr/local
install_dirs = $(prefix)/lib $(prefix)/include/isa-l
$(install_dirs): ; mkdir -p $@
install: $(sort $(extern_hdrs)) | $(install_dirs) $(lib_name) $(so_lib_name) isa-l.h
	install -m 644 $(lib_name) $(prefix)/lib/libisal.a
	install -m 644 $^ $(prefix)/include/isa-l/.
	install -m 664 isa-l.h $(prefix)/include/.
	install -m 664 include/types.h $(prefix)/include/isa-l/.
	install -m 664 $(so_lib_name) $(prefix)/lib/$(so_lib_ver)
	(cd $(prefix)/lib && ln -f -s $(so_lib_ver) $(soname) && ln -f -s $(so_lib_ver) $(so_lib_inst))
ifeq ($(shell uname),Darwin)
	(cd $(prefix)/lib && ln -f -s $(so_lib_ver) $(basename $(so_lib_inst)).dylib)
	which glibtool && glibtool --mode=finish $(prefix)/lib
else
	which libtool && libtool --mode=finish $(prefix)/lib || \
	echo 'Lib installed at $(prefix)/lib. Run system-dependent programs to add shared lib path.'
endif

uninstall:
	$(RM) $(prefix)/lib/libisal.a
	$(RM) $(prefix)/lib/$(soname)
	$(RM) $(prefix)/lib/$(so_lib_ver)
	$(RM) $(prefix)/lib/$(so_lib_inst)
	$(RM) -r $(prefix)/include/isa-l
	$(RM) $(prefix)/include/isa-l.h
	$(RM) $(prefix)/lib/$(basename $(so_lib_inst)).dylib

# Collect performance data
rpt_name = perf_report_$(shell uname -n)_$(shell date +%y%m%d).perf

perf_report:
	echo Results for $(rpt_name) >> $(rpt_name)
	$(MAKE) -f Makefile.unx -k perf | tee -a $(rpt_name)
	@echo Summary:
	-grep runtime $(rpt_name)


clean:
	@echo Cleaning up
	@$(RM) -r $(O) *.o *.a $(all_tests) $(lib_name) $(so_lib_name) $(all_llvm_fuzz_tests)


doc: isa-l.h
	(cat Doxyfile; echo 'PROJECT_NUMBER=$(version)') | doxygen -
	$(MAKE) -C generated_doc/latex &> generated_doc/latex_build_api.log
	cp generated_doc/latex/refman.pdf isa-l_api_$(version).pdf

