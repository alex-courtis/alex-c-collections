#include "assert-plist.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pslist.h"
#include "str.h"

#include "plist.h"

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static char* str_first(const void *val) {
	return strndup(val, 1);
}

static void plist_init__defaults(void **state) {
	const struct Plist *list = plist_init();

	assert_non_nul(list);

	assert_int_equal(list->size, 0);
	assert_int_equal(list->capacity, 10);

	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		plist_add(list, &v[i]);

	assert_int_equal(list->size, 25);
	assert_int_equal(list->capacity, 30);

	plist_free(list);
}

static void plist_clone__empty(void **state) {
	const struct Plist *list = plist_init();

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(plist_size(clone), 0);

	plist_free(list);
	plist_free(clone);
}

// also tests constructor
static void plist_clone__params(void **state) {
	const struct PlistParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.initial = 3,
		.grow  = 4,
	};
	const struct Plist *list = plist_init_with(params);

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(list->size, 0);
	assert_int_equal(list->capacity, 3);
	assert_int_equal(list->params.grow, 4);
	assert_ptr_equal(list->params.equal_val, mock_equal);
	assert_ptr_equal(list->params.alloc_val, mock_alloc);
	assert_ptr_equal(list->params.clone_val, mock_clone);

	plist_free(list);
	plist_free(clone);
}

static void plist_clone__many(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(plist_size(clone), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);
	assert_ptr_equal(plist_at(list, 2), V1);

	assert_plist_equal(list, clone);
	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_add(list, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(plist_size(clone), 1);

	assert_ptr_equal(plist_at(clone, 0), V0);

	assert_plist_equal(list, clone);
	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__clone_val(void **state) {
	const struct PlistParams params = { .clone_val = mock_clone, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));

	assert_true(plist_add(list, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct Plist *clone = plist_clone_deep(list);

	assert_int_equal(plist_size(clone), 2);

	assert_ptr_equal(plist_at(clone, 0), V2);
	assert_ptr_equal(plist_at(clone, 1), V3);

	assert_plist_not_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__no_clone_val(void **state) {
	const struct Plist *from = plist_init();

	assert_true(plist_add(from, V0));

	assert_nul(plist_clone_deep(from));

	plist_free(from);
}

static void plist_free_vals__null_free_val(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("0");

	plist_add(list, val);

	assert_int_equal(plist_size(list), 1);

	assert_ptr_equal(plist_at(list, 0), val);

	plist_free_vals(list);
}

static void plist_free_vals__missing_val(void **state) {
	const struct Plist *list = plist_init();

	char *val = strdup("0");

	plist_add(list, val);
	plist_add(list, val);

	assert_int_equal(plist_size(list), 2);

	list->vals[0] = NULL;
	list->vals[1] = NULL;

	plist_free_vals(list);
	free(val);
}

static void plist_free_vals__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_size(list), 3);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	plist_free_vals(list);
}

static void plist_add__new(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V1);

	plist_free(list);
}

static void plist_add__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_add(list, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_add(list, V0));

	assert_int_equal(plist_size(list), 2);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);

	plist_free(list);
}

static void plist_add__alloc_val_returned_null(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_false(plist_add(list, V0));

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_add__null(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));

	assert_int_equal(plist_size(list), 1);

	assert_false(plist_add(list, NULL));

	assert_int_equal(plist_size(list), 1);

	plist_free(list);
}

static void plist_add__grow(void **state) {
	const struct PlistParams params = { .initial = 2, .grow = 5 };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V0));

	assert_int_equal(list->size, 2);
	assert_int_equal(list->capacity, 2);
	assert_int_equal(list->params.grow, 5);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);

	assert_true(plist_add(list, V1));
	assert_int_equal(list->size, 3);
	assert_int_equal(list->capacity, 7);
	assert_ptr_equal(plist_at(list, 2), V1);

	assert_true(plist_add(list, V1));
	assert_int_equal(list->size, 4);
	assert_int_equal(list->capacity, 7);
	assert_ptr_equal(plist_at(list, 3), V1);

	assert_true(plist_add(list, V2));
	assert_true(plist_add(list, V2));
	assert_int_equal(list->size, 6);
	assert_int_equal(list->capacity, 7);

	assert_ptr_equal(plist_at(list, 4), V2);

	plist_free(list);
}

