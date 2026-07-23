#include "assert-ipmap.h"
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
#include "plist.h"
#include "ppmap.h"
#include "str.h"

#include "ipmap.h"

struct PPmap {
	const struct PPmapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct IPmap {
	const struct IPmapParams params;
	const struct PPmap *ppmap;
};

static void ipmap_get__key_removed(void **state) {

	const struct IPmap *actual = ipmap_init();
	assert_nul(ipmap_put(actual, 0, V0));

	int *removed_key = (int*)actual->ppmap->keys[0];
	actual->ppmap->keys[0] = NULL;

	assert_nul(ipmap_get(actual, 0));

	free(removed_key);
	ipmap_free(actual);
}

static void ipmap_clone__many(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *from = ipmap_init_with(params);

	assert_nul(ipmap_put(from, 0, V0));
	assert_nul(ipmap_put(from, 1, NULL));
	assert_nul(ipmap_put(from, 2, V2));

	const struct IPmap *to = ipmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(ipmap_size(to), 3);

	assert_ipmap_equal(from, to);

	ipmap_free(from);
	ipmap_free(to);
}

// also tests constructor
static void ipmap_clone__params(void **state) {
	const struct IPmapParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct IPmap *from = ipmap_init_with(params);

	const struct IPmap *to = ipmap_clone(from);

	assert_non_nul(to);

	assert_int_equal(to->ppmap->size, 0);
	assert_int_equal(to->ppmap->capacity, 99);
	assert_true(to->ppmap->params.allow_null_val);
	assert_int_equal(to->ppmap->params.grow, 1);
	assert_ptr_equal(to->ppmap->params.equal_val, mock_equal);
	assert_ptr_equal(to->ppmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(to->ppmap->params.free_key, free);
	assert_ptr_equal(to->ppmap->params.free_val, mock_free);
	assert_ptr_equal(to->ppmap->params.clone_val, mock_clone);

	assert_ptr_equal(to->params.equal_val, mock_equal);
	assert_ptr_equal(to->params.free_val, mock_free);
	assert_ptr_equal(to->params.clone_val, mock_clone);
	assert_ptr_equal(to->params.initial, 99);
	assert_ptr_equal(to->params.grow, 1);

	ipmap_free(from);
	ipmap_free(to);
}

static void ipmap_clone_deep__clone_val(void **state) {
	const struct IPmapParams params = { .clone_val = mock_clone, };
	const struct IPmap *from = ipmap_init_with(params);

	assert_nul(ipmap_put(from, 0, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct IPmap *to = ipmap_clone_deep(from);

	assert_non_nul(to);

	assert_int_equal(ipmap_size(to), 1);

	assert_ipmap_equal(from, to);

	ipmap_free(from);
	ipmap_free(to);
}

static void ipmap_clone_deep__no_clone_val(void **state) {
	const struct IPmap *from = ipmap_init();

	assert_nul(ipmap_put(from, 0, V0));

	const struct IPmap *to = ipmap_clone_deep(from);
	assert_non_nul(to);
	assert_int_equal(ipmap_size(to), 0);

	ipmap_free(from);
	ipmap_free(to);
}

static void ipmap_free_vals__(void **state) {
	const struct IPmap *map = ipmap_init();
	assert_nul(ipmap_put(map, 0, strdup("zero")));

	ipmap_free_vals(map);
}

static void ipmap_it_free__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	ipmap_it_free(it);
}

static void ipmap_contains_key__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_contains_key(map, 0));

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	assert_true(ipmap_contains_key(map, 0));
	assert_true(ipmap_contains_key(map, 1));

	assert_false(ipmap_contains_key(map, 2));

	ipmap_free(map);
}

static void ipmap_contains_val__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_contains_val(map, 0));

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	assert_true(ipmap_contains_val(map, V0));
	assert_true(ipmap_contains_val(map, V1));

	assert_false(ipmap_contains_val(map, V2));

	ipmap_free(map);
}

