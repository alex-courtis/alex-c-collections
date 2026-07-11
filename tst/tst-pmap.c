#include "assert-pmap.h"
#include "assert-pset.h"
#include "asserts.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pset.h"
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

struct PSet {
	const struct PSetParams params;
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

static void *alloc_key_duplicate(const void* const val) {
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

static void pmap_clone__empty(void **state) {
	const struct PMap *from = pmap_init();

	const struct PMap *to = pmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);

	pmap_free(from);
	pmap_free(to);
}

// also tests constructor
static void pmap_clone__params(void **state) {
	const struct PMapParams params = {
		.allow_null_val = true,
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

	const struct PMap *to = pmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);
	assert_int_equal(to->capacity, 99);
	assert_true(to->params.allow_null_val);
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

static void pmap_clone__many(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *from = pmap_init_with(params);

	assert_nul(pmap_put(from, K0, NULL));
	assert_nul(pmap_put(from, K1, V1));
	assert_nul(pmap_put(from, K2, NULL));
	assert_nul(pmap_put(from, K3, V3));
	assert_nul(pmap_put(from, K4, NULL));

	const struct PMap *to = pmap_clone(from);

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

static void pmap_clone__alloc_key(void **state) {
	const struct PMapParams params = {
		.alloc_key = mock_alloc,
		.allow_null_val = true,
	};
	const struct PMap *from = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);
	assert_nul(pmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, K1);
	will_return_ptr_type(mock_alloc, K1, void*);
	assert_nul(pmap_put(from, K1, V1));

	expect_ptr(mock_alloc, ptr, K2);
	will_return_ptr_type(mock_alloc, K2, void*);
	assert_nul(pmap_put(from, K2, NULL));

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K3, void*);
	expect_ptr(mock_alloc, ptr, K1);
	will_return_ptr_type(mock_alloc, K4, void*);
	expect_ptr(mock_alloc, ptr, K2);
	will_return_ptr_type(mock_alloc, K5, void*);

	const struct PMap *to = pmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 3);

	assert_pmap_not_equal(from, to);

	assert_ptr_equal(pmap_get(to, K3), V0);
	assert_ptr_equal(pmap_get(to, K4), V1);
	assert_ptr_equal(pmap_get(to, K5), NULL);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone__alloc_val(void **state) {
	const struct PMapParams params = {
		.alloc_val = mock_alloc,
	};
	const struct PMap *from = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(pmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PMap *to = pmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 1);

	assert_pmap_equal(from, to);

	assert_ptr_equal(pmap_get(to, K0), V0);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_deep__clone_val_allow_null_val(void **state) {
	const struct PMapParams params = {
		.allow_null_val = true,
		.clone_val = mock_clone,
	};
	const struct PMap *from = pmap_init_with(params);

	assert_nul(pmap_put(from, K0, V0));

	assert_nul(pmap_put(from, K1, V1));

	assert_nul(pmap_put(from, K2, NULL));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, ptr, V1);
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

static void pmap_clone_deep__clone_val_no_allow_null_val(void **state) {
	const struct PMapParams params = {
		.clone_val = mock_clone,
	};
	const struct PMap *from = pmap_init_with(params);

	assert_nul(pmap_put(from, K0, V0));

	assert_nul(pmap_put(from, K1, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, NULL, void*);

	const struct PMap *to = pmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(pmap_size(to), 1);

	assert_pmap_not_equal(from, to);

	assert_ptr_equal(pmap_get(to, K0), V2);

	pmap_free(from);
	pmap_free(to);
}

static void pmap_clone_deep__no_clone_val(void **state) {
	const struct PMap *from = pmap_init();

	assert_nul(pmap_put(from, K0, V0));
	assert_nul(pmap_put(from, K1, NULL));

	assert_nul(pmap_clone_deep(from));

	pmap_free(from);
}

static void pmap_clone_deep__alloc_val_and_clone_val(void **state) {
	const struct PMapParams params = {
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
	};
	const struct PMap *from = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(pmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PMap *to = pmap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(pmap_size(to), 1);

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
	const struct PMapParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V0);
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, V2);

	assert_int_equal(pmap_size(map), 3);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V2);

	pmap_free_vals(map);
}

static void free_pmap(const void *val) {
	pmap_free_vals(val);
}

static void pmap_free_vals__free_val_hierarchical(void **state) {
	const struct PMapParams params_outer = { .free_val = free_pmap, };
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

	expect_ptr(mock_free, ptr, V2);
	expect_ptr(mock_free, ptr, V3);
	expect_ptr(mock_free, ptr, V4);
	expect_ptr(mock_free, ptr, V5);

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

static void pmap_put__null_key(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, NULL, V0));
	assert_int_equal(pmap_size(map), 0);

	assert_false(pmap_contains_key(map, K0));

