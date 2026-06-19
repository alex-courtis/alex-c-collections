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
	const struct PTableParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PTableIterState {
	const struct PTable *tab;
	size_t position;
	fn_equal equal_key;
	fn_equal equal_val;
	const void *data;
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

static const void *fn_alloc_key_duplicate(const void* const val) {
	return sprintf_alloc("%s%s", (char*)val, (char*)val);
}

static void ptable_init__defaults(void **state) {
	const struct PTable *tab = ptable_init();

	assert_non_nul(tab);

	assert_int_equal(tab->size, 0);
	assert_int_equal(tab->capacity, 10);

	size_t k[25] = { 0 };
	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		ptable_put(tab, &k[i], &v[i]);

	assert_int_equal(tab->size, 25);
	assert_int_equal(tab->capacity, 30);

	ptable_free(tab);
}

static void ptable_clone_shallow__empty(void **state) {
	const struct PTable *from = ptable_init();

	const struct PTable *to = ptable_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);

	ptable_free(from);
	ptable_free(to);
}

// also tests constructor
static void ptable_clone_shallow__params(void **state) {
	const struct PTableParams params = {
		.equal_key = mock_equal,
		.equal_val = mock_equal,
		.alloc_key = mock_alloc,
		.free_key = mock_free,
		.initial = 99,
		.grow = 1,
	};
	const struct PTable *from = ptable_init_with(params);

	const struct PTable *to = ptable_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);
	assert_int_equal(to->capacity, 99);
	assert_int_equal(to->params.grow, 1);
	assert_ptr_equal(to->params.equal_key, mock_equal);
	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.alloc_key, mock_alloc);
	assert_ptr_equal(to->params.free_key, mock_free);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone_shallow__many(void **state) {
	const struct PTable *from = ptable_init();

	assert_nul(ptable_put(from, K0, NULL));
	assert_nul(ptable_put(from, K1, V1));
	assert_nul(ptable_put(from, K2, NULL));
	assert_nul(ptable_put(from, K3, V3));
	assert_nul(ptable_put(from, K4, NULL));

	const struct PTable *to = ptable_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 5);

	assert_ptable_equal(from, to);

	assert_ptr_equal(ptable_get(to, K0), NULL);
	assert_ptr_equal(ptable_get(to, K1), V1);
	assert_ptr_equal(ptable_get(to, K2), NULL);
	assert_ptr_equal(ptable_get(to, K3), V3);
	assert_ptr_equal(ptable_get(to, K4), NULL);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone_shallow__alloc_key(void **state) {
	const struct PTableParams params = { .alloc_key = mock_alloc, };
	const struct PTable *from = ptable_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);
	assert_nul(ptable_put(from, K0, V0));

	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K1, void*);
	assert_nul(ptable_put(from, K1, V1));

	expect_ptr(mock_alloc, val, K2);
	will_return_ptr_type(mock_alloc, K2, void*);
	assert_nul(ptable_put(from, K2, NULL));

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K3, void*);
	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K4, void*);
	expect_ptr(mock_alloc, val, K2);
	will_return_ptr_type(mock_alloc, K5, void*);

	const struct PTable *to = ptable_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 3);

	assert_ptable_not_equal(from, to);

	assert_ptr_equal(ptable_get(to, K3), V0);
	assert_ptr_equal(ptable_get(to, K4), V1);
	assert_ptr_equal(ptable_get(to, K5), NULL);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone_deep__alloc_val(void **state) {
	const struct PTableParams params = { .alloc_val = mock_alloc, };
	const struct PTable *from = ptable_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ptable_put(from, K0, V0));

	expect_ptr(mock_alloc, val, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_nul(ptable_put(from, K1, V1));

	assert_nul(ptable_put(from, K2, NULL));

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V2, void*);

	expect_ptr(mock_alloc, val, V1);
	will_return_ptr_type(mock_alloc, V3, void*);

	const struct PTable *to = ptable_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(ptable_size(to), 3);

	assert_ptable_not_equal(from, to);

	assert_ptr_equal(ptable_get(to, K0), V2);
	assert_ptr_equal(ptable_get(to, K1), V3);
	assert_ptr_equal(ptable_get(to, K2), NULL);

	ptable_free(from);
	ptable_free(to);
}

