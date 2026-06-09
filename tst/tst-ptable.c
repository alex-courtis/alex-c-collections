#include "tst.h"
#include "asserts.h"
#include "assert-ptable.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "ptable.h"

struct PTable {
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal_key;
	fn_alloc alloc_key;
	fn_free free_key;
	fn_str str_key;
	fn_clone clone_val;
};

static int keys[6] = { 10, 11, 12, 13, 14, 15, };
static void *K0 = &keys[0];
static void *K1 = &keys[1];
static void *K2 = &keys[2];
static void *K3 = &keys[3];
static void *K4 = &keys[4];
static void *K5 = &keys[5];

static int vals[6] = { 20, 21, 22, 23, 24, 25, };
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];
static void *V3 = &vals[3];
static void *V4 = &vals[4];
static void *V5 = &vals[5];

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

static void ptable_init__size(void **state) {
	const struct PTableParams params = { .initial = 2, .grow = 4, };
	const struct PTable *tab = ptable_init_with(params);

	assert_non_nul(tab);

	assert_int_equal(tab->size, 0);
	assert_int_equal(tab->capacity, 2);
	assert_int_equal(tab->grow, 4);

	ptable_free(tab);
}

static void ptable_init__defaults(void **state) {
	const struct PTable *tab = ptable_init();

	assert_non_nul(tab);

	assert_int_equal(tab->size, 0);
	assert_int_equal(tab->capacity, 10);
	assert_int_equal(tab->grow, 10);

	ptable_free(tab);
}

static void ptable_clone__empty(void **state) {
	const struct PTable *from = ptable_init();

	const struct PTable *to = ptable_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone__params(void **state) {
	const struct PTableParams params = {
		.equal_key = mock_equal,
		.alloc_key = mock_alloc,
		.free_key = mock_free,
		.str_key = mock_str,
		.clone_val = mock_clone,
		.initial = 99,
		.grow = 1,
	};
	const struct PTable *from = ptable_init_with(params);

	const struct PTable *to = ptable_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);
	assert_int_equal(to->capacity, 99);
	assert_int_equal(to->grow, 1);
	assert_ptr_equal(to->equal_key, mock_equal);
	assert_ptr_equal(to->alloc_key, mock_alloc);
	assert_ptr_equal(to->free_key, mock_free);
	assert_ptr_equal(to->str_key, mock_str);
	assert_ptr_equal(to->clone_val, mock_clone);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone__shallow_many(void **state) {
	const struct PTable *from = ptable_init();

	assert_nul(ptable_put(from, K0, NULL));
	assert_nul(ptable_put(from, K1, V1));
	assert_nul(ptable_put(from, K2, NULL));
	assert_nul(ptable_put(from, K3, V3));
	assert_nul(ptable_put(from, K4, NULL));

	const struct PTable *to = ptable_clone(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 5);

	assert_ptable_equal(from, to, NULL, NULL);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone__deep_many(void **state) {
	const struct PTableParams params = { .clone_val = mock_clone, };
	const struct PTable *from = ptable_init_with(params);

	assert_nul(ptable_put(from, K0, V0));
	assert_nul(ptable_put(from, K1, V1));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, val, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PTable *to = ptable_clone(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 2);

	assert_ptable_not_equal(from, to, NULL, NULL);

	assert_ptr_equal(ptable_get(to, K0), V2);
	assert_ptr_equal(ptable_get(to, K1), V3);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone__alloc_key(void **state) {
	const struct PTableParams params = { .alloc_key = mock_alloc, };
	const struct PTable *from = ptable_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);
	assert_nul(ptable_put(from, K0, V0));

	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K1, void*);
	assert_nul(ptable_put(from, K1, V1));

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K2, void*);
	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K3, void*);

	const struct PTable *to = ptable_clone(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 2);

	assert_ptable_not_equal(from, to, NULL, NULL);

	assert_ptr_equal(ptable_get(to, K2), V0);
	assert_ptr_equal(ptable_get(to, K3), V1);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_free_vals__null_free_val(void **state) {
	const struct PTable *tab = ptable_init();

	const char *val = strdup("0");

	ptable_put(tab, K0, val);

	assert_int_equal(ptable_size(tab), 1);

	ptable_free_vals(tab, NULL);
}

static void ptable_free_vals__free_val(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	assert_int_equal(ptable_size(tab), 3);

	expect_ptr(mock_free, val, V0);
	expect_ptr(mock_free, val, V2);

	ptable_free_vals(tab, mock_free);
}