static void ipmap_first_key__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_put(map, 0, V0));
	assert_false(ipmap_put(map, 1, V1));
	assert_false(ipmap_put(map, 2, V1));

	assert_false(ipmap_first_key(NULL, map, V0));

	size_t key;

	assert_true(ipmap_first_key(&key, map, V0));
	assert_int_equal(key, 0);

	assert_true(ipmap_first_key(&key, map, V1));
	assert_int_equal(key, 1);

	assert_false(ipmap_first_key(&key, map, V2));
	assert_int_equal(key, 0);

	ipmap_free(map);
}

static void ipmap_at__(void **state) {
	const struct IPmap *map = ipmap_init();
	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	assert_int_equal(ipmap_at(map, 1).key, 1);
	assert_ptr_equal(ipmap_at(map, 1).val, V1);

	int *removed_key = (int*)map->ppmap->keys[1];
	map->ppmap->keys[1] = NULL;
	free(removed_key);

	assert_int_equal(ipmap_at(map, 1).key, 0);
	assert_ptr_equal(ipmap_at(map, 1).val, V1);

	ipmap_free(map);
}

static void ipmap_find__variants(void **state) {
	const struct IPmap *map = ipmap_init();
	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// key
	expect_int_value(mock_pred_i, i, 0);
	will_return(mock_pred_i, false);
	expect_int_value(mock_pred_i, i, 1);
	will_return(mock_pred_i, true);

	const struct IPmapFilter filter_k = { .key = mock_pred_i, };
	const struct IPmapPair pair_k = ipmap_find(map, filter_k);
	assert_int_equal(pair_k.key, 1);
	assert_ptr_equal(pair_k.val, V1);

	// key_data
	expect_int_value(mock_pred_i_p, i, 0);
	expect_string(mock_pred_i_p, p, "x");
	will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 1);
	expect_string(mock_pred_i_p, p, "x");
	will_return(mock_pred_i_p, true);

	const struct IPmapFilter filter_kd = { .key_data = mock_pred_i_p, .data = "x", };
	const struct IPmapPair pair_kd = ipmap_find(map, filter_kd);
	assert_int_equal(pair_kd.key, 1);
	assert_ptr_equal(pair_kd.val, V1);

	// val
	expect_ptr(mock_pred_p, p, V0);
	will_return(mock_pred_p, false);
	expect_ptr(mock_pred_p, p, V1);
	will_return(mock_pred_p, true);

	const struct IPmapFilter filter_v = { .val = mock_pred_p, };
	const struct IPmapPair pair_v = ipmap_find(map, filter_v);
	assert_int_equal(pair_v.key, 1);
	assert_ptr_equal(pair_v.val, V1);

	// val_data
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_string(mock_pred_p_p, p2, "x");
	will_return(mock_pred_p_p, false);
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_string(mock_pred_p_p, p2, "x");
	will_return(mock_pred_p_p, true);

	const struct IPmapFilter filter_vd = { .val_data = mock_pred_p_p, .data = "x", };
	const struct IPmapPair pair_vd = ipmap_find(map, filter_vd);
	assert_int_equal(pair_vd.key, 1);
	assert_ptr_equal(pair_vd.val, V1);

	// key_val
	expect_int_value(mock_pred_i_p, i, 0);
	expect_ptr(mock_pred_i_p, p, V0);
	will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 1);
	expect_ptr(mock_pred_i_p, p, V1);
	will_return(mock_pred_i_p, true);

	const struct IPmapFilter filter_kv = { .key_val = mock_pred_i_p, };
	const struct IPmapPair pair_kv = ipmap_find(map, filter_kv);
	assert_int_equal(pair_kv.key, 1);
	assert_ptr_equal(pair_kv.val, V1);

	// key_val_data
	expect_int_value(mock_pred_i_p_p, i, 0);
	expect_ptr(mock_pred_i_p_p, p1, V0);
	expect_ptr(mock_pred_i_p_p, p2, "x");
	will_return(mock_pred_i_p_p, false);
	expect_int_value(mock_pred_i_p_p, i, 1);
	expect_ptr(mock_pred_i_p_p, p1, V1);
	expect_ptr(mock_pred_i_p_p, p2, "x");
	will_return(mock_pred_i_p_p, true);

	const struct IPmapFilter filter_kvd = { .key_val_data = mock_pred_i_p_p, .data = "x", };
	const struct IPmapPair pair_kvd = ipmap_find(map, filter_kvd);
	assert_int_equal(pair_kvd.key, 1);
	assert_ptr_equal(pair_kvd.val, V1);

	ipmap_free(map);
}