static void plist_add_all__many(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_add(to, V0));
	assert_true(plist_add(to, V0));
	assert_true(plist_add(to, V1));

	assert_int_equal(plist_size(to), 3);

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V1));
	assert_true(plist_add(from, V2));

	assert_int_equal(plist_size(from), 2);

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V1));
	assert_true(plist_add(expected, V1));
	assert_true(plist_add(expected, V2));

	assert_int_equal(plist_size(expected), 5);

	assert_int_equal(plist_add_all(to, from), 2);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_add_all__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *to = plist_init_with(params);
	assert_true(plist_add(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V0));
	assert_true(plist_add(from, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V1));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_int_equal(plist_add_all(to, from), 2);
	assert_int_equal(plist_size(to), 3);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_add_all_clone__many(void **state) {
	const struct PlistParams params = { .clone_val = mock_clone, };
	const struct Plist *to = plist_init_with(params);

	assert_true(plist_add(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V0));
	assert_true(plist_add(from, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);

	assert_int_equal(plist_add_all_clone(to, from), 2);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_add_all_clone__no_clone_val(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_add(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));

	assert_int_equal(plist_add_all_clone(to, from), 0);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_add_many__many(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_add(to, V0));
	assert_true(plist_add(to, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V1));
	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V1));
	assert_true(plist_add(expected, V2));

	assert_int_equal(plist_add_many(to, V0, V1, V2, NULL), 3);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(expected);
}

static void plist_add_many__no_vals(void **state) {
	const struct Plist *to = plist_init();

	assert_int_equal(plist_add_many(to, NULL), 0);

	plist_free(to);
}

static void plist_at__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_at(list, 0));
	assert_nul(plist_at(list, 123));

	plist_free(list);
}

static void plist_at__many(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V2));

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V1);
	assert_ptr_equal(plist_at(list, 3), V2);

	assert_nul(plist_at(list, 4));

	plist_free(list);
}

static void plist_remove__existing(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_size(list), 3);

	// 0
	assert_int_equal(plist_remove(list, V0), 2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V1);

	// 1
	assert_int_equal(plist_remove(list, V1), 1);

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_remove__inexistent(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_size(list), 2);

	assert_int_equal(plist_remove(list, V2), 0);

	assert_int_equal(plist_remove(list, NULL), 0);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);

	plist_free(list);
}

static void plist_remove__equal_val(void **state) {
	const struct PlistParams params = { .equal_val = mock_equal, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_int_equal(plist_remove(list, V0), 1);

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_remove_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_size(list), 3);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);
	assert_ptr_equal(plist_at(list, 2), V1);

	// only free V0 once
	expect_ptr_count(mock_free, ptr, V0, 1);

	// 0
	assert_int_equal(plist_remove_free(list, V0), 2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V1);

	// 1
	assert_int_equal(plist_remove(list, V1), 1);

	assert_int_equal(plist_size(list), 0);
	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_free__free(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("0");

	assert_true(plist_add(list, val));
	assert_true(plist_add(list, val));

	assert_int_equal(plist_remove_free(list, val), 2);

	assert_int_equal(plist_size(list), 0);
	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_all__many(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all(list), 0);

	assert_int_equal(plist_size(list), 0);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));

	assert_int_equal(plist_remove_all(list), 3);

	assert_int_equal(plist_size(list), 0);
	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_all_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all_free(list), 0);

	const char *val0 = strdup("0");
	const char *val1 = strdup("1");
	const char *val2 = strdup("2");

	assert_true(plist_add(list, val0));
	assert_true(plist_add(list, val1));
	assert_true(plist_add(list, val1));
	assert_true(plist_add(list, val2));
	assert_true(plist_add(list, val1));
	assert_true(plist_add(list, val0));
	assert_true(plist_add(list, val2));

	assert_int_equal(plist_remove_all_free(list), 7);

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_remove_all_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_int_equal(plist_remove_all_free(list), 0);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V0));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(plist_remove_all_free(list), 4);

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_in__many(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V1));
	assert_true(plist_add(from, V2));

	assert_int_equal(plist_remove_in(list, from), 2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
	plist_free(from);
}

static void plist_remove_in_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("should be freed");

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, val));
	assert_true(plist_add(list, val));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, val));
	assert_true(plist_add(from, V2));

	assert_int_equal(plist_remove_in_free(list, from), 2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
	plist_free(from);
}

