#include "tst.h"
#include "asserts.h"
#include "assert-pmap.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pmap.h"

struct PMap {
	const struct PMapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
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

static void *fn_clone_key_duplicate(const void* const val) {
	return sprintf_alloc("%s%s", (char*)val, (char*)val);
}

static void pmap_init__defaults(void **state) {
	const struct PMap *map = pmap_init();

	assert_non_nul(map);

	assert_int_equal(map->size, 0);
	assert_int_equal(map->capacity, 10);

	size_t k[25] = { 0 };
	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		pmap_put(map, &k[i], &v[i]);

	assert_int_equal(map->size, 25);
	assert_int_equal(map->capacity, 30);

	pmap_free(map);
}

static void pmap_clone_shallow__empty(void **state) {
	const struct PMap *from = pmap_init();

	const struct PMap *to = pmap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);

	pmap_free(from);
	pmap_free(to);
}

// also tests constructor
static void pmap_clone_shallow__params(void **state) {
	const struct PMapParams params = {
		.equal_key = mock_equal,
		.equal_val = mock_equal,
		.alloc_key = mock_alloc,
		.alloc_val = mock_alloc,
		.free_key = mock_free,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.initial = 99,
		.grow = 1,
	};
	const struct PMap *from = pmap_init_with(params);

	const struct PMap *to = pmap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);
	assert_int_equal(to->capacity, 99);
	assert_int_equal(to->params.grow, 1);
	assert_ptr_equal(to->params.equal_key, mock_equal);
	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.alloc_key, mock_alloc);
	assert_ptr_equal(to->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->params.free_key, mock_free);
	assert_ptr_equal(to->params.free_val, mock_free);
	assert_ptr_equal(to->params.clone_val, mock_clone);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_shallow__many(void **state) {
	const struct PMap *from = pmap_init();

	assert_nul(pmap_put(from, K0, NULL));
	assert_nul(pmap_put(from, K1, V1));
	assert_nul(pmap_put(from, K2, NULL));
	assert_nul(pmap_put(from, K3, V3));
	assert_nul(pmap_put(from, K4, NULL));

	const struct PMap *to = pmap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 5);

	assert_pmap_equal(from, to);

	assert_ptr_equal(pmap_get(to, K0), NULL);
	assert_ptr_equal(pmap_get(to, K1), V1);
	assert_ptr_equal(pmap_get(to, K2), NULL);
	assert_ptr_equal(pmap_get(to, K3), V3);
	assert_ptr_equal(pmap_get(to, K4), NULL);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_shallow__alloc_key(void **state) {
	const struct PMapParams params = { .alloc_key = mock_alloc, };
	const struct PMap *from = pmap_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);
	assert_nul(pmap_put(from, K0, V0));

	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K1, void*);
	assert_nul(pmap_put(from, K1, V1));

	expect_ptr(mock_alloc, val, K2);
	will_return_ptr_type(mock_alloc, K2, void*);
	assert_nul(pmap_put(from, K2, NULL));

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K3, void*);
	expect_ptr(mock_alloc, val, K1);
	will_return_ptr_type(mock_alloc, K4, void*);
	expect_ptr(mock_alloc, val, K2);
	will_return_ptr_type(mock_alloc, K5, void*);

	const struct PMap *to = pmap_clone_shallow(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 3);

	assert_pmap_not_equal(from, to);

	assert_ptr_equal(pmap_get(to, K3), V0);
	assert_ptr_equal(pmap_get(to, K4), V1);
	assert_ptr_equal(pmap_get(to, K5), NULL);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_deep__clone_val(void **state) {
	const struct PMapParams params = { .clone_val = mock_clone, };
	const struct PMap *from = pmap_init_with(params);

	assert_nul(pmap_put(from, K0, V0));

	assert_nul(pmap_put(from, K1, V1));

	assert_nul(pmap_put(from, K2, NULL));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, val, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PMap *to = pmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 3);

	assert_pmap_not_equal(from, to);

	assert_ptr_equal(pmap_get(to, K0), V2);
	assert_ptr_equal(pmap_get(to, K1), V3);
	assert_ptr_equal(pmap_get(to, K2), NULL);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_deep__no_clone_val(void **state) {
	const struct PMap *from = pmap_init();

	assert_nul(pmap_put(from, K0, V0));
	assert_nul(pmap_put(from, K1, NULL));

	const struct PMap *to = pmap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(pmap_size(to), 0);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_free_vals__null_free_val(void **state) {
	const struct PMap *map = pmap_init();

	const char *val = strdup("0");

	pmap_put(map, K0, val);

	assert_int_equal(pmap_size(map), 1);

	pmap_free_vals(map);
}

static void pmap_free_vals__free_val(void **state) {
	const struct PMapParams params = { .free_val = mock_free, };
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V0);
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, V2);

	assert_int_equal(pmap_size(map), 3);

	expect_ptr(mock_free, val, V0);
	expect_ptr(mock_free, val, V2);

	pmap_free_vals(map);
}

