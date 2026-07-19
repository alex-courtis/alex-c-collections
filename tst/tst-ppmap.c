#include "assert-ppmap.h"
#include "assert-pset.h"
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
#include "pset.h"
#include "pslist.h"
#include "str.h"

#include "ppmap.h"

struct PPmap {
	const struct PPmapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Pset {
	const struct PsetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static void *alloc_key_duplicate(const void* const val) {
	return sprintf_alloc("%s%s", (char*)val, (char*)val);
}

static void ppmap_init__defaults(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_non_nul(map);

	assert_int_equal(map->size, 0);
	assert_int_equal(map->capacity, 10);

	size_t k[25] = { 0 };
	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		ppmap_put(map, &k[i], &v[i]);

	assert_int_equal(map->size, 25);
	assert_int_equal(map->capacity, 30);

	ppmap_free(map);
}

static void ppmap_clone__empty(void **state) {
	const struct PPmap *from = ppmap_init();

	const struct PPmap *to = ppmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->size, 0);

	ppmap_free(from);
	ppmap_free(to);
}

// also tests constructor
static void ppmap_clone__params(void **state) {
	const struct PPmapParams params = {
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
	const struct PPmap *from = ppmap_init_with(params);

	const struct PPmap *to = ppmap_clone(from);

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

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone__many(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *from = ppmap_init_with(params);

	assert_nul(ppmap_put(from, K0, NULL));
	assert_nul(ppmap_put(from, K1, V1));
	assert_nul(ppmap_put(from, K2, NULL));
	assert_nul(ppmap_put(from, K3, V3));
	assert_nul(ppmap_put(from, K4, NULL));

	const struct PPmap *to = ppmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(ppmap_size(to), 5);

	assert_ppmap_equal(from, to);

	assert_ptr_equal(ppmap_get(to, K0), NULL);
	assert_ptr_equal(ppmap_get(to, K1), V1);
	assert_ptr_equal(ppmap_get(to, K2), NULL);
	assert_ptr_equal(ppmap_get(to, K3), V3);
	assert_ptr_equal(ppmap_get(to, K4), NULL);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone__alloc_key(void **state) {
	const struct PPmapParams params = {
		.alloc_key = mock_alloc,
		.allow_null_val = true,
	};
	const struct PPmap *from = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);
	assert_nul(ppmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, K1);
	will_return_ptr_type(mock_alloc, K1, void*);
	assert_nul(ppmap_put(from, K1, V1));

	expect_ptr(mock_alloc, ptr, K2);
	will_return_ptr_type(mock_alloc, K2, void*);
	assert_nul(ppmap_put(from, K2, NULL));

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K3, void*);
	expect_ptr(mock_alloc, ptr, K1);
	will_return_ptr_type(mock_alloc, K4, void*);
	expect_ptr(mock_alloc, ptr, K2);
	will_return_ptr_type(mock_alloc, K5, void*);

	const struct PPmap *to = ppmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(ppmap_size(to), 3);

	assert_ppmap_not_equal(from, to);

	assert_ptr_equal(ppmap_get(to, K3), V0);
	assert_ptr_equal(ppmap_get(to, K4), V1);
	assert_ptr_equal(ppmap_get(to, K5), NULL);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone__alloc_val(void **state) {
	const struct PPmapParams params = {
		.alloc_val = mock_alloc,
	};
	const struct PPmap *from = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ppmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PPmap *to = ppmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(ppmap_size(to), 1);

	assert_ppmap_equal(from, to);

	assert_ptr_equal(ppmap_get(to, K0), V0);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone_deep__clone_val_allow_null_val(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.clone_val = mock_clone,
	};
	const struct PPmap *from = ppmap_init_with(params);

	assert_nul(ppmap_put(from, K0, V0));

	assert_nul(ppmap_put(from, K1, V1));

	assert_nul(ppmap_put(from, K2, NULL));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct PPmap *to = ppmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(ppmap_size(to), 3);

	assert_ppmap_not_equal(from, to);

	assert_ptr_equal(ppmap_get(to, K0), V2);
	assert_ptr_equal(ppmap_get(to, K1), V3);
	assert_ptr_equal(ppmap_get(to, K2), NULL);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone_deep__clone_val_no_allow_null_val(void **state) {
	const struct PPmapParams params = {
		.clone_val = mock_clone,
	};
	const struct PPmap *from = ppmap_init_with(params);

	assert_nul(ppmap_put(from, K0, V0));

	assert_nul(ppmap_put(from, K1, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);

	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, NULL, void*);

	const struct PPmap *to = ppmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(ppmap_size(to), 1);

	assert_ppmap_not_equal(from, to);

	assert_ptr_equal(ppmap_get(to, K0), V2);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_clone_deep__no_clone_val(void **state) {
	const struct PPmap *from = ppmap_init();

	assert_nul(ppmap_put(from, K0, V0));
	assert_nul(ppmap_put(from, K1, NULL));

	assert_nul(ppmap_clone_deep(from));

	ppmap_free(from);
}

static void ppmap_clone_deep__alloc_val_and_clone_val(void **state) {
	const struct PPmapParams params = {
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
	};
	const struct PPmap *from = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ppmap_put(from, K0, V0));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct PPmap *to = ppmap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(ppmap_size(to), 1);

	ppmap_free(from);
	ppmap_free(to);
}

static void ppmap_free_vals__null_free_val(void **state) {
	const struct PPmap *map = ppmap_init();

	const char *val = strdup("0");

	ppmap_put(map, K0, val);

	assert_int_equal(ppmap_size(map), 1);

	ppmap_free_vals(map);
}

static void ppmap_free_vals__free_val(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, V2);

	assert_int_equal(ppmap_size(map), 3);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V2);

	ppmap_free_vals(map);
}

static void free_ppmap(const void *val) {
	ppmap_free_vals(val);
}

static void ppmap_free_vals__free_val_hierarchical(void **state) {
	const struct PPmapParams params_outer = { .free_val = (fn_free)free_ppmap, };
	const struct PPmap *outer = ppmap_init_with(params_outer);

	const struct PPmapParams params_inner = { .free_val = mock_free, };
	const struct PPmap *inner1 = ppmap_init_with(params_inner);
	const struct PPmap *inner2 = ppmap_init_with(params_inner);

	ppmap_put(outer, K0, (void*)inner1);
	ppmap_put(outer, K1, (void*)inner2);

	ppmap_put(inner1, K2, V2);
	ppmap_put(inner1, K3, V3);

	ppmap_put(inner2, K4, V4);
	ppmap_put(inner2, K5, V5);

	assert_int_equal(ppmap_size(outer), 2);

	expect_ptr(mock_free, ptr, V2);
	expect_ptr(mock_free, ptr, V3);
	expect_ptr(mock_free, ptr, V4);
	expect_ptr(mock_free, ptr, V5);

	ppmap_free_vals(outer);
}

static void ppmap_put__new(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(ppmap_size(map), 2);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);

	ppmap_free(map);
}

static void ppmap_put__overwrite(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));
	assert_nul(ppmap_put(map, K3, V3));

	assert_ptr_equal(ppmap_put(map, K1, V4), V1);

	assert_ptr_equal(ppmap_put(map, K3, V5), V3);

	assert_int_equal(ppmap_size(map), 4);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V4);
	assert_ptr_equal(ppmap_get(map, K2), V2);
	assert_ptr_equal(ppmap_get(map, K3), V5);

	ppmap_free(map);
}