static void ipmap_find__some_block(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// key blocks
	expect_int_value(mock_pred_i, i, 0);
	will_return(mock_pred_i, false);

	// key passes, val blocks
	expect_int_value(mock_pred_i, i, 1);
	will_return(mock_pred_i, true);
	expect_ptr(mock_pred_p, p, V1);
	will_return(mock_pred_p, false);

	// both pass
	expect_int_value(mock_pred_i, i, 2);
	will_return(mock_pred_i, true);
	expect_ptr(mock_pred_p, p, V2);
	will_return(mock_pred_p, true);

	const struct IPmapFilter filter = {
		.key = mock_pred_i,
		.val = mock_pred_p,
	};
	const struct IPmapPair pair = ipmap_find(map, filter);
	assert_int_equal(pair.key, 2);
	assert_int_equal(pair.val, V2);

	ipmap_free(map);
}

static void ipmap_find__all_block(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// key blocks
	expect_any_count(mock_pred_i, i, 3);
	will_return_int_count(mock_pred_i, false, 3);

	const struct IPmapFilter filter = {
		.key = mock_pred_i,
		.val = mock_pred_p,
	};
	const struct IPmapPair pair = ipmap_find(map, filter);
	assert_int_equal(pair.key, 0);
	assert_nul(pair.val);

	ipmap_free(map);
}

static void ipmap_find__none_block(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	expect_any(mock_pred_i, i);
	will_return_int(mock_pred_i, true);
	expect_any(mock_pred_p, p);
	will_return_int(mock_pred_p, true);

	const struct IPmapFilter filter = {
		.key = mock_pred_i,
		.val = mock_pred_p,
	};
	const struct IPmapPair pair = ipmap_find(map, filter);
	assert_int_equal(pair.key, 0);
	assert_ptr_equal(pair.val, V0);

	ipmap_free(map);
}

static void ipmap_it__many(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, NULL));
	assert_nul(ipmap_put(map, 2, V2));

	const struct IPmapIt *it = ipmap_it(map);

	assert_non_nul(it);
	assert_int_equal(it->key, 0);
	assert_ptr_equal(it->val, V0);

	it = ipmap_it_next(it);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_nul(it->val);

	ipmap_it_free(it);

	ipmap_free(map);
}

static void ipmap_it__empty(void **state) {

	const struct IPmap *map = ipmap_init();

	const struct IPmapIt *it = ipmap_it(map);

	assert_nul(it);

	ipmap_free(map);
}

static void ipmap_it_next__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_next(it));
}

static void ipmap_filter_it__empty(void **state) {
	const struct IPmap *map = ipmap_init();

	const struct IPmapFilter filter = { .key = mock_pred_i, };
	assert_nul(ipmap_filter_it(map, filter));

	ipmap_free(map);
}

