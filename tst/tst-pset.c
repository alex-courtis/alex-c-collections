#include "assert-pset.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pslist.h"
#include "str.h"

#include "pset.h"

struct Pset {
	const struct PsetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static char* str_first(const void *val) {
	return strndup(val, 1);
}

static void pset_init__defaults(void **state) {
	const struct Pset *set = pset_init();

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
	const struct Pset *set = pset_init();

	const struct Pset *clone = pset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(pset_size(clone), 0);

	pset_free(set);
	pset_free(clone);
}

// also tests constructor
static void pset_clone__params(void **state) {
	const struct PsetParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.initial = 3,
		.grow  = 4,
	};
	const struct Pset *set = pset_init_with(params);

	const struct Pset *clone = pset_clone(set);

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

static void pset_clone__many(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct Pset *clone = pset_clone(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V0));
	assert_true(pset_contains(clone, V1));

	assert_pset_equal(set, clone);
	assert_pset_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone__alloc_val(void **state) {
	const struct PsetParams params = { .alloc_val = mock_alloc, };
	const struct Pset *set = pset_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Pset *clone = pset_clone(set);

	assert_int_equal(pset_size(clone), 1);

	assert_true(pset_contains(clone, V0));

	assert_pset_equal(set, clone);
	assert_pset_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__clone_val(void **state) {
	const struct PsetParams params = { .clone_val = mock_clone, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	assert_true(pset_add(set, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct Pset *clone = pset_clone_deep(set);

	assert_int_equal(pset_size(clone), 2);

	assert_true(pset_contains(clone, V2));
	assert_true(pset_contains(clone, V3));

	assert_pset_not_equal(clone, set);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__no_clone_val(void **state) {
	const struct Pset *from = pset_init();

	assert_true(pset_add(from, V0));

	assert_nul(pset_clone_deep(from));

	pset_free(from);
}

static void pset_free_vals__null_free_val(void **state) {
	const struct Pset *set = pset_init();

	const char *val = strdup("0");

	pset_add(set, val);

	assert_int_equal(pset_size(set), 1);

	pset_free_vals(set);
}

static void pset_free_vals__missing_val(void **state) {
	const struct Pset *set = pset_init();

	char *val = strdup("0");

	pset_add(set, val);

	assert_int_equal(pset_size(set), 1);

	set->vals[0] = NULL;

	pset_free_vals(set);
	free(val);
}

static void pset_free_vals__free_val(void **state) {
	const struct PsetParams params = { .free_val = mock_free, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);
	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	pset_free_vals(set);
}

static void pset_add__new(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	assert_false(pset_add(set, V1));

	assert_true(pset_contains(set, V0));
	assert_true(pset_contains(set, V1));

	pset_free(set);
}

static void pset_add__equal_val(void **state) {
	const struct PsetParams params = { .equal_val = mock_equal, };
	const struct Pset *set = pset_init_with(params);

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
	const struct PsetParams params = { .alloc_val = mock_alloc, };
	const struct Pset *set = pset_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(pset_add(set, V0));

	pset_free(set);
}

static void pset_add__alloc_val_returned_null(void **state) {
	const struct PsetParams params = { .alloc_val = mock_alloc, };
	const struct Pset *set = pset_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_false(pset_add(set, V0));

	assert_false(pset_contains(set, V0));

	assert_int_equal(pset_size(set), 0);

	pset_free(set);
}

static void pset_add__null(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));

	assert_int_equal(pset_size(set), 1);

	assert_false(pset_contains(set, NULL));
	assert_false(pset_add(set, NULL));
	assert_false(pset_contains(set, NULL));

	assert_int_equal(pset_size(set), 1);

	pset_free(set);
}

static void pset_add__grow(void **state) {
	const struct PsetParams params = { .initial = 2, .grow = 5 };
	const struct Pset *set = pset_init_with(params);

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
	const struct Pset *to = pset_init();
	assert_true(pset_add(to, V0));
	assert_true(pset_add(to, V1));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V2));

	assert_int_equal(pset_add_all(to, from), 1);

	assert_pset_equal(to, expected);

	pset_free(to);
	pset_free(from);
	pset_free(expected);
}

static void pset_add_all__alloc_val(void **state) {
	const struct PsetParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Pset *to = pset_init_with(params);
	assert_true(pset_add(to, V0));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_int_equal(pset_add_all(to, from), 1);

	assert_pset_equal(to, expected);

	pset_free(to);
	pset_free(from);
	pset_free(expected);
}

static void pset_add_all_clone__many(void **state) {
	const struct PsetParams params = { .clone_val = mock_clone, };
	const struct Pset *to = pset_init_with(params);

	assert_true(pset_add(to, V0));
	assert_true(pset_add(to, V1));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V2));