static void ppmap_put__null_key(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, NULL, V0));
	assert_int_equal(ppmap_size(map), 0);

	assert_false(ppmap_contains_key(map, K0));

	ppmap_free(map);
}

static void ppmap_put__allow_null_val(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_int_equal(ppmap_size(map), 1);

	assert_nul(ppmap_put(map, K1, NULL));
	assert_int_equal(ppmap_size(map), 2);

	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_true(ppmap_contains_key(map, K1));

	assert_true(ppmap_put(map, K0, NULL));
	assert_nul(ppmap_get(map, K0));

	ppmap_free(map);
}

static void ppmap_put__no_allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_int_equal(ppmap_size(map), 1);

	assert_nul(ppmap_put(map, K1, NULL));
	assert_int_equal(ppmap_size(map), 1);

	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_false(ppmap_contains_key(map, K1));

	assert_false(ppmap_put(map, K0, NULL));
	assert_ptr_equal(ppmap_get(map, K0), V0);

	ppmap_free(map);
}

static void ppmap_put__grow(void **state) {
	const struct PPmapParams params = { .initial = 3, .grow = 5, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	assert_int_equal(map->size, 3);
	assert_int_equal(map->capacity, 3);
	assert_int_equal(map->params.grow, 5);

	assert_nul(ppmap_put(map, K3, V3));

	assert_int_equal(map->size, 4);
	assert_int_equal(map->capacity, 8);
	assert_int_equal(map->params.grow, 5);

	assert_nul(ppmap_put(map, K4, V4));
	assert_nul(ppmap_put(map, K5, V5));

	assert_int_equal(map->size, 6);
	assert_int_equal(map->capacity, 8);
	assert_int_equal(map->params.grow, 5);

	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);
	assert_ptr_equal(ppmap_get(map, K2), V2);

	assert_ptr_equal(ppmap_get(map, K3), V3);
	assert_ptr_equal(ppmap_get(map, K4), V4);
	assert_ptr_equal(ppmap_get(map, K5), V5);

	ppmap_free(map);
}

static void ppmap_put__alloc_key_free_key(void **state) {
	const struct PPmapParams params = {
		.equal_key = (fn_equal)equal_strcmp,
		.alloc_key = alloc_key_duplicate,
		.free_key = free,
	};
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, "zero", V0));
	assert_nul(ppmap_put(map, "one", V1));

	assert_ptr_equal(ppmap_get(map, "zerozero"), V0);
	assert_ptr_equal(ppmap_get(map, "oneone"), V1);

	assert_ptr_equal(ppmap_remove(map, "zerozero"), V0);

	assert_int_equal(ppmap_size(map), 1);
	assert_ptr_equal(ppmap_get(map, "oneone"), V1);

	ppmap_free(map);
}

static void ppmap_put__alloc_key_returned_null(void **state) {
	const struct PPmapParams params = { .alloc_key = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(ppmap_size(map), 0);

	ppmap_free(map);
}


static void ppmap_put__equal_key(void **state) {
	const struct PPmapParams params = { .equal_key = equal_ptr, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(ppmap_size(map), 2);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);

	assert_ptr_equal(ppmap_put(map, K0, V2), V0);

	assert_ptr_equal(ppmap_remove(map, K1), V1);

	ppmap_free(map);
}

static void ppmap_put__alloc_val_allow_null_val(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.alloc_val = mock_alloc,
	};
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_nul(ppmap_put(map, K1, NULL));

	assert_int_equal(ppmap_size(map), 2);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_ptr_equal(ppmap_put(map, K0, V1), V0);

	assert_ptr_equal(ppmap_put(map, K0, NULL), V1);

	assert_ptr_equal(ppmap_get(map, K0), NULL);

	ppmap_free(map);
}

static void ppmap_put__alloc_val_no_allow_null_val(void **state) {
	const struct PPmapParams params = { .alloc_val = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_nul(ppmap_put(map, K1, NULL));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(ppmap_size(map), 1);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K0, V1));

	assert_ptr_equal(ppmap_get(map, K0), V0);

	ppmap_free(map);
}

static void ppmap_put_free__free(void **state) {
	const struct PPmap *map = ppmap_init();

	const char *val = strdup("val");

	assert_nul(ppmap_put(map, K0, val));

	assert_false(ppmap_put_free(map, K1, V1));

	assert_true(ppmap_put_free(map, K0, V0));

	ppmap_free(map);
}

static void ppmap_put_free__free_val(void **state) {
	const struct PPmapParams params = { .free_val = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));

	assert_false(ppmap_put_free(map, K1, V1));

	expect_ptr(mock_free, ptr, V0);
	assert_true(ppmap_put_free(map, K0, V0));

	ppmap_free(map);
}

static void ppmap_put_if_absent__(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put_if_absent(map, K0, V0));
	assert_ptr_equal(ppmap_get(map, K0), V0);

	const void *existing = ppmap_put_if_absent(map, K0, V1);
	assert_ptr_equal(existing, V0);

	ppmap_free(map);
}

static void ppmap_find2__empty_filter(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	const struct PPmapFilter filter = { 0 };

	const struct PPmapPair kv_pair = ppmap_find2(map, filter);

	assert_ptr_equal(kv_pair.key, K0);
	assert_ptr_equal(kv_pair.val, V0);

	ppmap_free(map);
}

static void ppmap_find2__empty_map(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct PPmapFilter filter = {
		.key = mock_pred,
		.key_data = mock_2pred,
		.val = mock_pred,
		.val_data = mock_2pred,
		.key_val = mock_2pred,
		.key_val_data = mock_3pred,
		.data = D0,
	};
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);

	assert_nul(kv_pair.key);
	assert_nul(kv_pair.val);

	ppmap_free(map);
}