static void fn_free_pmap(const void *val) {
	pmap_free_vals(val);
}

static void pmap_free_vals__free_val_hierarchical(void **state) {
	const struct PMapParams params_outer = { .free_val = fn_free_pmap, };
	const struct PMap *outer = pmap_init_with(params_outer);

	const struct PMapParams params_inner = { .free_val = mock_free, };
	const struct PMap *inner1 = pmap_init_with(params_inner);
	const struct PMap *inner2 = pmap_init_with(params_inner);

	pmap_put(outer, K0, (void*)inner1);
	pmap_put(outer, K1, (void*)inner2);

	pmap_put(inner1, K2, V2);
	pmap_put(inner1, K3, V3);

	pmap_put(inner2, K4, V4);
	pmap_put(inner2, K5, V5);

	assert_int_equal(pmap_size(outer), 2);

	expect_ptr(mock_free, val, V2);
	expect_ptr(mock_free, val, V3);
	expect_ptr(mock_free, val, V4);
	expect_ptr(mock_free, val, V5);

	pmap_free_vals(outer);
}

static void pmap_put__new(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_int_equal(pmap_size(map), 2);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);

	pmap_free(map);
}

static void pmap_put__overwrite(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));

	assert_ptr_equal(pmap_put(map, K1, V4), V1);

	assert_ptr_equal(pmap_put(map, K3, V5), V3);

	assert_int_equal(pmap_size(map), 4);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V4);
	assert_ptr_equal(pmap_get(map, K2), V2);
	assert_ptr_equal(pmap_get(map, K3), V5);

	pmap_free(map);
}

static void pmap_put__null(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_int_equal(pmap_size(map), 1);

	assert_nul(pmap_put(map, K1, NULL));
	assert_int_equal(pmap_size(map), 2);

	assert_nul(pmap_put(map, NULL, V2));
	assert_int_equal(pmap_size(map), 2);

	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_nul(pmap_get(map, K1));
	assert_nul(pmap_get(map, K2));

	pmap_free(map);
}

static void pmap_put__null_overwrite(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));

	assert_ptr_equal(pmap_get(map, K0), V0);

	assert_ptr_equal(pmap_put(map, K0, NULL), V0);

	assert_int_equal(pmap_size(map), 1);
	assert_nul(pmap_get(map, K0));

	pmap_free(map);
}

static void pmap_put__grow(void **state) {
	const struct PMapParams params = { .initial = 3, .grow = 5, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	assert_int_equal(map->size, 3);
	assert_int_equal(map->capacity, 3);
	assert_int_equal(map->params.grow, 5);

	assert_nul(pmap_put(map, K3, V3));

	assert_int_equal(map->size, 4);
	assert_int_equal(map->capacity, 8);
	assert_int_equal(map->params.grow, 5);

	assert_nul(pmap_put(map, K4, V4));
	assert_nul(pmap_put(map, K5, V5));

	assert_int_equal(map->size, 6);
	assert_int_equal(map->capacity, 8);
	assert_int_equal(map->params.grow, 5);

	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);
	assert_ptr_equal(pmap_get(map, K2), V2);

	assert_ptr_equal(pmap_get(map, K3), V3);
	assert_ptr_equal(pmap_get(map, K4), V4);
	assert_ptr_equal(pmap_get(map, K5), V5);

	pmap_free(map);
}

