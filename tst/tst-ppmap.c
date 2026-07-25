#include "assert-ppmap.h"
#include "assert-pset.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <stdio.h>
#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "plist.h"
#include "pset.h"
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

static const char *starts_with_a_or_null(const char* const key) {
	return key && *key == 'a' ? strdup(key) : NULL;
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

static void ppmap_clone__null(void **state) {
	assert_nul(ppmap_clone(NULL));
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
static void ppmap_clone__params__constructor(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){
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
			});

	const struct PPmap *clone = ppmap_clone(map);

	assert_non_nul(clone);

	assert_int_equal(clone->size, 0);
	assert_int_equal(clone->capacity, 99);
	assert_true(clone->params.allow_null_val);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->params.equal_key, mock_equal);
	assert_ptr_equal(clone->params.equal_val, mock_equal);
	assert_ptr_equal(clone->params.alloc_key, mock_alloc);
	assert_ptr_equal(clone->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->params.free_key, mock_free);
	assert_ptr_equal(clone->params.free_val, mock_free);
	assert_ptr_equal(clone->params.clone_val, mock_clone);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone__val_ptr(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmap *clone = ppmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->keys[0], K0);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->keys[1], K1);
	assert_ptr_equal(clone->vals[1], V1);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone__alloc_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_key = mock_alloc, });

	expect_ptr(mock_alloc, ptr, K0); will_return_ptr_type(mock_alloc, K0, void*);
	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, K1); will_return_ptr_type(mock_alloc, K1, void*);
	ppmap_put(map, K1, V1);

	expect_ptr(mock_alloc, ptr, K0); will_return_ptr_type(mock_alloc, K2, void*);
	expect_ptr(mock_alloc, ptr, K1); will_return_ptr_type(mock_alloc, K3, void*);

	const struct PPmap *clone = ppmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->keys[0], K2);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->keys[1], K3);
	assert_ptr_equal(clone->vals[1], V1);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone__alloc_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V1, void*);

	const struct PPmap *clone = ppmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 1);
	assert_ptr_equal(clone->keys[0], K0); assert_ptr_equal(clone->vals[0], V1);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, NULL, K1, V1, NULL);

	const struct PPmap *clone = ppmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->keys[0], K0);
	assert_ptr_equal(clone->vals[0], NULL);
	assert_ptr_equal(clone->keys[1], K1);
	assert_ptr_equal(clone->vals[1], V1);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone_deep__null(void **state) {
	assert_nul(ppmap_clone_deep(NULL));
}

static void ppmap_clone_deep__clone_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V3, void*);

	const struct PPmap *clone = ppmap_clone_deep(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->keys[0], K0);
	assert_ptr_equal(clone->vals[0], V2);
	assert_ptr_equal(clone->keys[1], K1);
	assert_ptr_equal(clone->vals[1], V3);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone_deep__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_nul(ppmap_clone_deep(map));

	ppmap_free(map);
}

static void ppmap_clone_deep__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, .allow_null_val = true });
	ppmap_put_many(map, K0, NULL, K1, V1, K2, V2, NULL);

	expect_ptr(mock_clone, ptr, NULL); will_return_ptr_type(mock_clone, V3, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, NULL, void*);

	const struct PPmap *clone = ppmap_clone_deep(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 3);
	assert_ptr_equal(clone->keys[0], K0);
	assert_ptr_equal(clone->vals[0], V3);
	assert_ptr_equal(clone->keys[1], K1);
	assert_ptr_equal(clone->vals[1], V4);
	assert_ptr_equal(clone->keys[2], K2);
	assert_ptr_equal(clone->vals[2], NULL);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_clone_deep__alloc_val_overrides_clone_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, .clone_val = mock_clone, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);

	const struct PPmap *clone = ppmap_clone_deep(map);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 1);
	assert_ptr_equal(clone->keys[0], K0);
	assert_ptr_equal(clone->vals[0], V0);

	ppmap_free(map);
	ppmap_free(clone);
}

static void ppmap_free__null(void **state) {
	ppmap_free(NULL);
}

static void ppmap_free__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	ppmap_free(map);
}

static void ppmap_free__free_key(void **state) {
	char *key0 = strdup("will not be freed");
	char *key1 = strdup("will be freed");

	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .free_key = mock_free, });
	ppmap_put_many(map, key0, V0, key1, V0, NULL);

	map->keys[0] = NULL;

	expect_ptr(mock_free, ptr, key1);

	ppmap_free(map);

	free(key0);
	free(key1);
}

static void ppmap_free_vals__null(void **state) {
	ppmap_free_vals(NULL);
}

static void ppmap_free_vals__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	ppmap_free_vals(map);
}

static void ppmap_free_vals__missing_val(void **state) {
	char *val = strdup("will not be freed");

	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, val, NULL);

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->vals[0], val);

	map->vals[0] = NULL;

	ppmap_free_vals(map);
	free(val);
}

static void ppmap_free_vals__duplicate_val(void **state) {
	const char *val = strdup("no double free");

	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, val, K1, val, NULL);

	ppmap_free_vals(map);
}

static void ppmap_free_vals__free_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .free_val = mock_free, });
	ppmap_put_many(map, K0, V0, K1, V1, K2, V2, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V2); // only once

	ppmap_free_vals(map);
}