	pmap_free(map);
}

static void pmap_put__allow_null_val(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));
	assert_int_equal(pmap_size(map), 1);

	assert_nul(pmap_put(map, K1, NULL));
	assert_int_equal(pmap_size(map), 2);

	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_true(pmap_contains_key(map, K1));

	assert_true(pmap_put(map, K0, NULL));
	assert_nul(pmap_get(map, K0));

	pmap_free(map);
}

static void pmap_put__no_allow_null_val(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_int_equal(pmap_size(map), 1);

	assert_nul(pmap_put(map, K1, NULL));
	assert_int_equal(pmap_size(map), 1);

	assert_ptr_equal(pmap_get(map, K0), V0);
	assert_false(pmap_contains_key(map, K1));

	assert_false(pmap_put(map, K0, NULL));
	assert_ptr_equal(pmap_get(map, K0), V0);

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
		.equal_key = (fn_equal)equal_strcmp,
		.alloc_key = alloc_key_duplicate,
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

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(pmap_put(map, K0, V0));

	assert_int_equal(pmap_size(map), 0);

	pmap_free(map);
}


static void pmap_put__equal_key(void **state) {
	const struct PMapParams params = { .equal_key = equal_ptr, };
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

static void pmap_put__alloc_val_allow_null_val(void **state) {
	const struct PMapParams params = {
		.allow_null_val = true,
		.alloc_val = mock_alloc,
	};
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_put(map, K1, NULL));

	assert_int_equal(pmap_size(map), 2);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_ptr_equal(pmap_put(map, K0, V1), V0);

	assert_ptr_equal(pmap_put(map, K0, NULL), V1);

	assert_ptr_equal(pmap_get(map, K0), NULL);

	pmap_free(map);
}

static void pmap_put__alloc_val_no_allow_null_val(void **state) {
	const struct PMapParams params = { .alloc_val = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(pmap_put(map, K1, V1));

	assert_int_equal(pmap_size(map), 1);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(pmap_put(map, K0, V1));

	assert_ptr_equal(pmap_get(map, K0), V0);

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

	expect_ptr(mock_free, ptr, V0);
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
	expect_ptr(mock_match_ptr_ptr, key, K0);
	expect_ptr(mock_match_ptr_ptr, val, V0);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// get K1
	expect_ptr(mock_match_ptr_ptr, key, K1);
	expect_ptr(mock_match_ptr_ptr, val, V1);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, true);

	const struct PMapPair kv_pair = pmap_match(map, mock_match_ptr_ptr, D0);
	assert_ptr_equal(kv_pair.key, K1);
	assert_ptr_equal(kv_pair.val, V1);

	pmap_free(map);
}

static void pmap_match_key__matches(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	// skip V0
	expect_ptr(mock_match_ptr, val, K0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V1
	expect_ptr(mock_match_ptr, val, K1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	const struct PMapPair k_pair = pmap_match_key(map, mock_match_ptr, D0);
	assert_ptr_equal(k_pair.key, K1);
	assert_ptr_equal(k_pair.val, V1);

	pmap_free(map);
}

static void pmap_match_val__matches(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	const struct PMapPair v_pair = pmap_match_val(map, mock_match_ptr, D0);
	assert_ptr_equal(v_pair.key, K1);
	assert_ptr_equal(v_pair.val, V1);

	pmap_free(map);
}

static void pmap_match__no_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip K0
	expect_ptr(mock_match_ptr_ptr, key, K0);
	expect_ptr(mock_match_ptr_ptr, val, V0);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// skip K1
	expect_ptr(mock_match_ptr_ptr, key, K1);
	expect_ptr(mock_match_ptr_ptr, val, V1);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	const struct PMapPair kv_pair = pmap_match(map, mock_match_ptr_ptr, D0);
	assert_nul(kv_pair.key);
	assert_nul(kv_pair.val);

	pmap_free(map);
}

static void pmap_match_key__no_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip V0
	expect_ptr(mock_match_ptr, val, K0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// skip V1
	expect_ptr(mock_match_ptr, val, K1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	const struct PMapPair k_pair = pmap_match_key(map, mock_match_ptr, D0);
	assert_nul(k_pair.key);
	assert_nul(k_pair.val);

	pmap_free(map);
}

static void pmap_match_val__no_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// skip V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	const struct PMapPair v_pair = pmap_match_val(map, mock_match_ptr, D0);
	assert_nul(v_pair.key);
	assert_nul(v_pair.val);

	pmap_free(map);
}

static void pmap_match__null_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapPair kv_pair = pmap_match(map, NULL, D0);
	assert_nul(kv_pair.key);
	assert_nul(kv_pair.val);

	pmap_free(map);
}