static void fn_free_ptable(const void *val) {
	ptable_free_vals(val, mock_free);
}

static void ptable_free_vals__free_val_hierarchical(void **state) {
	const struct PTable *outer = ptable_init();
	const struct PTable *inner1 = ptable_init();
	const struct PTable *inner2 = ptable_init();

	ptable_put(outer, K0, (void*)inner1);
	ptable_put(outer, K1, (void*)inner2);

	ptable_put(inner1, K2, V2);
	ptable_put(inner1, K3, V3);

	ptable_put(inner2, K4, V4);
	ptable_put(inner2, K5, V5);

	assert_int_equal(ptable_size(outer), 2);

	expect_ptr(mock_free, val, V2);
	expect_ptr(mock_free, val, V3);
	expect_ptr(mock_free, val, V4);
	expect_ptr(mock_free, val, V5);

	ptable_free_vals(outer, fn_free_ptable);
}

static void ptable_put__new(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);

	ptable_free(tab);
}

static void ptable_put__overwrite(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));
	assert_nul(ptable_put(tab, K3, V3));

	assert_ptr_equal(ptable_put(tab, K1, V4), V1);

	assert_ptr_equal(ptable_put(tab, K3, V5), V3);

	assert_int_equal(ptable_size(tab), 4);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V4);
	assert_ptr_equal(ptable_get(tab, K2), V2);
	assert_ptr_equal(ptable_get(tab, K3), V5);

	ptable_free(tab);
}

static void ptable_put__null(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_int_equal(ptable_size(tab), 1);

	assert_nul(ptable_put(tab, K1, NULL));
	assert_int_equal(ptable_size(tab), 2);

	assert_nul(ptable_put(tab, K2, V2));
	assert_int_equal(ptable_size(tab), 3);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_ptr_equal(ptable_get(tab, K2), V2);

	ptable_free(tab);
}

static void ptable_put__null_overwrite(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));

	assert_ptr_equal(ptable_get(tab, K0), V0);

	assert_ptr_equal(ptable_put(tab, K0, NULL), V0);

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, K0));

	ptable_free(tab);
}

static void ptable_put__grow(void **state) {
	const struct PTableParams params = { .initial = 3, .grow = 5, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(tab->size, 3);
	assert_int_equal(tab->capacity, 3);
	assert_int_equal(tab->grow, 5);

	assert_nul(ptable_put(tab, K3, V3));

	assert_int_equal(tab->size, 4);
	assert_int_equal(tab->capacity, 8);
	assert_int_equal(tab->grow, 5);

	assert_nul(ptable_put(tab, K4, V4));
	assert_nul(ptable_put(tab, K5, V5));

	assert_int_equal(tab->size, 6);
	assert_int_equal(tab->capacity, 8);
	assert_int_equal(tab->grow, 5);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	assert_ptr_equal(ptable_get(tab, K3), V3);
	assert_ptr_equal(ptable_get(tab, K4), V4);
	assert_ptr_equal(ptable_get(tab, K5), V5);

	ptable_free(tab);
}

static void ptable__equal_key(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_ptr, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);

	assert_ptr_equal(ptable_put(tab, K0, V2), V0);

	assert_ptr_equal(ptable_remove(tab, K1), V1);

	ptable_free(tab);
}

static const void *fn_alloc_key_duplicate(const void* const val) {
	return sprintf_alloc("%s%s", (char*)val, (char*)val);
}

static void ptable__alloc_key_free_key(void **state) {
	const struct PTableParams params = {
		.equal_key = fn_equal_strcmp,
		.alloc_key = fn_alloc_key_duplicate,
		.free_key = (fn_free)free,
	};
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, "zero", V0));
	assert_nul(ptable_put(tab, "one", V1));

	assert_ptr_equal(ptable_get(tab, "zerozero"), V0);
	assert_ptr_equal(ptable_get(tab, "oneone"), V1);

	assert_ptr_equal(ptable_remove(tab, "zerozero"), V0);

	assert_int_equal(ptable_size(tab), 1);
	assert_ptr_equal(ptable_get(tab, "oneone"), V1);

	ptable_free(tab);
}

static void ptable_iter__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_iter(tab));

	ptable_free(tab);
}

static void ptable_iter__free(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_ptr_equal(ptable_iter_val(iter), V0);

	ptable_iter_free(iter);

	ptable_free(tab);
}

