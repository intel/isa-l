FORCE:
Makefile.nmake: FORCE
	@echo Regenerating $@
	@echo '########################################################################' > $@
	@cat LICENSE | sed -e 's/^/#/ ' >> $@
	@echo '########################################################################' >> $@
	@echo ''			>> $@
	@echo '# This file can be auto-regenerated with $$make -f Makefile.unx $@' >> $@
	@echo ''			>> $@
	@echo -n 'objs =' >> $@
	@$(foreach o, $(subst /,\\,$(objs:.o=.obj)), printf " %s\n\t%s" \\ $(o) >> $@; )
	@echo ''			>> $@
	@echo ''			>> $@
	@echo 'INCLUDES  = $(INCLUDE)'	>> $@
	@echo 'LINKFLAGS = /nologo'	>> $@
	@echo 'CFLAGS   = -O2 -D NDEBUG /nologo -D_USE_MATH_DEFINES -Qstd=c99 $$(INCLUDES) $$(D)'	>> $@
	@echo 'AFLAGS   = -f win64 $$(INCLUDES) $$(D)'	>> $@
	@echo 'CC       = icl'		>> $@
	@echo 'AS       = yasm'		>> $@
	@echo ''			>> $@
	@echo 'lib: bin static dll'	>> $@
	@echo 'static: bin isa-l_static.lib'	>> $@
	@echo 'dll: bin isa-l.dll'	>> $@
	@echo ''			>> $@
	@echo 'bin: ; -mkdir $$@'	>> $@
	@echo ''			>> $@
	@echo 'isa-l_static.lib: $$(objs)'	>> $@
	@echo '	lib -out:$$@ @<<'	>> $@
	@echo '$$?'			>> $@
	@echo '<<'			>> $@
	@echo ''			>> $@
	@echo 'isa-l.dll: $$(objs)'	>> $@
	@echo '	link -out:$$@ -dll -def:isa-l.def @<<'	>> $@
	@echo '$$?'			>> $@
	@echo '<<'			>> $@
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
	@echo 'clean:'					>> $@
	@echo '	-if exist *.obj del *.obj'		>> $@
	@echo '	-if exist bin\*.obj del bin\*.obj'	>> $@
	@echo '	-if exist isa-l_static.lib del isa-l_static.lib'	>> $@
	@echo '	-if exist *.exe del *.exe'		>> $@
	@echo '	-if exist isa-l.lib del isa-l.lib'	>> $@
	@echo '	-if exist isa-l.dll del isa-l.dll'	>> $@
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