static void pmap_match_key__null_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapPair k_pair = pmap_match_key(map, NULL, D0);
	assert_nul(k_pair.key);
	assert_nul(k_pair.val);

	pmap_free(map);
}

static void pmap_match_val__null_match(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapPair v_pair = pmap_match_val(map, NULL, D0);
	assert_nul(v_pair.key);
	assert_nul(v_pair.val);

	pmap_free(map);
}

static void pmap_it__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_it(map));

	pmap_free(map);
}

static void pmap_it__free(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	const struct PMapIt *it = pmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	pmap_it_free(it);

	pmap_free(map);
}

static void pmap_it__many(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, NULL));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, NULL));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, NULL));

	assert_int_equal(pmap_size(map), 5);

	// zero
	const struct PMapIt *it = pmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_nul(it->val);

	// one
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// two
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K2);
	assert_nul(it->val);

	// three
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// four
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K4);
	assert_nul(it->val);

	// end
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_it__removed(void **state) {
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
	const struct PMapIt *it = pmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// three
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// end
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_it_free__partial(void **state) {
	const struct PMapIt *it = calloc(1, sizeof(struct PMapIt));

	pmap_it_free(it);
}

static void pmap_it_next__partial(void **state) {
	const struct PMapIt *it = calloc(1, sizeof(struct PMapIt));

	assert_nul(pmap_it_next(it));
}

static void pmap_match_it__many(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, V4));

	assert_int_equal(pmap_size(map), 5);

	// skip K0
	expect_ptr(mock_match_ptr_ptr, key, K0);
	expect_ptr(mock_match_ptr_ptr, val, V0);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// get K1
	expect_ptr(mock_match_ptr_ptr, key, K1);
	expect_ptr(mock_match_ptr_ptr, val, V1);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, true);

	const struct PMapIt *it = pmap_match_it(map, mock_match_ptr_ptr, D0);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// skip K2
	expect_ptr(mock_match_ptr_ptr, key, K2);
	expect_ptr(mock_match_ptr_ptr, val, V2);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// get K3
	expect_ptr(mock_match_ptr_ptr, key, K3);
	expect_ptr(mock_match_ptr_ptr, val, V3);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, true);

	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// skip K4
	expect_ptr(mock_match_ptr_ptr, key, K4);
	expect_ptr(mock_match_ptr_ptr, val, V4);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// done
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_match_key_it__many(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, V4));

	// skip K0
	expect_ptr(mock_match_ptr, val, K0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get K1
	expect_ptr(mock_match_ptr, val, K1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	const struct PMapIt *it = pmap_match_key_it(map, mock_match_ptr, D0);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// skip K2
	expect_ptr(mock_match_ptr, val, K2);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get K3
	expect_ptr(mock_match_ptr, val, K3);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// skip K4
	expect_ptr(mock_match_ptr, val, K4);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// done
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_match_val_it__many(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));
	assert_nul(pmap_put(map, K3, V3));
	assert_nul(pmap_put(map, K4, V4));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	const struct PMapIt *it = pmap_match_val_it(map, mock_match_ptr, D0);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_ptr(mock_match_ptr, val, V2);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// get V3
	expect_ptr(mock_match_ptr, val, V3);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, true);

	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// skip V4
	expect_ptr(mock_match_ptr, val, V4);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// done
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_match_it__none(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	//
	// key/val
	//

	// skip K0
	expect_ptr(mock_match_ptr_ptr, key, K0);
	expect_ptr(mock_match_ptr_ptr, val, V0);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	// skip K1
	expect_ptr(mock_match_ptr_ptr, key, K1);
	expect_ptr(mock_match_ptr_ptr, val, V1);
	expect_ptr(mock_match_ptr_ptr, data, D0);
	will_return(mock_match_ptr_ptr, false);

	assert_nul(pmap_match_it(map, mock_match_ptr_ptr, D0));

	pmap_free(map);
}

static void pmap_match_key_it__none(void **state) {

	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip K0
	expect_ptr(mock_match_ptr, val, K0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// skip K1
	expect_ptr(mock_match_ptr, val, K1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	assert_nul(pmap_match_key_it(map, mock_match_ptr, D0));

	pmap_free(map);
}

static void pmap_match_val_it__none(void **state) {

	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	// skip V0
	expect_ptr(mock_match_ptr, val, V0);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	// skip V1
	expect_ptr(mock_match_ptr, val, V1);
	expect_ptr(mock_match_ptr, data, D0);
	will_return(mock_match_ptr, false);

	assert_nul(pmap_match_val_it(map, mock_match_ptr, D0));

	pmap_free(map);
}

static void pmap_match_it__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_match_it(map, mock_match_ptr_ptr, D0));

	pmap_free(map);
}

static void pmap_match_key_it__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_match_key_it(map, mock_match_ptr, D0));

	pmap_free(map);
}