static void ppmap_free_vals__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, .free_val = mock_free, });
	ppmap_put_many(map, K0, V0, K1, NULL, NULL);

	expect_ptr(mock_free, ptr, V0);

	ppmap_free_vals(map);
}

static void ppmap_it_free__null(void **state) {
	ppmap_it_free(NULL);
}

static void ppmap_it_free__incomplete(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	ppmap_it_free(it);
}

static void ppmap_get__null(void **state) {
	assert_nul(ppmap_get(NULL, NULL));
}

static void ppmap_get__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_get(map, K0));

	ppmap_free(map);
}

static void ppmap_get__null_key(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_nul(ppmap_get(map, NULL));

	ppmap_free(map);
}

static void ppmap_get__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_ptr_equal(ppmap_get(map, K1), V1);

	assert_nul(ppmap_get(map, K2));

	ppmap_free(map);
}

static void ppmap_get__equal_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_key = equal_ptr, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_ptr_equal(ppmap_get(map, K1), V1);

	assert_nul(ppmap_get(map, K2));

	ppmap_free(map);
}

static void ppmap_get__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, V0, K1, NULL, NULL);

	assert_nul(ppmap_get(map, K1));

	ppmap_free(map);
}

static void ppmap_contains_key__null(void **state) {
	assert_false(ppmap_contains_key(NULL, K0));
}

static void ppmap_contains_key__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_contains_key(map, K0));

	ppmap_free(map);
}

static void ppmap_contains_key__null_key(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_false(ppmap_contains_key(map, NULL));

	ppmap_free(map);
}

static void ppmap_contains_key__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_true(ppmap_contains_key(map, K1));

	assert_false(ppmap_contains_key(map, K2));

	ppmap_free(map);
}

static void ppmap_contains_key__equal_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_key = equal_ptr, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_true(ppmap_contains_key(map, K1));

	assert_false(ppmap_contains_key(map, K2));

	ppmap_free(map);
}

static void ppmap_contains_val__null(void **state) {
	assert_false(ppmap_contains_val(NULL, V0));
}

static void ppmap_contains_val__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_false(ppmap_contains_val(map, V0));

	ppmap_free(map);
}

static void ppmap_contains_val__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_true(ppmap_contains_val(map, V1));

	assert_false(ppmap_contains_val(map, V2));

	ppmap_free(map);
}

static void ppmap_contains_val__equal_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_val = equal_ptr, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_true(ppmap_contains_val(map, V1));

	assert_false(ppmap_contains_val(map, V2));

	ppmap_free(map);
}

static void ppmap_contains_val__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true });
	ppmap_put_many(map, K0, V0, K1, NULL, K2, V2, NULL);

	assert_true(ppmap_contains_val(map, NULL));

	assert_true(ppmap_contains_val(map, V2));

	ppmap_free(map);
}

static void ppmap_first_key__null(void **state) {
	assert_nul(ppmap_first_key(NULL, V0));
}

static void ppmap_first_key__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_first_key(map, V0));

	ppmap_free(map);
}

static void ppmap_first_key__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_ptr_equal(ppmap_first_key(map, V0), K0);

	assert_nul(ppmap_first_key(map, V1));

	ppmap_free(map);
}

static void ppmap_first_key__equal_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_val = equal_ptr, });
	ppmap_put_many(map, K0, V0, NULL);

	assert_ptr_equal(ppmap_first_key(map, V0), K0);

	assert_nul(ppmap_first_key(map, V1));

	ppmap_free(map);
}

static void ppmap_first_key__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, NULL, K1, V1, NULL);

	assert_ptr_equal(ppmap_first_key(map, NULL), K0);

	ppmap_free(map);
}

static void ppmap_at__null(void **state) {
	assert_nul(ppmap_at(NULL, 0).key);
	assert_nul(ppmap_at(NULL, 0).val);
}

static void ppmap_at__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_at(map, 0).val);
	assert_nul(ppmap_at(map, 123).val);

	ppmap_free(map);
}

static void ppmap_at__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_ptr_equal(ppmap_at(map, 0).key, K0);
	assert_ptr_equal(ppmap_at(map, 0).val, V0);

	assert_ptr_equal(ppmap_at(map, 1).key, K1);
	assert_ptr_equal(ppmap_at(map, 1).val, V1);

	assert_nul(ppmap_at(map, 2).val);
	assert_nul(ppmap_at(map, 2).val);

	ppmap_free(map);
}

static void ppmap_at__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, NULL, K1, V1, NULL);

	assert_ptr_equal(ppmap_at(map, 0).key, K0);
	assert_nul(ppmap_at(map, 0).val);

	assert_ptr_equal(ppmap_at(map, 1).key, K1);
	assert_ptr_equal(ppmap_at(map, 1).val, V1);

	ppmap_free(map);
}

static void ppmap_find__null(void **state) {
	assert_nul(ppmap_find(NULL, (struct PPmapFilter){ 0 }).key);
	assert_nul(ppmap_find(NULL, (struct PPmapFilter){ 0 }).val);
}

static void ppmap_find__map_empty(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key = mock_pred_p, });

	assert_nul(pair.key);
	assert_nul(pair.val);

	ppmap_free(map);
}

