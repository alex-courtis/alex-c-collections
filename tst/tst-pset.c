#include "tst.h"
#include "asserts.h"
#include "assert-pset.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pset.h"

static int vals[6] = { 20, 21, 22, 23, 24, 25, };
static void *V0 = &V0;
static void *V1 = &V1;
static void *V2 = &V2;
static void *V3 = &vals[3];
static void *V4 = &vals[4];
static void *V5 = &vals[5];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

struct PSet {
	const struct PSetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

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

static void pset_init__defaults(void **state) {
	const struct PSet *set = pset_init();

	assert_non_nul(set);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 10);

	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		pset_add(set, &v[i]);

	assert_int_equal(set->size, 25);
	assert_int_equal(set->capacity, 30);

	pset_free(set);
}

static void pset_clone__empty(void **state) {
	const struct PSet *set = pset_init();

	const struct PSet *clone = pset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	pset_free(set);
	pset_free(clone);
}

// also tests constructor
static void pset_clone__params(void **state) {
	const struct PSetParams params = {
		.equal_val = mock_equal,
		.less_than_val = mock_less_than,
		.free_val = mock_free,
		.str_val = mock_str,
		.clone_val = mock_clone,
		.initial = 3,
		.grow  = 4,
	};
	const struct PSet *set = pset_init_with(params);

	const struct PSet *clone = pset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 3);
	assert_int_equal(set->params.grow, 4);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.less_than_val, mock_less_than);
	assert_ptr_equal(set->params.free_val, mock_free);
	assert_ptr_equal(set->params.str_val, mock_str);
	assert_ptr_equal(set->params.clone_val, mock_clone);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone__shallow_many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct PSet *clone = pset_clone(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V0));
	assert_true(pset_contains(clone, V1));

	assert_pset_equal(set, clone);
	assert_pset_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone__deep_many(void **state) {
	const struct PSetParams params = { .clone_val = mock_clone, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, val, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PSet *clone = pset_clone(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V2));
	assert_true(pset_contains(clone, V3));

	pset_free(clone);
	pset_free(set);
}

static void pset_free_vals__null_free_val(void **state) {
	const struct PSet *set = pset_init();

	const char *val = strdup("0");

	pset_add(set, val);

	assert_int_equal(pset_size(set), 1);

	pset_free_vals(set);
}

static void pset_free_vals__free_val(void **state) {
	const struct PSetParams params = { .free_val = mock_free, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	expect_ptr(mock_free, val, V0);
	expect_ptr(mock_free, val, V1);

	pset_free_vals(set);
}

static void pset_add__new(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, V1));

	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	pset_free(set);
}

static void pset_add__equal_val(void **state) {
	const struct PSetParams params = { .equal_val = mock_equal, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V1);
	will_return(mock_equal, false);

	assert_true(pset_add(set, V1));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_false(pset_add(set, V0));

	pset_free(set);
}

static void pset_add__null(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));

	assert_int_equal(pset_size(set), 1);

	assert_false(pset_contains(set, NULL));
	assert_false(pset_add(set, NULL));
	assert_false(pset_contains(set, NULL));

	assert_int_equal(pset_size(set), 1);

	pset_free(set);
}

static void pset_add__grow(void **state) {
	const struct PSetParams params = { .initial = 2, .grow = 5 };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(set->size, 2);
	assert_int_equal(set->capacity, 2);
	assert_int_equal(set->params.grow, 5);

	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	assert_true(pset_add(set, V2));
	assert_int_equal(set->size, 3);
	assert_int_equal(set->capacity, 7);
	assert_true(pset_contains(set, V2));

	assert_true(pset_add(set, V3));
	assert_int_equal(set->size, 4);
	assert_int_equal(set->capacity, 7);
	assert_true(pset_contains(set, V3));

	assert_true(pset_add(set, V4));
	assert_true(pset_add(set, V5));
	assert_int_equal(set->size, 6);
	assert_int_equal(set->capacity, 7);

	assert_true(pset_contains(set, V4));
	assert_true(pset_contains(set, V5));

	pset_free(set);
}

static void pset_remove__existing(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	// 0
	assert_non_nul(pset_remove(set, V0));

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	// 1
	assert_non_nul(pset_remove(set, V1));

	assert_int_equal(pset_size(set), 0);
	assert_false(pset_contains(set, V0));
	assert_false(pset_contains(set, V1));

	pset_free(set);
}

static void pset_remove__inexistent(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	assert_null(pset_remove(set, V2));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	pset_free(set);
}

static void pset_remove__equal_val(void **state) {
	const struct PSetParams params = { .equal_val = mock_equal, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_true(pset_contains(set, V0));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_true(pset_remove(set, V0));

	assert_int_equal(pset_size(set), 0);

	pset_free(set);
}

static void pset_iter__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free(set);
}

static void pset_iter__free(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V0);

	pset_iter_free(iter);

	pset_free(set);
}

static void pset_iter__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V0);

	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V1);

	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free(set);
}

static void pset_iter__cleared(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	assert_non_nul(pset_remove(set, V0));
	assert_non_nul(pset_remove(set, V1));

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_iter(set));

	pset_free(set);
}

static void pset_iter__state_deleted(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));

	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);

	const struct PSetIterState *st = iter->st;
	((struct PSetIter*)iter)->st = NULL;

	iter = pset_iter_next(iter);
	assert_nul(iter);

	free((void*)st);
	pset_free(set);
}

