# Build examples from installed lib

examples = crc_combine_example

CFLAGS = -Wall -O2
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
