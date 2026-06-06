include config.mk

INC_H = $(wildcard inc/*.h)

SRC_C = $(wildcard src/*.c)
SRC_O = $(SRC_C:.c=.o)

TST_H = $(wildcard tst/*.h)
TST_C = $(wildcard tst/*.c)
TST_O = $(TST_C:.c=.o)
TST_E = $(filter tst/tst%,$(TST_O:.o=))

UTL_O = $(filter-out tst/tst%,$(TST_O))

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

$(TST_E): $(SRC_O) $(UTL_O)

#
# link test-x targets to tst/tst-x and execute them
#
test: $(patsubst tst/tst%,test%,$(TST_E))
test-%: tst/tst-%
	./$(^)

test-vg: $(patsubst tst/tst%,test%-vg,$(TST_E))
test-%-vg: tst/tst-%
	$(VALGRIND) ./$(^)

#
# iwyu
#
iwyu: override CC = include-what-you-use \
	-Xiwyu --no_fwd_decls \
	-Xiwyu --error=1 \
	-Xiwyu --verbose=3 \
	-Xiwyu --mapping_file=.iwyu.imp \
	-Xiwyu --check_also="inc/*h" \
	-Xiwyu --check_also="tst/*h"
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