static void ptable_clone_deep__no_alloc_val(void **state) {
	const struct PTable *from = ptable_init();

	assert_nul(ptable_put(from, K0, V0));
	assert_nul(ptable_put(from, K1, NULL));

	assert_nul(ptable_clone_deep(from));

	ptable_free(from);
}

static void ptable_free_vals__null_free_val(void **state) {
	const struct PTable *tab = ptable_init();

	const char *val = strdup("0");

	ptable_put(tab, K0, val);

	assert_int_equal(ptable_size(tab), 1);

	ptable_free_vals(tab);
}

static void ptable_free_vals__free_val(void **state) {
	const struct PTableParams params = { .free_val = mock_free, };
	const struct PTable *tab = ptable_init_with(params);

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	assert_int_equal(ptable_size(tab), 3);

	expect_ptr(mock_free, val, V0);
	expect_ptr(mock_free, val, V2);

	ptable_free_vals(tab);
}

static void fn_free_ptable(const void *val) {
	ptable_free_vals(val);
}

static void ptable_free_vals__free_val_hierarchical(void **state) {
	const struct PTableParams params_outer = { .free_val = fn_free_ptable, };
	const struct PTable *outer = ptable_init_with(params_outer);

	const struct PTableParams params_inner = { .free_val = mock_free, };
	const struct PTable *inner1 = ptable_init_with(params_inner);
	const struct PTable *inner2 = ptable_init_with(params_inner);

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

	ptable_free_vals(outer);
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

	assert_nul(ptable_put(tab, NULL, V2));
	assert_int_equal(ptable_size(tab), 2);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_nul(ptable_get(tab, K1));
	assert_nul(ptable_get(tab, K2));

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
	assert_int_equal(tab->params.grow, 5);

	assert_nul(ptable_put(tab, K3, V3));

	assert_int_equal(tab->size, 4);
	assert_int_equal(tab->capacity, 8);
	assert_int_equal(tab->params.grow, 5);

	assert_nul(ptable_put(tab, K4, V4));
	assert_nul(ptable_put(tab, K5, V5));

	assert_int_equal(tab->size, 6);
	assert_int_equal(tab->capacity, 8);
	assert_int_equal(tab->params.grow, 5);

	assert_ptr_equal(ptable_get(tab, K0), V0);
	assert_ptr_equal(ptable_get(tab, K1), V1);
	assert_ptr_equal(ptable_get(tab, K2), V2);

	assert_ptr_equal(ptable_get(tab, K3), V3);
	assert_ptr_equal(ptable_get(tab, K4), V4);
	assert_ptr_equal(ptable_get(tab, K5), V5);

	ptable_free(tab);
}

static void ptable_put__alloc_key_free_key(void **state) {
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

static void ptable_put__equal_key(void **state) {
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

static void ptable_put__alloc_val(void **state) {
	const struct PTableParams params = { .alloc_val = mock_alloc, };
	const struct PTable *tab = ptable_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ptable_put(tab, K0, V0));

	assert_nul(ptable_put(tab, K1, NULL));

	expect_ptr(mock_alloc, val, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_ptr_equal(ptable_put(tab, K0, V1), V0);

	assert_ptr_equal(ptable_put(tab, K0, NULL), V1);

	ptable_free(tab);
}

static void ptable_put_free__free(void **state) {
	const struct PTable *tab = ptable_init();

	const char *val = strdup("val");

	assert_nul(ptable_put(tab, K0, val));

	assert_false(ptable_put_free(tab, K1, V1));

	assert_true(ptable_put_free(tab, K0, V0));

	ptable_free(tab);
}

static void ptable_put_free__free_val(void **state) {
	const struct PTableParams params = { .free_val = mock_free, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, K0, V0));

	assert_false(ptable_put_free(tab, K1, V1));

	expect_ptr(mock_free, val, V0);
	assert_true(ptable_put_free(tab, K0, V0));

	ptable_free(tab);
}

static void ptable_put_if_absent__(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put_if_absent(tab, K0, V0));
	assert_ptr_equal(ptable_get(tab, K0), V0);

	const void *existing = ptable_put_if_absent(tab, K0, V1);
	assert_ptr_equal(existing, V0);

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
	assert_ptr_equal(iter->key, K0);
	assert_ptr_equal(iter->val, V0);

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
	assert_ptr_equal(iter->key, K0);
	assert_nul(iter->val);

	// one
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// two
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K2);
	assert_nul(iter->val);

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// four
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K4);
	assert_nul(iter->val);

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
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// three
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// end
	iter = ptable_iter_next(iter);
	assert_nul(iter);

	ptable_free(tab);
}

