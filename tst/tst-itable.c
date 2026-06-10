#include "tst.h"
#include "asserts.h"
#include "assert-itable.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "itable.h"

struct PTable {
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal_key;
	fn_equal equal_val;
	fn_alloc alloc_key;
	fn_free free_key;
	fn_free free_val;
	fn_str str_key;
	fn_str str_val;
	fn_clone clone_val;
};

struct ITable {
	const struct PTable *ptab;
};

/*
   diff --color=always -U 10000 <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' tst/tst-itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' tst/tst-stable.c) | less
   */

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

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
	const struct ITableParams params = { 0 };
	const struct ITable *tab = itable_init_with(params);

	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, V1));
	assert_nul(itable_put(tab, 2, V2));

	assert_int_equal(itable_size(tab), 3);

	assert_ptr_equal(itable_get(tab, 1), V1);

	assert_nul(itable_get(tab, 999));

	assert_ptr_equal(itable_remove(tab, 1), V1);

	assert_nul(itable_get(tab, 1));

	itable_free(tab);
}

static void itable_free_vals__(void **state) {
	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, strdup("zero")));

	itable_free_vals(tab);
}

static void itable_iter__(void **state) {

	const struct ITable *tab = itable_init();
	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, NULL));
	assert_nul(itable_put(tab, 2, V2));

	const struct ITableIter *iter = itable_iter(tab);

	assert_non_nul(iter);
	assert_int_equal(iter->key, 0);
	assert_ptr_equal(iter->val, V0);

	iter = itable_iter_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_nul(iter->val);

	itable_iter_free(iter);

	itable_free(tab);
}

static void itable_filter_iter__(void **state) {
	const struct ITable *tab = itable_init();

	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, V1));
	assert_nul(itable_put(tab, 2, V2));

	// skip K0
	expect_int_value(mock_test_size_t, val, 0);
	expect_ptr(mock_test_size_t, data, D0);
	will_return(mock_test_size_t, false);

	// pass K1
	expect_int_value(mock_test_size_t, val, 1);
	expect_ptr(mock_test_size_t, data, D0);
	will_return(mock_test_size_t, true);

	// pass V1
	expect_ptr(mock_test, val, V1);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);

	const struct ITableIter *iter = itable_filter_iter(tab, mock_test_size_t, mock_test, D0);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_ptr_equal(iter->val, V1);

	// pass K2
	expect_int_value(mock_test_size_t, val, 2);
	expect_ptr(mock_test_size_t, data, D0);
	will_return(mock_test_size_t, true);

	// skip V2
	expect_ptr(mock_test, val, V2);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// done
	iter = itable_iter_next(iter);
	assert_nul(iter);

	itable_free(tab);
}

static void itable_equal__(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, V0));
	assert_nul(itable_put(actual, 1, V1));

	const struct ITable *expected = itable_init();
	assert_nul(itable_put(expected, 0, V0));
	assert_nul(itable_put(expected, 1, V1));

	assert_itable_equal(actual, expected);

	assert_nul(itable_put(actual, 2, V2));

	assert_itable_not_equal(actual, expected);

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

	char *actual = itable_str(tab);

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

static void itable_clone__shallow(void **state) {
	const struct ITable *from = itable_init();

	assert_nul(itable_put(from, 0, V0));
	assert_nul(itable_put(from, 1, NULL));
	assert_nul(itable_put(from, 2, V2));

	const struct ITable *to = itable_clone(from);

	assert_non_nul(to);

	assert_int_equal(itable_size(to), 3);

	assert_itable_equal(from, to);

	itable_free(from);
	itable_free(to);
}

// also tests constructor
static void itable_clone__params(void **state) {
	const struct ITableParams params = {
		.equal_val = mock_equal,
		.free_val = mock_free,
		.str_val = mock_str,
		.clone_val = mock_clone,
		.initial = 99,
		.grow = 1,
	};
	const struct ITable *from = itable_init_with(params);

	const struct ITable *to = itable_clone(from);

	assert_non_nul(to);

	// commented out are tested elsewhere
	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->grow, 1);
	// assert_ptr_equal(to->ptab->equal_key, mock_equal);
	assert_ptr_equal(to->ptab->equal_val, mock_equal);
	// assert_ptr_equal(to->ptab->alloc_key, (fn_alloc)strdup);
	assert_ptr_equal(to->ptab->free_key, (fn_free)free);
	assert_ptr_equal(to->ptab->free_val, mock_free);
	// assert_ptr_equal(to->ptab->str_key, fn_str_str_or_null);
	assert_ptr_equal(to->ptab->str_val, mock_str);
	assert_ptr_equal(to->ptab->clone_val, mock_clone);

	itable_free(from);
	itable_free(to);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_put_get_remove),

		TEST(itable_free_vals__),

		TEST(itable_iter__),
		TEST(itable_filter_iter__),

		TEST(itable_equal__),

		TEST(itable_str__),

		TEST(itable_vals_slist__many),

		TEST(itable_clone__shallow),
		TEST(itable_clone__params),
	};

	return RUN(tests);
}