static void pset_filter_iter__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));
	assert_true(pset_add(set, V3));
	assert_true(pset_add(set, V4));

	assert_int_equal(pset_size(set), 5);

	// skip V0
	expect_ptr(mock_test, val, V0);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// get V1
	expect_ptr(mock_test, val, V1);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);

	const struct PSetIter *iter = pset_filter_iter(set, mock_test, D0);
	assert_non_nul(iter);
	assert_ptr_equal(iter->val, V1);

	// skip V2
	expect_ptr(mock_test, val, V2);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// get V3
	expect_ptr(mock_test, val, V3);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);

	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->val, V3);

	// skip V4
	expect_ptr(mock_test, val, V4);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// done
	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free(set);
}

static void pset_add__again(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));
	assert_true(pset_add(set, V3));

	assert_int_equal(pset_size(set), 4);

	// remove 1
	assert_non_nul(pset_remove(set, V1));
	assert_int_equal(pset_size(set), 3);

	// put 1 again afterwards
	assert_true(pset_add(set, V1));
	assert_int_equal(pset_size(set), 4);

	// 0
	const struct PSetIter *iter = pset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V0);

	// 2
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V2);

	// 3
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V3);

	// 0 moved later
	iter = pset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->val, V1);

	// end
	iter = pset_iter_next(iter);
	assert_nul(iter);

	pset_free(set);
}

static void pset_sort__empty(void **state) {
	const struct PSetParams params = { .less_than_val = mock_test, };
	const struct PSet *actual = pset_init_with(params);
	const struct PSet *expected = pset_init_with(params);

	pset_sort(actual);

	assert_int_equal(pset_size(actual), 0);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__one(void **state) {
	const struct PSetParams params = { .less_than_val = mock_test, };
	const struct PSet *actual = pset_init_with(params);

	assert_true(pset_add(actual, V0));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0));

	pset_sort(actual);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static bool test_a_is_V0(const void* const a, const void* const b) {
	return a == V0;
}

static void pset_sort__two(void **state) {
	const struct PSetParams params = { .less_than_val = test_a_is_V0, };
	const struct PSet *actual = pset_init_with(params);

	assert_true(pset_add(actual, V1));
	assert_true(pset_add(actual, V0));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));

	pset_sort(actual);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_equal__length_different(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V1));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__val_pointers_ok(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0));
	assert_true(pset_add(a, V1));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V1));

	assert_pset_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__val_pointers_different(void **state) {
	const struct PSet *a = pset_init();
	const struct PSet *b = pset_init();

	assert_true(pset_add(a, V0));
	assert_true(pset_add(a, V1));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V2));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__equal_val_ok(void **state) {
	const struct PSetParams params = { .equal_val = fn_equal_strcmp, };
	const struct PSet *a = pset_init_with(params);
	const struct PSet *b = pset_init_with(params);

	assert_true(pset_add(a, V0));
	assert_true(pset_add(a, V1));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V1));

	assert_pset_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__equal_val_different(void **state) {
	const struct PSetParams params = { .equal_val = fn_equal_strcmp, };
	const struct PSet *a = pset_init_with(params);
	const struct PSet *b = pset_init_with(params);

	assert_true(pset_add(a, "0"));
	assert_true(pset_add(a, "1"));

	assert_true(pset_add(b, "0"));
	assert_true(pset_add(b, "2"));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_vals_slist__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_nul(pset_slist(set));

	pset_free(set);
}

static void pset_vals_slist__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	struct SList *list = pset_slist(set);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), V0);
	assert_str_equal(slist_at(list, 1), V1);

	slist_free(&list);
	pset_free(set);
}

static void pset_str__empty(void **state) {
	const struct PSetParams params = { .str_val = mock_str, };
	const struct PSet *set = pset_init_with(params);

	char *str = pset_str(set);
	assert_str_equal(str, "");

	free(str);
	pset_free(set);
}

static void pset_str__pointers(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n"
			"%p\n",
			V0,
			V1,
			V2
			);

	char *actual = pset_str(set);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pset_free(set);
}

static void pset_str__str_val(void **state) {
	const struct PSetParams params = { .str_val = fn_str_first, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, "ONE"));
	assert_true(pset_add(set, "TWO"));
	assert_true(pset_add(set, "THREE"));

	char *str = pset_str(set);
	assert_str_equal(str,
			"O\n"
			"T\n"
			"T\n"
			);

	free(str);
	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__defaults),

		TEST(pset_clone__empty),
		TEST(pset_clone__params),
		TEST(pset_clone__shallow_many),
		TEST(pset_clone__deep_many),

		TEST(pset_free_vals__null_free_val),
		TEST(pset_free_vals__free_val),

		TEST(pset_add__new),
		TEST(pset_add__equal_val),
		TEST(pset_add__null),
		TEST(pset_add__grow),

		TEST(pset_remove__existing),
		TEST(pset_remove__inexistent),
		TEST(pset_remove__equal_val),

		TEST(pset_iter__empty),
		TEST(pset_iter__free),
		TEST(pset_iter__many),
		TEST(pset_iter__cleared),
		TEST(pset_iter__state_deleted),

		TEST(pset_filter_iter__many),

		TEST(pset_add__again),

		TEST(pset_sort__empty),
		TEST(pset_sort__one),
		TEST(pset_sort__two),

		TEST(pset_equal__length_different),
		TEST(pset_equal__val_pointers_ok),
		TEST(pset_equal__val_pointers_different),
		TEST(pset_equal__equal_val_ok),
		TEST(pset_equal__equal_val_different),

		TEST(pset_vals_slist__empty),
		TEST(pset_vals_slist__many),

		TEST(pset_str__empty),
		TEST(pset_str__pointers),
		TEST(pset_str__str_val),
	};

	return RUN(tests);
}