static void ppmap_find2__key(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip K0
	expect_ptr(mock_pred, ptr, K0);
	will_return(mock_pred, false);

	// get K1
	expect_ptr(mock_pred, ptr, K1);
	will_return(mock_pred, true);

	const struct PPmapFilter filter = { .key = mock_pred, };
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);

	assert_ptr_equal(kv_pair.key, K1);
	assert_ptr_equal(kv_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__key_data(void **state) {
	const struct PPmap *map = ppmap_init();


	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip K0
	expect_ptr(mock_2pred, ptr, K0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// get K1
	expect_ptr(mock_2pred, ptr, K1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	const struct PPmapFilter filter = { .key_data = mock_2pred, .data = D0, };
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);

	assert_ptr_equal(kv_pair.key, K1);
	assert_ptr_equal(kv_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip V0
	expect_ptr(mock_pred, ptr, V0);
	will_return(mock_pred, false);

	// get V1
	expect_ptr(mock_pred, ptr, V1);
	will_return(mock_pred, true);

	const struct PPmapFilter filter = { .val = mock_pred, };
	const struct PPmapPair v_pair = ppmap_find2(map, filter);

	assert_ptr_equal(v_pair.key, K1);
	assert_ptr_equal(v_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__val_data(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip V0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// get V1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	const struct PPmapFilter filter = { .val_data = mock_2pred, .data = D0, };
	const struct PPmapPair v_pair = ppmap_find2(map, filter);

	assert_ptr_equal(v_pair.key, K1);
	assert_ptr_equal(v_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__key_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip K0
	expect_ptr(mock_2pred, ptr, K0);
	expect_ptr(mock_2pred, data, V0);
	will_return(mock_2pred, false);

	// get K1
	expect_ptr(mock_2pred, ptr, K1);
	expect_ptr(mock_2pred, data, V1);
	will_return(mock_2pred, true);

	const struct PPmapFilter filter = { .key_val = mock_2pred, };
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);
	assert_ptr_equal(kv_pair.key, K1);
	assert_ptr_equal(kv_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__key_val_data(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// skip K0
	expect_ptr(mock_3pred, ptr1, K0);
	expect_ptr(mock_3pred, ptr2, V0);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, false);

	// get K1
	expect_ptr(mock_3pred, ptr1, K1);
	expect_ptr(mock_3pred, ptr2, V1);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, true);

	const struct PPmapFilter filter = { .key_val_data = mock_3pred, .data = D0, };
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);
	assert_ptr_equal(kv_pair.key, K1);
	assert_ptr_equal(kv_pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find2__some_block(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// key blocks
	expect_ptr(mock_pred, ptr, K0);
	will_return(mock_pred, false);

	// key passes, val blocks
	expect_ptr(mock_pred, ptr, K1);
	will_return(mock_pred, true);
	expect_ptr(mock_pred, ptr, V1);
	will_return(mock_pred, false);

	// both pass
	expect_ptr(mock_pred, ptr, K2);
	will_return(mock_pred, true);
	expect_ptr(mock_pred, ptr, V2);
	will_return(mock_pred, true);

	const struct PPmapFilter filter = {
		.key = mock_pred,
		.val = mock_pred,
	};
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);
	assert_ptr_equal(kv_pair.key, K2);
	assert_ptr_equal(kv_pair.val, V2);

	ppmap_free(map);
}

static void ppmap_find2__all_block(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	// key blocks, val will not be evaluated
	expect_any_count(mock_pred, ptr, 3);
	will_return_int_count(mock_pred, false, 3);

	const struct PPmapFilter filter = {
		.key = mock_pred,
		.val = mock_pred,
	};
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);
	assert_nul(kv_pair.key);
	assert_nul(kv_pair.val);

	ppmap_free(map);
}

static void ppmap_find2__none_block(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	// both pass
	expect_ptr(mock_pred, ptr, K0);
	will_return(mock_pred, true);
	expect_ptr(mock_pred, ptr, V0);
	will_return(mock_pred, true);

	const struct PPmapFilter filter = {
		.key = mock_pred,
		.val = mock_pred,
	};
	const struct PPmapPair kv_pair = ppmap_find2(map, filter);
	assert_ptr_equal(kv_pair.key, K0);
	assert_ptr_equal(kv_pair.val, V0);

	ppmap_free(map);
}

static void ppmap_it__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_it(map));

	ppmap_free(map);
}

static void ppmap_it__free(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	const struct PPmapIt *it = ppmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	ppmap_it_free(it);

	ppmap_free(map);
}

static void ppmap_it__many(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, NULL));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, NULL));
	assert_nul(ppmap_put(map, K3, V3));
	assert_nul(ppmap_put(map, K4, NULL));

	assert_int_equal(ppmap_size(map), 5);

	// zero
	const struct PPmapIt *it = ppmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_nul(it->val);

	// one
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// two
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K2);
	assert_nul(it->val);

	// three
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// four
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K4);
	assert_nul(it->val);

	// end
	it = ppmap_it_next(it);
	assert_nul(it);

	ppmap_free(map);
}

static void ppmap_it__removed(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));
	assert_nul(ppmap_put(map, K3, V3));
	assert_nul(ppmap_put(map, K4, V4));

	assert_ptr_equal(ppmap_remove(map, K0), V0);

	assert_ptr_equal(ppmap_remove(map, K2), V2);

	assert_ptr_equal(ppmap_remove(map, K4), V4);

	assert_int_equal(ppmap_size(map), 2);

	// one
	const struct PPmapIt *it = ppmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// three
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// end
	it = ppmap_it_next(it);
	assert_nul(it);

	ppmap_free(map);
}

static void ppmap_it_free__partial(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	ppmap_it_free(it);
}

static void ppmap_it_next__partial(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	assert_nul(ppmap_it_next(it));
}

static void ppmap_filter_it2__empty_filter(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	const struct PPmapFilter filter = { 0 };

	const struct PPmapIt *it = ppmap_filter_it2(map, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	ppmap_it_free(it);

	ppmap_free(map);
}

static void ppmap_filter_it2__empty_map(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct PPmapFilter filter = {
		.key = mock_pred,
		.key_data = mock_2pred,
		.val = mock_pred,
		.val_data = mock_2pred,
		.key_val = mock_2pred,
		.key_val_data = mock_3pred,
		.data = D0,
	};
	assert_nul(ppmap_filter_it2(map, filter));

	ppmap_free(map);
}

static void ppmap_filter_it2__many(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));
	assert_nul(ppmap_put(map, K3, V3));
	assert_nul(ppmap_put(map, K4, V4));

	assert_int_equal(ppmap_size(map), 5);

	// skip K0
	expect_ptr(mock_3pred, ptr1, K0);
	expect_ptr(mock_3pred, ptr2, V0);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, false);

	// get K1
	expect_ptr(mock_3pred, ptr1, K1);
	expect_ptr(mock_3pred, ptr2, V1);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, true);

	const struct PPmapFilter filter = { .key_val_data = mock_3pred, .data = D0, };
	const struct PPmapIt *it = ppmap_filter_it2(map, filter);

	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// skip K2
	expect_ptr(mock_3pred, ptr1, K2);
	expect_ptr(mock_3pred, ptr2, V2);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, false);

	// get K3
	expect_ptr(mock_3pred, ptr1, K3);
	expect_ptr(mock_3pred, ptr2, V3);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, true);

	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K3);
	assert_ptr_equal(it->val, V3);

	// skip K4
	expect_ptr(mock_3pred, ptr1, K4);
	expect_ptr(mock_3pred, ptr2, V4);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, false);

	// done
	it = ppmap_it_next(it);
	assert_nul(it);

	ppmap_free(map);
}

static void ppmap_filter_it2__none(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(ppmap_size(map), 1);

	// skip K0
	expect_ptr(mock_3pred, ptr1, K0);
	expect_ptr(mock_3pred, ptr2, V0);
	expect_ptr(mock_3pred, data, D0);
	will_return(mock_3pred, false);

	const struct PPmapFilter filter = { .key_val_data = mock_3pred, .data = D0, };
	assert_nul(ppmap_filter_it2(map, filter));

	ppmap_free(map);
}