static void ipmap_filter_it__many(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_int_value(mock_pred_i_p_p, i, 0);
	expect_ptr(mock_pred_i_p_p, p1, V0);
	expect_ptr(mock_pred_i_p_p, p2, D0);
	will_return(mock_pred_i_p_p, false);

	// pass 1
	expect_int_value(mock_pred_i_p_p, i, 1);
	expect_ptr(mock_pred_i_p_p, p1, V1);
	expect_ptr(mock_pred_i_p_p, p2, D0);
	will_return(mock_pred_i_p_p, true);

	const struct IPmapFilter filter = { .key_val_data = mock_pred_i_p_p, .data = D0, };
	const struct IPmapIt *it = ipmap_filter_it(map, filter);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	// skip 2
	expect_int_value(mock_pred_i_p_p, i, 2);
	expect_ptr(mock_pred_i_p_p, p1, V2);
	expect_ptr(mock_pred_i_p_p, p2, D0);
	will_return(mock_pred_i_p_p, false);

	// done
	it = ipmap_it_next(it);
	assert_nul(it);

	ipmap_free(map);
}

static void ipmap_equal__(void **state) {

	const struct IPmap *actual = ipmap_init();
	assert_nul(ipmap_put(actual, 0, V0));
	assert_nul(ipmap_put(actual, 1, V1));

	assert_ipmap_not_equal(actual, NULL);

	const struct IPmap *expected = ipmap_init();
	assert_nul(ipmap_put(expected, 0, V0));
	assert_nul(ipmap_put(expected, 1, V1));

	assert_ipmap_equal(actual, expected);

	assert_nul(ipmap_put(actual, 2, V2));

	assert_ipmap_not_equal(actual, expected);

	ipmap_free(actual);
	ipmap_free(expected);
}

static void ipmap_equal__key_removed(void **state) {
	const struct IPmap *a = ipmap_init();
	assert_nul(ipmap_put(a, 0, V0));
	assert_nul(ipmap_put(a, 1, V1));

	const struct IPmap *b = ipmap_init();
	assert_nul(ipmap_put(b, 0, V0));
	assert_nul(ipmap_put(b, 1, V1));

	int *removed_key = (int*)b->ppmap->keys[0];
	b->ppmap->keys[0] = NULL;

	assert_ipmap_not_equal(a, b);

	free(removed_key);
	ipmap_free(a);
	ipmap_free(b);
}

static void ipmap_vals_plist__many(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, V2);

	const struct Plist *list = ipmap_vals_plist(map);

	assert_int_equal(plist_size(list), 3);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_nul(plist_at(list, 1));
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
	ipmap_free(map);
}

static void ipmap_vals_plist_clone__many(void **state) {
	const struct IPmapParams params = {
		.allow_null_val = true,
		.clone_val = (fn_clone)clone_strdup,
	};
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, "0");
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, "2");

	const struct Plist *list = ipmap_vals_plist_clone(map);

	assert_int_equal(plist_size(list), 3);
	assert_str_equal(plist_at(list, 0), "0");
	assert_nul(plist_at(list, 1));
	assert_str_equal(plist_at(list, 2), "2");

	plist_free_vals(list);
	ipmap_free(map);
}

static void ipmap_put_get_remove(void **state) {
	const struct IPmapParams params = { 0 };
	const struct IPmap *map = ipmap_init_with(params);

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	assert_int_equal(ipmap_size(map), 3);

	assert_ptr_equal(ipmap_get(map, 1), V1);

	assert_nul(ipmap_get(map, 999));

	assert_ptr_equal(ipmap_remove(map, 1), V1);

	assert_nul(ipmap_get(map, 1));

	ipmap_free(map);
}

static void ipmap_put_if_absent__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put_if_absent(map, 0, V0));
	assert_ptr_equal(ipmap_get(map, 0), V0);

	const void *existing = ipmap_put_if_absent(map, 0, V1);
	assert_ptr_equal(existing, V0);

	ipmap_free(map);
}

static void ipmap_put_free__(void **state) {
	const struct IPmap *map = ipmap_init();

	const char *val = strdup("val");

	assert_nul(ipmap_put(map, 0, val));

	assert_false(ipmap_put_free(map, 1, V1));

	assert_true(ipmap_put_free(map, 0, V0));

	ipmap_free(map);
}

