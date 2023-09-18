include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_O = $(SRC_C:.c=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_E = $(patsubst tst/%.c,%,$(wildcard tst/tst-*.c))

all: test

$(SRC_O): $(INC_H) config.mk GNUmakefile
$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile
$(TST_E): $(SRC_O) $(TST_O)
	$(CC) -o $(@) tst/$(@).o $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(SRC_O) $(TST_O) $(TST_E)

test: $(TST_E)
	@for e in $(^); do \
		echo ;\
		echo $$e ;\
		$(TST_WITH) ./$$e ;\
		if [ $$? -ne 0 ]; then \
		exit 1 ;\
		fi ;\
		done

valgrind: TST_WITH = valgrind --leak-check=full --show-leak-kinds=all
valgrind: test

IWYU = include-what-you-use -Xiwyu --no_fwd_decls -Xiwyu --error=1 -Xiwyu --verbose=3
iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
iwyu: clean $(SRC_O) $(TST_O)

cppcheck: $(INC_H) $(SRC_C) $(TST_H) $(TST_C)
	cppcheck $(^) --enable=warning,unusedFunction,performance,portability --suppressions-list=.cppcheck.supp $(CPPFLAGS)

.PHONY: all clean test valgrind iwyu cppcheck