static void ppmap_it_remove__start(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_put(map, K0, V0));
	assert_false(ppmap_put(map, K1, V1));
	assert_false(ppmap_put(map, K2, V2));

	const struct PPmap *expected = ppmap_init();

	assert_false(ppmap_put(expected, K2, V2));

	size_t iterations = 0;
	for (const struct PPmapIt *it = ppmap_it(map); it; it = ppmap_it_next(it)) {
		iterations++;
		if (it->val == V0 || it->val == V1) {
			ppmap_it_remove(it);
		}
	}

	assert_int_equal(ppmap_size(map), 1);
	assert_int_equal(iterations, 3);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);
	ppmap_free(expected);
}

static void ppmap_it_remove__mid(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_put(map, K0, V0));
	assert_false(ppmap_put(map, K1, V1));
	assert_false(ppmap_put(map, K2, V2));
	assert_false(ppmap_put(map, K3, V3));
	assert_false(ppmap_put(map, K4, V4));

	const struct PPmap *expected = ppmap_init();

	assert_false(ppmap_put(expected, K0, V0));
	assert_false(ppmap_put(expected, K2, V2));
	assert_false(ppmap_put(expected, K4, V4));

	size_t iterations = 0;
	for (const struct PPmapIt *it = ppmap_it(map); it; it = ppmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			ppmap_it_remove(it);
		}
	}

	assert_int_equal(ppmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);
	ppmap_free(expected);
}

static void ppmap_it_remove__end(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_put(map, K0, V0));
	assert_false(ppmap_put(map, K1, V1));
	assert_false(ppmap_put(map, K2, V2));

	const struct PPmap *expected = ppmap_init();

	assert_false(ppmap_put(expected, K0, V0));

	size_t iterations = 0;
	for (const struct PPmapIt *it = ppmap_it(map); it; it = ppmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V2) {
			ppmap_it_remove(it);
		}
	}

	assert_int_equal(ppmap_size(map), 1);
	assert_int_equal(iterations, 3);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);
	ppmap_free(expected);
}

static void ppmap_it_remove__all(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_put(map, K0, V0));
	assert_false(ppmap_put(map, K1, V1));
	assert_false(ppmap_put(map, K2, V2));

	size_t iterations = 0;
	for (const struct PPmapIt *it = ppmap_it(map); it; it = ppmap_it_next(it)) {
		iterations++;
		ppmap_it_remove(it);
	}

	assert_int_equal(ppmap_size(map), 0);
	assert_int_equal(iterations, 3);

	ppmap_free(map);
}

static void ppmap_it_remove__partial(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	ppmap_it_remove(it);
}

static void ppmap_it_remove_free__many(void **state) {
	const struct PPmapParams params = { .free_val = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_false(ppmap_put(map, K0, V0));
	assert_false(ppmap_put(map, K1, V1));
	assert_false(ppmap_put(map, K2, V2));
	assert_false(ppmap_put(map, K3, V3));
	assert_false(ppmap_put(map, K4, V4));

	const struct PPmap *expected = ppmap_init();

	assert_false(ppmap_put(expected, K0, V0));
	assert_false(ppmap_put(expected, K2, V2));
	assert_false(ppmap_put(expected, K4, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	size_t iterations = 0;
	for (const struct PPmapIt *it = ppmap_it(map); it; it = ppmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			ppmap_it_remove_free(it);
		}
	}

	assert_int_equal(ppmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);
	ppmap_free(expected);
}

static void ppmap_put__again(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(ppmap_size(map), 2);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);

	// remove zero
	assert_ptr_equal(ppmap_remove(map, K0), V0);

	assert_int_equal(ppmap_size(map), 1);
	assert_nul(ppmap_get(map, K0));

	// put zero again afterwards
	assert_nul(ppmap_put(map, K0, V0));
	assert_int_equal(ppmap_size(map), 2);

	// one
	const struct PPmapIt *it = ppmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	// zero moved later
	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	// end
	it = ppmap_it_next(it);
	assert_nul(it);

	ppmap_free(map);
}

static void ppmap_put_all__many(void **state) {
	const struct PPmapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PPmap *to = ppmap_init_with(params);
	assert_nul(ppmap_put(to, K0, V0));
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V3));
	assert_nul(ppmap_put(from, K2, V4));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K0, V0));
	assert_nul(ppmap_put(expected, K1, V3));
	assert_nul(ppmap_put(expected, K2, V4));

	assert_int_equal(ppmap_put_all(to, from), 1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all__alloc_val(void **state) {
	const struct PPmapParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	const struct PPmap *to = ppmap_init_with(params);
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V3));
	assert_nul(ppmap_put(from, K2, V4));


	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V3));
	assert_nul(ppmap_put(expected, K2, V4));

	expect_ptr(mock_alloc, ptr, V3);
	will_return_ptr_type(mock_alloc, V3, void*);

	expect_ptr(mock_alloc, ptr, V4);
	will_return_ptr_type(mock_alloc, V4, void*);

	assert_int_equal(ppmap_put_all(to, from), 1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all_free__many(void **state) {
	const struct PPmapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PPmap *to = ppmap_init_with(params);
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V3));
	assert_nul(ppmap_put(from, K2, V4));


	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V3));
	assert_nul(ppmap_put(expected, K2, V4));

	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(ppmap_put_all_free(to, from), 1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all_clone__one(void **state) {
	const struct PPmapParams params = {
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	const struct PPmap *to = ppmap_init_with(params);
	assert_nul(ppmap_put(to, K1, V1));


	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V2));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V3));

	expect_ptr(mock_clone, ptr, V2);
	will_return_ptr_type(mock_clone, V3, void*);

	assert_int_equal(ppmap_put_all_clone(to, from), 1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all_clone__no_clone_val(void **state) {
	const struct PPmap *to = ppmap_init();
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V2));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V1));

	assert_int_equal(ppmap_put_all_clone(to, from), 0);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all_clone_free__one(void **state) {
	const struct PPmapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct PPmap *to = ppmap_init_with(params);
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V2));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V3));

	expect_ptr(mock_clone, ptr, V2);
	will_return_ptr_type(mock_clone, V3, void*);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(ppmap_put_all_clone_free(to, from), 1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_all_clone_free__no_clone_val(void **state) {
	const struct PPmap *to = ppmap_init();
	assert_nul(ppmap_put(to, K1, V1));

	const struct PPmap *from = ppmap_init();
	assert_nul(ppmap_put(from, K1, V2));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K1, V1));

	assert_int_equal(ppmap_put_all_clone_free(to, from), 0);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_put_many__many(void **state) {
	const struct PPmap *to = ppmap_init();
	assert_nul(ppmap_put(to, K0, V0));
	assert_nul(ppmap_put(to, K1, strdup("replaced")));
	assert_nul(ppmap_put(to, K2, strdup("replaced")));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K0, V0));
	assert_nul(ppmap_put(expected, K1, V1));
	assert_nul(ppmap_put(expected, K2, V2));

	assert_int_equal(ppmap_put_many(to,
				K1, V1,
				K2, V2,
				NULL),
			2);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(expected);
}

static void ppmap_put_many__no_keyvals(void **state) {
	const struct PPmap *to = ppmap_init();

	assert_int_equal(ppmap_put_many(to, NULL), 0);

	ppmap_free(to);
}