static void pmap_put__alloc_key_free_key(void **state) {
	const struct PMapParams params = {
		.equal_key = fn_equal_strcmp,
		.alloc_key = fn_clone_key_duplicate,
		.free_key = (fn_free)free,
	};
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, "zero", V0));
	assert_nul(pmap_put(map, "one", V1));

	assert_ptr_equal(pmap_get(map, "zerozero"), V0);
	assert_ptr_equal(pmap_get(map, "oneone"), V1);

	assert_ptr_equal(pmap_remove(map, "zerozero"), V0);

	assert_int_equal(pmap_size(map), 1);
	assert_ptr_equal(pmap_get(map, "oneone"), V1);

	pmap_free(map);
}

static void pmap_put__alloc_key_returned_null(void **state) {
	const struct PMapParams params = { .alloc_key = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(pmap_put(map, K0, V0));

	assert_int_equal(pmap_size(map), 0);

	pmap_free(map);
}


static void pmap_put__equal_key(void **state) {
	const struct PMapParams params = { .equal_key = fn_equal_ptr, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_int_equal(pmap_size(map), 2);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);

	assert_ptr_equal(pmap_put(map, K0, V2), V0);

	assert_ptr_equal(pmap_remove(map, K1), V1);

	pmap_free(map);
}

static void pmap_put__clone_val(void **state) {
	const struct PMapParams params = { .alloc_val = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, val, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_alloc, val, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_ptr_equal(pmap_put(map, K0, V1), V0);

	assert_ptr_equal(pmap_put(map, K0, NULL), V1);

	pmap_free(map);
}

static void pmap_put_free__free(void **state) {
	const struct PMap *map = pmap_init();

	const char *val = strdup("val");

	assert_nul(pmap_put(map, K0, val));

	assert_false(pmap_put_free(map, K1, V1));

	assert_true(pmap_put_free(map, K0, V0));

	pmap_free(map);
}

static void pmap_put_free__free_val(void **state) {
	const struct PMapParams params = { .free_val = mock_free, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));

	assert_false(pmap_put_free(map, K1, V1));

	expect_ptr(mock_free, val, V0);
	assert_true(pmap_put_free(map, K0, V0));

	pmap_free(map);
}

static void pmap_put_if_absent__(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put_if_absent(map, K0, V0));
	assert_ptr_equal(pmap_get(map, K0), V0);

	const void *existing = pmap_put_if_absent(map, K0, V1);
	assert_ptr_equal(existing, V0);

	pmap_free(map);
}

static void pmap_match__matches(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	// skip K0
	expect_ptr(mock_match_key_val, key, K0);
	expect_ptr(mock_match_key_val, val, V0);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// get K1
	expect_ptr(mock_match_key_val, key, K1);
	expect_ptr(mock_match_key_val, val, V1);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, true);

	const struct PMapPair pair = pmap_match(map, mock_match_key_val, D0);
	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	pmap_free(map);
}

static void pmap_match__no_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip K0
	expect_ptr(mock_match_key_val, key, K0);
	expect_ptr(mock_match_key_val, val, V0);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// skip K1
	expect_ptr(mock_match_key_val, key, K1);
	expect_ptr(mock_match_key_val, val, V1);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	const struct PMapPair pair = pmap_match(map, mock_match_key_val, D0);
	assert_nul(pair.key);
	assert_nul(pair.val);

	pmap_free(map);
}

static void pmap_match__null_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapPair pair = pmap_match(map, NULL, D0);
	assert_nul(pair.key);
	assert_nul(pair.val);

	pmap_free(map);
}

static void pmap_iter__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_iter(map));

	pmap_free(map);
}

static void pmap_iter__free(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapIter *iter = pmap_iter(map);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K0);
	assert_ptr_equal(iter->val, V0);

	pmap_iter_free(iter);

	pmap_free(map);
}

