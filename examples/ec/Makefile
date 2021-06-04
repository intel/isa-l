# Build examples from installed lib

include Makefile.am

incdir = $(shell pkg-config --variable=includedir libisal)

CFLAGS += -include $(incdir)/isa-l.h # Add standard header
CFLAGS += -I $(incdir)/isa-l # Add path to remove error
LDFLAGS = $(shell pkg-config --libs libisal)

progs = $(notdir $(examples))

ex: $(progs)
run: $(addsuffix .run,$(progs))

$(progs): % : %.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(addsuffix .run,$(progs)): %.run : %
	./$<
	@echo Completed run: $<

clean:
	$(RM) $(progs)