static void ppmap_find__filter_empty(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ 0 });

	assert_ptr_equal(pair.key, K0);
	assert_ptr_equal(pair.val, V0);

	ppmap_free(map);
}

static void ppmap_find__key(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p, p, K0); will_return(mock_pred_p, false);

	// get 1
	expect_ptr(mock_pred_p, p, K1); will_return(mock_pred_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key = mock_pred_p, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__key_data(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, K0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, K1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key_data = mock_pred_p_p, .data = D0, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__val(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);

	// get 1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .val = mock_pred_p, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__val_data(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__key_val(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, K0); expect_ptr(mock_pred_p_p, p2, V0); will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, K1); expect_ptr(mock_pred_p_p, p2, V1); will_return(mock_pred_p_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key_val = mock_pred_p_p, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__key_val_data(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p_p, p1, K0); expect_ptr(mock_pred_p_p_p, p2, V0);
	expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p_p, p1, K1); expect_ptr(mock_pred_p_p_p, p2, V1);
	expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key_val_data = mock_pred_p_p_p, .data = D0, });

	assert_ptr_equal(pair.key, K1);
	assert_ptr_equal(pair.val, V1);

	ppmap_free(map);
}

static void ppmap_find__some_block(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, K2, V2, NULL);

	// K0 blocks
	expect_ptr(mock_pred_p, p, K0); will_return(mock_pred_p, false);

	// K1 passes, V1 blocks
	expect_ptr(mock_pred_p, p, K1); will_return(mock_pred_p, true);
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, false);

	// both pass
	expect_ptr(mock_pred_p, p, K2); will_return(mock_pred_p, true);
	expect_ptr(mock_pred_p, p, V2); will_return(mock_pred_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key = mock_pred_p, .val = mock_pred_p, });

	assert_ptr_equal(pair.key, K2);
	assert_ptr_equal(pair.val, V2);

	ppmap_free(map);
}

static void ppmap_find__all_block(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, K2, V2, NULL);

	expect_any_count(mock_pred_p, p, 3); will_return_int_count(mock_pred_p, false, 3);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key = mock_pred_p, });

	assert_nul(pair.key);
	assert_nul(pair.val);

	ppmap_free(map);
}

static void ppmap_find__none_block(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// both pass
	expect_ptr(mock_pred_p, p, K0); will_return(mock_pred_p, true);
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, true);

	const struct PPmapPair pair = ppmap_find(map, (struct PPmapFilter){ .key = mock_pred_p, .val = mock_pred_p, });

	assert_ptr_equal(pair.key, K0);
	assert_ptr_equal(pair.val, V0);

	ppmap_free(map);
}

static void ppmap_it__null(void **state) {
	assert_nul(ppmap_it(NULL));
}

static void ppmap_it__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_it(map));

	ppmap_free(map);
}

static void ppmap_it__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmapIt *it = ppmap_it(map);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	it = ppmap_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	it = ppmap_it_next(it);
	assert_nul(it);

	ppmap_free(map);
}

static void ppmap_it_next__null(void **state) {
	assert_nul(ppmap_it_next(NULL));
}

static void ppmap_it_next__incomplete(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	assert_nul(ppmap_it_next(it));
}

static void ppmap_filter_it__null(void **state) {
	assert_null(ppmap_filter_it(NULL, (struct PPmapFilter) { 0 }));
}

static void ppmap_filter_it__map_empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_null(ppmap_filter_it(NULL, (struct PPmapFilter) { .key = mock_pred_p, }));

	ppmap_free(map);
}

static void ppmap_filter_it__filter_empty(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmapIt *it = ppmap_filter_it(map, (struct PPmapFilter) { 0 });

	assert_non_nul(it);
	assert_ptr_equal(it->key, K0);
	assert_ptr_equal(it->val, V0);

	ppmap_it_free(it);

	ppmap_free(map);
}

static void ppmap_filter_it__match(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p_p, p1, K0); expect_ptr(mock_pred_p_p_p, p2, V0); expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p_p, p1, K1); expect_ptr(mock_pred_p_p_p, p2, V1); expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, true);

	const struct PPmapIt *it = ppmap_filter_it(map, (struct PPmapFilter){ .key_val_data = mock_pred_p_p_p, .data = D0, });

	assert_non_nul(it);
	assert_ptr_equal(it->key, K1);
	assert_ptr_equal(it->val, V1);

	assert_nul(ppmap_it_next(it));

	ppmap_free(map);
}

static void ppmap_filter_it__no_match(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p_p, p1, K0); expect_ptr(mock_pred_p_p_p, p2, V0); expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, false);

	// skip 1
	expect_ptr(mock_pred_p_p_p, p1, K1); expect_ptr(mock_pred_p_p_p, p2, V1); expect_ptr(mock_pred_p_p_p, p3, D0); will_return(mock_pred_p_p_p, false);

	assert_nul(ppmap_filter_it(map, (struct PPmapFilter){ .key_val_data = mock_pred_p_p_p, .data = D0, }));

	ppmap_free(map);
}

static void ppmap_put__null(void **state) {
	assert_null(ppmap_put(NULL, K0, V0));
}

static void ppmap_put__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);

	ppmap_free(map);
}