static void ipmap_put_all__variants(void **state) {
	const struct IPmap *from = ipmap_init();
	assert_nul(ipmap_put(from, 0, V0));
	assert_nul(ipmap_put(from, 1, V1));

	const struct IPmap *expected = ipmap_init();
	assert_nul(ipmap_put(expected, 0, V0));
	assert_nul(ipmap_put(expected, 1, V1));

	const struct IPmapParams params = {
		.free_val = mock_free,
		.clone_val = mock_clone,
	};

	const struct IPmap *to = ipmap_init_with(params);
	assert_nul(ipmap_put(to, 0, V3));

	// put_all
	const struct IPmap *actual = ipmap_clone(to);

	assert_int_equal(ipmap_put_all(actual, from), 1);

	assert_ipmap_equal(actual, expected);
	ipmap_free(actual);

	// put_all_free
	actual = ipmap_clone(to);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(ipmap_put_all_free(actual, from), 1);

	assert_ipmap_equal(actual, expected);
	ipmap_free(actual);

	// put_all_clone
	actual = ipmap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);

	assert_int_equal(ipmap_put_all_clone(actual, from), 1);

	assert_ipmap_equal(actual, expected);
	ipmap_free(actual);

	// put_all_clone_free
	actual = ipmap_clone(to);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_free, ptr, V3);

	assert_int_equal(ipmap_put_all_clone_free(actual, from), 1);

	assert_ipmap_equal(actual, expected);
	ipmap_free(actual);

	ipmap_free(to);
	ipmap_free(from);
	ipmap_free(expected);
}


static void ipmap_put_many__many(void **state) {
	const struct IPmap *to = ipmap_init();
	assert_false(ipmap_put(to, 0, V0));
	assert_false(ipmap_put(to, 1, strdup("replaced")));

	const struct IPmap *expected = ipmap_init();
	assert_false(ipmap_put(expected, 0, V0));
	assert_false(ipmap_put(expected, 1, V1));
	assert_false(ipmap_put(expected, 2, V2));

	assert_int_equal(ipmap_put_many(to,
				1, V1,
				2, V2,
				0),
			1);

	assert_ipmap_equal(to, expected);

	ipmap_free(to);
	ipmap_free(expected);
}

static void ipmap_put_many__no_keyvals(void **state) {
	const struct IPmap *to = ipmap_init();

	assert_int_equal(ipmap_put_many(to, 0), 0);

	ipmap_free(to);
}

static void ipmap_remove_free__(void **state) {
	const struct IPmap *map = ipmap_init();

	const char *val = strdup("val");

	assert_nul(ipmap_put(map, 0, val));

	assert_true(ipmap_remove_free(map, 0));

	assert_false(ipmap_remove_free(map, 1));

	ipmap_free(map);
}

static void ipmap_remove_all__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_remove_all(map), 0);

	assert_false(ipmap_put(map, 0, V0));
	assert_false(ipmap_put(map, 1, V1));

	assert_int_equal(ipmap_remove_all(map), 2);

	assert_int_equal(ipmap_size(map), 0);

	assert_false(ipmap_contains_key(map, 0));
	assert_false(ipmap_contains_val(map, V0));
	assert_false(ipmap_contains_key(map, 1));
	assert_false(ipmap_contains_val(map, V1));

	ipmap_free(map);
}