static void pmap_iter__many(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, NULL));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, NULL));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, NULL));

	assert_int_equal(pmap_size(map), 5);

	// zero
	const struct PMapIter *iter = pmap_iter(map);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K0);
	assert_nul(iter->val);

	// one
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// two
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K2);
	assert_nul(iter->val);

	// three
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// four
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K4);
	assert_nul(iter->val);

	// end
	iter = pmap_iter_next(iter);
	assert_nul(iter);

	pmap_free(map);
}

static void pmap_iter__removed(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, V4));

	assert_ptr_equal(pmap_remove(map, K0), V0);

	assert_ptr_equal(pmap_remove(map, K2), V2);

	assert_ptr_equal(pmap_remove(map, K4), V4);

	assert_int_equal(pmap_size(map), 2);

	// one
	const struct PMapIter *iter = pmap_iter(map);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// three
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// end
	iter = pmap_iter_next(iter);
	assert_nul(iter);

	pmap_free(map);
}

static void pmap_iter_free__partial(void **state) {
	const struct PMapIter *iter = calloc(1, sizeof(struct PMapIter));

	pmap_iter_free(iter);
}

static void pmap_iter_next__partial(void **state) {
	const struct PMapIter *iter = calloc(1, sizeof(struct PMapIter));

	assert_nul(pmap_iter_next(iter));
}

static void pmap_match_iter__many(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, V4));

	assert_int_equal(pmap_size(map), 5);

	// skip K0
	expect_ptr(mock_match_key_val, key, K0);
	expect_ptr(mock_match_key_val, val, V0);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// get K1
	expect_ptr(mock_match_key_val, key, K1);
	expect_ptr(mock_match_key_val, val, V1);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, true);

	const struct PMapIter *iter = pmap_match_iter(map, mock_match_key_val, D0);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// skip K2
	expect_ptr(mock_match_key_val, key, K2);
	expect_ptr(mock_match_key_val, val, V2);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// get K3
	expect_ptr(mock_match_key_val, key, K3);
	expect_ptr(mock_match_key_val, val, V3);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, true);

	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K3);
	assert_ptr_equal(iter->val, V3);

	// skip K4
	expect_ptr(mock_match_key_val, key, K4);
	expect_ptr(mock_match_key_val, val, V4);
	expect_ptr(mock_match_key_val, data, D0);
	will_return(mock_match_key_val, false);

	// done
	iter = pmap_iter_next(iter);
	assert_nul(iter);

	pmap_free(map);
}


static void pmap_put__again(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_int_equal(pmap_size(map), 2);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);

	// remove zero
	assert_ptr_equal(pmap_remove(map, K0), V0);

	assert_int_equal(pmap_size(map), 1);
	assert_nul(pmap_get(map, K0));

	// put zero again afterwards
	assert_nul(pmap_put(map, K0, V0));
	assert_int_equal(pmap_size(map), 2);

	// one
	const struct PMapIter *iter = pmap_iter(map);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K1);
	assert_ptr_equal(iter->val, V1);

	// zero moved later
	iter = pmap_iter_next(iter);
	assert_non_nul(iter);
	assert_ptr_equal(iter->key, K0);
	assert_ptr_equal(iter->val, V0);

	// end
	iter = pmap_iter_next(iter);
	assert_nul(iter);

	pmap_free(map);
}

static void pmap_remove__existing(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	assert_int_equal(pmap_size(map), 3);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);
	assert_ptr_equal(pmap_get(map, K2), V2);

	// K1
	assert_ptr_equal(pmap_remove(map, K1), V1);
	assert_int_equal(pmap_size(map), 2);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_nul(pmap_get(map, K1));
	assert_ptr_equal(pmap_get(map, K2), V2);

	// K2
	assert_ptr_equal(pmap_remove(map, K2), V2);
	assert_int_equal(pmap_size(map), 1);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_nul(pmap_get(map, K1));
	assert_nul(pmap_get(map, K2));

	// K0
	assert_ptr_equal(pmap_remove(map, K0), V0);
	assert_int_equal(pmap_size(map), 0);
	assert_nul(pmap_get(map, K0));
	assert_nul(pmap_get(map, K1));
	assert_nul(pmap_get(map, K2));

	pmap_free(map);
}

