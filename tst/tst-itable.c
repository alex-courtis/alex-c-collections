#include "tst.h"
#include "asserts.h"
#include "assert-itable.h"

#include <cmocka.h>
#include <stdlib.h>
#include <string.h>

#include "slist.h"
#include "str.h"

#include "itable.h"

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

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

static void itable_put_get_remove(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, V1));
	assert_nul(itable_put(tab, 2, V2));

	assert_ptr_equal(itable_get(tab, 1), V1);

	assert_nul(itable_get(tab, 999));

	assert_ptr_equal(itable_remove(tab, 1), V1);

	assert_nul(itable_get(tab, 1));

	itable_free(tab);
}

static void itable_free_vals__(void **state) {
	const struct ITable *tab = itable_init_with(10, 10);
	assert_nul(itable_put(tab, 0, strdup("zero")));

	// valgrind will indicate that key and val have been free'd
	itable_free_vals(tab, NULL);
}

static void itable_iter__(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, NULL));
	assert_nul(itable_put(tab, 2, V2));

	const struct ITableIter *iter = itable_iter(tab);

	assert_non_nul(iter);
	assert_int_equal(itable_iter_key(iter), 0);
	assert_ptr_equal(itable_iter_val(iter), V0);

	iter = itable_iter_next(iter);
	assert_non_nul(iter);
	assert_int_equal(itable_iter_key(iter), 1);
	assert_nul(itable_iter_val(iter));

	itable_iter_free(iter);

	itable_free(tab);
}

static void itable_equal__(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, V0));
	assert_nul(itable_put(actual, 1, V1));

	const struct ITable *expected = itable_init();
	assert_nul(itable_put(expected, 0, V0));
	assert_nul(itable_put(expected, 1, V1));

	assert_itable_equal(actual, expected, NULL, NULL);

	itable_free(actual);
	itable_free(expected);
}

static void itable_str__(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, NULL));
	assert_nul(itable_put(tab, 999, V2));

	char *expected = sprintf_alloc(
			"0 = %p\n"
			"1 = (null)\n"
			"999 = %p\n",
			V0,
			V2
			);

	char *actual = itable_str(tab, NULL);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	itable_free(tab);
}

static void itable_vals_slist__many(void **state) {
	const struct ITable *tab = itable_init();

	itable_put(tab, 0, V0);
	itable_put(tab, 1, NULL);
	itable_put(tab, 2, V2);

	struct SList *list = itable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	itable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_put_get_remove),

		TEST(itable_free_vals__),

		TEST(itable_iter__),

		TEST(itable_equal__),

		TEST(itable_str__),

		TEST(itable_vals_slist__many),
	};

	return RUN(tests);
}