static void ppmap_put_many__null_val_allowed(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *to = ppmap_init_with(params);

	assert_nul(ppmap_put(to, K0, V0));
	assert_nul(ppmap_put(to, K1, strdup("replaced")));
	assert_nul(ppmap_put(to, K2, strdup("replaced")));

	const struct PPmap *expected = ppmap_init_with(params);
	assert_nul(ppmap_put(expected, K0, V0));
	assert_nul(ppmap_put(expected, K1, NULL));
	assert_nul(ppmap_put(expected, K2, V5));

	assert_int_equal(ppmap_put_many(to,
				K1, NULL,
				K2, V5,
				NULL),
			2);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(expected);
}

static void ppmap_put_many__null_val_not_allowed(void **state) {
	const struct PPmap *to = ppmap_init();

	assert_nul(ppmap_put(to, K0, V0));
	assert_nul(ppmap_put(to, K1, strdup("replaced")));

	const struct PPmap *expected = ppmap_init();
	assert_nul(ppmap_put(expected, K0, V0));
	assert_nul(ppmap_put(expected, K1, V1));

	assert_int_equal(ppmap_put_many(to,
				K0, NULL,
				K1, V1,
				NULL),
			1);

	assert_ppmap_equal(to, expected);

	ppmap_free(to);
	ppmap_free(expected);
}

static void ppmap_remove__existing(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	assert_int_equal(ppmap_size(map), 3);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);
	assert_ptr_equal(ppmap_get(map, K2), V2);

	// K1
	assert_ptr_equal(ppmap_remove(map, K1), V1);
	assert_int_equal(ppmap_size(map), 2);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_nul(ppmap_get(map, K1));
	assert_ptr_equal(ppmap_get(map, K2), V2);

	// K2
	assert_ptr_equal(ppmap_remove(map, K2), V2);
	assert_int_equal(ppmap_size(map), 1);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_nul(ppmap_get(map, K1));
	assert_nul(ppmap_get(map, K2));

	// K0
	assert_ptr_equal(ppmap_remove(map, K0), V0);
	assert_int_equal(ppmap_size(map), 0);
	assert_nul(ppmap_get(map, K0));
	assert_nul(ppmap_get(map, K1));
	assert_nul(ppmap_get(map, K2));

	ppmap_free(map);
}

static void ppmap_remove__inexistent(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	assert_int_equal(ppmap_size(map), 3);
	assert_ptr_equal(ppmap_get(map, K0), V0);
	assert_ptr_equal(ppmap_get(map, K1), V1);
	assert_ptr_equal(ppmap_get(map, K2), V2);

	assert_nul(ppmap_remove(map, K3));
	assert_int_equal(ppmap_size(map), 3);

	ppmap_free(map);
}

static void ppmap_remove_free__free(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	const char *val = strdup("val");

	assert_nul(ppmap_put(map, K0, val));
	assert_nul(ppmap_put(map, K1, NULL));

	assert_true(ppmap_remove_free(map, K0));

	assert_true(ppmap_remove_free(map, K1));

	assert_false(ppmap_remove_free(map, K2));

	ppmap_free(map);
}

static void ppmap_remove_free__free_val(void **state) {
	const struct PPmapParams params = {
		.free_val = mock_free,
		.allow_null_val = true,
	};
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));

	assert_false(ppmap_remove_free(map, K1));

	assert_nul(ppmap_put(map, K1, NULL));

	expect_ptr(mock_free, ptr, V0);
	assert_true(ppmap_remove_free(map, K0));

	assert_true(ppmap_remove_free(map, K1));

	ppmap_free(map);
}

static void ppmap_remove_all__many(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_remove_all(map), 0);

	assert_int_equal(ppmap_size(map), 0);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(ppmap_remove_all(map), 2);

	assert_int_equal(ppmap_size(map), 0);

	assert_false(ppmap_contains_key(map, K0));
	assert_false(ppmap_contains_val(map, V0));
	assert_false(ppmap_contains_key(map, K1));
	assert_false(ppmap_contains_val(map, V1));

	ppmap_free(map);
}

static void ppmap_remove_all_free__no_free_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_remove_all_free(map), 0);

	assert_nul(ppmap_put(map, K0, strdup("to be freed")));
	assert_nul(ppmap_put(map, K1, strdup("to be freed")));

	assert_int_equal(ppmap_remove_all_free(map), 2);

	assert_int_equal(ppmap_size(map), 0);

	assert_false(ppmap_contains_key(map, K0));
	assert_false(ppmap_contains_key(map, K1));

	ppmap_free(map);
}

static void ppmap_remove_all_free__free_key_free_val(void **state) {
	const struct PPmapParams params = { .free_val = mock_free, .free_key = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_int_equal(ppmap_remove_all_free(map), 0);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	expect_ptr(mock_free, ptr, K0);
	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, K1);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(ppmap_remove_all_free(map), 2);

	assert_int_equal(ppmap_size(map), 0);

	assert_false(ppmap_contains_key(map, K0));
	assert_false(ppmap_contains_val(map, V0));
	assert_false(ppmap_contains_key(map, K1));
	assert_false(ppmap_contains_val(map, V1));

	ppmap_free(map);
}

static void ppmap_remove_from__free_key(void **state) {
	const struct PPmapParams params = { .free_val = mock_free, .free_key = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	const struct PPmap *from = ppmap_init();

	assert_nul(ppmap_put(from, K0, V0));
	assert_nul(ppmap_put(from, K2, V2));
	assert_nul(ppmap_put(from, K3, V3));

	const struct PPmap *expected = ppmap_init();

	assert_nul(ppmap_put(expected, K1, V1));

	expect_ptr(mock_free, ptr, K0);
	expect_ptr(mock_free, ptr, K2);

	assert_int_equal(ppmap_remove_from(map, from), 2);

	assert_ppmap_equal(map, expected);

	expect_ptr(mock_free, ptr, K1);

	ppmap_free(map);

	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_remove_from_free__free_val(void **state) {
	const struct PPmapParams params = { .free_val = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	const struct PPmap *from = ppmap_init();

	assert_nul(ppmap_put(from, K0, V0));
	assert_nul(ppmap_put(from, K2, V2));
	assert_nul(ppmap_put(from, K3, V3));

	const struct PPmap *expected = ppmap_init();

	assert_nul(ppmap_put(expected, K1, V1));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V2);

	assert_int_equal(ppmap_remove_from_free(map, from), 2);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);

	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_contains_key__pointers(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_contains_key(map, K0));

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_true(ppmap_contains_key(map, K0));
	assert_true(ppmap_contains_key(map, K1));

	assert_false(ppmap_contains_key(map, K2));

	assert_false(ppmap_contains_key(map, NULL));

	ppmap_free(map);
}

static void ppmap_contains_key__equal_key(void **state) {
	const struct PPmapParams params = { .equal_key = equal_ptr, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_false(ppmap_contains_key(map, K0));

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_true(ppmap_contains_key(map, K0));
	assert_true(ppmap_contains_key(map, K1));

	assert_false(ppmap_contains_key(map, K2));

	assert_false(ppmap_contains_key(map, NULL));

	ppmap_free(map);
}

static void ppmap_contains_val__pointers(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_false(ppmap_contains_val(map, K0));

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, NULL));

	assert_true(ppmap_contains_val(map, V0));
	assert_true(ppmap_contains_val(map, V1));

	assert_false(ppmap_contains_val(map, V2));

	assert_true(ppmap_contains_val(map, NULL));

	ppmap_free(map);
}