static void ipmap_remove_all_free__(void **state) {
	const struct IPmapParams params = { .free_val = mock_free, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_int_equal(ipmap_remove_all_free(map), 0);

	assert_false(ipmap_put(map, 0, V0));
	assert_false(ipmap_put(map, 1, V1));
	assert_false(ipmap_put(map, 2, V1));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(ipmap_remove_all_free(map), 3);

	assert_int_equal(ipmap_size(map), 0);

	assert_false(ipmap_contains_key(map, 0));
	assert_false(ipmap_contains_val(map, V0));
	assert_false(ipmap_contains_key(map, 1));
	assert_false(ipmap_contains_val(map, V1));

	ipmap_free(map);
}

static void ipmap_remove_in__(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	const struct IPmap *from = ipmap_init();

	assert_nul(ipmap_put(from, 1, V1));
	assert_nul(ipmap_put(from, 3, V3));

	const struct IPmap *expected = ipmap_init();

	assert_nul(ipmap_put(expected, 0, V0));
	assert_nul(ipmap_put(expected, 2, V2));

	assert_int_equal(ipmap_remove_in(map, from), 1);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(from);
	ipmap_free(expected);
}

static void ipmap_remove_in_free__(void **state) {
	const struct IPmapParams params = { .free_val = mock_free, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_nul(ipmap_put(map, 0, V0));

	const struct IPmap *from = ipmap_init();

	assert_nul(ipmap_put(from, 0, V0));

	expect_ptr(mock_free, ptr, V0);

	assert_int_equal(ipmap_remove_in_free(map, from), 1);

	ipmap_free(map);
	ipmap_free(from);
}

static void ipmap_it_remove__many(void **state) {
	const struct IPmapParams params = { .free_val = mock_free, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_false(ipmap_put(map, 0, V0));
	assert_false(ipmap_put(map, 1, V1));
	assert_false(ipmap_put(map, 2, V2));
	assert_false(ipmap_put(map, 3, V3));
	assert_false(ipmap_put(map, 4, V4));

	const struct IPmap *expected = ipmap_init();

	assert_false(ipmap_put(expected, 0, V0));
	assert_false(ipmap_put(expected, 2, V2));
	assert_false(ipmap_put(expected, 4, V4));

	size_t iterations = 0;
	for (const struct IPmapIt *it = ipmap_it(map); it; it = ipmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			assert_non_nul(ipmap_it_remove(it));
		}
	}

	assert_int_equal(ipmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(expected);
}

static void ipmap_it_remove__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_remove(it));
}

static void ipmap_it_remove_free__many(void **state) {
	const struct IPmapParams params = { .free_val = mock_free, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_false(ipmap_put(map, 0, V0));
	assert_false(ipmap_put(map, 1, V1));
	assert_false(ipmap_put(map, 2, V2));
	assert_false(ipmap_put(map, 3, V3));
	assert_false(ipmap_put(map, 4, V4));

	const struct IPmap *expected = ipmap_init();

	assert_false(ipmap_put(expected, 0, V0));
	assert_false(ipmap_put(expected, 2, V2));
	assert_false(ipmap_put(expected, 4, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	size_t iterations = 0;
	for (const struct IPmapIt *it = ipmap_it(map); it; it = ipmap_it_next(it)) {
		iterations++;
		if (it->val == V1 || it->val == V3) {
			assert_true(ipmap_it_remove_free(it));
		}
	}

	assert_int_equal(ipmap_size(map), 3);
	assert_int_equal(iterations, 5);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(expected);
}

static void ipmap_it_remove_free__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_false(ipmap_it_remove_free(it));
}

static void ipmap_str__(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, NULL));
	assert_nul(ipmap_put(map, 999, V2));

	char *expected = sprintf_alloc(
			"0 = %p\n"
			"1 = (null)\n"
			"999 = %p\n",
			V0,
			V2
			);

	char *actual = ipmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ipmap_free(map);
}

static void ipmap__null_inputs(void **state) {
	const struct IPmap *map = ipmap_init();
	const struct IPmapFilter filter = { 0 };

	assert_nul(ipmap_clone(NULL));
	assert_nul(ipmap_clone_deep(NULL));
	ipmap_free(NULL);
	ipmap_free_vals(NULL);
	ipmap_it_free(NULL);
	assert_false(ipmap_get(NULL, 0));
	assert_false(ipmap_contains_key(NULL, 0));
	assert_false(ipmap_contains_val(NULL, 0));
	assert_false(ipmap_first_key(NULL, NULL, NULL));
	assert_nul(ipmap_at(NULL, 0).val);
	ipmap_find(NULL, filter);
	assert_nul(ipmap_it(NULL));
	assert_nul(ipmap_filter_it(NULL, filter));
	assert_nul(ipmap_it_next(NULL));
	assert_false(ipmap_put(NULL, 0, NULL));
	assert_nul(ipmap_put_if_absent(NULL, 0, NULL));
	assert_nul(ipmap_put_if_absent(map, 0, NULL));
	assert_false(ipmap_put_free(NULL, 0, NULL));
	assert_int_equal(ipmap_put_all(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all(map, NULL), 0);
	assert_int_equal(ipmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_free(map, NULL), 0);
	assert_int_equal(ipmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_clone(map, NULL), 0);
	assert_int_equal(ipmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_clone_free(map, NULL), 0);
	assert_int_equal(ipmap_put_many(NULL, NULL), 0);
	assert_nul(ipmap_remove(NULL, 0));
	assert_nul(ipmap_remove(map, 0));
	assert_false(ipmap_remove_free(NULL, 0));
	assert_false(ipmap_remove_free(map, 0));
	assert_int_equal(ipmap_remove_all(NULL), 0);
	assert_int_equal(ipmap_remove_all_free(NULL), 0);
	assert_int_equal(ipmap_remove_in(NULL, NULL), 0);
	assert_int_equal(ipmap_remove_in(map, NULL), 0);
	assert_int_equal(ipmap_remove_in(NULL, map), 0);
	assert_int_equal(ipmap_remove_in_free(NULL, NULL), 0);
	assert_int_equal(ipmap_remove_in_free(map, NULL), 0);
	assert_int_equal(ipmap_remove_in_free(NULL, map), 0);
	assert_nul(ipmap_it_remove(NULL));
	assert_false(ipmap_it_remove_free(NULL));
	assert_false(ipmap_equal(NULL, NULL));
	assert_false(ipmap_equal(map, NULL));
	assert_nul(ipmap_vals_plist(NULL));
	assert_nul(ipmap_vals_plist_clone(NULL));
	assert_nul(ipmap_str(NULL));
	assert_int_equal(ipmap_size(NULL), 0);

	ipmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ipmap_get__key_removed),

		TEST(ipmap_clone__many),
		TEST(ipmap_clone__params),

		TEST(ipmap_clone_deep__clone_val),
		TEST(ipmap_clone_deep__no_clone_val),

		TEST(ipmap_free_vals__),

		TEST(ipmap_it_free__partial),

		TEST(ipmap_contains_key__),

		TEST(ipmap_contains_val__),

		TEST(ipmap_first_key__),

		TEST(ipmap_at__),

		TEST(ipmap_find__variants),
		TEST(ipmap_find__some_block),
		TEST(ipmap_find__all_block),
		TEST(ipmap_find__none_block),

		TEST(ipmap_it__many),
		TEST(ipmap_it__empty),

		TEST(ipmap_it_next__partial),

		TEST(ipmap_filter_it__empty),
		TEST(ipmap_filter_it__many),

		TEST(ipmap_equal__),
		TEST(ipmap_equal__key_removed),

		TEST(ipmap_vals_plist__many),
		TEST(ipmap_vals_plist_clone__many),

		TEST(ipmap_put_get_remove),

		TEST(ipmap_put_if_absent__),

		TEST(ipmap_put_free__),

		TEST(ipmap_put_all__variants),

		TEST(ipmap_put_many__many),
		TEST(ipmap_put_many__no_keyvals),

		TEST(ipmap_remove_free__),

		TEST(ipmap_remove_all__),
		TEST(ipmap_remove_all_free__),

		TEST(ipmap_remove_in__),

		TEST(ipmap_remove_in_free__),

		TEST(ipmap_it_remove__many),
		TEST(ipmap_it_remove__partial),

		TEST(ipmap_it_remove_free__many),
		TEST(ipmap_it_remove_free__partial),

		TEST(ipmap_str__),

		TEST(ipmap__null_inputs),
	};

	return RUN(tests);
}