static void ppmap_put__no_null_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K0, NULL));
	assert_nul(ppmap_put(map, K1, NULL));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(map);
}

static void ppmap_put__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, NULL));
	assert_ptr_equal(ppmap_put(map, K0, NULL), V0);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], NULL);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], NULL);

	ppmap_free(map);
}

static void ppmap_put__overwrite(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_ptr_equal(ppmap_put(map, K0, V2), V0);

	assert_ptr_equal(ppmap_put(map, K1, V3), V1);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V2);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V3);

	ppmap_free(map);
}

static void ppmap_put__null_key(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_nul(ppmap_put(map, NULL, V0));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(map);
}

static void ppmap_put__alloc_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_key = alloc_key_duplicate, .free_key = free, });

	assert_nul(ppmap_put(map, "zero", V0));
	assert_nul(ppmap_put(map, "one", V1));

	assert_int_equal(map->size, 2);
	assert_str_equal(map->keys[0], "zerozero");
	assert_ptr_equal(map->vals[0], V0);
	assert_str_equal(map->keys[1], "oneone");
	assert_ptr_equal(map->vals[1], V1);

	ppmap_free(map);
}

static void ppmap_put__alloc_key_returned_null(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_key = mock_alloc, });

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(map->size, 0);

	ppmap_free(map);
}

static void ppmap_put__equal_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_key = equal_ptr, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	assert_ptr_equal(ppmap_put(map, K0, V2), V0);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V2);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);

	ppmap_free(map);
}

static void ppmap_put__alloc_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V2, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V2);

	ppmap_free(map);
}

static void ppmap_put__alloc_val_returned_null(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(map->size, 0);

	expect_ptr(mock_alloc, ptr, V1); will_return_ptr_type(mock_alloc, V1, void*);

	assert_nul(ppmap_put(map, K1, V1));

	expect_ptr(mock_alloc, ptr, V2); will_return_ptr_type(mock_alloc, NULL, void*);

	assert_ptr_equal(ppmap_put(map, K1, V2), V1);

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K1);
	assert_ptr_equal(map->vals[0], V1);

	ppmap_free(map);
}

static void ppmap_put__alloc_val_allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, .allow_null_val = true, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(ppmap_put(map, K0, V0));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], NULL);

	expect_ptr(mock_alloc, ptr, V1); will_return_ptr_type(mock_alloc, V1, void*);

	assert_nul(ppmap_put(map, K1, V1));

	expect_ptr(mock_alloc, ptr, V2); will_return_ptr_type(mock_alloc, NULL, void*);

	assert_ptr_equal(ppmap_put(map, K1, V2), V1);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], NULL);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], NULL);

	ppmap_free(map);
}

static void ppmap_put__grow(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .initial = 3, .grow = 5, });

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

	assert_int_equal(map->size, 6);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V2);
	assert_ptr_equal(map->keys[3], K3);
	assert_ptr_equal(map->vals[3], V3);
	assert_ptr_equal(map->keys[4], K4);
	assert_ptr_equal(map->vals[4], V4);
	assert_ptr_equal(map->keys[5], K5);
	assert_ptr_equal(map->vals[5], V5);

	ppmap_free(map);
}

static void ppmap_put_if_absent__null(void **state) {
	assert_nul(ppmap_put_if_absent(NULL, K0, V0));
}

static void ppmap_put_if_absent__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put_if_absent(map, K0, V0));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(map);
}

static void ppmap_put_if_absent__missing(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_nul(ppmap_put_if_absent(map, K1, V1));

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);

	ppmap_free(map);
}

static void ppmap_put_if_absent__present(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	assert_ptr_equal(ppmap_put_if_absent(map, K0, V2), V0);

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(map);
}

static void ppmap_put_if_absent__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });

	assert_nul(ppmap_put_if_absent(map, K0, NULL));

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], NULL);

	ppmap_free(map);
}

static void ppmap_put_if_absent__equal_key(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .equal_key = mock_equal, });
	ppmap_put_many(map, K0, V0, NULL);

	expect_ptr(mock_equal, a, K0); expect_ptr(mock_equal, b, K0); will_return(mock_equal, true);
	assert_ptr_equal(ppmap_put_if_absent(map, K0, V5), V0);

	expect_ptr(mock_equal, a, K0); expect_ptr(mock_equal, b, K1); will_return(mock_equal, false);
	assert_nul(ppmap_put_if_absent(map, K1, V1));

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);

	ppmap_free(map);
}

static void ppmap_put_free__null(void **state) {
	assert_null(ppmap_put_free(NULL, K0, V0));
}

static void ppmap_put_free__free(void **state) {
	const struct PPmap *map = ppmap_init();

	const char *val0 = strdup("val0");
	const char *val1 = strdup("val1");

	assert_nul(ppmap_put(map, K0, val0));

	assert_true(ppmap_put_free(map, K0, val1));

	assert_true(ppmap_put_free(map, K0, V2));

	ppmap_free(map);
}

static void ppmap_put_free__free_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .free_val = mock_free, });

	assert_nul(ppmap_put(map, K0, V0));

	assert_false(ppmap_put_free(map, K1, V1));

	expect_ptr(mock_free, ptr, V0);
	assert_true(ppmap_put_free(map, K0, V0));

	assert_false(ppmap_put_free(map, K2, V2));

	ppmap_free(map);
}