static void pmap_match_val_it__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_match_val_it(map, mock_match_ptr, D0));

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
	const struct PMapIt *it = pmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// zero moved later
	it = pmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	// end
	it = pmap_it_next(it);
	assert_nul(it);

	pmap_free(map);
}

static void pmap_put_all__many(void **state) {
	const struct PMapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PMap *to = pmap_init_with(params);
	assert_nul(pmap_put(to, K0, V0));
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V3));
	assert_nul(pmap_put(from, K2, V4));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K0, V0));
	assert_nul(pmap_put(expected, K1, V3));
	assert_nul(pmap_put(expected, K2, V4));

	assert_int_equal(pmap_put_all(to, from), 1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all__alloc_val(void **state) {
	const struct PMapParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	const struct PMap *to = pmap_init_with(params);
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V3));
	assert_nul(pmap_put(from, K2, V4));


	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V3));
	assert_nul(pmap_put(expected, K2, V4));

	expect_ptr(mock_alloc, ptr, V3);
	will_return_ptr_type(mock_alloc, V3, void*);

	expect_ptr(mock_alloc, ptr, V4);
	will_return_ptr_type(mock_alloc, V4, void*);

	assert_int_equal(pmap_put_all(to, from), 1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all_free__many(void **state) {
	const struct PMapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PMap *to = pmap_init_with(params);
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V3));
	assert_nul(pmap_put(from, K2, V4));


	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V3));
	assert_nul(pmap_put(expected, K2, V4));

	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(pmap_put_all_free(to, from), 1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all_clone__one(void **state) {
	const struct PMapParams params = {
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	const struct PMap *to = pmap_init_with(params);
	assert_nul(pmap_put(to, K1, V1));


	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V2));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V3));

	expect_ptr(mock_clone, ptr, V2);
	will_return_ptr_type(mock_clone, V3, void*);

	assert_int_equal(pmap_put_all_clone(to, from), 1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all_clone__no_clone_val(void **state) {
	const struct PMap *to = pmap_init();
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V2));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V1));

	assert_int_equal(pmap_put_all_clone(to, from), 0);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all_clone_free__one(void **state) {
	const struct PMapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PMap *to = pmap_init_with(params);
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V2));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V3));

	expect_ptr(mock_clone, ptr, V2);
	will_return_ptr_type(mock_clone, V3, void*);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(pmap_put_all_clone_free(to, from), 1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_all_clone_free__no_clone_val(void **state) {
	const struct PMap *to = pmap_init();
	assert_nul(pmap_put(to, K1, V1));

	const struct PMap *from = pmap_init();
	assert_nul(pmap_put(from, K1, V2));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K1, V1));

	assert_int_equal(pmap_put_all_clone_free(to, from), 0);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(from);
	pmap_free(expected);
}

static void pmap_put_many__many(void **state) {
	const struct PMap *to = pmap_init();
	assert_nul(pmap_put(to, K0, V0));
	assert_nul(pmap_put(to, K1, strdup("replaced")));
	assert_nul(pmap_put(to, K2, strdup("replaced")));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K0, V0));
	assert_nul(pmap_put(expected, K1, V1));
	assert_nul(pmap_put(expected, K2, V2));

	assert_int_equal(pmap_put_many(to,
				K1, V1,
				K2, V2,
				NULL),
			2);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(expected);
}

static void pmap_put_many__no_keyvals(void **state) {
	const struct PMap *to = pmap_init();

	assert_int_equal(pmap_put_many(to, NULL), 0);

	pmap_free(to);
}

static void pmap_put_many__null_val_allowed(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *to = pmap_init_with(params);

	assert_nul(pmap_put(to, K0, V0));
	assert_nul(pmap_put(to, K1, strdup("replaced")));
	assert_nul(pmap_put(to, K2, strdup("replaced")));

	const struct PMap *expected = pmap_init_with(params);
	assert_nul(pmap_put(expected, K0, V0));
	assert_nul(pmap_put(expected, K1, NULL));
	assert_nul(pmap_put(expected, K2, V5));

	assert_int_equal(pmap_put_many(to,
				K1, NULL,
				K2, V5,
				NULL),
			2);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(expected);
}

static void pmap_put_many__null_val_not_allowed(void **state) {
	const struct PMap *to = pmap_init();

	assert_nul(pmap_put(to, K0, V0));
	assert_nul(pmap_put(to, K1, strdup("replaced")));

	const struct PMap *expected = pmap_init();
	assert_nul(pmap_put(expected, K0, V0));
	assert_nul(pmap_put(expected, K1, V1));

	assert_int_equal(pmap_put_many(to,
				K0, NULL,
				K1, V1,
				NULL),
			1);

	assert_pmap_equal(to, expected);

	pmap_free(to);
	pmap_free(expected);
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
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	const char *val = strdup("val");

	assert_nul(pmap_put(map, K0, val));
	assert_nul(pmap_put(map, K1, NULL));

	assert_true(pmap_remove_free(map, K0));

	assert_true(pmap_remove_free(map, K1));

	assert_false(pmap_remove_free(map, K2));

	pmap_free(map);
}