static void ptable_iter__many(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, NULL));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, NULL));
	assert_nul(ptable_put(tab, K3, V3));
	assert_nul(ptable_put(tab, K4, NULL));

	assert_int_equal(ptable_size(tab), 5);

	// zero
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_nul(ptable_iter_val(iter));

	// one
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// two
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K2);
	assert_nul(ptable_iter_val(iter));

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K3);
	assert_ptr_equal(ptable_iter_val(iter), V3);

	// four
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K4);
	assert_nul(ptable_iter_val(iter));

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_iter__removed(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));
	assert_nul(ptable_put(tab, K3, V3));
	assert_nul(ptable_put(tab, K4, V4));

	assert_ptr_equal(ptable_remove(tab, K0), V0);

	assert_ptr_equal(ptable_remove(tab, K2), V2);

	assert_ptr_equal(ptable_remove(tab, K4), V4);

	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K3);
	assert_ptr_equal(ptable_iter_val(iter), V3);

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_filter_iter__many(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));
	assert_nul(ptable_put(tab, K3, V3));
	assert_nul(ptable_put(tab, K4, V4));

	assert_int_equal(ptable_size(tab), 5);

	// skip K0
	expect_ptr(mock_test, val, K0);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// get K1
	expect_ptr(mock_test, val, K1);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);
	expect_ptr(mock_test, val, V1);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);

	const struct PTableIter *iter = ptable_filter_iter(tab, mock_test, mock_test, D0);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// skip V2
	expect_ptr(mock_test, val, K2);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);
	expect_ptr(mock_test, val, V2);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// get V3
	expect_ptr(mock_test, val, K3);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);
	expect_ptr(mock_test, val, V3);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);

	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K3);
	assert_ptr_equal(ptable_iter_val(iter), V3);

	// skip V4
	expect_ptr(mock_test, val, K4);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, true);
	expect_ptr(mock_test, val, V4);
	expect_ptr(mock_test, data, D0);
	will_return(mock_test, false);

	// done
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_put__again(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);

	// remove zero
	assert_ptr_equal(ptable_remove(tab, K0), V0);

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, K0));

	// put zero again afterwards
	assert_nul(ptable_put(tab, K0, V0));
	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K1);
	assert_ptr_equal(ptable_iter_val(iter), V1);

	// zero moved later
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(ptable_iter_key(iter), K0);
	assert_ptr_equal(ptable_iter_val(iter), V0);

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_remove__existing(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(ptable_size(tab), 3);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	// K1
	assert_ptr_equal(ptable_remove(tab, K1), V1);
	assert_int_equal(ptable_size(tab), 2);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_ptr_equal(ptable_get(tab, K2), V2);

	// K2
	assert_ptr_equal(ptable_remove(tab, K2), V2);
	assert_int_equal(ptable_size(tab), 1);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_nul(ptable_get(tab, K2));

	// K0
	assert_ptr_equal(ptable_remove(tab, K0), V0);
	assert_int_equal(ptable_size(tab), 0);
	assert_nul(ptable_get(tab, K0));
	assert_nul(ptable_get(tab, K1));
	assert_nul(ptable_get(tab, K2));

	ptable_free(tab);
}

static void ptable_remove__inexistent(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));
	assert_nul(ptable_put(tab, K2, V2));

	assert_int_equal(ptable_size(tab), 3);
	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	assert_nul(ptable_remove(tab, K3));
	assert_int_equal(ptable_size(tab), 3);

	ptable_free(tab);
}

