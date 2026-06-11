#!/bin/sh

set -e

INFO_FILE="/tmp/coverage.info" 
REP_PATH="/tmp/coverage-report"

usage() {
	printf "usage: ${0} <test target> [objects to cover...]\n"
	exit 0
}

if [ $# -lt 1 ]; then
	usage
fi
TEST_TARGET="${1}"
shift

rm -f "${INFO_FILE}"
rm -rf "${REP_PATH}"

make clean

# build with coverage flag to generate .gcno
if [ $# -eq 1 ]; then

	# build all objects with coverage
	make CFLAGS="--coverage" all
else
	# build specified objects with coverage
	for obj in ${@}; do
		make CFLAGS="--coverage" "src/${obj}.o"
	done

	# remainder without coverage
	make all
fi

# execute test targets to generate .gcda
make LDFLAGS="--coverage" "${TEST_TARGET}"

lcov \
	--capture \
	--directory src \
	--output-file "${INFO_FILE}"

genhtml \
	"${INFO_FILE}" \
	--output-directory "${REP_PATH}"

xdg-open \
	"${REP_PATH}/index.html"