static void ppmap_put_free__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .free_val = mock_free, .allow_null_val = true, });

	assert_nul(ppmap_put(map, K0, NULL));

	// no free
	assert_true(ppmap_put_free(map, K0, V0));

	ppmap_free(map);
}

static void ppmap_put_all__null(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_put_all(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all(NULL, map), 0);
	assert_int_equal(ppmap_put_all(map, NULL), 0);

	ppmap_free(map);
}

static void ppmap_put_all__no_null_vals(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K0, NULL, K1, V4, K2, V5, K3, NULL, NULL);

	assert_int_equal(ppmap_put_all(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V4);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V5);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, V0, K1, NULL, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K1, V3, K2, NULL, NULL);

	assert_int_equal(ppmap_put_all(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V3);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], NULL);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all__alloc_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_nul(ppmap_put(map, K0, V0));

	const struct PPmap *from = ppmap_init();
	ppmap_put_many(from, K0, V4, K1, V5, NULL);

	expect_ptr(mock_alloc, ptr, V4);
	will_return_ptr_type(mock_alloc, V4, void*);

	expect_ptr(mock_alloc, ptr, V5);
	will_return_ptr_type(mock_alloc, V5, void*);

	assert_int_equal(ppmap_put_all(map, from), 1);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V4);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V5);

	ppmap_free(map);
	ppmap_free(from);
}

static void ppmap_put_all_free__null(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_free(NULL, map), 0);
	assert_int_equal(ppmap_put_all_free(map, NULL), 0);

	ppmap_free(map);
}

static void ppmap_put_all_free__no_null_vals(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, K1, strdup("to be freed"), NULL);

	const struct PPmap *from = ppmap_init();
	ppmap_put_many(from, K1, V3, K2, V4, NULL);

	assert_int_equal(ppmap_put_all_free(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V3);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V4);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_free__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(map, K0, V0, K1, NULL, K2, strdup("to be freed"), NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K1, V3, K2, NULL, NULL);

	assert_int_equal(ppmap_put_all_free(map, from), 2);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V3);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], NULL);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_free__alloc_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, strdup("to be freed"), void*);

	assert_nul(ppmap_put(map, K0, V0));

	const struct PPmap *from = ppmap_init();
	ppmap_put_many(from, K0, V4, K1, V5, NULL);

	expect_ptr(mock_alloc, ptr, V4);
	will_return_ptr_type(mock_alloc, V4, void*);

	expect_ptr(mock_alloc, ptr, V5);
	will_return_ptr_type(mock_alloc, V5, void*);

	assert_int_equal(ppmap_put_all_free(map, from), 1);

	assert_int_equal(map->size, 2);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V4);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V5);

	ppmap_free(map);
	ppmap_free(from);
}

static void ppmap_put_all_clone__null(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_clone(NULL, map), 0);
	assert_int_equal(ppmap_put_all_clone(map, NULL), 0);

	ppmap_free(map);
}

static void ppmap_put_all_clone__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init();
	ppmap_put_many(map, K0, V0, NULL);

	const struct PPmap *from = ppmap_init();
	ppmap_put_many(from, K1, V1, NULL);

	assert_int_equal(ppmap_put_all_clone(map, from), 0);

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_clone__no_null_vals(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K0, NULL, K1, V1, K2, V2, NULL);

	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ppmap_put_all_clone(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V4);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V5);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_clone__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, .allow_null_val = true, });
	ppmap_put_many(map, K0, V0, K1, NULL, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K1, V2, K2, NULL, NULL);

	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, NULL); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ppmap_put_all_clone(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], NULL);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V5);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_clone_free__null(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(ppmap_put_all_clone_free(NULL, map), 0);
	assert_int_equal(ppmap_put_all_clone_free(map, NULL), 0);

	ppmap_free(map);
}

static void ppmap_put_all_clone_free__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .free_val = mock_free, });
	ppmap_put_many(map, K0, V0, NULL);

	const struct PPmap *from = ppmap_init();
	ppmap_put_many(from, K1, V1, NULL);

	assert_int_equal(ppmap_put_all_clone_free(map, from), 0);

	assert_int_equal(map->size, 1);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_clone_free__no_null_vals(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, .free_val = mock_free, });
	ppmap_put_many(map, K0, V0, K1, V1, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K0, NULL, K1, V1, K2, V2, NULL);

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ppmap_put_all_clone_free(map, from), 1);

	assert_int_equal(map->size, 3);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], V1);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], V5);

	ppmap_free(from);
	ppmap_free(map);
}

