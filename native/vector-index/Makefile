CC ?= cc
CFLAGS ?= -O2
CPPFLAGS ?=
WARNINGS = -Wall -Wextra -Werror -Wpedantic

.PHONY: all clean test

all: ib-vector-index

ib-vector-index: ib_vector_index.c
	$(CC) $(CPPFLAGS) $(CFLAGS) $(WARNINGS) -std=c99 $< -lm -o $@

test: ib-vector-index
	sh ../../tests/vector-index-smoke.sh ./ib-vector-index

clean:
	rm -f ib-vector-index
