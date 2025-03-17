include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_O = $(SRC_C:.c=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_E = $(patsubst tst/%.c,%,$(wildcard tst/tst-*.c))
TST_T = $(patsubst tst-%,test-%,$(TST_E))

all: $(SRC_O)

$(SRC_O): $(INC_H) config.mk GNUmakefile
$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile
$(TST_E): $(SRC_O) $(TST_O)
	$(CC) -o $(@) tst/$(@).o $(SRC_O) $(LDFLAGS) $(LDLIBS)

clean:
	rm -f $(SRC_O) $(TST_O) $(TST_E)

%-vg: VALGRIND = valgrind \
	--error-exitcode=1 \
	--leak-check=full \
	--show-leak-kinds=all \
	--errors-for-leak-kinds=all \
	--gen-suppressions=all
%-vg: % ;

test: $(TST_T)
test-vg: $(TST_T)

$(TST_T): $(TST_E)
	$(VALGRIND) ./$(patsubst test-%,tst-%,$(@))

IWYU = include-what-you-use \
	   -Xiwyu --no_fwd_decls \
	   -Xiwyu --error=1 \
	   -Xiwyu --verbose=3

iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h"
iwyu: clean $(SRC_O) $(TST_O)

cppcheck: $(INC_H) $(SRC_C) $(TST_H) $(TST_C)
	cppcheck $(^) \
		--enable=warning,unusedFunction,performance,portability \
		--check-level=exhaustive \
		--suppressions-list=.cppcheck.supp \
		--error-exitcode=1 \
		$(CPPFLAGS)

.PHONY: all clean test test-vg $(TST_T) iwyu cppcheck

.NOTPARALLEL: iwyu test test-vg