	expect_ptr(mock_clone, ptr, V2);
	will_return_ptr_type(mock_clone, V2, void*);

	assert_int_equal(pset_add_all_clone(to, from), 1);

	assert_pset_equal(to, expected);

	pset_free(to);
	pset_free(from);
	pset_free(expected);
}

static void pset_add_all_clone__no_clone_val(void **state) {
	const struct Pset *to = pset_init();
	assert_true(pset_add(to, V0));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V0));

	assert_int_equal(pset_add_all_clone(to, from), 0);

	assert_pset_equal(to, expected);

	pset_free(to);
	pset_free(from);
	pset_free(expected);
}

static void pset_add_many__many(void **state) {
	const struct Pset *to = pset_init();
	assert_true(pset_add(to, V0));
	assert_true(pset_add(to, V1));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V2));

	assert_int_equal(pset_add_many(to, V0, V1, V2, NULL), 1);

	assert_pset_equal(to, expected);

	pset_free(to);
	pset_free(expected);
}

static void pset_add_many__no_vals(void **state) {
	const struct Pset *to = pset_init();

	assert_int_equal(pset_add_many(to, NULL), 0);

	pset_free(to);
}

static void pset_at__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_at(set, 0));
	assert_nul(pset_at(set, 123));

	pset_free(set);
}

static void pset_at__many(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	assert_ptr_equal(pset_at(set, 0), V0);
	assert_ptr_equal(pset_at(set, 1), V1);
	assert_ptr_equal(pset_at(set, 2), V2);

	assert_nul(pset_at(set, 3));

	pset_free(set);
}

static void pset_remove__existing(void **state) {
	const struct Pset *set = pset_init();

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
	const struct Pset *set = pset_init();

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
	const struct PsetParams params = { .equal_val = mock_equal, };
	const struct Pset *set = pset_init_with(params);

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
	const struct PsetParams params = { .free_val = mock_free, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	expect_ptr(mock_free, ptr, V0);

	assert_true(pset_remove_free(set, V0));

	assert_false(pset_remove_free(set, NULL));

	assert_false(pset_remove_free(set, V2));

	pset_free(set);
}

static void pset_remove_free__free(void **state) {
	const struct Pset *set = pset_init();

	const char *val = strdup("0");

	assert_true(pset_add(set, val));

	assert_true(pset_remove_free(set, val));

	pset_free(set);
}

static void pset_remove_all__many(void **state) {
	const struct PsetParams params = { .free_val = mock_free, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	assert_int_equal(pset_remove_all(set, from), 1);

	pset_free(set);
	pset_free(from);
}

static void pset_remove_all_free__no_free_val(void **state) {
	const struct Pset *set = pset_init();

	const char *val = strdup("should be freed");

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, val));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, val));
	assert_true(pset_add(from, V2));

	assert_int_equal(pset_remove_all_free(set, from), 1);

	pset_free(set);
	pset_free(from);
}