static void ptable_iter__state_deleted(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));

	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);

	const struct PTableIterState *st = iter->st;
	((struct PTableIter*)iter)->st = NULL;

	iter = ptable_iter_next(iter);
	assert_nul(iter);

	free((void*)st);
	ptable_free(tab);
}

static void ptable_iter__state_tab_deleted(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));

	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);

	struct PTableIterState *st = iter->st;
	st->tab = NULL;

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
	expect_ptr(mock_equal, a, K0);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// get K1
	expect_ptr(mock_equal, a, K1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	const struct PTableIter *iter = ptable_filter_iter(tab, mock_equal, mock_equal, D0);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// skip V2
	expect_ptr(mock_equal, a, K2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V2);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

	// get V3
	expect_ptr(mock_equal, a, K3);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V3);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);

	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// skip V4
	expect_ptr(mock_equal, a, K4);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, true);
	expect_ptr(mock_equal, a, V4);
	expect_ptr(mock_equal, b, D0);
	will_return(mock_equal, false);

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
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// zero moved later
	iter = ptable_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K0);
	assert_ptr_equal(iter->val, V0);

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

static void ptable_remove_free__free(void **state) {
	const struct PTable *tab = ptable_init();

	const char *val = strdup("val");

	assert_nul(ptable_put(tab, K0, val));
	assert_nul(ptable_put(tab, K1, NULL));

	assert_true(ptable_remove_free(tab, K0));

	assert_true(ptable_remove_free(tab, K1));

	assert_false(ptable_remove_free(tab, K2));

	ptable_free(tab);
}

static void ptable_remove_free__free_val(void **state) {
	const struct PTableParams params = { .free_val = mock_free, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, K0, V0));

	assert_false(ptable_remove_free(tab, K1));

	assert_nul(ptable_put(tab, K1, NULL));

	expect_ptr(mock_free, val, V0);
	assert_true(ptable_remove_free(tab, K0));

	assert_true(ptable_remove_free(tab, K1));

	ptable_free(tab);
}

static void ptable_contains_key__pointers(void **state) {
	const struct PTable *tab = ptable_init();

	assert_false(ptable_contains_key(tab, K0));

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_true(ptable_contains_key(tab, K0));
	assert_true(ptable_contains_key(tab, K1));

	assert_false(ptable_contains_key(tab, K2));

	assert_false(ptable_contains_key(tab, NULL));

	ptable_free(tab);
}

static void ptable_contains_key__equal_key(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_ptr, };
	const struct PTable *tab = ptable_init_with(params);

	assert_false(ptable_contains_key(tab, K0));

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, V1));

	assert_true(ptable_contains_key(tab, K0));
	assert_true(ptable_contains_key(tab, K1));

	assert_false(ptable_contains_key(tab, K2));

	assert_false(ptable_contains_key(tab, NULL));

	ptable_free(tab);
}

static void ptable_equal__length_different(void **state) {
	const struct PTable *a = ptable_init();
	const struct PTable *b = ptable_init();

	assert_nul(ptable_put(a, K0, V0));
	assert_nul(ptable_put(a, K1, V1));

	assert_nul(ptable_put(b, K1, V2));

	assert_ptable_not_equal(a, b);

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

	assert_ptable_equal(a, b);

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

	assert_ptable_not_equal(a, b);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_val_ok(void **state) {
	const struct PTableParams params = { .equal_val = fn_equal_strcmp, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "a"));

	assert_ptable_equal(a, b);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_val_different(void **state) {
	const struct PTableParams params = { .equal_val = fn_equal_strcmp, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, K0, "a"));

	assert_nul(ptable_put(b, K0, "b"));

	assert_ptable_not_equal(a, b);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_key_ok(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_strcasecmp, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, "zero", V0));
	assert_nul(ptable_put(a, "one", V1));
	assert_nul(ptable_put(a, "two", V2));

	assert_nul(ptable_put(b, "ZERO", V0));
	assert_nul(ptable_put(b, "ONE", V1));
	assert_nul(ptable_put(b, "TWO", V2));

	assert_ptable_equal(a, b);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_equal__equal_key_different(void **state) {
	const struct PTableParams params = { .equal_key = fn_equal_strcasecmp, };
	const struct PTable *a = ptable_init_with(params);
	const struct PTable *b = ptable_init_with(params);

	assert_nul(ptable_put(a, "zero", V0));
	assert_nul(ptable_put(a, "one", V1));
	assert_nul(ptable_put(a, "two", V2));

	assert_nul(ptable_put(b, "ZERO", V0));
	assert_nul(ptable_put(b, "ONE", V1));
	assert_nul(ptable_put(b, "THREE", V2));

	assert_ptable_not_equal(a, b);

	ptable_free(a);
	ptable_free(b);
}