static void ppmap_put_all_clone_free__allow_null_val(void **state) {
	const struct PPmap *map = ppmap_init_with((struct PPmapParams){ .clone_val = mock_clone, .free_val = mock_free, .allow_null_val = true, });
	ppmap_put_many(map, K0, V0, K1, NULL, K2, V2, NULL);

	const struct PPmap *from = ppmap_init_with((struct PPmapParams){ .allow_null_val = true, });
	ppmap_put_many(from, K1, V1, K2, NULL, K3, V3, NULL);

	expect_ptr(mock_free, ptr, V2);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, NULL); will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, V3); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ppmap_put_all_clone_free(map, from), 2);

	assert_int_equal(map->size, 4);
	assert_ptr_equal(map->keys[0], K0);
	assert_ptr_equal(map->vals[0], V0);
	assert_ptr_equal(map->keys[1], K1);
	assert_ptr_equal(map->vals[1], NULL);
	assert_ptr_equal(map->keys[2], K2);
	assert_ptr_equal(map->vals[2], NULL);
	assert_ptr_equal(map->keys[3], K3);
	assert_ptr_equal(map->vals[3], V5);

	ppmap_free(from);
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
			assert_non_nul(ppmap_it_remove(it));
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
			assert_non_nul(ppmap_it_remove(it));
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
			assert_non_nul(ppmap_it_remove(it));
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
		assert_non_nul(ppmap_it_remove(it));
	}

	assert_int_equal(ppmap_size(map), 0);
	assert_int_equal(iterations, 3);

	ppmap_free(map);
}

static void ppmap_it_remove__partial(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	assert_nul(ppmap_it_remove(it));
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
			assert_true(ppmap_it_remove_free(it));
		}
	}

	assert_int_equal(ppmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);
	ppmap_free(expected);
}

static void ppmap_it_remove_free__partial(void **state) {
	const struct PPmapIt *it = calloc(1, sizeof(struct PPmapIt));

	assert_false(ppmap_it_remove_free(it));
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

static void ppmap_remove_all__free_key(void **state) {
	const struct PPmapParams params = { .free_key = mock_free, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, V1));

	expect_ptr(mock_free, ptr, K0);
	expect_ptr(mock_free, ptr, K1);

	assert_int_equal(ppmap_remove_all(map), 2);

	ppmap_free(map);
}

static void ppmap_remove_all_free__no_free_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_int_equal(ppmap_remove_all_free(map), 0);

	const char *val = strdup("to be freed only once");

	assert_nul(ppmap_put(map, K0, val));
	assert_nul(ppmap_put(map, K1, val));

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
	assert_nul(ppmap_put(map, K2, V0));

	expect_ptr(mock_free, ptr, K0);
	expect_ptr(mock_free, ptr, K1);
	expect_ptr(mock_free, ptr, K2);
	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(ppmap_remove_all_free(map), 3);

	assert_int_equal(ppmap_size(map), 0);

	assert_false(ppmap_contains_key(map, K0));
	assert_false(ppmap_contains_val(map, V0));
	assert_false(ppmap_contains_key(map, K1));
	assert_false(ppmap_contains_val(map, V1));

	ppmap_free(map);
}

static void ppmap_remove_in__free_key(void **state) {
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

	assert_int_equal(ppmap_remove_in(map, from), 2);

	assert_ppmap_equal(map, expected);

	expect_ptr(mock_free, ptr, K1);

	ppmap_free(map);

	ppmap_free(from);
	ppmap_free(expected);
}

static void ppmap_remove_in_free__free_val(void **state) {
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

	assert_int_equal(ppmap_remove_in_free(map, from), 2);

	assert_ppmap_equal(map, expected);

	ppmap_free(map);

	ppmap_free(from);
	ppmap_free(expected);
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

static void ppmap_keys_plist__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct Plist *list = ppmap_keys_plist(map);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 0);

	ppmap_free(map);
	plist_free(list);
}

static void ppmap_keys_plist__many(void **state) {
	const struct PPmap *map = ppmap_init();

	ppmap_put(map, K0, V0);
	ppmap_put(map, K1, V1);

	const struct Plist *list = ppmap_keys_plist(map);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), K0);
	assert_ptr_equal(plist_at(list, 1), K1);

	ppmap_free(map);
	plist_free(list);
}

static void ppmap_keys_plist__alloc_key(void **state) {
	const struct PPmapParams params = { .alloc_key = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, K0);
	will_return_ptr_type(mock_alloc, K0, void*);

	const struct Plist *list = ppmap_keys_plist(map);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), K0);

	plist_free(list);
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

static void ppmap_vals_plist__empty(void **state) {
	const struct PPmap *map = ppmap_init();

	const struct Plist *list = ppmap_vals_plist(map);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 0);

	ppmap_free(map);
	plist_free(list);
}

static void ppmap_vals_plist__many(void **state) {
	const struct PPmapParams params = { .allow_null_val = true, };
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, V1);
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, V3);

	const struct Plist *list = ppmap_vals_plist(map);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V1);
	assert_nul(plist_at(list, 1));
	assert_ptr_equal(plist_at(list, 2), V3);

	plist_free(list);
	ppmap_free(map);
}

static void ppmap_vals_plist__alloc_val(void **state) {
	const struct PPmapParams params = { .alloc_val = mock_alloc, };
	const struct PPmap *map = ppmap_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	ppmap_put(map, K0, V0);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *list = ppmap_vals_plist(map);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
	ppmap_free(map);
}

static void ppmap_vals_plist_clone__clone_val(void **state) {
	const struct PPmapParams params = { .clone_val = mock_clone, };
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, K0, V0));

	assert_nul(ppmap_put(map, K1, NULL));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Plist *list = ppmap_vals_plist_clone(map);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);

	plist_free(list);
	ppmap_free(map);
}

