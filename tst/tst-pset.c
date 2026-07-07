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
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];
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

static char* str_first(const void *val) {
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

static void pset_clone_shallow__empty(void **state) {
	const struct PSet *set = pset_init();

	const struct PSet *clone = pset_clone_shallow(set);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	pset_free(set);
	pset_free(clone);
}

// also tests constructor
static void pset_clone_shallow__params(void **state) {
	const struct PSetParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.initial = 3,
		.grow  = 4,
	};
	const struct PSet *set = pset_init_with(params);

	const struct PSet *clone = pset_clone_shallow(set);

	assert_non_nul(clone);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 3);
	assert_int_equal(set->params.grow, 4);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.alloc_val, mock_alloc);
	assert_ptr_equal(set->params.clone_val, mock_clone);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone_shallow__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct PSet *clone = pset_clone_shallow(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V0));
	assert_true(pset_contains(clone, V1));

	assert_pset_equal(set, clone);
	assert_pset_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__clone_val(void **state) {
	const struct PSetParams params = { .clone_val = mock_clone, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	assert_true(pset_add(set, V1));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, val, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PSet *clone = pset_clone_deep(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V2));
	assert_true(pset_contains(clone, V3));

	assert_pset_not_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__no_clone_val(void **state) {
	const struct PSet *from = pset_init();

	assert_true(pset_add(from, V0));

	const struct PSet *to = pset_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(pset_size(to), 0);

	assert_pset_not_equal(from, to);

	pset_free(from);
	pset_free(to);
}

static void pset_free_vals__null_free_val(void **state) {
	const struct PSet *set = pset_init();

	const char *val = strdup("0");

	pset_add(set, val);

	assert_int_equal(pset_size(set), 1);

	pset_free_vals(set);
}

static void pset_free_vals__missing_val(void **state) {
	const struct PSet *set = pset_init();

	char *val = strdup("0");

	pset_add(set, val);

	assert_int_equal(pset_size(set), 1);

	set->vals[0] = NULL;

	pset_free_vals(set);
	free(val);
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

static void pset_add__alloc_val(void **state) {
	const struct PSetParams params = { .alloc_val = mock_alloc, };
	const struct PSet *set = pset_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(pset_add(set, V0));

	pset_free(set);
}

static void pset_add__alloc_val_returned_null(void **state) {
	const struct PSetParams params = { .alloc_val = mock_alloc, };
	const struct PSet *set = pset_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_false(pset_add(set, V0));

	assert_false(pset_contains(set, V0));

	assert_int_equal(pset_size(set), 0);

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

static void pset_add_all__many(void **state) {
	const struct PSet *to = pset_init();
	assert_true(pset_add(to, V0));
	assert_true(pset_add(to, V1));

	const struct PSet *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V2));

	assert_int_equal(pset_add_all(to, from), 1);

	assert_pset_equal(to, expected);

	pset_free(to);
}

static void pset_remove__existing(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	// 0
	assert_true(pset_remove(set, V0));

	assert_int_equal(pset_size(set), 1);
	assert_false(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	// 1
	assert_true(pset_remove(set, V1));

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

	assert_false(pset_remove(set, V2));

	assert_false(pset_remove(set, NULL));

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

static void pset_remove_free__free_val(void **state) {
	const struct PSetParams params = { .free_val = mock_free, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	expect_ptr(mock_free, val, V0);

	assert_true(pset_remove_free(set, V0));

	assert_false(pset_remove_free(set, NULL));

	assert_false(pset_remove_free(set, V2));

	pset_free(set);
}

static void pset_remove_free__free(void **state) {
	const struct PSet *set = pset_init();

	const char *val = strdup("0");

	assert_true(pset_add(set, val));

	assert_true(pset_remove_free(set, val));

	pset_free(set);
}

static void pset_match__matches(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	assert_ptr_equal(pset_match(set, mock_match_ptr, D0), V1);

	pset_free(set);
}

static void pset_match__no_match(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// skip V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	assert_nul(pset_match(set, mock_match_ptr, D0));

	pset_free(set);
}

static void pset_match__null_match(void **state) {
	const struct PSet *set = pset_init();

	assert_nul(pset_match(set, mock_match_ptr, D0));

	pset_free(set);
}

static void pset_it__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_it(set));

	pset_free(set);
}

static void pset_it__free(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct PSetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_str_equal(it->val, V0);

	pset_it_free(it);

	pset_free(set);
}

static void pset_it__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	const struct PSetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_str_equal(it->val, V0);

	it = pset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, V1);

	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_it__cleared(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	assert_true(pset_remove(set, V0));
	assert_true(pset_remove(set, V1));

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_it(set));

	pset_free(set);
}