static void pmap_remove__inexistent(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	assert_int_equal(pmap_size(map), 3);
	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_ptr_equal(pmap_get(map, K1), V1);
	assert_ptr_equal(pmap_get(map, K2), V2);

	assert_nul(pmap_remove(map, K3));
	assert_int_equal(pmap_size(map), 3);

	pmap_free(map);
}

static void pmap_remove_free__free(void **state) {
	const struct PMap *map = pmap_init();

	const char *val = strdup("val");

	assert_nul(pmap_put(map, K0, val));
	assert_nul(pmap_put(map, K1, NULL));

	assert_true(pmap_remove_free(map, K0));

	assert_true(pmap_remove_free(map, K1));

	assert_false(pmap_remove_free(map, K2));

	pmap_free(map);
}

static void pmap_remove_free__free_val(void **state) {
	const struct PMapParams params = { .free_val = mock_free, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));

	assert_false(pmap_remove_free(map, K1));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_free, val, V0);
	assert_true(pmap_remove_free(map, K0));

	assert_true(pmap_remove_free(map, K1));

	pmap_free(map);
}

static void pmap_contains_key__pointers(void **state) {
	const struct PMap *map = pmap_init();

	assert_false(pmap_contains_key(map, K0));

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_true(pmap_contains_key(map, K0));
	assert_true(pmap_contains_key(map, K1));

	assert_false(pmap_contains_key(map, K2));

	assert_false(pmap_contains_key(map, NULL));

	pmap_free(map);
}

static void pmap_contains_key__equal_key(void **state) {
	const struct PMapParams params = { .equal_key = fn_equal_ptr, };
	const struct PMap *map = pmap_init_with(params);

	assert_false(pmap_contains_key(map, K0));

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_true(pmap_contains_key(map, K0));
	assert_true(pmap_contains_key(map, K1));

	assert_false(pmap_contains_key(map, K2));

	assert_false(pmap_contains_key(map, NULL));

	pmap_free(map);
}

static void pmap_equal__length_different(void **state) {
	const struct PMap *a = pmap_init();
	const struct PMap *b = pmap_init();

	assert_nul(pmap_put(a, K0, V0));
	assert_nul(pmap_put(a, K1, V1));

	assert_nul(pmap_put(b, K1, V2));

	assert_pmap_not_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__key_pointers_ok(void **state) {
	const struct PMap *a = pmap_init();
	const struct PMap *b = pmap_init();

	assert_nul(pmap_put(a, K0, V0));
	assert_nul(pmap_put(a, K1, V1));
	assert_nul(pmap_put(a, K2, V2));

	assert_nul(pmap_put(b, K0, V0));
	assert_nul(pmap_put(b, K1, V1));
	assert_nul(pmap_put(b, K2, V2));

	assert_pmap_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__key_pointers_different(void **state) {
	const struct PMap *a = pmap_init();
	const struct PMap *b = pmap_init();

	assert_nul(pmap_put(a, K0, V0));
	assert_nul(pmap_put(a, K1, V1));
	assert_nul(pmap_put(a, K2, V2));

	assert_nul(pmap_put(b, K0, V0));
	assert_nul(pmap_put(b, K1, V0));
	assert_nul(pmap_put(b, K2, V0));

	assert_pmap_not_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_val_ok(void **state) {
	const struct PMapParams params = { .equal_val = fn_equal_strcmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, K0, "a"));

	assert_nul(pmap_put(b, K0, "a"));

	assert_pmap_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_val_different(void **state) {
	const struct PMapParams params = { .equal_val = fn_equal_strcmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, K0, "a"));

	assert_nul(pmap_put(b, K0, "b"));

	assert_pmap_not_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_key_ok(void **state) {
	const struct PMapParams params = { .equal_key = fn_equal_strcasecmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, "zero", V0));
	assert_nul(pmap_put(a, "one", V1));
	assert_nul(pmap_put(a, "two", V2));

	assert_nul(pmap_put(b, "ZERO", V0));
	assert_nul(pmap_put(b, "ONE", V1));
	assert_nul(pmap_put(b, "TWO", V2));

	assert_pmap_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_key_different(void **state) {
	const struct PMapParams params = { .equal_key = fn_equal_strcasecmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, "zero", V0));
	assert_nul(pmap_put(a, "one", V1));
	assert_nul(pmap_put(a, "two", V2));

	assert_nul(pmap_put(b, "ZERO", V0));
	assert_nul(pmap_put(b, "ONE", V1));
	assert_nul(pmap_put(b, "THREE", V2));

	assert_pmap_not_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_keys_slist_shallow__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_keys_slist_shallow(map));

	pmap_free(map);
}

static void pmap_keys_slist_shallow__many(void **state) {
	const struct PMap *map = pmap_init();

	pmap_put(map, K0, V0);
	pmap_put(map, K1, V1);

	struct SList *list = pmap_keys_slist_shallow(map);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), K0);
	assert_ptr_equal(slist_at(list, 1), K1);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_keys_slist_deep__clone_key(void **state) {
	const struct PMapParams params = { .alloc_key = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	assert_nul(pmap_put(map, K0, V0));

	expect_ptr(mock_alloc, val, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	struct SList *list = pmap_keys_slist_deep(map);

	assert_ptr_equal(slist_at(list, 0), K0);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_keys_slist_deep__no_alloc_key(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_keys_slist_deep(map));

	pmap_free(map);
}

static void pmap_vals_slist_shallow__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_vals_slist_shallow(map));

	pmap_free(map);
}