static void pmap_remove_free__free_val(void **state) {
	const struct PMapParams params = {
		.free_val = mock_free,
		.allow_null_val = true,
	};
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));

	assert_false(pmap_remove_free(map, K1));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_free, ptr, V0);
	assert_true(pmap_remove_free(map, K0));

	assert_true(pmap_remove_free(map, K1));

	pmap_free(map);
}

static void pmap_remove_all__free_key(void **state) {
	const struct PMapParams params = { .free_val = mock_free, .free_key = mock_free, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	const struct PMap *from = pmap_init();

	assert_nul(pmap_put(from, K0, V0));
	assert_nul(pmap_put(from, K2, V2));
	assert_nul(pmap_put(from, K3, V3));

	const struct PMap *expected = pmap_init();

	assert_nul(pmap_put(expected, K1, V1));

	expect_ptr(mock_free, ptr, K0);
	expect_ptr(mock_free, ptr, K2);

	assert_int_equal(pmap_remove_all(map, from), 2);

	assert_pmap_equal(map, expected);

	expect_ptr(mock_free, ptr, K1);

	pmap_free(map);

	pmap_free(from);
	pmap_free(expected);
}

static void pmap_remove_all_free__free_val(void **state) {
	const struct PMapParams params = { .free_val = mock_free, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, V2));

	const struct PMap *from = pmap_init();

	assert_nul(pmap_put(from, K0, V0));
	assert_nul(pmap_put(from, K2, V2));
	assert_nul(pmap_put(from, K3, V3));

	const struct PMap *expected = pmap_init();

	assert_nul(pmap_put(expected, K1, V1));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V2);

	assert_int_equal(pmap_remove_all_free(map, from), 2);

	assert_pmap_equal(map, expected);

	pmap_free(map);

	pmap_free(from);
	pmap_free(expected);
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
	const struct PMapParams params = { .equal_key = equal_ptr, };
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

static void pmap_contains_val__pointers(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	assert_false(pmap_contains_val(map, K0));

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));
	assert_nul(pmap_put(map, K2, NULL));

	assert_true(pmap_contains_val(map, V0));
	assert_true(pmap_contains_val(map, V1));

	assert_false(pmap_contains_val(map, V2));

	assert_true(pmap_contains_val(map, NULL));

	pmap_free(map);
}

static void pmap_contains_val__equal_val(void **state) {
	const struct PMapParams params = { .equal_val = equal_ptr, };
	const struct PMap *map = pmap_init_with(params);

	assert_false(pmap_contains_val(map, V0));

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, V1));

	assert_true(pmap_contains_val(map, V0));
	assert_true(pmap_contains_val(map, V1));

	assert_false(pmap_contains_val(map, V2));

	assert_false(pmap_contains_val(map, NULL));

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
	const struct PMapParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, K0, "a"));

	assert_nul(pmap_put(b, K0, "a"));

	assert_pmap_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_val_different(void **state) {
	const struct PMapParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct PMap *a = pmap_init_with(params);
	const struct PMap *b = pmap_init_with(params);

	assert_nul(pmap_put(a, K0, "a"));

	assert_nul(pmap_put(b, K0, "b"));

	assert_pmap_not_equal(a, b);

	pmap_free(a);
	pmap_free(b);
}

static void pmap_equal__equal_key_ok(void **state) {
	const struct PMapParams params = { .equal_key = (fn_equal)equal_strcasecmp, };
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
	const struct PMapParams params = { .equal_key = (fn_equal)equal_strcasecmp, };
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

static void pmap_keys_slist__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_keys_slist(map));

	pmap_free(map);
}

