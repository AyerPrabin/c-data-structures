CC ?= cc
CFLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O2 -g
SAN_FLAGS = -std=c11 -Wall -Wextra -Werror -pedantic -O1 -g \
	-fsanitize=address,undefined -fno-sanitize-recover=undefined \
	-fno-omit-frame-pointer

SRC = $(wildcard src/*.c)
HDR = $(wildcard src/*.h)
TEST_SRC = $(wildcard tests/*.c)

.PHONY: all test bench sanitize valgrind clean

all: test_ds bench_ds

test_ds: $(SRC) $(HDR) $(TEST_SRC) tests/check.h
	$(CC) $(CFLAGS) -o $@ $(TEST_SRC) $(SRC)

# Separate binary name so the sanitizer build never gets run under Valgrind.
test_ds_san: $(SRC) $(HDR) $(TEST_SRC) tests/check.h
	$(CC) $(SAN_FLAGS) -o $@ $(TEST_SRC) $(SRC)

bench_ds: $(SRC) $(HDR) bench/bench.c
	$(CC) $(CFLAGS) -o $@ bench/bench.c $(SRC)

test: test_ds
	./test_ds

bench: bench_ds
	./bench_ds

sanitize: test_ds_san
	./test_ds_san

valgrind: test_ds
	valgrind --leak-check=full --show-leak-kinds=all --error-exitcode=1 ./test_ds

clean:
	rm -f test_ds test_ds_san bench_ds *.exe