static void ppmap_vals_plist_clone__no_clone_val(void **state) {
	const struct PPmap *map = ppmap_init();

	assert_nul(ppmap_put(map, K0, V0));
	assert_nul(ppmap_put(map, K1, NULL));

	assert_nul(ppmap_vals_plist_clone(map));

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

static void ppmap_str__str_val(void **state) {
	const struct PPmapParams params = {
		.allow_null_val = true,
		.str_val = (fn_str)starts_with_a_or_null,
	};
	const struct PPmap *map = ppmap_init_with(params);

	ppmap_put(map, K0, "a0");
	ppmap_put(map, K1, NULL);
	ppmap_put(map, K2, "b2");

	char *expected = sprintf_alloc(
			"%p = a0\n"
			"%p = (null)\n"
			"%p = (null)\n",
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
		.str_key = (fn_str)starts_with_a_or_null,
	};
	const struct PPmap *map = ppmap_init_with(params);

	assert_nul(ppmap_put(map, "a0", V0));
	assert_nul(ppmap_put(map, "a1", NULL));
	assert_nul(ppmap_put(map, "b2", V2));

	const void **k = map->keys;
	k[2] = NULL;

	char *expected = sprintf_alloc(
			"a0 = %p\n"
			"a1 = (null)\n"
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
	assert_nul(ppmap_first_key(NULL, NULL));
	assert_nul(ppmap_at(NULL, 0).val);
	ppmap_find(NULL, filter);
	assert_nul(ppmap_it(NULL));
	assert_nul(ppmap_filter_it(NULL, filter));
	assert_nul(ppmap_it_next(NULL));
	assert_nul(ppmap_it_remove(NULL)),
		assert_false(ppmap_it_remove_free(NULL)),
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
	assert_false(ppmap_remove_free(NULL, NULL));
	assert_false(ppmap_remove_free(map, NULL));
	assert_int_equal(ppmap_remove_all(NULL), 0);
	assert_int_equal(ppmap_remove_all_free(NULL), 0);
	assert_int_equal(ppmap_remove_in(NULL, NULL), 0);
	assert_int_equal(ppmap_remove_in(map, NULL), 0);
	assert_int_equal(ppmap_remove_in(NULL, map), 0);
	assert_int_equal(ppmap_remove_in_free(NULL, NULL), 0);
	assert_int_equal(ppmap_remove_in_free(map, NULL), 0);
	assert_int_equal(ppmap_remove_in_free(NULL, map), 0);
	assert_false(ppmap_equal(NULL, NULL));
	assert_false(ppmap_equal(map, NULL));
	assert_nul(ppmap_keys_plist(NULL));
	assert_nul(ppmap_keys_pset(NULL));
	assert_nul(ppmap_vals_plist_clone(NULL));
	assert_nul(ppmap_vals_plist(NULL));
	assert_nul(ppmap_str(NULL));
	assert_int_equal(ppmap_size(NULL), 0);

	ppmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ppmap_init__defaults),

		TEST(ppmap_clone__null),
		TEST(ppmap_clone__empty),
		TEST(ppmap_clone__params__constructor),
		TEST(ppmap_clone__val_ptr),
		TEST(ppmap_clone__alloc_key),
		TEST(ppmap_clone__alloc_val),
		TEST(ppmap_clone__allow_null_val),

		TEST(ppmap_clone_deep__null),
		TEST(ppmap_clone_deep__clone_val),
		TEST(ppmap_clone_deep__no_clone_val),
		TEST(ppmap_clone_deep__allow_null_val),
		TEST(ppmap_clone_deep__alloc_val_overrides_clone_val),

		TEST(ppmap_free__null),
		TEST(ppmap_free__empty),
		TEST(ppmap_free__free_key),

		TEST(ppmap_free_vals__null),
		TEST(ppmap_free_vals__empty),
		TEST(ppmap_free_vals__missing_val),
		TEST(ppmap_free_vals__duplicate_val),
		TEST(ppmap_free_vals__free_val),
		TEST(ppmap_free_vals__allow_null_val),

		TEST(ppmap_it_free__null),
		TEST(ppmap_it_free__incomplete),

		TEST(ppmap_get__null),
		TEST(ppmap_get__empty),
		TEST(ppmap_get__null_key),
		TEST(ppmap_get__present),
		TEST(ppmap_get__equal_key),
		TEST(ppmap_get__allow_null_val),

		TEST(ppmap_contains_key__null),
		TEST(ppmap_contains_key__empty),
		TEST(ppmap_contains_key__null_key),
		TEST(ppmap_contains_key__present),
		TEST(ppmap_contains_key__equal_key),

		TEST(ppmap_contains_val__null),
		TEST(ppmap_contains_val__empty),
		TEST(ppmap_contains_val__present),
		TEST(ppmap_contains_val__equal_val),
		TEST(ppmap_contains_val__allow_null_val),

		TEST(ppmap_first_key__null),
		TEST(ppmap_first_key__empty),
		TEST(ppmap_first_key__present),
		TEST(ppmap_first_key__equal_val),
		TEST(ppmap_first_key__allow_null_val),

		TEST(ppmap_at__null),
		TEST(ppmap_at__empty),
		TEST(ppmap_at__present),
		TEST(ppmap_at__allow_null_val),

		TEST(ppmap_find__null),
		TEST(ppmap_find__filter_empty),
		TEST(ppmap_find__map_empty),

		TEST(ppmap_find__key),
		TEST(ppmap_find__key_data),

		TEST(ppmap_find__val),
		TEST(ppmap_find__val_data),

		TEST(ppmap_find__key_val),
		TEST(ppmap_find__key_val_data),

		TEST(ppmap_find__all_block),
		TEST(ppmap_find__some_block),
		TEST(ppmap_find__none_block),

		TEST(ppmap_it__null),
		TEST(ppmap_it__empty),
		TEST(ppmap_it__present),

		TEST(ppmap_it_next__null),
		TEST(ppmap_it_next__incomplete),

		TEST(ppmap_filter_it__null),
		TEST(ppmap_filter_it__filter_empty),
		TEST(ppmap_filter_it__map_empty),
		TEST(ppmap_filter_it__match),
		TEST(ppmap_filter_it__no_match),

		TEST(ppmap_put__null),
		TEST(ppmap_put__empty),
		TEST(ppmap_put__no_null_val),
		TEST(ppmap_put__allow_null_val),
		TEST(ppmap_put__overwrite),
		TEST(ppmap_put__null_key),
		TEST(ppmap_put__alloc_key),
		TEST(ppmap_put__alloc_key_returned_null),
		TEST(ppmap_put__equal_key),
		TEST(ppmap_put__alloc_val),
		TEST(ppmap_put__alloc_val_returned_null),
		TEST(ppmap_put__alloc_val_allow_null_val),
		TEST(ppmap_put__grow),

		TEST(ppmap_put_if_absent__null),
		TEST(ppmap_put_if_absent__empty),
		TEST(ppmap_put_if_absent__missing),
		TEST(ppmap_put_if_absent__present),
		TEST(ppmap_put_if_absent__allow_null_val),
		TEST(ppmap_put_if_absent__equal_key),

		TEST(ppmap_put_free__null),
		TEST(ppmap_put_free__free),
		TEST(ppmap_put_free__free_val),
		TEST(ppmap_put_free__allow_null_val),

		TEST(ppmap_put_all__null),
		TEST(ppmap_put_all__no_null_vals),
		TEST(ppmap_put_all__allow_null_val),
		TEST(ppmap_put_all__alloc_val),

		TEST(ppmap_put_all_free__null),
		TEST(ppmap_put_all_free__no_null_vals),
		TEST(ppmap_put_all_free__allow_null_val),
		TEST(ppmap_put_all_free__alloc_val),

		TEST(ppmap_put_all_clone__null),
		TEST(ppmap_put_all_clone__no_null_vals),
		TEST(ppmap_put_all_clone__no_clone_val),
		TEST(ppmap_put_all_clone__allow_null_val),

		TEST(ppmap_put_all_clone_free__null),
		TEST(ppmap_put_all_clone_free__no_clone_val),
		TEST(ppmap_put_all_clone_free__no_null_vals),
		TEST(ppmap_put_all_clone_free__allow_null_val),





		TEST(ppmap_it_remove__start),
		TEST(ppmap_it_remove__mid),
		TEST(ppmap_it_remove__end),
		TEST(ppmap_it_remove__all),
		TEST(ppmap_it_remove__partial),

		TEST(ppmap_it_remove_free__many),
		TEST(ppmap_it_remove_free__partial),

		TEST(ppmap_put__again),

		TEST(ppmap_put_many__many),
		TEST(ppmap_put_many__no_keyvals),
		TEST(ppmap_put_many__null_val_allowed),
		TEST(ppmap_put_many__null_val_not_allowed),

		TEST(ppmap_remove__existing),
		TEST(ppmap_remove__inexistent),

		TEST(ppmap_remove_free__free),
		TEST(ppmap_remove_free__free_val),

		TEST(ppmap_remove_all__many),
		TEST(ppmap_remove_all__free_key),

		TEST(ppmap_remove_all_free__no_free_val),
		TEST(ppmap_remove_all_free__free_key_free_val),

		TEST(ppmap_remove_in__free_key),
		TEST(ppmap_remove_in_free__free_val),

		TEST(ppmap_equal__length_different),
		TEST(ppmap_equal__key_pointers_ok),
		TEST(ppmap_equal__key_pointers_different),
		TEST(ppmap_equal__equal_val_ok),
		TEST(ppmap_equal__equal_val_different),
		TEST(ppmap_equal__equal_key_ok),
		TEST(ppmap_equal__equal_key_different),

		TEST(ppmap_keys_plist__empty),
		TEST(ppmap_keys_plist__many),
		TEST(ppmap_keys_plist__alloc_key),

		TEST(ppmap_keys_pset__empty),
		TEST(ppmap_keys_pset__many),
		TEST(ppmap_keys_pset__params),

		TEST(ppmap_vals_plist__empty),
		TEST(ppmap_vals_plist__many),
		TEST(ppmap_vals_plist__alloc_val),

		TEST(ppmap_vals_plist_clone__clone_val),
		TEST(ppmap_vals_plist_clone__no_clone_val),

		TEST(ppmap_str__empty),
		TEST(ppmap_str__pointers),
		TEST(ppmap_str__str_val),
		TEST(ppmap_str__str_key),

		TEST(ppmap__null_inputs),
	};

	return RUN(tests);
}

