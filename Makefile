CC = gcc
CFLAGS = -I./include -Wall -Wextra -Werror -Wformat=2 -Wundef -Wshadow -Wpointer-arith -Wcast-align -Wstrict-prototypes -Wstrict-overflow=4
PRJ = my_secmalloc
OBJS = src/my_secmalloc.o src/utils.o
SLIB = lib${PRJ}.a
LIB = lib${PRJ}.so
GCOVFLAGS = --coverage

all: ${LIB}

${LIB} : CFLAGS += -fpic -shared
${LIB} : ${OBJS}

${SLIB}: ${OBJS}

dynamic: CFLAGS += -DDYNAMIC
dynamic: distclean ${LIB}

static: ${SLIB}

my_sec:
	${CC} ${CFLAGS} -o my_sec src/my_secmalloc.c

clean:
	${RM} src/.*.swp src/*~ src/*.o test/*.o

distclean: clean
	${RM} ${SLIB} ${LIB}

build_test: CFLAGS += -DTEST ${GCOVFLAGS}
build_test: ${OBJS} test/test.o
	$(CC) -o test/test $^ -lcriterion -Llib -lgcov

test: build_test
	LD_LIBRARY_PATH=./lib test/test

coverage: test
	lcov --capture --directory . --output-file coverage.info
	genhtml coverage.info --output-directory out

.PHONY: all clean build_test dynamic test static distclean coverage

%.so:
	$(LINK.c) -shared $^ $(LDLIBS) -o $@

%.a:
	${AR} ${ARFLAGS} $@ $^