static void ppmap_contains_val__equal_val(void **state) {
	const struct PPmapParams params = { .equal_val = equal_ptr, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_false(ppmap_contains_val(map, V0));

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_true(ppmap_contains_val(map, V0));
	assert_true(ppmap_contains_val(map, V1));

	assert_false(ppmap_contains_val(map, V2));

	assert_false(ppmap_contains_val(map, NULL));

	ppmap_free(map);
}

static void ppmap_at__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_at(map, 0).val);
	assert_nul(ppmap_at(map, 123).val);

	ppmap_free(map);
}

static void ppmap_at__many(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));
	assert_nul(ppmap_put(map, K2, V2));

	assert_ptr_equal(ppmap_at(map, 0).key, K0);
	assert_ptr_equal(ppmap_at(map, 0).val, V0);

	assert_ptr_equal(ppmap_at(map, 1).key, K1);
	assert_ptr_equal(ppmap_at(map, 1).val, V1);

	assert_ptr_equal(ppmap_at(map, 2).key, K2);
	assert_ptr_equal(ppmap_at(map, 2).val, V2);

	assert_nul(ppmap_at(map, 3).key);
	assert_nul(ppmap_at(map, 3).val);

	ppmap_free(map);
}

static void ppmap_equal__length_different(void **state) {
	const struct PPmap *a = ppmap_init();
	const struct PPmap *b = ppmap_init();

	assert_nul(ppmap_put(a, K0, V0));
	assert_nul(ppmap_put(a, K1, V1));

	assert_nul(ppmap_put(b, K1, V2));

	assert_ppmap_not_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__key_pointers_ok(void **state) {
	const struct PPmap *a = ppmap_init();
	const struct PPmap *b = ppmap_init();

	assert_nul(ppmap_put(a, K0, V0));
	assert_nul(ppmap_put(a, K1, V1));
	assert_nul(ppmap_put(a, K2, V2));

	assert_nul(ppmap_put(b, K0, V0));
	assert_nul(ppmap_put(b, K1, V1));
	assert_nul(ppmap_put(b, K2, V2));

	assert_ppmap_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__key_pointers_different(void **state) {
	const struct PPmap *a = ppmap_init();
	const struct PPmap *b = ppmap_init();

	assert_nul(ppmap_put(a, K0, V0));
	assert_nul(ppmap_put(a, K1, V1));
	assert_nul(ppmap_put(a, K2, V2));

	assert_nul(ppmap_put(b, K0, V0));
	assert_nul(ppmap_put(b, K1, V0));
	assert_nul(ppmap_put(b, K2, V0));

	assert_ppmap_not_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__equal_val_ok(void **state) {
	const struct PPmapParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct PPmap *a = ppmap_init_with(params);
	const struct PPmap *b = ppmap_init_with(params);

	assert_nul(ppmap_put(a, K0, "a"));

	assert_nul(ppmap_put(b, K0, "a"));

	assert_ppmap_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__equal_val_different(void **state) {
	const struct PPmapParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct PPmap *a = ppmap_init_with(params);
	const struct PPmap *b = ppmap_init_with(params);

	assert_nul(ppmap_put(a, K0, "a"));

	assert_nul(ppmap_put(b, K0, "b"));

	assert_ppmap_not_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__equal_key_ok(void **state) {
	const struct PPmapParams params = { .equal_key = (fn_equal)equal_strcasecmp, };
	const struct PPmap *a = ppmap_init_with(params);
	const struct PPmap *b = ppmap_init_with(params);

	assert_nul(ppmap_put(a, "zero", V0));
	assert_nul(ppmap_put(a, "one", V1));
	assert_nul(ppmap_put(a, "two", V2));

	assert_nul(ppmap_put(b, "ZERO", V0));
	assert_nul(ppmap_put(b, "ONE", V1));
	assert_nul(ppmap_put(b, "TWO", V2));

	assert_ppmap_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_equal__equal_key_different(void **state) {
	const struct PPmapParams params = { .equal_key = (fn_equal)equal_strcasecmp, };
	const struct PPmap *a = ppmap_init_with(params);
	const struct PPmap *b = ppmap_init_with(params);

	assert_nul(ppmap_put(a, "zero", V0));
	assert_nul(ppmap_put(a, "one", V1));
	assert_nul(ppmap_put(a, "two", V2));

	assert_nul(ppmap_put(b, "ZERO", V0));
	assert_nul(ppmap_put(b, "ONE", V1));
	assert_nul(ppmap_put(b, "THREE", V2));

	assert_ppmap_not_equal(a, b);

	ppmap_free(a);
	ppmap_free(b);
}

static void ppmap_keys_pslist__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_keys_pslist(map));

	ppmap_free(map);
}

static void ppmap_keys_pslist__many(void **state) {
	const struct PPmap *map = ppmap_init();

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, V1);

	struct Pslist *list = ppmap_keys_pslist(map);

	assert_int_equal(pslist_length(list), 2);
	assert_ptr_equal(pslist_at(list, 0), K0);
	assert_ptr_equal(pslist_at(list, 1), K1);

	pslist_free(&list);
	ppmap_free(map);
}

static void ppmap_keys_pslist__alloc_key(void **state) {
	const struct PPmapParams params = { .alloc_key = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	struct Pslist *list = ppmap_keys_pslist(map);

	assert_int_equal(pslist_length(list), 1);
	assert_ptr_equal(pslist_at(list, 0), K0);

	pslist_free(&list);
	ppmap_free(map);
}

static void ppmap_keys_pset__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct Pset *set = ppmap_keys_pset(map);

	assert_non_nul(set);
	assert_int_equal(pset_size(set), 0);

	ppmap_free(map);
	pset_free(set);
}

static void ppmap_keys_pset__many(void **state) {
	const struct PPmapParams params = {
		.initial = 1,
		.grow = 1,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, V1);
	ppmap_put(map, K2, V1);

	const struct Pset *expected = pset_init();
	pset_add(expected, K0);
	pset_add(expected, K1);
	pset_add(expected, K2);

	const struct Pset *actual = ppmap_keys_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 3);
	assert_int_equal(actual->capacity, 3);

	ppmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ppmap_keys_pset__params(void **state) {
	const struct PPmapParams params = {
		.equal_key = mock_equal,
		.alloc_key = mock_alloc,
		.free_key = mock_free,
		.str_key = mock_str,
		.initial = 99,
		.grow = 1,
	};
	const struct PPmap *map = ppmap_init_with(params);

	const struct Pset *set = ppmap_keys_pset(map);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 99);
	assert_int_equal(set->params.grow, 1);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.alloc_val, mock_alloc);
	assert_ptr_equal(set->params.free_val, mock_free);
	assert_ptr_equal(set->params.clone_val, mock_alloc);
	assert_ptr_equal(set->params.str_val, mock_str);

	ppmap_free(map);

	pset_free(set);
}

static void ppmap_vals_pslist__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_vals_pslist(map));

	ppmap_free(map);
}