static void pset_remove_all_free__free_val(void **state) {
	const struct PsetParams params = { .free_val = mock_free, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(pset_remove_all_free(set, from), 1);

	pset_free(set);
	pset_free(from);
}

static void pset_find__matches(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	// skip V0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// get V1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	assert_ptr_equal(pset_find(set, mock_2pred, D0), V1);

	pset_free(set);
}

static void pset_find__no_match(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	// skip V0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// skip V1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	assert_nul(pset_find(set, mock_2pred, D0));

	pset_free(set);
}

static void pset_find__null_match(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_find(set, mock_2pred, D0));

	pset_free(set);
}

static void pset_it__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_size(set), 0);

	assert_nul(pset_it(set));

	pset_free(set);
}

static void pset_it__free(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct PsetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	pset_it_free(it);

	pset_free(set);
}

static void pset_it__many(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(pset_size(set), 2);

	const struct PsetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_it__cleared(void **state) {
	const struct Pset *set = pset_init();

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
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	pset_it_free(it);
}

static void pset_it_next__partial(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	assert_nul(pset_it_next(it));
}

static void pset_filter_it__many(void **state) {
	const struct Pset *set = pset_init();

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

	const struct PsetIt *it = pset_filter_it(set, mock_equal, D0);
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

static void pset_filter_it__none(void **state) {
	const struct Pset *set = pset_init();

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

	assert_nul(pset_filter_it(set, mock_equal, D0));

	pset_free(set);
}

static void pset_filter_it__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_filter_it(set, mock_equal, D0));

	pset_free(set);
}

static void pset_it_remove__start(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	const struct Pset *expected = pset_init();

	assert_true(pset_add(expected, V2));

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		if (it->val == V0 || it->val == V1) {
			pset_it_remove(it);
		}
	}

	assert_int_equal(pset_size(set), 1);
	assert_int_equal(iterations, 3);

	assert_pset_equal(set, expected);

	pset_free(set);
	pset_free(expected);
}

static void pset_it_remove__mid(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));
	assert_true(pset_add(set, V3));
	assert_true(pset_add(set, V4));

	const struct Pset *expected = pset_init();

	assert_true(pset_add(expected, V0));
	assert_true(pset_add(expected, V2));
	assert_true(pset_add(expected, V4));

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			pset_it_remove(it);
		}
	}

	assert_int_equal(pset_size(set), 3);
	assert_int_equal(iterations, 5);

	assert_pset_equal(set, expected);

	pset_free(set);
	pset_free(expected);
}

static void pset_it_remove__end(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	const struct Pset *expected = pset_init();

	assert_true(pset_add(expected, V0));

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V2) {
			pset_it_remove(it);
		}
	}

	assert_int_equal(pset_size(set), 1);
	assert_int_equal(iterations, 3);

	assert_pset_equal(set, expected);

	pset_free(set);
	pset_free(expected);
}

static void pset_it_remove__all(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		pset_it_remove(it);
	}

	assert_int_equal(pset_size(set), 0);
	assert_int_equal(iterations, 3);

	pset_free(set);
}

static void pset_it_remove__partial(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	pset_it_remove(it);
}

static void pset_it_remove_free__many(void **state) {
	const struct PsetParams params = { .free_val = mock_free, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));
	assert_true(pset_add(set, V2));
	assert_true(pset_add(set, V3));
	assert_true(pset_add(set, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		if (it->val == V1 || it->val == V3) {
			pset_it_remove_free(it);
		}
	}

	pset_free(set);
}