static void plist_remove_in_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_add(list, V0));
	assert_true(plist_add(list, V1));
	assert_true(plist_add(list, V1));

	const struct Plist *from = plist_init();
	assert_true(plist_add(from, V1));
	assert_true(plist_add(from, V1));
	assert_true(plist_add(from, V2));

	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(plist_remove_in_free(list, from), 2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
	plist_free(from);
}

static void plist_find__empty_filter(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));

	const struct PlistFilter filter = { 0 };

	assert_ptr_equal(plist_find(set, filter), V0);

	plist_free(set);
}

static void plist_find__empty_set(void **state) {
	const struct Plist *set = plist_init();

	const struct PlistFilter filter = {
		.val = mock_pred_p,
		.data = D0,
		.val_data = mock_pred_p_p,
	};

	assert_nul(plist_find(set, filter));

	plist_free(set);
}

static void plist_find__val(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	// skip V0
	expect_ptr(mock_pred_p, p, V0);
	will_return(mock_pred_p, false);

	// get V1
	expect_ptr(mock_pred_p, p, V1);
	will_return(mock_pred_p, true);

	const struct PlistFilter filter = { .val = mock_pred_p, };

	assert_ptr_equal(plist_find(set, filter), V1);

	plist_free(set);
}

static void plist_find__val_data(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get V1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };

	assert_ptr_equal(plist_find(set, filter), V1);

	plist_free(set);
}

static void plist_it__empty(void **state) {
	const struct Plist *set = plist_init();

	assert_int_equal(plist_size(set), 0);

	assert_nul(plist_it(set));

	plist_free(set);
}

static void plist_it__free(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));

	const struct PlistIt *it = plist_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);

	plist_free(set);
}

static void plist_it__many(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));

	assert_int_equal(plist_size(set), 2);

	const struct PlistIt *it = plist_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_next(it);
	assert_nul(it);

	plist_free(set);
}

static void plist_it__cleared(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));

	assert_int_equal(plist_size(set), 2);

	assert_int_equal(plist_remove(set, V0), 1);
	assert_int_equal(plist_remove(set, V1), 1);

	assert_int_equal(plist_size(set), 0);

	assert_nul(plist_it(set));

	plist_free(set);
}

static void plist_it_free__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	plist_it_free(it);
}

static void plist_it_next__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_next(it));
}

static void plist_filter_it__empty_filter(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	const struct PlistFilter filter = { 0 };

	const struct PlistIt *it = plist_filter_it(set, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);

	plist_free(set);
}

static void plist_filter_it__empty_set(void **state) {
	const struct Plist *set = plist_init();

	const struct PlistFilter filter = {
		.val = mock_pred_p,
		.val_data = mock_pred_p_p,
		.data = D0,
	};
	assert_nul(plist_filter_it(set, filter));

	plist_free(set);
}

static void plist_filter_it__many(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));
	assert_true(plist_add(set, V3));
	assert_true(plist_add(set, V4));

	assert_int_equal(plist_size(set), 5);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	const struct PlistIt *it = plist_filter_it(set, filter);

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// skip K2
	expect_ptr(mock_pred_p_p, p1, V2);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get K3
	expect_ptr(mock_pred_p_p, p1, V3);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// skip K4
	expect_ptr(mock_pred_p_p, p1, V4);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// done
	it = plist_it_next(it);
	assert_nul(it);

	plist_free(set);
}

static void plist_filter_it__none(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));

	assert_int_equal(plist_size(set), 1);

	// skip K0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	assert_nul(plist_filter_it(set, filter));

	plist_free(set);
}

static void plist_it_remove__start(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	const struct Plist *expected = plist_init();

	assert_true(plist_add(expected, V2));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it(set); it; it = plist_it_next(it)) {
		iterations++;
		if (it->val == V0 || it->val == V1) {
			plist_it_remove(it);
		}
	}

	assert_int_equal(plist_size(set), 1);
	assert_int_equal(iterations, 3);

	assert_plist_equal(set, expected);

	plist_free(set);
	plist_free(expected);
}

