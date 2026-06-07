#ifndef ASSERT_PSET_H
#define ASSERT_PSET_H

#include <cmocka.h>

#include "util-file.h"

#include "fn.h"
#include "pset.h"

void _assert_pset_equal(const struct PSet *a, const struct PSet *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (!pset_equal(a, b, equal)) {
		write_file("actual.pset", pset_str(a, str));
		write_file("expected.pset", pset_str(b, str));
		cmocka_print_error("\n%s != \n%s", pset_str(a, str), pset_str(b, str));
		_fail(file, line);
	}
}
#define assert_pset_equal(a, b, equal, str) _assert_pset_equal(a, b, equal, str, __FILE__, __LINE__)

void _assert_pset_not_equal(const struct PSet *a, const struct PSet *b, fn_equal equal, fn_str str, const char * const file, const int line) {
	if (pset_equal(a, b, equal)) {
		write_file("actual.pset", pset_str(a, str));
		write_file("expected.pset", pset_str(b, str));
		cmocka_print_error("\n%s == \n%s", pset_str(a, str), pset_str(b, str));
		_fail(file, line);
	}
}
#define assert_pset_not_equal(a, b, equal, str) _assert_pset_not_equal(a, b, equal, str, __FILE__, __LINE__)

#endif // ASSERT_PSET_H