static void pset_add__again(void **state) {
	const struct Pset *set = pset_init();

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
	const struct PsetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	// 2
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	// 3
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// 0 moved later
	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// end
	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_sort__empty(void **state) {
	const struct Pset *actual = pset_init();
	const struct Pset *expected = pset_init();

	pset_sort(actual, mock_less_than);

	assert_int_equal(pset_size(actual), 0);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__one(void **state) {
	const struct Pset *actual = pset_init();

	assert_true(pset_add(actual, V0));

	const struct Pset *expected = pset_init();
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
	const struct Pset *actual = pset_init();

	const int v[6] = { 0, 1, 2, 3, 4, 5 };

	assert_true(pset_add(actual, &v[2]));
	assert_true(pset_add(actual, &v[0]));
	assert_true(pset_add(actual, &v[3]));
	assert_true(pset_add(actual, &v[5]));
	assert_true(pset_add(actual, &v[1]));
	assert_true(pset_add(actual, &v[4]));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, &v[0]));
	assert_true(pset_add(expected, &v[1]));
	assert_true(pset_add(expected, &v[2]));
	assert_true(pset_add(expected, &v[3]));
	assert_true(pset_add(expected, &v[4]));
	assert_true(pset_add(expected, &v[5]));

	pset_sort(actual, test_less_than_int);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_sort__no_less_than(void **state) {
	const struct Pset *actual = pset_init();

	assert_true(pset_add(actual, V1));
	assert_true(pset_add(actual, V0));

	const struct Pset *expected = pset_init();
	assert_true(pset_add(expected, V1));
	assert_true(pset_add(expected, V0));

	pset_sort(actual, NULL);

	assert_pset_equal(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_equal__length_different(void **state) {
	const struct Pset *a = pset_init();
	const struct Pset *b = pset_init();

	assert_true(pset_add(a, V0));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V1));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__val_pointers_ok(void **state) {
	const struct Pset *a = pset_init();
	const struct Pset *b = pset_init();

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
	const struct Pset *a = pset_init();
	const struct Pset *b = pset_init();

	assert_true(pset_add(a, V0));
	assert_true(pset_add(a, V1));

	assert_true(pset_add(b, V0));
	assert_true(pset_add(b, V2));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__equal_val_ok(void **state) {
	const struct PsetParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Pset *a = pset_init_with(params);
	const struct Pset *b = pset_init_with(params);

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
	const struct PsetParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Pset *a = pset_init_with(params);
	const struct Pset *b = pset_init_with(params);

	assert_true(pset_add(a, "0"));
	assert_true(pset_add(a, "1"));

	assert_true(pset_add(b, "0"));
	assert_true(pset_add(b, "2"));

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_pslist__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_pslist(set));

	pset_free(set);
}

static void pset_pslist__many(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	struct Pslist *list = pset_pslist(set);

	assert_int_equal(pslist_length(list), 2);
	assert_ptr_equal(pslist_at(list, 0), V0);
	assert_ptr_equal(pslist_at(list, 1), V1);

	pslist_free(&list);
	pset_free(set);
}

static void pset_pslist__alloc_val(void **state) {
	const struct PsetParams params = { .alloc_val = mock_alloc, };
	const struct Pset *set = pset_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	struct Pslist *list = pset_pslist(set);

	assert_int_equal(pslist_length(list), 1);
	assert_ptr_equal(pslist_at(list, 0), V0);

	pslist_free(&list);
	pset_free(set);
}

static void pset_pslist_clone__clone_val(void **state) {
	const struct PsetParams params = { .clone_val = mock_clone, };
	const struct Pset *set = pset_init_with(params);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct Pslist *list = pset_pslist_clone(set);

	pslist_free(&list);
	pset_free(set);
}

static void pset_pslist_clone__no_clone_val(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));

	assert_nul(pset_pslist_clone(set));

	pset_free(set);
}

static void pset_str__empty(void **state) {
	const struct PsetParams params = { .str_val = mock_str, };
	const struct Pset *set = pset_init_with(params);

	char *str = pset_str(set);
	assert_str_equal(str, "");

	free(str);
	pset_free(set);
}

