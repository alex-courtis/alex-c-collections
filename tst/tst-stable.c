#include "tst.h"
#include "asserts.h"
#include "assert-stable.h"
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

#include "stable.h"

struct PTable {
	const struct PTableParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct STable {
	const struct STableParams params;
	const struct PTable *ptab;
};

struct STableIterState {
	const struct PTableIter *pit;
};

/*
   diff --color=always -U 10000 <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' tst/tst-itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' tst/tst-stable.c) | less

   diff --color=always -U 10000 <(sed -e 's/sstable/xtable/g ; s/SSTable/XTable/g' tst/tst-sstable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' tst/tst-stable.c) | less
   */

static int vals[3] = { 20, 21, 22, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

static void stable_put_get_remove__case_sensitive(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", V1));
	assert_nul(stable_put(tab, "c", V2));

	assert_int_equal(stable_size(tab), 3);

	assert_ptr_equal(stable_get(tab, "b"), V1);

	assert_nul(stable_get(tab, "x"));

	assert_ptr_equal(stable_remove(tab, "b"), V1);

	assert_nul(stable_get(tab, "b"));

	stable_free(tab);
}

static void stable_put_get_remove__case_insensitive(void **state) {
	const struct STableParams params = { .case_insensitive = true, };
	const struct STable *tab = stable_init_with(params);

	assert_nul(stable_put(tab, "A", V0));
	assert_nul(stable_put(tab, "B", V1));

	assert_ptr_equal(stable_get(tab, "b"), V1);

	assert_nul(stable_get(tab, "x"));

	assert_ptr_equal(stable_remove(tab, "b"), V1);

	assert_nul(stable_get(tab, "b"));

	stable_free(tab);
}

static void stable_free_vals__(void **state) {
	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", strdup("zero")));

	stable_free_vals(tab);
}

static void stable_iter__(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", NULL));
	assert_nul(stable_put(tab, "c", V2));

	const struct STableIter *iter = stable_iter(tab);

	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_ptr_equal(iter->val, V0);

	iter = stable_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_nul(iter->val);

	stable_iter_free(iter);

	stable_free(tab);
}

static void stable_iter__state_deleted(void **state) {
	const struct STable *tab = stable_init();

	assert_nul(stable_put(tab, "a", V0));

	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);

	const struct STableIterState *st = iter->st;
	((struct STableIter*)iter)->st = NULL;

	iter = stable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(st->pit);
	free((void*)st);
	stable_free(tab);
}

static void stable_iter__state_tab_deleted(void **state) {
	const struct STable *tab = stable_init();

	assert_nul(stable_put(tab, "a", V0));

	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);

	const struct PTableIter *piter = iter->st->pit;
	iter->st->pit = NULL;

	iter = stable_iter_next(iter);
	assert_nul(iter);

	ptable_iter_free(piter);
	stable_free(tab);
}

static void stable_iter__empty(void **state) {

	const struct STable *tab = stable_init();

	const struct STableIter *iter = stable_iter(tab);

	assert_nul(iter);

	stable_free(tab);
}

static void stable_filter_iter__(void **state) {
	const struct STable *tab = stable_init();

	assert_nul(stable_put(tab, "0", V0));
	assert_nul(stable_put(tab, "1", V1));
	assert_nul(stable_put(tab, "2", V2));

	// skip "0"
	expect_string(mock_equal, a, "0");
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// get 1
	expect_string(mock_equal, a, "1");
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	const struct STableIter *iter = stable_filter_iter(tab, mock_equal, mock_equal, D0);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "1");
	assert_ptr_equal(iter->val, V1);

	// skip V2
	expect_string(mock_equal, a, "2");
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// done
	iter = stable_iter_next(iter);
	assert_nul(iter);

	stable_free(tab);
}

static void stable_equal__case_sensitive(void **state) {

	const struct STable *actual = stable_init();
	assert_nul(stable_put(actual, "a", V0));
	assert_nul(stable_put(actual, "b", V1));

	assert_stable_not_equal(actual, NULL);

	const struct STable *expected = stable_init();
	assert_nul(stable_put(expected, "a", V0));
	assert_nul(stable_put(expected, "b", V1));

	assert_stable_equal(actual, expected);

	stable_free(actual);
	stable_free(expected);
}

static void stable_equal__case_insensitive(void **state) {

	const struct STableParams params = { .case_insensitive = true, };
	const struct STable *actual = stable_init_with(params);

	assert_nul(stable_put(actual, "a", V0));
	assert_nul(stable_put(actual, "b", V1));

	const struct STable *expected = stable_init();
	assert_nul(stable_put(expected, "A", V0));
	assert_nul(stable_put(expected, "B", V1));

	assert_stable_equal(actual, expected);

	assert_nul(stable_put(actual, "c", V2));

	assert_stable_not_equal(actual, expected);

	stable_free(actual);
	stable_free(expected);
}

