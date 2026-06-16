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
#include "ptable.h"
#include "slist.h"
#include "str.h"

#include "itable.h"

struct PTable {
	const struct PTableParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct ITable {
	const struct ITableParams params;
	const struct PTable *ptab;
};

struct ITableIterState {
	const struct PTableIter *pit;
	fn_equal_size_t equal_key;
	fn_equal equal_val;
	const void *data;
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

static void itable_iter__empty(void **state) {

	const struct ITable *tab = itable_init();

	const struct ITableIter *iter = itable_iter(tab);

	assert_nul(iter);

	itable_free(tab);
}

static void itable_iter__state_deleted(void **state) {
	const struct ITable *tab = itable_init();

	assert_nul(itable_put(tab, 0, V0));

	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);

	const struct ITableIterState *st = iter->st;
	((struct ITableIter*)iter)->st = NULL;

	iter = itable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(st->pit);
	free((void*)st);
	itable_free(tab);
}

static void itable_iter__state_tab_deleted(void **state) {
	const struct ITable *tab = itable_init();

	assert_nul(itable_put(tab, 0, V0));

	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);

	const struct PTableIter *piter = iter->st->pit;
	iter->st->pit = NULL;

	iter = itable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(piter);
	itable_free(tab);
}

static void itable_filter_iter__(void **state) {
	const struct ITable *tab = itable_init();

	assert_nul(itable_put(tab, 0, V0));
	assert_nul(itable_put(tab, 1, V1));
	assert_nul(itable_put(tab, 2, V2));

	// skip K0
	expect_int_value(mock_equal_size_t, a, 0);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, false);

	// pass K1
	expect_int_value(mock_equal_size_t, a, 1);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, true);

	// pass V1
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	const struct ITableIter *iter = itable_filter_iter(tab, mock_equal_size_t, mock_equal, D0);
	assert_non_nul(iter);
	assert_int_equal(iter->key, 1);
	assert_ptr_equal(iter->val, V1);

	// pass K2
	expect_int_value(mock_equal_size_t, a, 2);
	expect_ptr(mock_equal_size_t, b, D0);
	will_return(mock_equal_size_t, true);

	// skip V2
	expect_ptr(mock_equal, a, V2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// done
	iter = itable_iter_next(iter);
	assert_nul(iter);

	itable_free(tab);
}

static void itable_equal__(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, V0));
	assert_nul(itable_put(actual, 1, V1));

	assert_itable_not_equal(actual, NULL);

	const struct ITable *expected = itable_init();
	assert_nul(itable_put(expected, 0, V0));
	assert_nul(itable_put(expected, 1, V1));

	assert_itable_equal(actual, expected);

	assert_nul(itable_put(actual, 2, V2));

	assert_itable_not_equal(actual, expected);

	itable_free(actual);
	itable_free(expected);
}

static void itable_equal__key_removed(void **state) {
	const struct ITable *a = itable_init();
	assert_nul(itable_put(a, 0, V0));
	assert_nul(itable_put(a, 1, V1));

	const struct ITable *b = itable_init();
	assert_nul(itable_put(b, 0, V0));
	assert_nul(itable_put(b, 1, V1));

	int *removed_key = (int*)b->ptab->keys[0];
	b->ptab->keys[0] = NULL;

	assert_itable_not_equal(a, b);

	free(removed_key);
	itable_free(a);
	itable_free(b);
}

static void itable_get__key_removed(void **state) {

	const struct ITable *actual = itable_init();
	assert_nul(itable_put(actual, 0, V0));

	int *removed_key = (int*)actual->ptab->keys[0];
	actual->ptab->keys[0] = NULL;

	assert_nul(itable_get(actual, 0));

	free(removed_key);
	itable_free(actual);
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

	const struct ITable *to = itable_clone(from, NULL);

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
		.initial = 99,
		.grow = 1,
	};
	const struct ITable *from = itable_init_with(params);

	const struct ITable *to = itable_clone(from, mock_clone);

	assert_non_nul(to);

	// commented out are tested elsewhere
	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->params.grow, 1);
	assert_ptr_equal(to->ptab->params.equal_val, mock_equal);
	assert_ptr_equal(to->ptab->params.free_key, (fn_free)free);
	assert_ptr_equal(to->ptab->params.free_val, mock_free);

	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	itable_free(from);
	itable_free(to);
}

static void itable__null_inputs(void **state) {
	assert_nul(itable_clone(NULL, NULL));
	itable_free(NULL);
	itable_free_vals(NULL);
	itable_iter_free(NULL);
	assert_false(itable_get(NULL, 0));
	assert_nul(itable_iter(NULL));
	assert_nul(itable_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(itable_iter_next(NULL));
	assert_false(itable_put(NULL, 0, NULL));
	assert_nul(itable_remove(NULL, 0));
	assert_false(itable_equal(NULL, NULL));
	assert_nul(itable_vals_slist(NULL));
	assert_nul(itable_str(NULL));
	assert_int_equal(itable_size(NULL), 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_put_get_remove),

		TEST(itable_free_vals__),

		TEST(itable_iter__),
		TEST(itable_iter__empty),
		TEST(itable_iter__state_deleted),
		TEST(itable_iter__state_tab_deleted),

		TEST(itable_filter_iter__),

		TEST(itable_equal__),
		TEST(itable_equal__key_removed),

		TEST(itable_get__key_removed),

		TEST(itable_str__),

		TEST(itable_vals_slist__many),

		TEST(itable_clone__shallow),
		TEST(itable_clone__params),

		TEST(itable__null_inputs),
	};

	return RUN(tests);
}