static void pmap_vals_slist_shallow__many(void **state) {
	const struct PMap *map = pmap_init();

	pmap_put(map, K0, V1);
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, V3);

	struct SList *list = pmap_vals_slist_shallow(map);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V1);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V3);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_vals_slist_deep__clone_val(void **state) {
	const struct PMapParams params = { .clone_val = mock_clone, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_clone, val, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct SList *list = pmap_vals_slist_deep(map);

	assert_ptr_equal(slist_at(list, 0), V0);
	assert_ptr_equal(slist_at(list, 1), NULL);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_vals_slist_deep__no_clone_val(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, NULL));

	assert_nul(pmap_vals_slist_deep(map));

	pmap_free(map);
}

static void pmap_str__empty(void **state) {
	const struct PMap *map = pmap_init();

	char *actual = pmap_str(map);
	assert_str_equal(actual, "");

	free(actual);
	pmap_free(map);
}

static void pmap_str__pointers(void **state) {
	const struct PMap *map = pmap_init();

	pmap_put(map, K0, V0);
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, V2);

	const void **k = map->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"%p = %p\n"
			"%p = (null)\n"
			"(null) = %p\n",
			K0, V0,
			K1,
			V2
			);

	char *actual = pmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pmap_free(map);
}

static char* fn_str_first(const void *val) {
	return strndup(val, 1);
}

static void pmap_str__str_val(void **state) {
	const struct PMapParams params = { .str_val = fn_str_first, };
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, "AAA");
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, "BBB");

	char *expected = sprintf_alloc(
			"%p = A\n"
			"%p = (null)\n"
			"%p = B\n",
			K0,
			K1,
			K2
			);

	char *actual = pmap_str(map);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pmap_free(map);
}

static void pmap_str__str_key(void **state) {
	const struct PMapParams params = { .str_key = fn_str_or_null, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, "zero", V0));
	assert_nul(pmap_put(map, "one", NULL));
	assert_nul(pmap_put(map, "two", V2));

	const void **k = map->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"zero = %p\n"
			"one = (null)\n"
			"(null) = %p\n",
			V0,
			V2
			);

	char *actual = pmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pmap_free(map);
}

