#!/bin/sh

set -e

INFO_PATH="/tmp/coverage.info" 
REP_PATH="/tmp/coverage-report"

usage() {
	echo "usage: ${0} [target e.g. slist ...]"
	exit 0
}

if [ "${1}" = "-h" ]; then
	usage
fi

rm -rf "${REP_PATH}"
rm -rf "${INFO_PATH}"
mkdir "${INFO_PATH}"

make clean

# build with coverage flag to generate .gcno
make CC=gcc CFLAGS="--coverage -fcondition-coverage" all

if [ $# -gt 0 ]; then
	TESTS="${*}"
else
	for TEST_C in tst/tst-*c; do
		TESTS="${TESTS} $(echo "${TEST_C}" | sed -E 's/tst\/tst\-(.*)\.c/\1/g')"
	done
fi

for TEST in ${TESTS}; do

	# remove previous test execution
	rm -f src/*gcda

	# execute test target to generate .gcda
	make CC="gcc" LDFLAGS="--coverage" "test-${TEST}"

	# generate coverage info for the individual test
	geninfo \
		--test-name "tst_${TEST}" \
		--mcdc-coverage \
		--all \
		--output-file "${INFO_PATH}/${TEST}.info" \
		src
	:
done

# combined report for all coverage info
genhtml \
	--show-details \
	--mcdc-coverage \
	--show-proportion \
	--dark-mode \
	--num-spaces 4 \
	--flat \
	--rc genhtml_hi_limit=100 \
	--output-directory "${REP_PATH}" \
	${INFO_PATH}

xdg-open \
	"${REP_PATH}/index-detail.html"