static void plist_it_remove__mid(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));
	assert_true(plist_add(set, V3));
	assert_true(plist_add(set, V4));

	const struct Plist *expected = plist_init();

	assert_true(plist_add(expected, V0));
	assert_true(plist_add(expected, V2));
	assert_true(plist_add(expected, V4));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it(set); it; it = plist_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			plist_it_remove(it);
		}
	}

	assert_int_equal(plist_size(set), 3);
	assert_int_equal(iterations, 5);

	assert_plist_equal(set, expected);

	plist_free(set);
	plist_free(expected);
}

static void plist_it_remove__end(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	const struct Plist *expected = plist_init();

	assert_true(plist_add(expected, V0));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it(set); it; it = plist_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V2) {
			plist_it_remove(it);
		}
	}

	assert_int_equal(plist_size(set), 1);
	assert_int_equal(iterations, 3);

	assert_plist_equal(set, expected);

	plist_free(set);
	plist_free(expected);
}

static void plist_it_remove__all(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it(set); it; it = plist_it_next(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(plist_size(set), 0);
	assert_int_equal(iterations, 3);

	plist_free(set);
}

static void plist_it_remove__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	plist_it_remove(it);
}

static void plist_it_remove_free__many(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *set = plist_init_with(params);

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));
	assert_true(plist_add(set, V3));
	assert_true(plist_add(set, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	for (const struct PlistIt *it = plist_it(set); it; it = plist_it_next(it)) {
		if (it->val == V1 || it->val == V3) {
			plist_it_remove_free(it);
		}
	}

	plist_free(set);
}

static void plist_add__again(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));
	assert_true(plist_add(set, V3));

	assert_int_equal(plist_size(set), 4);

	// remove 1
	assert_int_equal(plist_remove(set, V1), 1);
	assert_int_equal(plist_size(set), 3);

	// put 1 again afterwards
	assert_true(plist_add(set, V1));
	assert_int_equal(plist_size(set), 4);

	// 0
	const struct PlistIt *it = plist_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	// 2
	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	// 3
	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// 0 moved later
	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// end
	it = plist_it_next(it);
	assert_nul(it);

	plist_free(set);
}