static void pmap__null_inputs(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_clone_shallow(NULL));
	assert_nul(pmap_clone_deep(NULL));
	pmap_free(NULL);
	pmap_free_vals(NULL);
	pmap_iter_free(NULL);
	assert_false(pmap_get(NULL, NULL));
	assert_false(pmap_get(map, NULL));
	assert_false(pmap_contains_key(NULL, NULL));
	assert_false(pmap_contains_key(map, NULL));
	pmap_match(NULL, NULL, NULL);
	pmap_match(NULL, mock_match_key_val, NULL);
	assert_nul(pmap_iter(NULL));
	assert_nul(pmap_match_iter(NULL, NULL, NULL));
	assert_nul(pmap_iter_next(NULL));
	assert_false(pmap_put(NULL, NULL, NULL));
	assert_false(pmap_put(map, NULL, NULL));
	assert_nul(pmap_put_if_absent(NULL, NULL, NULL));
	assert_nul(pmap_put_if_absent(map, NULL, NULL));
	assert_false(pmap_put_free(NULL, NULL, NULL));
	assert_false(pmap_put_free(map, NULL, NULL));
	assert_nul(pmap_remove(NULL, NULL));
	assert_nul(pmap_remove(map, NULL));
	assert_false(pmap_equal(NULL, NULL));
	assert_false(pmap_equal(map, NULL));
	assert_nul(pmap_keys_slist_deep(NULL));
	assert_nul(pmap_keys_slist_shallow(NULL));
	assert_nul(pmap_vals_slist_deep(NULL));
	assert_nul(pmap_vals_slist_shallow(NULL));
	assert_nul(pmap_str(NULL));
	assert_int_equal(pmap_size(NULL), 0);

	pmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pmap_init__defaults),

		TEST(pmap_clone_shallow__empty),
		TEST(pmap_clone_shallow__params),
		TEST(pmap_clone_shallow__many),
		TEST(pmap_clone_shallow__alloc_key),

		TEST(pmap_clone_deep__clone_val),
		TEST(pmap_clone_deep__no_clone_val),

		TEST(pmap_free_vals__null_free_val),
		TEST(pmap_free_vals__free_val),
		TEST(pmap_free_vals__free_val_hierarchical),

		TEST(pmap_put__new),
		TEST(pmap_put__overwrite),
		TEST(pmap_put__null),
		TEST(pmap_put__null_overwrite),
		TEST(pmap_put__grow),
		TEST(pmap_put__alloc_key_free_key),
		TEST(pmap_put__alloc_key_returned_null),
		TEST(pmap_put__equal_key),
		TEST(pmap_put__clone_val),

		TEST(pmap_put_free__free),
		TEST(pmap_put_free__free_val),

		TEST(pmap_put_if_absent__),

		TEST(pmap_match__matches),
		TEST(pmap_match__no_match),
		TEST(pmap_match__null_match),

		TEST(pmap_iter__empty),
		TEST(pmap_iter__free),
		TEST(pmap_iter__many),
		TEST(pmap_iter__removed),

		TEST(pmap_iter_free__partial),

		TEST(pmap_iter_next__partial),

		TEST(pmap_match_iter__many),

		TEST(pmap_put__again),

		TEST(pmap_remove__existing),
		TEST(pmap_remove__inexistent),

		TEST(pmap_remove_free__free),
		TEST(pmap_remove_free__free_val),

		TEST(pmap_contains_key__pointers),
		TEST(pmap_contains_key__equal_key),

		TEST(pmap_equal__length_different),
		TEST(pmap_equal__key_pointers_ok),
		TEST(pmap_equal__key_pointers_different),
		TEST(pmap_equal__equal_val_ok),
		TEST(pmap_equal__equal_val_different),
		TEST(pmap_equal__equal_key_ok),
		TEST(pmap_equal__equal_key_different),

		TEST(pmap_keys_slist_shallow__empty),
		TEST(pmap_keys_slist_shallow__many),

		TEST(pmap_keys_slist_deep__clone_key),
		TEST(pmap_keys_slist_deep__no_alloc_key),

		TEST(pmap_vals_slist_shallow__empty),
		TEST(pmap_vals_slist_shallow__many),

		TEST(pmap_vals_slist_deep__clone_val),
		TEST(pmap_vals_slist_deep__no_clone_val),

		TEST(pmap_str__empty),
		TEST(pmap_str__pointers),
		TEST(pmap_str__str_val),
		TEST(pmap_str__str_key),

		TEST(pmap__null_inputs),
	};

	return RUN(tests);
}

