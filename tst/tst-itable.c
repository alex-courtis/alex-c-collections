#include "tst.h"
#include "asserts.h"
#include "assert-itable.h"
#include "expects.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

#include "itable.h"

/* itable specific smoke tests only, tst-stable fully tests itable */

static void *v0 = NULL, *v1 = NULL, *v2 = NULL;

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	return 0;
}

static int after_each(void **state) {
	return 0;
}

static void mock_free_val(const void* const val) {
	check_expected_ptr(val);
}

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

// put, get, remove, free_vals
static void itable_smoke(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, &v0));
	assert_nul(itable_put(actual, 10, &v1));
	assert_nul(itable_put(actual, 20, &v2));

	assert_ptr_equal(itable_get(actual, 10), &v1);

	assert_nul(itable_get(actual, 999));

	assert_ptr_equal(itable_remove(actual, 10), &v1);

	assert_nul(itable_get(actual, 10));

	expect_str(mock_free_val, val, &v0);
	expect_str(mock_free_val, val, &v1);

	itable_free_vals(actual, mock_free_val);
}

static void itable_iter__(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, &v0));
	assert_nul(itable_put(tab, 10, &v1));

	const struct ITableIter *iter = itable_iter(tab);

	assert_non_nul(iter);
	assert_int_equal(itable_iter_key(iter), 0);
	assert_ptr_equal(itable_iter_val(iter), &v0);

	iter = itable_iter_next(iter);
	assert_non_nul(iter);
	assert_int_equal(itable_iter_key(iter), 10);
	assert_ptr_equal(itable_iter_val(iter), &v1);

	assert_nul(itable_iter_next(iter));

	itable_free(tab);
}

static void itable_equal__(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, &v0));
	assert_nul(itable_put(actual, 1, &v1));

	const struct ITable *expected = itable_init();
	assert_nul(itable_put(expected, 0, &v0));
	assert_nul(itable_put(expected, 1, &v1));

	assert_itable_equal(actual, expected, NULL, NULL);

	itable_free(actual);
	itable_free(expected);
}

static void itable_str__(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, "000"));
	assert_nul(itable_put(tab, 999, "111"));

	char *expected = sprintf_alloc(
			"0 = 0\n"
			"999 = 1\n"
			);

	char *actual = itable_str(tab, fn_str_first);
	assert_str_equal(expected, actual);

	free(actual);
	free(expected);
	itable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_smoke),

		TEST(itable_iter__),

		TEST(itable_equal__),
		TEST(itable_str__),
	};

	return RUN(tests);
}