static void ptable_equal__length_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));

	assert_nul(ptable_put(b, K1, V2));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__key_pointers_ok(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));
	assert_nul(ptable_put(a, K2, V2));

	assert_nul(ptable_put(b, K0, V0));
	assert_nul(ptable_put(b, K1, V1));
	assert_nul(ptable_put(b, K2, V2));

	assert_ptable_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__key_pointers_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));
	assert_nul(ptable_put(a, K2, V2));

	assert_nul(ptable_put(b, K0, V0));
	assert_nul(ptable_put(b, K1, V0));
	assert_nul(ptable_put(b, K2, V0));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_val_ok(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "a"));

	assert_ptable_equal(a, b, fn_equal_strcmp, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_val_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "b"));

	assert_ptable_not_equal(a, b, fn_equal_strcmp, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_key_ok(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_strcasecmp, .str_key = (fn_str)strdup, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, "zero", V0));
	assert_nul(ptable_put(a, "one", V1));
	assert_nul(ptable_put(a, "two", V2));

	assert_nul(ptable_put(b, "ZERO", V0));
	assert_nul(ptable_put(b, "ONE", V1));
	assert_nul(ptable_put(b, "TWO", V2));

	assert_ptable_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_key_different(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_strcasecmp, .str_key = (fn_str)strdup, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, "zero", V0));
	assert_nul(ptable_put(a, "one", V1));
	assert_nul(ptable_put(a, "two", V2));

	assert_nul(ptable_put(b, "ZERO", V0));
	assert_nul(ptable_put(b, "ONE", V1));
	assert_nul(ptable_put(b, "THREE", V2));

	assert_ptable_not_equal(a, b, NULL, NULL);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_keys_slist__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_keys_slist(tab));

	ptable_free(tab);
}

static void ptable_keys_slist__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, V1);

	struct SList *list = ptable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), K0);
	assert_ptr_equal(slist_at(list, 1), K1);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_vals_slist__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_vals_slist(tab));

	ptable_free(tab);
}

static void ptable_vals_slist__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V1);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V3);

	struct SList *list = ptable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V1);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V3);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_str__null(void **state) {
	assert_nul(ptable_str(NULL, NULL));
}

static void ptable_str__empty(void **state) {
	const struct PTable *tab = ptable_init();

	char *actual = ptable_str(tab, NULL);
	assert_str_equal(actual, "");

	free(actual);
	ptable_free(tab);
}

static void ptable_str__pointers(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	char *expected = sprintf_alloc(
			"%p = %p\n"
			"%p = (null)\n"
			"%p = %p\n",
			K0, V0,
			K1,
			K2, V2
			);

	char *actual = ptable_str(tab, NULL);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void ptable_str__fn_str(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, "AAA");
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, "BBB");

	char *expected = sprintf_alloc(
			"%p = A\n"
			"%p = (null)\n"
			"%p = B\n",
			K0,
			K1,
			K2
			);

	char *actual = ptable_str(tab, fn_str_first);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

static void ptable_str__fn_str_key(void **state) {
	const struct PTableParams params = { .str_key = (fn_str)strdup, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, "zero", V0));
	assert_nul(ptable_put(tab, "one", NULL));
	assert_nul(ptable_put(tab, "two", V2));

	char *expected = sprintf_alloc(
			"zero = %p\n"
			"one = (null)\n"
			"two = %p\n",
			V0,
			V2
			);

	char *actual = ptable_str(tab, NULL);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ptable_init__size),
		TEST(ptable_init__defaults),

		TEST(ptable_clone__empty),
		TEST(ptable_clone__params),
		TEST(ptable_clone__shallow_many),
		TEST(ptable_clone__deep_many),
		TEST(ptable_clone__alloc_key),

		TEST(ptable_free_vals__null_free_val),
		TEST(ptable_free_vals__free_val),
		TEST(ptable_free_vals__free_val_hierarchical),

		TEST(ptable_put__new),
		TEST(ptable_put__overwrite),
		TEST(ptable_put__null),
		TEST(ptable_put__null_overwrite),
		TEST(ptable_put__grow),

		TEST(ptable_iter__empty),
		TEST(ptable_iter__free),
		TEST(ptable_iter__many),
		TEST(ptable_iter__removed),

		TEST(ptable_filter_iter__many),

		TEST(ptable_put__again),

		TEST(ptable_remove__existing),
		TEST(ptable_remove__inexistent),

		TEST(ptable__equal_key),

		TEST(ptable__alloc_key_free_key),

		TEST(ptable_equal__length_different),
		TEST(ptable_equal__key_pointers_ok),
		TEST(ptable_equal__key_pointers_different),
		TEST(ptable_equal__equal_val_ok),
		TEST(ptable_equal__equal_val_different),
		TEST(ptable_equal__equal_key_ok),
		TEST(ptable_equal__equal_key_different),

		TEST(ptable_keys_slist__empty),
		TEST(ptable_keys_slist__many),

		TEST(ptable_vals_slist__empty),
		TEST(ptable_vals_slist__many),

		TEST(ptable_str__null),
		TEST(ptable_str__empty),
		TEST(ptable_str__pointers),
		TEST(ptable_str__fn_str),
		TEST(ptable_str__fn_str_key),
	};

	return RUN(tests);
}

