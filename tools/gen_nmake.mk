# Regenerate nmake file from makefiles or check its consistency

test_nmake_file: tst.nmake
	@diff -u Makefile.nmake tst.nmake || (echo Potential nmake consistency issue; $(RM) tst.nmake; false;)
	@echo No nmake consistency issues
	@$(RM) tst.nmake

FORCE:
Makefile.nmake tst.nmake: FORCE
	@echo Regenerating $@
	@echo '########################################################################' > $@
	@cat LICENSE | sed -e 's/^/#/ ' >> $@
	@echo '########################################################################' >> $@
	@echo ''			>> $@
	@echo '# This file can be auto-regenerated with $$make -f Makefile.unx Makefile.nmake' >> $@
	@echo ''			>> $@
	@echo -n 'objs =' >> $@
	@$(foreach o, $(subst /,\\,$(objs:.o=.obj)), printf " %s\n\t%s" \\ $(o) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'INCLUDES   = $(INCLUDE)'	>> $@
	@echo '# Modern asm feature level, consider upgrading nasm/yasm before decreasing feature_level'	>> $@
	@echo 'FEAT_FLAGS = -DHAVE_AS_KNOWS_AVX512 -DAS_FEATURE_LEVEL=10'	>> $@
	@echo 'CFLAGS_REL = -O2 -DNDEBUG /Z7 /Gy'		>> $@
	@echo 'CFLAGS_DBG = -Od -DDEBUG /Z7'			>> $@
	@echo 'LINKFLAGS  = -nologo -incremental:no -debug'	>> $@
	@echo 'CFLAGS     = $$(CFLAGS_REL) -nologo -D_USE_MATH_DEFINES $$(FEAT_FLAGS) $$(INCLUDES) $$(D)'	>> $@
	@echo 'AFLAGS     = -f win64 $$(FEAT_FLAGS) $$(INCLUDES) $$(D)'	>> $@
	@echo 'CC         = cl'			>> $@
	@echo '# or CC    = icl -Qstd=c99'	>> $@
	@echo 'AS         = nasm'		>> $@
	@echo ''			>> $@
	@echo 'lib: bin static dll'	>> $@
	@echo 'static: bin isa-l_static.lib isa-l.h'	>> $@
	@echo 'dll: bin isa-l.dll isa-l.h'	>> $@
	@echo ''			>> $@
	@echo 'bin: ; -mkdir $$@'	>> $@
	@echo ''			>> $@
	@echo 'isa-l_static.lib: $$(objs)'	>> $@
	@echo '	lib -out:$$@ @<<'	>> $@
	@echo '$$?'			>> $@
	@echo '<<'			>> $@
	@echo ''			>> $@
	@echo '!IF [rc] == 0'		>> $@
	@echo 'isa-l.dll: isa-l.res'	>> $@
	@echo '!ELSE'			>> $@
	@echo '!MESSAGE Optionally install rc to set file version info' >> $@
	@echo '!ENDIF'			>> $@
	@echo ''			>> $@
	@echo 'isa-l.dll: $$(objs)'	>> $@
	@echo '	link -out:$$@ -dll -def:isa-l.def $$(LINKFLAGS) @<<'	>> $@
	@echo '$$?'			>> $@
	@echo '<<'			>> $@
	@echo ''			>> $@
	@echo 'isa-l.res: isa-l.h'	>> $@
	@echo '	rc /fo $$@ isa-l.rc'	>> $@
	@echo ''			>> $@
	@$(foreach b, $(units), \
		printf "{%s}.c.obj:\n\t\$$(CC) \$$(CFLAGS) /c -Fo\$$@ \$$?\n{%s}.asm.obj:\n\t\$$(AS) \$$(AFLAGS) -o \$$@ \$$?\n\n" $(b) $(b) >> $@; )
	@echo ''			>> $@
ifneq (,$(examples))
	@echo "# Examples"	>> $@
	@echo -n 'ex =' >> $@
	@$(foreach ex, $(notdir $(examples)), printf " %s\n\t%s.exe" \\ $(ex) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@$(foreach d, $(subst /,\\, $(sort $(patsubst %/,%,$(filter examples/%,$(dir $(examples)))))), \
		printf "{%s}.c.obj:\n\t\$$(CC) \$$(CFLAGS) /c -Fo\$$@ \$$?\n\n" $(d) >> $@; )
	@echo 'ex: lib $$(ex)'		>> $@
	@echo ''			>> $@
	@echo '$$(ex): $$(@B).obj'	>> $@
endif
	@echo ''			>> $@
	@echo '.obj.exe:'		>> $@
	@echo '	link /out:$$@ $$(LINKFLAGS) isa-l.lib $$?'	>> $@
	@echo ''			>> $@
	@echo '# Check tests'		>> $@
	@echo -n 'checks =' 		>> $@
	@$(foreach check, $(notdir $(check_tests)), printf " %s\n\t%s.exe" \\ $(check) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'checks: lib $$(checks)'	>> $@
	@echo '$$(checks): $$(@B).obj'	>> $@
	@echo 'check: $$(checks)'	>> $@
	@echo '	!$$?'			>> $@
	@echo ''			>> $@
	@echo '# Unit tests'		>> $@
	@echo -n 'tests =' 		>> $@
	@$(foreach test, $(notdir $(unit_tests)), printf " %s\n\t%s.exe" \\ $(test) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'tests: lib $$(tests)'	>> $@
	@echo '$$(tests): $$(@B).obj'	>> $@
	@echo ''			>> $@
	@echo '# Performance tests'	>> $@
	@echo -n 'perfs =' 		>> $@
	@$(foreach perf, $(notdir $(perf_tests)), printf " %s\n\t%s.exe" \\ $(perf) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'perfs: lib $$(perfs)'	>> $@
	@echo '$$(perfs): $$(@B).obj'	>> $@
	@echo ''			>> $@
	@echo -n 'progs ='		>> $@
	@$(foreach prog, $(notdir $(bin_PROGRAMS)), printf " %s\n\t%s.exe" \\ $(prog) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'progs: lib $$(progs)'	>> $@
	@$(foreach p, $(notdir $(bin_PROGRAMS)), \
		printf "%s.exe: %s\n\tlink /out:\$$@ \$$(LINKFLAGS) isa-l.lib \$$?\n" $(p) $(subst /,\\,$(programs_$(p)_SOURCES:.c=.obj)) >> $@; )
	@echo ''			>> $@
	@echo 'isa-l.h:'		>> $@
	@echo '	@echo /**>> $$@' >> $@
	@echo '	@echo *  @file isa-l.h>> $$@' >> $@
	@echo '	@echo *  @brief Include for ISA-L library>> $$@' >> $@
	@echo '	@echo */>> $$@' >> $@
	@echo '	@echo.>> $$@' >> $@
	@echo '	@echo #ifndef _ISAL_H_>> $$@' >> $@
	@echo '	@echo #define _ISAL_H_>> $$@' >> $@
	@echo '	@echo.>> $$@' >> $@
	@echo '#define.ISAL_MAJOR_VERSION.${version}' |  ${AWK} -F . '{print "\t@echo", $$1, $$2, $$3, ">> $$@"}' >> $@
	@echo '#define.ISAL_MINOR_VERSION.${version}' |  ${AWK} -F . '{print "\t@echo", $$1, $$2, $$4, ">> $$@"}' >> $@
	@echo '#define.ISAL_PATCH_VERSION.${version}' |  ${AWK} -F . '{print "\t@echo", $$1, $$2, $$5, ">> $$@"}' >> $@
	@echo '	@echo #define ISAL_MAKE_VERSION(maj, min, patch)  ((maj) * 0x10000 + (min) * 0x100 + (patch))>> $$@' >> $@
	@echo '	@echo #define ISAL_VERSION ISAL_MAKE_VERSION(ISAL_MAJOR_VERSION, ISAL_MINOR_VERSION, ISAL_PATCH_VERSION)>> $$@' >> $@
	@echo '	@echo.>> $$@' >> $@
	@echo '	@echo #ifndef RC_INVOKED>> $$@' >> $@
	@for unit in $(sort $(extern_hdrs)); do echo "	@echo #include ^<isa-l/$$unit^>>> \$$@" | sed -e 's;include/;;' >> $@; done
	@echo '	@echo #endif // RC_INVOKED>> $$@' >> $@
	@echo '	@echo #endif //_ISAL_H_>> $$@' >> $@
	@echo ''			>> $@
	@echo 'clean:'					>> $@
	@echo '	-if exist *.obj del *.obj'		>> $@
	@echo '	-if exist bin\*.obj del bin\*.obj'	>> $@
	@echo '	-if exist isa-l_static.lib del isa-l_static.lib'	>> $@
	@echo '	-if exist *.exe del *.exe'		>> $@
	@echo '	-if exist *.pdb del *.pdb'		>> $@
	@echo '	-if exist isa-l.lib del isa-l.lib'	>> $@
	@echo '	-if exist isa-l.dll del isa-l.dll'	>> $@
	@echo '	-if exist isa-l.exp del isa-l.exp'	>> $@
	@echo '	-if exist isa-l.res del isa-l.res'	>> $@
	@echo ''		>> $@
	$(if $(findstring igzip,$(units)),@echo 'zlib.lib:'	>> $@ )
	@cat $(foreach unit,$(units), $(unit)/Makefile.am)  | sed  \
		-e '/: /!d' \
		-e 's/\([^ :]*\)[ ]*/\1.exe /g' \
		-e :c -e 's/:\(.*\).exe/:\1/;tc' \
		-e 's/\.o[ $$]/.obj /g' \
		-e 's/\.o\.exe[ ]:/.obj:/g' \
		-e '/CFLAGS_.*+=/d' \
		-e '/:.*\%.*:/d' \
		-e 's/ :/:/' \
		-e 's/LDLIBS *+=//' \
		-e 's/-lz/zlib.lib/' \
		-e 's/ $$//' \
			>> $@