static void ptable_keys_slist_shallow__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_keys_slist_shallow(tab));

	ptable_free(tab);
}

static void ptable_keys_slist_shallow__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, V1);

	struct SList *list = ptable_keys_slist_shallow(tab);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), K0);
	assert_ptr_equal(slist_at(list, 1), K1);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_keys_slist_deep__alloc_key(void **state) {
	const struct PTableParams params = { .alloc_key = mock_alloc, };
	const struct PTable *tab = ptable_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	assert_nul(ptable_put(tab, K0, V0));

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	struct SList *list = ptable_keys_slist_deep(tab);

	assert_ptr_equal(slist_at(list, 0), K0);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_keys_slist_deep__no_alloc_key(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));

	assert_nul(ptable_keys_slist_deep(tab));

	ptable_free(tab);
}

static void ptable_vals_slist_shallow__empty(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_vals_slist_shallow(tab));

	ptable_free(tab);
}

static void ptable_vals_slist_shallow__many(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V1);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V3);

	struct SList *list = ptable_vals_slist_shallow(tab);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V1);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V3);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_vals_slist_deep__alloc_val(void **state) {
	const struct PTableParams params = { .alloc_val = mock_alloc, };
	const struct PTable *tab = ptable_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ptable_put(tab, K0, V0));

	assert_nul(ptable_put(tab, K1, NULL));

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	struct SList *list = ptable_vals_slist_deep(tab);

	assert_ptr_equal(slist_at(list, 0), V0);
	assert_ptr_equal(slist_at(list, 1), NULL);

	slist_free(&list);
	ptable_free(tab);
}

static void ptable_vals_slist_deep__no_alloc_val(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_put(tab, K0, V0));
	assert_nul(ptable_put(tab, K1, NULL));

	assert_nul(ptable_vals_slist_deep(tab));

	ptable_free(tab);
}

static void ptable_str__empty(void **state) {
	const struct PTable *tab = ptable_init();

	char *actual = ptable_str(tab);
	assert_str_equal(actual, "");

	free(actual);
	ptable_free(tab);
}

static void ptable_str__pointers(void **state) {
	const struct PTable *tab = ptable_init();

	ptable_put(tab, K0, V0);
	ptable_put(tab, K1, NULL);
	ptable_put(tab, K2, V2);

	const void **k = tab->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"%p = %p\n"
			"%p = (null)\n"
			"(null) = %p\n",
			K0, V0,
			K1,
			V2
			);

	char *actual = ptable_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void ptable_str__str_val(void **state) {
	const struct PTableParams params = { .str_val = fn_str_first, };
	const struct PTable *tab = ptable_init_with(params);

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

	char *actual = ptable_str(tab);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

static void ptable_str__str_key(void **state) {
	const struct PTableParams params = { .str_key = (fn_str)strdup, };
	const struct PTable *tab = ptable_init_with(params);

	assert_nul(ptable_put(tab, "zero", V0));
	assert_nul(ptable_put(tab, "one", NULL));
	assert_nul(ptable_put(tab, "two", V2));

	const void **k = tab->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"zero = %p\n"
			"one = (null)\n"
			"(null) = %p\n",
			V0,
			V2
			);

	char *actual = ptable_str(tab);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ptable_free(tab);
}

