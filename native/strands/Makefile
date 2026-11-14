CC ?= cc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Werror -pedantic
LDLIBS ?= -lcrypto
all: test
ib_strands.o: ib_strands.c ib_strands.h
	$(CC) $(CFLAGS) -c ib_strands.c -o $@
test: ib_strands.o ../../tests/test_strands.c
	$(CC) $(CFLAGS) -I. ../../tests/test_strands.c ib_strands.o $(LDLIBS) -o $@
check: test
	./test
clean:
	rm -f test ib_strands.o
.PHONY: all check clean