static void ppmap_vals_pslist__many(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V1);
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, V3);

	struct Pslist *list = ppmap_vals_pslist(map);

	assert_int_equal(pslist_length(list), 3);
	assert_ptr_equal(pslist_at(list, 0), V1);
	assert_nul(pslist_at(list, 1));
	assert_ptr_equal(pslist_at(list, 2), V3);

	pslist_free(&list);
	ppmap_free(map);
}

static void ppmap_vals_pslist__alloc_val(void **state) {
	const struct PPmapParams params = { .alloc_val = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	struct Pslist *list = ppmap_vals_pslist(map);

	assert_int_equal(pslist_length(list), 1);
	assert_ptr_equal(pslist_at(list, 0), V0);

	pslist_free(&list);
	ppmap_free(map);
}

static void ppmap_vals_pset__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct Pset *set = ppmap_vals_pset(map);

	assert_non_nul(set);
	assert_int_equal(pset_size(set), 0);

	ppmap_free(map);
	pset_free(set);
}

static void ppmap_vals_pset__many(void **state) {
	const struct PPmapParams params = {
		.initial = 1,
		.grow = 1,
		.allow_null_val = true,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, V1);
	ppmap_put(map, K2, V1);
	ppmap_put(map, K3, NULL);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);
	pset_add(expected, V1);

	const struct Pset *actual = ppmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 2);
	assert_int_equal(actual->capacity, 4);

	ppmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ppmap_vals_pset__alloc_val(void **state) {
	const struct PPmapParams params = { .alloc_val = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	ppmap_put(map, K0, V0);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Pset *actual = ppmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 1);

	ppmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ppmap_vals_pset_clone__many(void **state) {
	const struct PPmapParams params = {
		.clone_val = mock_clone,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V0);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Pset *actual = ppmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	assert_int_equal(actual->size, 1);

	ppmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ppmap_vals_pset_clone__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init();

	ppmap_put(map, K0, V0);

	assert_nul(ppmap_vals_pset_clone(map));

	ppmap_free(map);
}

static void ppmap_vals_pset_clone__alloc_val_and_clone_val(void **state) {
	const struct PPmapParams params = {
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
	};
	const struct PPmap *map = ppmap_init_with(params);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Pset *actual = ppmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	ppmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ppmap_vals_pset__params(void **state) {
	const struct PPmapParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.str_val = mock_str,
		.initial = 99,
		.grow = 1,
	};
	const struct PPmap *map = ppmap_init_with(params);

	const struct Pset *set = ppmap_vals_pset(map);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 99);
	assert_int_equal(set->params.grow, 1);
	assert_ptr_equal(set->params.equal_val, mock_equal);
	assert_ptr_equal(set->params.alloc_val, mock_alloc);
	assert_ptr_equal(set->params.free_val, mock_free);
	assert_ptr_equal(set->params.clone_val, mock_clone);
	assert_ptr_equal(set->params.str_val, mock_str);

	ppmap_free(map);

	pset_free(set);
}

static void ppmap_vals_pslist_clone__clone_val(void **state) {
	const struct PPmapParams params = { .clone_val = mock_clone, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));

	assert_nul(ppmap_put(map, K1, NULL));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	struct Pslist *list = ppmap_vals_pslist_clone(map);

	assert_ptr_equal(pslist_at(list, 0), V0);
	assert_ptr_equal(pslist_at(list, 1), NULL);

	pslist_free(&list);
	ppmap_free(map);
}

static void ppmap_vals_pslist_clone__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, NULL));

	assert_nul(ppmap_vals_pslist_clone(map));

	ppmap_free(map);
}

static void ppmap_str__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	char *actual = ppmap_str(map);
	assert_str_equal(actual, "");

	free(actual);
	ppmap_free(map);
}

static void ppmap_str__pointers(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, V2);

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

	char *actual = ppmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ppmap_free(map);
}

static char* str_first(const void *val) {
	return strndup(val, 1);
}

static void ppmap_str__str_val(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.str_val = str_first,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, "AAA");
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, "BBB");

	char *expected = sprintf_alloc(
			"%p = A\n"
			"%p = (null)\n"
			"%p = B\n",
			K0,
			K1,
			K2
			);

	char *actual = ppmap_str(map);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ppmap_free(map);
}

static void ppmap_str__str_key(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.str_key = (fn_str)str_or_null,
	};
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, "zero", V0));
	assert_nul(ppmap_put(map, "one", NULL));
	assert_nul(ppmap_put(map, "two", V2));

	const void **k = map->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"zero = %p\n"
			"one = (null)\n"
			"(null) = %p\n",
			V0,
			V2
			);

	char *actual = ppmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ppmap_free(map);
}