static void ptable__null_inputs(void **state) {
	const struct PTable *tab = ptable_init();

	assert_nul(ptable_clone_shallow(NULL));
	assert_nul(ptable_clone_deep(NULL));
	ptable_free(NULL);
	ptable_free_vals(NULL);
	ptable_iter_free(NULL);
	assert_false(ptable_get(NULL, NULL));
	assert_false(ptable_get(tab, NULL));
	assert_false(ptable_contains_key(NULL, NULL));
	assert_false(ptable_contains_key(tab, NULL));
	assert_nul(ptable_iter(NULL));
	assert_nul(ptable_filter_iter(NULL, NULL, NULL, NULL));
	assert_nul(ptable_iter_next(NULL));
	assert_false(ptable_put(NULL, NULL, NULL));
	assert_false(ptable_put(tab, NULL, NULL));
	assert_nul(ptable_put_if_absent(NULL, NULL, NULL));
	assert_nul(ptable_put_if_absent(tab, NULL, NULL));
	assert_false(ptable_put_free(NULL, NULL, NULL));
	assert_nul(ptable_remove(NULL, NULL));
	assert_nul(ptable_remove(tab, NULL));
	assert_false(ptable_equal(NULL, NULL));
	assert_false(ptable_equal(tab, NULL));
	assert_nul(ptable_keys_slist_deep(NULL));
	assert_nul(ptable_keys_slist_shallow(NULL));
	assert_nul(ptable_vals_slist_deep(NULL));
	assert_nul(ptable_vals_slist_shallow(NULL));
	assert_nul(ptable_str(NULL));
	assert_int_equal(ptable_size(NULL), 0);

	ptable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ptable_init__defaults),

		TEST(ptable_clone_shallow__empty),
		TEST(ptable_clone_shallow__params),
		TEST(ptable_clone_shallow__many),
		TEST(ptable_clone_shallow__alloc_key),

		TEST(ptable_clone_deep__alloc_val),
		TEST(ptable_clone_deep__no_alloc_val),

		TEST(ptable_free_vals__null_free_val),
		TEST(ptable_free_vals__free_val),
		TEST(ptable_free_vals__free_val_hierarchical),

		TEST(ptable_put__new),
		TEST(ptable_put__overwrite),
		TEST(ptable_put__null),
		TEST(ptable_put__null_overwrite),
		TEST(ptable_put__grow),
		TEST(ptable_put__alloc_key_free_key),
		TEST(ptable_put__equal_key),
		TEST(ptable_put__alloc_val),

		TEST(ptable_put_free__free),
		TEST(ptable_put_free__free_val),

		TEST(ptable_put_if_absent__),

		TEST(ptable_iter__empty),
		TEST(ptable_iter__free),
		TEST(ptable_iter__many),
		TEST(ptable_iter__removed),
		TEST(ptable_iter__state_deleted),
		TEST(ptable_iter__state_tab_deleted),

		TEST(ptable_filter_iter__many),

		TEST(ptable_put__again),

		TEST(ptable_remove__existing),
		TEST(ptable_remove__inexistent),

		TEST(ptable_remove_free__free),
		TEST(ptable_remove_free__free_val),

		TEST(ptable_contains_key__pointers),
		TEST(ptable_contains_key__equal_key),

		TEST(ptable_equal__length_different),
		TEST(ptable_equal__key_pointers_ok),
		TEST(ptable_equal__key_pointers_different),
		TEST(ptable_equal__equal_val_ok),
		TEST(ptable_equal__equal_val_different),
		TEST(ptable_equal__equal_key_ok),
		TEST(ptable_equal__equal_key_different),

		TEST(ptable_keys_slist_shallow__empty),
		TEST(ptable_keys_slist_shallow__many),

		TEST(ptable_keys_slist_deep__alloc_key),
		TEST(ptable_keys_slist_deep__no_alloc_key),

		TEST(ptable_vals_slist_shallow__empty),
		TEST(ptable_vals_slist_shallow__many),

		TEST(ptable_vals_slist_deep__alloc_val),
		TEST(ptable_vals_slist_deep__no_alloc_val),

		TEST(ptable_str__empty),
		TEST(ptable_str__pointers),
		TEST(ptable_str__str_val),
		TEST(ptable_str__str_key),

		TEST(ptable__null_inputs),
	};

	return RUN(tests);
}