static void pset_it_free__partial(void **state) {
	const struct PSetIt *it = calloc(1, sizeof(struct PSetIt));

	pset_it_free(it);
}

static void pset_it_next__partial(void **state) {
	const struct PSetIt *it = calloc(1, sizeof(struct PSetIt));

	assert_nul(pset_it_next(it));
}

static void pset_match_it__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));
	assert_true(pset_add(set, V3));
	assert_true(pset_add(set, V4));

	assert_int_equal(pset_size(set), 5);

	// skip V0
	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// get V1
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	const struct PSetIt *it = pset_match_it(set, mock_equal, D0);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_ptr(mock_equal, a, V2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// get V3
	expect_ptr(mock_equal, a, V3);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// skip V4
	expect_ptr(mock_equal, a, V4);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// done
	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_match_it__none(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	// skip V0
	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// skip V1
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	assert_nul(pset_match_it(set, mock_equal, D0));

	pset_free(set);
}

static void pset_match_it__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_nul(pset_match_it(set, mock_equal, D0));

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
	assert_true(pset_remove(set, V1));
	assert_int_equal(pset_size(set), 3);

	// put 1 again afterwards
	assert_true(pset_add(set, V1));
	assert_int_equal(pset_size(set), 4);

	// 0
	const struct PSetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_str_equal(it->val, V0);

	// 2
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, V2);

	// 3
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, V3);

	// 0 moved later
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->val, V1);

	// end
	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_sort__empty(void **state) {
	const struct PSet *actual = pset_init();
	const struct PSet *expected = pset_init();

	pset_sort(actual, mock_less_than);

	assert_int_equal(pset_size(actual), 0);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__one(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, V0));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0));

	pset_sort(actual, mock_less_than);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static bool test_less_than_int(const void* const a, const void* const b) {
	return *(int*)a < *(int*)b;
}

static void pset_sort__many(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, V2));
	assert_true(pset_add(actual, V0));
	assert_true(pset_add(actual, V3));
	assert_true(pset_add(actual, V5));
	assert_true(pset_add(actual, V1));
	assert_true(pset_add(actual, V4));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V2));
	assert_true(pset_add(expected, V3));
	assert_true(pset_add(expected, V4));
	assert_true(pset_add(expected, V5));

	pset_sort(actual, test_less_than_int);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__no_less_than(void **state) {
	const struct PSet *actual = pset_init();

	assert_true(pset_add(actual, V1));
	assert_true(pset_add(actual, V0));

	const struct PSet *expected = pset_init();
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V0));

	pset_sort(actual, NULL);

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

	assert_pset_not_equal(a, NULL);

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
	const struct PSetParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct PSet *a = pset_init_with(params);
	const struct PSet *b = pset_init_with(params);

	assert_true(pset_add(a, V0));
	assert_true(pset_add(a, V1));

	assert_true(pset_contains(a, V0));
	assert_true(pset_contains(a, V1));
	assert_false(pset_contains(a, V2));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V1));

	assert_pset_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__equal_val_different(void **state) {
	const struct PSetParams params = { .equal_val = (fn_equal)equal_strcmp, };
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

static void pset_vals_slist_shallow__empty(void **state) {
	const struct PSet *set = pset_init();

	assert_nul(pset_slist_shallow(set));

	pset_free(set);
}

static void pset_vals_slist_shallow__many(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	struct SList *list = pset_slist_shallow(set);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), V0);
	assert_str_equal(slist_at(list, 1), V1);

	slist_free(&list);
	pset_free(set);
}

