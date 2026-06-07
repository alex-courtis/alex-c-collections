#include "tst.h"
#include "asserts.h"
#include "assert-ptable.h"
#include "expects.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "str.h"

#include "ptable.h"

/* ptable pointer specific smoke tests only, tst-stable fully tests ptable */

static void *k0 = NULL, *k1 = NULL, *k2 = NULL;
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

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void mock_free_val(const void* const val) {
	check_expected_ptr(val);
}

// put, get, remove, free_vals
static void ptable_smoke(void **state) {

	const struct PTable *actual = ptable_init();
	assert_nul(ptable_put(actual, &k0, &v0));
	assert_nul(ptable_put(actual, &k1, &v1));
	assert_nul(ptable_put(actual, &k2, &v2));

	assert_ptr_equal(ptable_get(actual, &k0), &v0);

	void *foo = NULL;
	assert_nul(ptable_get(actual, &foo));

	assert_ptr_equal(ptable_remove(actual, &k0), &v0);

	assert_nul(ptable_get(actual, &k0));

	expect_str(mock_free_val, val, &v0);
	expect_str(mock_free_val, val, &v1);

	ptable_free_vals(actual, mock_free_val);
}

static void ptable_equal__(void **state) {

	const struct PTable *actual = ptable_init();
	assert_nul(ptable_put(actual, &k0, &v0));
	assert_nul(ptable_put(actual, &k1, &v1));

	const struct PTable *expected = ptable_init();
	assert_nul(ptable_put(expected, &k0, &v0));
	assert_nul(ptable_put(expected, &k1, &v1));

	assert_ptable_equal(actual, expected, NULL, NULL);

	ptable_free(actual);
	ptable_free(expected);
}

static void ptable_str__(void **state) {

	const struct PTable *tab = ptable_init();
	assert_nul(ptable_put(tab, &k0, "000"));
	assert_nul(ptable_put(tab, &k1, "111"));

	char *expected = sprintf_alloc(
			"%p = 0\n"
			"%p = 1\n",
			(void*)&k0,
			(void*)&k1
			);

	char *actual = ptable_str(tab, fn_str_first);
	assert_str_equal(expected, actual);

	free(actual);
	free(expected);
	ptable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ptable_smoke),
		TEST(ptable_equal__),
		TEST(ptable_str__),
	};

	return RUN(tests);
}