static void pset_str__pointers(void **state) {
	const struct Pset *set = pset_init();

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
	const struct PsetParams params = { .str_val = str_first, };
	const struct Pset *set = pset_init_with(params);

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
	const struct Pset *set = pset_init();

	assert_int_equal(pset_add_all(NULL, NULL), 0);
	assert_int_equal(pset_add_all(set, NULL), 0);
	assert_int_equal(pset_add_all_clone(NULL, NULL), 0);
	assert_int_equal(pset_add_all_clone(set, NULL), 0);
	assert_nul(pset_clone_deep(NULL));
	assert_nul(pset_clone(NULL));
	pset_free(NULL);
	pset_free_vals(NULL);
	pset_it_free(NULL);
	assert_false(pset_contains(NULL, NULL));
	assert_false(pset_contains(set, NULL));
	assert_nul(pset_at(NULL, 0));
	assert_nul(pset_find(NULL, NULL, NULL));
	assert_nul(pset_find(set, NULL, NULL));
	assert_nul(pset_it(NULL));
	assert_nul(pset_filter_it(NULL, NULL, NULL));
	assert_nul(pset_filter_it(set, NULL, NULL));
	assert_nul(pset_it_next(NULL));
	pset_it_remove(NULL);
	pset_it_remove_free(NULL);
	assert_false(pset_add(NULL, NULL));
	assert_false(pset_add(set, NULL));
	assert_int_equal(pset_add_many(NULL), 0);
	assert_int_equal(pset_add_many_v(NULL, NULL), 0);
	assert_false(pset_remove(NULL, NULL));
	assert_false(pset_remove(set, NULL));
	assert_false(pset_remove_free(NULL, NULL));
	assert_false(pset_remove_free(set, NULL));
	assert_int_equal(pset_remove_all(NULL, NULL), 0);
	assert_int_equal(pset_remove_all(set, NULL), 0);
	assert_int_equal(pset_remove_all(NULL, set), 0);
	assert_int_equal(pset_remove_all_free(NULL, NULL), 0);
	assert_int_equal(pset_remove_all_free(set, NULL), 0);
	assert_int_equal(pset_remove_all_free(NULL, set), 0);
	pset_sort(NULL, NULL);
	assert_false(pset_equal(NULL, NULL));
	assert_false(pset_equal(set, NULL));
	assert_nul(pset_pslist(NULL));
	assert_nul(pset_pslist_clone(NULL));
	assert_nul(pset_str(NULL));
	assert_int_equal(pset_size(NULL), 0);

	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__defaults),

		TEST(pset_clone__empty),
		TEST(pset_clone__params),
		TEST(pset_clone__many),
		TEST(pset_clone__alloc_val),

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
		TEST(pset_add_all__alloc_val),

		TEST(pset_add_all_clone__many),
		TEST(pset_add_all_clone__no_clone_val),

		TEST(pset_add_many__many),
		TEST(pset_add_many__no_vals),

		TEST(pset_at__empty),
		TEST(pset_at__many),

		TEST(pset_remove__existing),
		TEST(pset_remove__inexistent),
		TEST(pset_remove__equal_val),

		TEST(pset_remove_free__free_val),
		TEST(pset_remove_free__free),

		TEST(pset_remove_all__many),

		TEST(pset_remove_all_free__no_free_val),
		TEST(pset_remove_all_free__free_val),

		TEST(pset_find__matches),
		TEST(pset_find__no_match),
		TEST(pset_find__null_match),

		TEST(pset_it__empty),
		TEST(pset_it__free),
		TEST(pset_it__many),
		TEST(pset_it__cleared),

		TEST(pset_it_free__partial),

		TEST(pset_it_next__partial),

		TEST(pset_filter_it__many),
		TEST(pset_filter_it__none),
		TEST(pset_filter_it__empty),

		TEST(pset_it_remove__start),
		TEST(pset_it_remove__mid),
		TEST(pset_it_remove__end),
		TEST(pset_it_remove__all),
		TEST(pset_it_remove__partial),

		TEST(pset_it_remove_free__many),

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

		TEST(pset_pslist__empty),
		TEST(pset_pslist__many),
		TEST(pset_pslist__alloc_val),
		TEST(pset_pslist_clone__clone_val),
		TEST(pset_pslist_clone__no_clone_val),

		TEST(pset_str__empty),
		TEST(pset_str__pointers),
		TEST(pset_str__str_val),

		TEST(pset__null_inputs),
	};

	return RUN(tests);
}