static void pmap_keys_slist__many(void **state) {
	const struct PMap *map = pmap_init();

	pmap_put(map, K0, V0);
	pmap_put(map, K1, V1);

	struct SList *list = pmap_keys_slist(map);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), K0);
	assert_ptr_equal(slist_at(list, 1), K1);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_keys_slist__alloc_key(void **state) {
	const struct PMapParams params = { .alloc_key = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	pmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	struct SList *list = pmap_keys_slist(map);

	assert_int_equal(slist_length(list), 1);
	assert_ptr_equal(slist_at(list, 0), K0);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_keys_pset__empty(void **state) {
	const struct PMap *map = pmap_init();

	const struct PSet *set = pmap_keys_pset(map);

	assert_non_nul(set);
	assert_int_equal(pset_size(set), 0);

	pmap_free(map);
	pset_free(set);
}

static void pmap_keys_pset__many(void **state) {
	const struct PMapParams params = {
		.initial = 1,
		.grow = 1,
	};
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V0);
	pmap_put(map, K1, V1);
	pmap_put(map, K2, V1);

	const struct PSet *expected = pset_init();
	pset_add(expected, K0);
	pset_add(expected, K1);
	pset_add(expected, K2);

	const struct PSet *actual = pmap_keys_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 3);
	assert_int_equal(actual->capacity, 3);

	pmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void pmap_keys_pset__params(void **state) {
	const struct PMapParams params = {
		.equal_key = mock_equal,
		.alloc_key = mock_alloc,
		.free_key = mock_free,
		.str_key = mock_str,
		.initial = 99,
		.grow = 1,
	};
	const struct PMap *map = pmap_init_with(params);

	const struct PSet *set = pmap_keys_pset(map);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 99);
	assert_int_equal(set->params.grow, 1);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.alloc_val, mock_alloc);
	assert_ptr_equal(set->params.free_val, mock_free);
	assert_ptr_equal(set->params.clone_val, mock_alloc);
	assert_ptr_equal(set->params.str_val, mock_str);

	pmap_free(map);

	pset_free(set);
}

static void pmap_vals_slist__empty(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_vals_slist(map));

	pmap_free(map);
}

static void pmap_vals_slist__many(void **state) {
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V1);
	pmap_put(map, K1, NULL);
	pmap_put(map, K2, V3);

	struct SList *list = pmap_vals_slist(map);

	assert_int_equal(slist_length(list), 3);
	assert_ptr_equal(slist_at(list, 0), V1);
	assert_nul(slist_at(list, 1));
	assert_ptr_equal(slist_at(list, 2), V3);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_vals_slist__alloc_val(void **state) {
	const struct PMapParams params = { .alloc_val = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	pmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	struct SList *list = pmap_vals_slist(map);

	assert_int_equal(slist_length(list), 1);
	assert_ptr_equal(slist_at(list, 0), V0);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_vals_pset__empty(void **state) {
	const struct PMap *map = pmap_init();

	const struct PSet *set = pmap_vals_pset(map);

	assert_non_nul(set);
	assert_int_equal(pset_size(set), 0);

	pmap_free(map);
	pset_free(set);
}

static void pmap_vals_pset__many(void **state) {
	const struct PMapParams params = {
		.initial = 1,
		.grow = 1,
		.allow_null_val = true,
	};
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V0);
	pmap_put(map, K1, V1);
	pmap_put(map, K2, V1);
	pmap_put(map, K3, NULL);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);
	pset_add(expected, V1);

	const struct PSet *actual = pmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 2);
	assert_int_equal(actual->capacity, 4);

	pmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void pmap_vals_pset__alloc_val(void **state) {
	const struct PMapParams params = { .alloc_val = mock_alloc, };
	const struct PMap *map = pmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	pmap_put(map, K0, V0);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PSet *actual = pmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 1);

	pmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void pmap_vals_pset_clone__many(void **state) {
	const struct PMapParams params = {
		.clone_val = mock_clone,
	};
	const struct PMap *map = pmap_init_with(params);

	pmap_put(map, K0, V0);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct PSet *actual = pmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 1);

	pmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void pmap_vals_pset_clone__no_clone_val(void **state) {
	const struct PMap *map = pmap_init();

	pmap_put(map, K0, V0);

	assert_nul(pmap_vals_pset_clone(map));

	pmap_free(map);
}

static void pmap_vals_pset_clone__alloc_val_and_clone_val(void **state) {
	const struct PMapParams params = {
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
	};
	const struct PMap *map = pmap_init_with(params);

	const struct PSet *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	pmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PSet *actual = pmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	pmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void pmap_vals_pset__params(void **state) {
	const struct PMapParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.str_val = mock_str,
		.initial = 99,
		.grow = 1,
	};
	const struct PMap *map = pmap_init_with(params);

	const struct PSet *set = pmap_vals_pset(map);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 99);
	assert_int_equal(set->params.grow, 1);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.alloc_val, mock_alloc);
	assert_ptr_equal(set->params.free_val, mock_free);
	assert_ptr_equal(set->params.clone_val, mock_clone);
	assert_ptr_equal(set->params.str_val, mock_str);

	pmap_free(map);

	pset_free(set);
}

static void pmap_vals_slist_clone__clone_val(void **state) {
	const struct PMapParams params = { .clone_val = mock_clone, };
	const struct PMap *map = pmap_init_with(params);

	assert_nul(pmap_put(map, K0, V0));

	assert_nul(pmap_put(map, K1, NULL));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct SList *list = pmap_vals_slist_clone(map);

	assert_ptr_equal(slist_at(list, 0), V0);
	assert_ptr_equal(slist_at(list, 1), NULL);

	slist_free(&list);
	pmap_free(map);
}