static void ppmap__null_inputs(void **state) {
	const struct PPmap *map = ppmap_init();
	const struct PPmapFilter filter = { 0 };

	assert_nul(ppmap_clone(NULL));
	assert_nul(ppmap_clone_deep(NULL));
	ppmap_free(NULL);
	ppmap_free_vals(NULL);
	ppmap_it_free(NULL);
	assert_false(ppmap_get(NULL, NULL));
	assert_false(ppmap_get(map, NULL));
	assert_false(ppmap_contains_key(NULL, NULL));
	assert_false(ppmap_contains_key(map, NULL));
	assert_false(ppmap_contains_val(NULL, NULL));
	assert_false(ppmap_contains_val(map, NULL));
	assert_nul(ppmap_at(NULL, 0).val);
	ppmap_find2(NULL, filter);
	ppmap_find(NULL, NULL, NULL);
	ppmap_find(map, NULL, NULL);
	ppmap_find_val(NULL, NULL, NULL);
	ppmap_find_val(map, NULL, NULL);
	ppmap_find_key(NULL, NULL, NULL);
	ppmap_find_key(map, NULL, NULL);
	assert_nul(ppmap_it(NULL));
	assert_nul(ppmap_filter_it2(NULL, filter));
	assert_nul(ppmap_filter_it(NULL, NULL, NULL));
	assert_nul(ppmap_filter_it(map, NULL, NULL));
	assert_nul(ppmap_filter_it(NULL, mock_3pred, NULL));
	assert_nul(ppmap_key_filter_it(NULL, NULL, NULL));
	assert_nul(ppmap_key_filter_it(map, NULL, NULL));
	assert_nul(ppmap_key_filter_it(NULL, mock_2pred, NULL));
	assert_nul(ppmap_val_filter_it(NULL, NULL, NULL));
	assert_nul(ppmap_val_filter_it(map, NULL, NULL));
	assert_nul(ppmap_val_filter_it(NULL, mock_2pred, NULL));
	assert_nul(ppmap_it_next(NULL));
	ppmap_it_remove(NULL),
	ppmap_it_remove_free(NULL),
	assert_false(ppmap_put(NULL, NULL, NULL));
	assert_false(ppmap_put(map, NULL, NULL));
	assert_nul(ppmap_put_if_absent(NULL, NULL, NULL));
	assert_nul(ppmap_put_if_absent(map, NULL, NULL));
	assert_false(ppmap_put_free(NULL, NULL, NULL));
	assert_false(ppmap_put_free(map, NULL, NULL));
	assert_int_equal(ppmap_put_many(NULL, NULL), 0);
	assert_int_equal(ppmap_put_many_v(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all(map, NULL), 0);
	assert_int_equal(ppmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_free(map, NULL), 0);
	assert_int_equal(ppmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_clone(map, NULL), 0);
	assert_int_equal(ppmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_clone_free(map, NULL), 0);
	assert_nul(ppmap_remove(NULL, NULL));
	assert_nul(ppmap_remove(map, NULL));
	assert_int_equal(ppmap_remove_all(NULL), 0);
	assert_int_equal(ppmap_remove_all_free(NULL), 0);
	assert_int_equal(ppmap_remove_from(NULL, NULL), 0);
	assert_int_equal(ppmap_remove_from(map, NULL), 0);
	assert_int_equal(ppmap_remove_from(NULL, map), 0);
	assert_int_equal(ppmap_remove_from_free(NULL, NULL), 0);
	assert_int_equal(ppmap_remove_from_free(map, NULL), 0);
	assert_int_equal(ppmap_remove_from_free(NULL, map), 0);
	assert_false(ppmap_equal(NULL, NULL));
	assert_false(ppmap_equal(map, NULL));
	assert_nul(ppmap_keys_pslist(NULL));
	assert_nul(ppmap_keys_pset(NULL));
	assert_nul(ppmap_vals_pslist_clone(NULL));
	assert_nul(ppmap_vals_pslist(NULL));
	assert_nul(ppmap_vals_pset(NULL));
	assert_nul(ppmap_vals_pset_clone(NULL));
	assert_nul(ppmap_str(NULL));
	assert_int_equal(ppmap_size(NULL), 0);

	ppmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ppmap_init__defaults),

		TEST(ppmap_clone__empty),
		TEST(ppmap_clone__params),
		TEST(ppmap_clone__many),
		TEST(ppmap_clone__alloc_key),
		TEST(ppmap_clone__alloc_val),

		TEST(ppmap_clone_deep__clone_val_allow_null_val),
		TEST(ppmap_clone_deep__clone_val_no_allow_null_val),
		TEST(ppmap_clone_deep__no_clone_val),
		TEST(ppmap_clone_deep__alloc_val_and_clone_val),

		TEST(ppmap_free_vals__null_free_val),
		TEST(ppmap_free_vals__free_val),
		TEST(ppmap_free_vals__free_val_hierarchical),

		TEST(ppmap_put__new),
		TEST(ppmap_put__overwrite),
		TEST(ppmap_put__null_key),
		TEST(ppmap_put__allow_null_val),
		TEST(ppmap_put__no_allow_null_val),
		TEST(ppmap_put__grow),
		TEST(ppmap_put__alloc_key_free_key),
		TEST(ppmap_put__alloc_key_returned_null),
		TEST(ppmap_put__equal_key),
		TEST(ppmap_put__alloc_val_allow_null_val),
		TEST(ppmap_put__alloc_val_no_allow_null_val),

		TEST(ppmap_put_free__free),
		TEST(ppmap_put_free__free_val),

		TEST(ppmap_put_if_absent__),

		TEST(ppmap_find2__empty_filter),
		TEST(ppmap_find2__empty_map),

		TEST(ppmap_find2__key),
		TEST(ppmap_find2__key_data),

		TEST(ppmap_find2__val),
		TEST(ppmap_find2__val_data),

		TEST(ppmap_find2__key_val),
		TEST(ppmap_find2__key_val_data),

		TEST(ppmap_find2__all_block),
		TEST(ppmap_find2__some_block),
		TEST(ppmap_find2__none_block),

		TEST(ppmap_it__empty),
		TEST(ppmap_it__free),
		TEST(ppmap_it__many),
		TEST(ppmap_it__removed),

		TEST(ppmap_it_free__partial),

		TEST(ppmap_it_next__partial),

		TEST(ppmap_filter_it2__empty_filter),
		TEST(ppmap_filter_it2__empty_map),
		TEST(ppmap_filter_it2__many),
		TEST(ppmap_filter_it2__none),

		TEST(ppmap_it_remove__start),
		TEST(ppmap_it_remove__mid),
		TEST(ppmap_it_remove__end),
		TEST(ppmap_it_remove__all),
		TEST(ppmap_it_remove__partial),

		TEST(ppmap_it_remove_free__many),

		TEST(ppmap_put__again),

		TEST(ppmap_put_all__many),
		TEST(ppmap_put_all__alloc_val),

		TEST(ppmap_put_all_free__many),

		TEST(ppmap_put_all_clone__one),
		TEST(ppmap_put_all_clone__no_clone_val),

		TEST(ppmap_put_all_clone_free__one),
		TEST(ppmap_put_all_clone_free__no_clone_val),

		TEST(ppmap_put_many__many),
		TEST(ppmap_put_many__no_keyvals),
		TEST(ppmap_put_many__null_val_allowed),
		TEST(ppmap_put_many__null_val_not_allowed),

		TEST(ppmap_remove__existing),
		TEST(ppmap_remove__inexistent),

		TEST(ppmap_remove_free__free),
		TEST(ppmap_remove_free__free_val),

		TEST(ppmap_remove_all__many),

		TEST(ppmap_remove_all_free__no_free_val),
		TEST(ppmap_remove_all_free__free_key_free_val),

		TEST(ppmap_remove_from__free_key),
		TEST(ppmap_remove_from_free__free_val),

		TEST(ppmap_contains_key__pointers),
		TEST(ppmap_contains_key__equal_key),

		TEST(ppmap_contains_val__pointers),
		TEST(ppmap_contains_val__equal_val),

		TEST(ppmap_at__empty),
		TEST(ppmap_at__many),

		TEST(ppmap_equal__length_different),
		TEST(ppmap_equal__key_pointers_ok),
		TEST(ppmap_equal__key_pointers_different),
		TEST(ppmap_equal__equal_val_ok),
		TEST(ppmap_equal__equal_val_different),
		TEST(ppmap_equal__equal_key_ok),
		TEST(ppmap_equal__equal_key_different),

		TEST(ppmap_keys_pslist__empty),
		TEST(ppmap_keys_pslist__many),
		TEST(ppmap_keys_pslist__alloc_key),

		TEST(ppmap_keys_pset__empty),
		TEST(ppmap_keys_pset__many),
		TEST(ppmap_keys_pset__params),

		TEST(ppmap_vals_pslist__empty),
		TEST(ppmap_vals_pslist__many),
		TEST(ppmap_vals_pslist__alloc_val),

		TEST(ppmap_vals_pslist_clone__clone_val),
		TEST(ppmap_vals_pslist_clone__no_clone_val),

		TEST(ppmap_vals_pset__empty),
		TEST(ppmap_vals_pset__many),
		TEST(ppmap_vals_pset__alloc_val),
		TEST(ppmap_vals_pset__params),

		TEST(ppmap_vals_pset_clone__many),
		TEST(ppmap_vals_pset_clone__no_clone_val),
		TEST(ppmap_vals_pset_clone__alloc_val_and_clone_val),

		TEST(ppmap_str__empty),
		TEST(ppmap_str__pointers),
		TEST(ppmap_str__str_val),
		TEST(ppmap_str__str_key),

		TEST(ppmap__null_inputs),
	};

	return RUN(tests);
}

