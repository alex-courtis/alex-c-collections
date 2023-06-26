#ifndef ASSERTS_H
#define ASSERTS_H

#include <cmocka.h>
#include <stdio.h>
#include <string.h>

static char FMT[1024];

void _assert_string_n_equal(const char *a, const char *b, const size_t n, const char * const file, const int line) {
	if (strncmp(a, b, n) != 0) {
		snprintf(FMT, sizeof(FMT), "\"%%.%zus\" != \"%%.%zus\"\n", n, n);
		cm_print_error(FMT, a, b);
		_fail(file, line);
	}
}
#define assert_string_n_equal(a, b, n) _assert_string_n_equal(a, b, n, __FILE__, __LINE__)

#endif // ASSERTS_H