static void plist_sort__empty(void **state) {
	const struct Plist *actual = plist_init();
	const struct Plist *expected = plist_init();

	plist_sort(actual, mock_less_than);

	assert_int_equal(plist_size(actual), 0);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_sort__one(void **state) {
	const struct Plist *actual = plist_init();

	assert_true(plist_add(actual, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V0));

	plist_sort(actual, mock_less_than);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static bool test_less_than_int(const void* const a, const void* const b) {
	return *(int*)a < *(int*)b;
}

static void plist_sort__many(void **state) {
	const struct Plist *actual = plist_init();

	const int v[6] = { 0, 1, 2, 3, 4, 5 };

	assert_true(plist_add(actual, &v[2]));
	assert_true(plist_add(actual, &v[0]));
	assert_true(plist_add(actual, &v[3]));
	assert_true(plist_add(actual, &v[5]));
	assert_true(plist_add(actual, &v[1]));
	assert_true(plist_add(actual, &v[4]));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, &v[0]));
	assert_true(plist_add(expected, &v[1]));
	assert_true(plist_add(expected, &v[2]));
	assert_true(plist_add(expected, &v[3]));
	assert_true(plist_add(expected, &v[4]));
	assert_true(plist_add(expected, &v[5]));

	plist_sort(actual, test_less_than_int);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_sort__no_less_than(void **state) {
	const struct Plist *actual = plist_init();

	assert_true(plist_add(actual, V1));
	assert_true(plist_add(actual, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_add(expected, V1));
	assert_true(plist_add(expected, V0));

	plist_sort(actual, NULL);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_equal__length_different(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_add(a, V0));

	assert_true(plist_add(b, V0));
	assert_true(plist_add(b, V1));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_ok(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_add(a, V0));
	assert_true(plist_add(a, V1));

	assert_plist_not_equal(a, NULL);

	assert_true(plist_add(b, V0));
	assert_true(plist_add(b, V1));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_different(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_add(a, V0));
	assert_true(plist_add(a, V1));

	assert_true(plist_add(b, V0));
	assert_true(plist_add(b, V2));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val_ok(void **state) {
	const struct PlistParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Plist *a = plist_init_with(params);
	const struct Plist *b = plist_init_with(params);

	assert_true(plist_add(a, V0));
	assert_true(plist_add(a, V1));

	assert_true(plist_add(b, V0));
	assert_true(plist_add(b, V1));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val_different(void **state) {
	const struct PlistParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Plist *a = plist_init_with(params);
	const struct Plist *b = plist_init_with(params);

	assert_true(plist_add(a, "0"));
	assert_true(plist_add(a, "1"));

	assert_true(plist_add(b, "0"));
	assert_true(plist_add(b, "2"));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_pslist__empty(void **state) {
	const struct Plist *set = plist_init();

	assert_nul(plist_pslist(set));

	plist_free(set);
}

static void plist_pslist__many(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));

	struct Pslist *list = plist_pslist(set);

	assert_int_equal(pslist_length(list), 2);
	assert_ptr_equal(pslist_at(list, 0), V0);
	assert_ptr_equal(pslist_at(list, 1), V1);

	pslist_free(&list);
	plist_free(set);
}

static void plist_pslist__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *set = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_add(set, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	struct Pslist *list = plist_pslist(set);

	assert_int_equal(pslist_length(list), 1);
	assert_ptr_equal(pslist_at(list, 0), V0);

	pslist_free(&list);
	plist_free(set);
}

static void plist_pslist_clone__clone_val(void **state) {
	const struct PlistParams params = { .clone_val = mock_clone, };
	const struct Plist *set = plist_init_with(params);

	assert_true(plist_add(set, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct Pslist *list = plist_pslist_clone(set);

	pslist_free(&list);
	plist_free(set);
}

static void plist_pslist_clone__no_clone_val(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));

	assert_nul(plist_pslist_clone(set));

	plist_free(set);
}

static void plist_str__empty(void **state) {
	const struct PlistParams params = { .str_val = mock_str, };
	const struct Plist *set = plist_init_with(params);

	char *str = plist_str(set);
	assert_str_equal(str, "");

	free(str);
	plist_free(set);
}

static void plist_str__pointers(void **state) {
	const struct Plist *set = plist_init();

	assert_true(plist_add(set, V0));
	assert_true(plist_add(set, V1));
	assert_true(plist_add(set, V2));

	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n"
			"%p\n",
			V0,
			V1,
			V2
			);

	char *actual = plist_str(set);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	plist_free(set);
}

static void plist_str__str_val(void **state) {
	const struct PlistParams params = { .str_val = str_first, };
	const struct Plist *set = plist_init_with(params);

	assert_true(plist_add(set, "ONE"));
	assert_true(plist_add(set, "TWO"));
	assert_true(plist_add(set, "THREE"));

	char *str = plist_str(set);
	assert_str_equal(str,
			"O\n"
			"T\n"
			"T\n"
			);

	free(str);
	plist_free(set);
}

static void plist__null_inputs(void **state) {
	const struct Plist *set = plist_init();
	const struct PlistFilter filter = { 0 };

	assert_int_equal(plist_add_all(NULL, NULL), 0);
	assert_int_equal(plist_add_all(set, NULL), 0);
	assert_int_equal(plist_add_all_clone(NULL, NULL), 0);
	assert_int_equal(plist_add_all_clone(set, NULL), 0);
	assert_nul(plist_clone_deep(NULL));
	assert_nul(plist_clone(NULL));
	plist_free(NULL);
	plist_free_vals(NULL);
	plist_it_free(NULL);
	assert_false(plist_contains(NULL, NULL));
	assert_false(plist_contains(set, NULL));
	assert_nul(plist_at(NULL, 0));
	assert_nul(plist_find(NULL, filter));
	assert_nul(plist_it(NULL));
	assert_nul(plist_filter_it(NULL, filter));
	assert_nul(plist_it_next(NULL));
	plist_it_remove(NULL);
	plist_it_remove_free(NULL);
	assert_false(plist_add(NULL, NULL));
	assert_false(plist_add(set, NULL));
	assert_int_equal(plist_add_many(NULL), 0);
	assert_int_equal(plist_add_many_v(NULL, NULL), 0);
	assert_int_equal(plist_remove(NULL, NULL), 0);
	assert_int_equal(plist_remove(set, NULL), 0);
	assert_int_equal(plist_remove_free(NULL, NULL), 0);
	assert_int_equal(plist_remove_free(set, NULL), 0);
	assert_int_equal(plist_remove_all(NULL), 0);
	assert_int_equal(plist_remove_all_free(NULL), 0);
	assert_int_equal(plist_remove_in(NULL, NULL), 0);
	assert_int_equal(plist_remove_in(set, NULL), 0);
	assert_int_equal(plist_remove_in(NULL, set), 0);
	assert_int_equal(plist_remove_in_free(NULL, NULL), 0);
	assert_int_equal(plist_remove_in_free(set, NULL), 0);
	assert_int_equal(plist_remove_in_free(NULL, set), 0);
	plist_sort(NULL, NULL);
	assert_false(plist_equal(NULL, NULL));
	assert_false(plist_equal(set, NULL));
	assert_nul(plist_pslist(NULL));
	assert_nul(plist_pslist_clone(NULL));
	assert_nul(plist_str(NULL));
	assert_int_equal(plist_size(NULL), 0);

	plist_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(plist_init__defaults),

		TEST(plist_clone__empty),
		TEST(plist_clone__params),
		TEST(plist_clone__many),
		TEST(plist_clone__alloc_val),

		TEST(plist_clone_deep__clone_val),
		TEST(plist_clone_deep__no_clone_val),

		TEST(plist_free_vals__null_free_val),
		TEST(plist_free_vals__missing_val),
		TEST(plist_free_vals__free_val),

		TEST(plist_add__new),
		TEST(plist_add__alloc_val),
		TEST(plist_add__alloc_val_returned_null),
		TEST(plist_add__null),
		TEST(plist_add__grow),

		TEST(plist_add_all__many),
		TEST(plist_add_all__alloc_val),

		TEST(plist_add_all_clone__many),
		TEST(plist_add_all_clone__no_clone_val),

		TEST(plist_add_many__many),
		TEST(plist_add_many__no_vals),

		// TODO explicit contains

		TEST(plist_at__empty),
		TEST(plist_at__many),

		TEST(plist_remove__existing),
		TEST(plist_remove__inexistent),
		TEST(plist_remove__equal_val),

		TEST(plist_remove_free__free_val),
		TEST(plist_remove_free__free),

		TEST(plist_remove_all__many),

		TEST(plist_remove_all_free__no_free_val),
		TEST(plist_remove_all_free__free_val),

		TEST(plist_remove_in__many),

		TEST(plist_remove_in_free__no_free_val),
		TEST(plist_remove_in_free__free_val),
		//
		// TEST(plist_find__empty_filter),
		// TEST(plist_find__empty_set),
		//
		// TEST(plist_find__val),
		// TEST(plist_find__val_data),
		//
		// TEST(plist_it__empty),
		// TEST(plist_it__free),
		// TEST(plist_it__many),
		// TEST(plist_it__cleared),
		//
		// TEST(plist_it_free__partial),
		//
		// TEST(plist_it_next__partial),
		//
		// TEST(plist_filter_it__empty_filter),
		// TEST(plist_filter_it__empty_set),
		// TEST(plist_filter_it__many),
		// TEST(plist_filter_it__none),
		//
		// TEST(plist_it_remove__start),
		// TEST(plist_it_remove__mid),
		// TEST(plist_it_remove__end),
		// TEST(plist_it_remove__all),
		// TEST(plist_it_remove__partial),
		//
		// TEST(plist_it_remove_free__many),
		//
		// TEST(plist_add__again),
		//
		// TEST(plist_sort__empty),
		// TEST(plist_sort__one),
		// TEST(plist_sort__many),
		// TEST(plist_sort__no_less_than),
		//
		// TEST(plist_equal__length_different),
		// TEST(plist_equal__val_pointers_ok),
		// TEST(plist_equal__val_pointers_different),
		// TEST(plist_equal__equal_val_ok),
		// TEST(plist_equal__equal_val_different),
		//
		// TEST(plist_pslist__empty),
		// TEST(plist_pslist__many),
		// TEST(plist_pslist__alloc_val),
		// TEST(plist_pslist_clone__clone_val),
		// TEST(plist_pslist_clone__no_clone_val),
		//
		// TEST(plist_str__empty),
		// TEST(plist_str__pointers),
		// TEST(plist_str__str_val),
		//
		// TEST(plist__null_inputs),
	};

	return RUN(tests);
}

