include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_O = $(SRC_C:.c=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_U = $(filter-out tst/tst%,$(TST_O))
TST_E = $(filter tst/tst%,$(TST_O:.o=))

all: $(SRC_O)

clean:
	rm -f $(SRC_O) $(TST_O) $(TST_E)

#
# lib
#
$(SRC_O): $(INC_H) config.mk GNUmakefile

#
# test
#
$(TST_O): $(TST_H) $(SRC_O) config.mk GNUmakefile

$(TST_E): $(SRC_O) $(TST_U)

test-%: tst/tst-%
	./$(^)

test-%-vg: tst/tst-%
	$(VALGRIND) ./$(^)

test: $(patsubst tst/tst%,test%,$(TST_E))

test-vg: $(patsubst tst/tst%,test%-vg,$(TST_E))

#
# valgrind
#
VALGRIND = valgrind \
		   --error-exitcode=1 \
		   --leak-check=full \
		   --show-leak-kinds=all \
		   --errors-for-leak-kinds=all \
		   $(VG_SUPP) \
		   --gen-suppressions=all

#
# iwyu
#
IWYU = include-what-you-use \
	   -Xiwyu --no_fwd_decls \
	   -Xiwyu --error=1 \
	   -Xiwyu --verbose=3

iwyu: CC = $(IWYU) -Xiwyu --check_also="inc/*h" -Xiwyu --check_also="tst/*h"
iwyu: clean $(SRC_O) $(TST_O)

#
# cppcheck
#
cppcheck: $(INC_H) $(SRC_C) $(TST_H) $(TST_C)
	cppcheck $(^) \
		--enable=warning,unusedFunction,performance,portability,style \
		--check-level=exhaustive \
		--suppressions-list=.cppcheck.supp \
		--error-exitcode=1 \
		$(CPPFLAGS)

.PHONY: all clean test test-vg iwyu cppcheck

.NOTPARALLEL: iwyu test test-vg