static void pmap_vals_slist_clone__no_clone_val(void **state) {
	const struct PMap *map = pmap_init();

	assert_nul(pmap_put(map, K0, V0));
	assert_nul(pmap_put(map, K1, NULL));

	assert_nul(pmap_vals_slist_clone(map));

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
	const struct PMapParams params = { .allow_null_val = true, };
	const struct PMap *map = pmap_init_with(params);

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

static char* str_first(const void *val) {
	return strndup(val, 1);
}

static void pmap_str__str_val(void **state) {
	const struct PMapParams params = {
		.allow_null_val = true,
		.str_val = str_first,
	};
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
	const struct PMapParams params = {
		.allow_null_val = true,
		.str_key = (fn_str)str_or_null,
	};
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

	assert_nul(pmap_clone(NULL));
	assert_nul(pmap_clone_deep(NULL));
	pmap_free(NULL);
	pmap_free_vals(NULL);
	pmap_it_free(NULL);
	assert_false(pmap_get(NULL, NULL));
	assert_false(pmap_get(map, NULL));
	assert_false(pmap_contains_key(NULL, NULL));
	assert_false(pmap_contains_key(map, NULL));
	assert_false(pmap_contains_val(NULL, NULL));
	assert_false(pmap_contains_val(map, NULL));
	pmap_match(NULL, NULL, NULL);
	pmap_match(map, NULL, NULL);
	pmap_match_val(NULL, NULL, NULL);
	pmap_match_val(map, NULL, NULL);
	pmap_match_key(NULL, NULL, NULL);
	pmap_match_key(map, NULL, NULL);
	assert_nul(pmap_it(NULL));
	assert_nul(pmap_match_it(NULL, NULL, NULL));
	assert_nul(pmap_match_it(map, NULL, NULL));
	assert_nul(pmap_match_it(NULL, mock_match_ptr_ptr, NULL));
	assert_nul(pmap_match_key_it(NULL, NULL, NULL));
	assert_nul(pmap_match_key_it(map, NULL, NULL));
	assert_nul(pmap_match_key_it(NULL, mock_match_ptr, NULL));
	assert_nul(pmap_match_val_it(NULL, NULL, NULL));
	assert_nul(pmap_match_val_it(map, NULL, NULL));
	assert_nul(pmap_match_val_it(NULL, mock_match_ptr, NULL));
	assert_nul(pmap_it_next(NULL));
	assert_false(pmap_put(NULL, NULL, NULL));
	assert_false(pmap_put(map, NULL, NULL));
	assert_nul(pmap_put_if_absent(NULL, NULL, NULL));
	assert_nul(pmap_put_if_absent(map, NULL, NULL));
	assert_false(pmap_put_free(NULL, NULL, NULL));
	assert_false(pmap_put_free(map, NULL, NULL));
	assert_int_equal(pmap_put_many(NULL, NULL), 0);
	assert_int_equal(pmap_put_many_v(NULL, NULL), 0);
	assert_int_equal(pmap_put_all(NULL, NULL), 0);
	assert_int_equal(pmap_put_all(map, NULL), 0);
	assert_int_equal(pmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(pmap_put_all_free(map, NULL), 0);
	assert_int_equal(pmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(pmap_put_all_clone(map, NULL), 0);
	assert_int_equal(pmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(pmap_put_all_clone_free(map, NULL), 0);
	assert_nul(pmap_remove(NULL, NULL));
	assert_nul(pmap_remove(map, NULL));
	assert_int_equal(pmap_remove_all(NULL, NULL), 0);
	assert_int_equal(pmap_remove_all(map, NULL), 0);
	assert_int_equal(pmap_remove_all(NULL, map), 0);
	assert_int_equal(pmap_remove_all_free(NULL, NULL), 0);
	assert_int_equal(pmap_remove_all_free(map, NULL), 0);
	assert_int_equal(pmap_remove_all_free(NULL, map), 0);
	assert_false(pmap_equal(NULL, NULL));
	assert_false(pmap_equal(map, NULL));
	assert_nul(pmap_keys_slist(NULL));
	assert_nul(pmap_keys_pset(NULL));
	assert_nul(pmap_vals_slist_clone(NULL));
	assert_nul(pmap_vals_slist(NULL));
	assert_nul(pmap_vals_pset(NULL));
	assert_nul(pmap_vals_pset_clone(NULL));
	assert_nul(pmap_str(NULL));
	assert_int_equal(pmap_size(NULL), 0);

	pmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pmap_init__defaults),

		TEST(pmap_clone__empty),
		TEST(pmap_clone__params),
		TEST(pmap_clone__many),
		TEST(pmap_clone__alloc_key),
		TEST(pmap_clone__alloc_val),

		TEST(pmap_clone_deep__clone_val_allow_null_val),
		TEST(pmap_clone_deep__clone_val_no_allow_null_val),
		TEST(pmap_clone_deep__no_clone_val),
		TEST(pmap_clone_deep__alloc_val_and_clone_val),

		TEST(pmap_free_vals__null_free_val),
		TEST(pmap_free_vals__free_val),
		TEST(pmap_free_vals__free_val_hierarchical),

		TEST(pmap_put__new),
		TEST(pmap_put__overwrite),
		TEST(pmap_put__null_key),
		TEST(pmap_put__allow_null_val),
		TEST(pmap_put__no_allow_null_val),
		TEST(pmap_put__grow),
		TEST(pmap_put__alloc_key_free_key),
		TEST(pmap_put__alloc_key_returned_null),
		TEST(pmap_put__equal_key),
		TEST(pmap_put__alloc_val_allow_null_val),
		TEST(pmap_put__alloc_val_no_allow_null_val),

		TEST(pmap_put_free__free),
		TEST(pmap_put_free__free_val),

		TEST(pmap_put_if_absent__),

		TEST(pmap_match__matches),
		TEST(pmap_match_key__matches),
		TEST(pmap_match_val__matches),

		TEST(pmap_match__no_match),
		TEST(pmap_match_key__no_match),
		TEST(pmap_match_val__no_match),

		TEST(pmap_match__null_match),
		TEST(pmap_match_key__null_match),
		TEST(pmap_match_val__null_match),

		TEST(pmap_it__empty),
		TEST(pmap_it__free),
		TEST(pmap_it__many),
		TEST(pmap_it__removed),

		TEST(pmap_it_free__partial),

		TEST(pmap_it_next__partial),

		TEST(pmap_match_it__many),
		TEST(pmap_match_key_it__many),
		TEST(pmap_match_val_it__many),

		TEST(pmap_match_it__none),
		TEST(pmap_match_key_it__none),
		TEST(pmap_match_val_it__none),

		TEST(pmap_match_it__empty),
		TEST(pmap_match_key_it__empty),
		TEST(pmap_match_val_it__empty),

		TEST(pmap_put__again),

		TEST(pmap_put_all__many),
		TEST(pmap_put_all__alloc_val),

		TEST(pmap_put_all_free__many),

		TEST(pmap_put_all_clone__one),
		TEST(pmap_put_all_clone__no_clone_val),

		TEST(pmap_put_all_clone_free__one),
		TEST(pmap_put_all_clone_free__no_clone_val),

		TEST(pmap_put_many__many),
		TEST(pmap_put_many__no_keyvals),
		TEST(pmap_put_many__null_val_allowed),
		TEST(pmap_put_many__null_val_not_allowed),

		TEST(pmap_remove__existing),
		TEST(pmap_remove__inexistent),

		TEST(pmap_remove_free__free),
		TEST(pmap_remove_free__free_val),

		TEST(pmap_remove_all__free_key),
		TEST(pmap_remove_all_free__free_val),

		TEST(pmap_contains_key__pointers),
		TEST(pmap_contains_key__equal_key),

		TEST(pmap_contains_val__pointers),
		TEST(pmap_contains_val__equal_val),

		TEST(pmap_equal__length_different),
		TEST(pmap_equal__key_pointers_ok),
		TEST(pmap_equal__key_pointers_different),
		TEST(pmap_equal__equal_val_ok),
		TEST(pmap_equal__equal_val_different),
		TEST(pmap_equal__equal_key_ok),
		TEST(pmap_equal__equal_key_different),

		TEST(pmap_keys_slist__empty),
		TEST(pmap_keys_slist__many),
		TEST(pmap_keys_slist__alloc_key),

		TEST(pmap_keys_pset__empty),
		TEST(pmap_keys_pset__many),
		TEST(pmap_keys_pset__params),

		TEST(pmap_vals_slist__empty),
		TEST(pmap_vals_slist__many),
		TEST(pmap_vals_slist__alloc_val),

		TEST(pmap_vals_slist_clone__clone_val),
		TEST(pmap_vals_slist_clone__no_clone_val),

		TEST(pmap_vals_pset__empty),
		TEST(pmap_vals_pset__many),
		TEST(pmap_vals_pset__alloc_val),
		TEST(pmap_vals_pset__params),

		TEST(pmap_vals_pset_clone__many),
		TEST(pmap_vals_pset_clone__no_clone_val),
		TEST(pmap_vals_pset_clone__alloc_val_and_clone_val),

		TEST(pmap_str__empty),
		TEST(pmap_str__pointers),
		TEST(pmap_str__str_val),
		TEST(pmap_str__str_key),

		TEST(pmap__null_inputs),
	};

	return RUN(tests);
}