static void stable_contains_key__(void **state) {
	const struct STable *tab = stable_init();

	assert_false(stable_contains_key(tab, "a"));

	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", V1));

	assert_true(stable_contains_key(tab, "a"));
	assert_true(stable_contains_key(tab, "b"));

	assert_false(stable_contains_key(tab, "c"));

	assert_false(stable_contains_key(tab, NULL));

	stable_free(tab);
}

static void stable_put_free__(void **state) {
	const struct STableParams params = { .free_val = mock_free, };
	const struct STable *tab = stable_init_with(params);

	assert_nul(stable_put(tab, "a", V0));

	assert_false(stable_put_free(tab, "b", V1));

	expect_ptr(mock_free, val, V0);
	assert_true(stable_put_free(tab, "a", V0));

	stable_free(tab);
}

static void stable_str__(void **state) {

	const struct STable *tab = stable_init();
	assert_nul(stable_put(tab, "a", V0));
	assert_nul(stable_put(tab, "b", NULL));
	assert_nul(stable_put(tab, "c", V2));

	char *expected = sprintf_alloc(
			"a = %p\n"
			"b = (null)\n"
			"c = %p\n",
			V0,
			V2
			);

	char *actual = stable_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	stable_free(tab);
}

static void stable_keys_slist__many(void **state) {
	const struct STable *tab = stable_init();

	stable_put(tab, "a", V0);
	stable_put(tab, "b", V1);

	struct SList *list = stable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	stable_free(tab);
	slist_free_vals(&list, NULL);
}

static void stable_vals_slist__many(void **state) {
	const struct STable *tab = stable_init();

	stable_put(tab, "a", V0);
	stable_put(tab, "b", NULL);
	stable_put(tab, "c", V2);

	struct SList *list = stable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V0);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V2);

	slist_free(&list);
	stable_free(tab);
}

static void stable_clone__shallow(void **state) {
	const struct STable *from = stable_init();

	assert_nul(stable_put(from, "a", V0));
	assert_nul(stable_put(from, "b", NULL));
	assert_nul(stable_put(from, "c", V2));

	const struct STable *to = stable_clone(from, NULL);

	assert_non_nul(to);

	assert_int_equal(stable_size(to), 3);

	assert_stable_equal(from, to);

	stable_free(from);
	stable_free(to);
}

// also tests constructor
static void stable_clone__params(void **state) {
	const struct STableParams params = {
		.case_insensitive = true,
		.equal_val = mock_equal,
		.free_val = mock_free,
		.initial = 99,
		.grow = 1,
	};
	const struct STable *from = stable_init_with(params);

	const struct STable *to = stable_clone(from, mock_clone);

	assert_non_nul(to);

	assert_int_equal(to->ptab->size, 0);
	assert_int_equal(to->ptab->capacity, 99);
	assert_int_equal(to->ptab->params.grow, 1);
	assert_ptr_equal(to->ptab->params.equal_key, fn_equal_strcasecmp);
	assert_ptr_equal(to->ptab->params.equal_val, mock_equal);
	assert_ptr_equal(to->ptab->params.alloc_key, (fn_alloc)strdup);
	assert_ptr_equal(to->ptab->params.free_key, (fn_free)free);
	assert_ptr_equal(to->ptab->params.free_val, mock_free);

	assert_true(to->params.case_insensitive);
	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	stable_free(from);
	stable_free(to);
}

static void stable__null_inputs(void **state) {
	assert_nul(stable_clone(NULL, NULL));
	stable_free(NULL);
	stable_free_vals(NULL);
	stable_iter_free(NULL);
	assert_false(stable_get(NULL, NULL));
	assert_false(stable_contains_key(NULL, NULL));
	assert_nul(stable_iter(NULL));
	assert_nul(stable_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(stable_iter_next(NULL));
	assert_nul(stable_put(NULL, NULL, NULL));
	assert_false(stable_put_free(NULL, NULL, NULL));
	assert_nul(stable_remove(NULL, NULL));
	assert_false(stable_equal(NULL, NULL));
	assert_nul(stable_keys_slist(NULL));
	assert_nul(stable_vals_slist(NULL));
	assert_nul(stable_str(NULL));
	assert_int_equal(stable_size(NULL), 0);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(stable_put_get_remove__case_sensitive),
		TEST(stable_put_get_remove__case_insensitive),

		TEST(stable_free_vals__),

		TEST(stable_iter__),
		TEST(stable_iter__empty),
		TEST(stable_iter__state_deleted),
		TEST(stable_iter__state_tab_deleted),

		TEST(stable_filter_iter__),

		TEST(stable_equal__case_sensitive),
		TEST(stable_equal__case_insensitive),

		TEST(stable_contains_key__),

		TEST(stable_put_free__),

		TEST(stable_str__),

		TEST(stable_keys_slist__many),

		TEST(stable_vals_slist__many),

		TEST(stable_clone__shallow),
		TEST(stable_clone__params),

		TEST(stable__null_inputs),
	};

	return RUN(tests);
}