static void pset_vals_slist_deep__clone_val(void **state) {
	const struct PSetParams params = { .clone_val = mock_clone, };
	const struct PSet *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct SList *list = pset_slist_deep(set);

	slist_free(&list);
	pset_free(set);
}

static void pset_vals_slist_deep__no_clone_val(void **state) {
	const struct PSet *set = pset_init();

	assert_true(pset_add(set, V0));

	assert_nul(pset_slist_deep(set));

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
	const struct PSetParams params = { .str_val = str_first, };
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

static void pset__null_inputs(void **state) {
	const struct PSet *set = pset_init();

	assert_int_equal(pset_add_all(NULL, NULL), 0);
	assert_int_equal(pset_add_all(set, NULL), 0);
	assert_nul(pset_clone_deep(NULL));
	assert_nul(pset_clone_shallow(NULL));
	pset_free(NULL);
	pset_free_vals(NULL);
	pset_it_free(NULL);
	assert_false(pset_contains(NULL, NULL));
	assert_false(pset_contains(set, NULL));
	assert_nul(pset_match(NULL, NULL, NULL));
	assert_nul(pset_match(set, NULL, NULL));
	assert_nul(pset_it(NULL));
	assert_nul(pset_match_it(NULL, NULL, NULL));
	assert_nul(pset_match_it(set, NULL, NULL));
	assert_nul(pset_it_next(NULL));
	assert_false(pset_add(NULL, NULL));
	assert_false(pset_add(set, NULL));
	assert_false(pset_remove(NULL, NULL));
	assert_false(pset_remove(set, NULL));
	assert_false(pset_remove_free(NULL, NULL));
	assert_false(pset_remove_free(set, NULL));
	pset_sort(NULL, NULL);
	assert_false(pset_equal(NULL, NULL));
	assert_false(pset_equal(set, NULL));
	assert_nul(pset_slist_shallow(NULL));
	assert_nul(pset_slist_deep(NULL));
	assert_nul(pset_str(NULL));
	assert_int_equal(pset_size(NULL), 0);

	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__defaults),

		TEST(pset_clone_shallow__empty),
		TEST(pset_clone_shallow__params),
		TEST(pset_clone_shallow__many),

		TEST(pset_clone_deep__clone_val),
		TEST(pset_clone_deep__no_clone_val),

		TEST(pset_free_vals__null_free_val),
		TEST(pset_free_vals__missing_val),
		TEST(pset_free_vals__free_val),

		TEST(pset_add__new),
		TEST(pset_add__equal_val),
		TEST(pset_add__alloc_val),
		TEST(pset_add__alloc_val_returned_null),
		TEST(pset_add__null),
		TEST(pset_add__grow),

		TEST(pset_add_all__many),

		TEST(pset_remove__existing),
		TEST(pset_remove__inexistent),
		TEST(pset_remove__equal_val),

		TEST(pset_remove_free__free_val),
		TEST(pset_remove_free__free),

		TEST(pset_match__matches),
		TEST(pset_match__no_match),
		TEST(pset_match__null_match),

		TEST(pset_it__empty),
		TEST(pset_it__free),
		TEST(pset_it__many),
		TEST(pset_it__cleared),

		TEST(pset_it_free__partial),

		TEST(pset_it_next__partial),

		TEST(pset_match_it__many),
		TEST(pset_match_it__none),
		TEST(pset_match_it__empty),

		TEST(pset_add__again),

		TEST(pset_sort__empty),
		TEST(pset_sort__one),
		TEST(pset_sort__many),
		TEST(pset_sort__no_less_than),

		TEST(pset_equal__length_different),
		TEST(pset_equal__val_pointers_ok),
		TEST(pset_equal__val_pointers_different),
		TEST(pset_equal__equal_val_ok),
		TEST(pset_equal__equal_val_different),

		TEST(pset_vals_slist_shallow__empty),
		TEST(pset_vals_slist_shallow__many),
		TEST(pset_vals_slist_deep__clone_val),
		TEST(pset_vals_slist_deep__no_clone_val),

		TEST(pset_str__empty),
		TEST(pset_str__pointers),
		TEST(pset_str__str_val),

		TEST(pset__null_inputs),
	};

	return RUN(tests);
}

