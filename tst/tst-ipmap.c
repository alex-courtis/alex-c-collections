#include "assert-ipmap.h"
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
#include "ppmap.h"
#include "pslist.h"
#include "pset.h"
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

static int vals[5] = { 20, 21, 22, 23, 24,};
static void *V0 = &vals[0];
static void *V1 = &vals[1];
static void *V2 = &vals[2];
static void *V3 = &vals[3];
static void *V4 = &vals[4];

static int datas[1] = { 30, };
static void *D0 = &datas[0];

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

static void ipmap_free_vals__(void **state) {
	const struct IPmap *map = ipmap_init();
	assert_nul(ipmap_put(map, 0, strdup("zero")));

	ipmap_free_vals(map);
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

static void ipmap_it_free__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	ipmap_it_free(it);
}

static void ipmap_it_next__partial(void **state) {
	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_next(it));
}

static void ipmap_filter_it__many(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_int_value(mock_3pred_szt_ptr, i, 0);
	expect_ptr(mock_3pred_szt_ptr, ptr, V0);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	// pass 1
	expect_int_value(mock_3pred_szt_ptr, i, 1);
	expect_ptr(mock_3pred_szt_ptr, ptr, V1);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, true);

	const struct IPmapIt *it = ipmap_filter_it(map, mock_3pred_szt_ptr, D0);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	// skip 2
	expect_int_value(mock_3pred_szt_ptr, i, 2);
	expect_ptr(mock_3pred_szt_ptr, ptr, V2);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	// done
	it = ipmap_it_next(it);
	assert_nul(it);

	ipmap_free(map);
}

static void ipmap_key_filter_it__many(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_int_value(mock_2pred_szt, i, 0);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	// pass 1
	expect_int_value(mock_2pred_szt, i, 1);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, true);

	const struct IPmapIt *it = ipmap_key_filter_it(map, mock_2pred_szt, D0);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	// skip 2
	expect_int_value(mock_2pred_szt, i, 2);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	// done
	it = ipmap_it_next(it);
	assert_nul(it);

	ipmap_free(map);
}

static void ipmap_val_filter_it__many(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip V0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// pass V1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	const struct IPmapIt *it = ipmap_val_filter_it(map, mock_2pred, D0);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	// skip V2
	expect_ptr(mock_2pred, ptr, V2);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// done
	it = ipmap_it_next(it);
	assert_nul(it);

	ipmap_free(map);
}

static void ipmap_filter_it__none(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_int_value(mock_3pred_szt_ptr, i, 0);
	expect_ptr(mock_3pred_szt_ptr, ptr, V0);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	// skip 1
	expect_int_value(mock_3pred_szt_ptr, i, 1);
	expect_ptr(mock_3pred_szt_ptr, ptr, V1);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	assert_nul(ipmap_filter_it(map, mock_3pred_szt_ptr, D0));

	ipmap_free(map);
}

static void ipmap_filter_it_key__none(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_int_value(mock_2pred_szt, i, 0);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	// skip 1
	expect_int_value(mock_2pred_szt, i, 1);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	assert_nul(ipmap_key_filter_it(map, mock_2pred_szt, D0));

	ipmap_free(map);
}

static void ipmap_filter_it_val__none(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// skip 1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	assert_nul(ipmap_val_filter_it(map, mock_2pred, D0));

	ipmap_free(map);
}

static void ipmap_filter_it__empty(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_filter_it(map, mock_3pred_szt_ptr, D0));

	ipmap_free(map);
}

static void ipmap_filter_it_key__empty(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_key_filter_it(map, mock_2pred_szt, D0));

	ipmap_free(map);
}

static void ipmap_filter_it_val__empty(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_val_filter_it(map, mock_2pred, D0));

	ipmap_free(map);
}

static void ipmap_find__matches(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_int_value(mock_3pred_szt_ptr, i, 0);
	expect_ptr(mock_3pred_szt_ptr, ptr, V0);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	// get 1
	expect_int_value(mock_3pred_szt_ptr, i, 1);
	expect_ptr(mock_3pred_szt_ptr, ptr, V1);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, true);

	const struct IPmapPair v_pair = ipmap_find(map, mock_3pred_szt_ptr, D0);
	assert_ptr_equal(v_pair.key, 1);
	assert_ptr_equal(v_pair.val, V1);

	ipmap_free(map);
}

static void ipmap_find_key__matches(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_int_value(mock_2pred_szt, i, 0);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	// get 1
	expect_int_value(mock_2pred_szt, i, 1);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, true);

	const struct IPmapPair k_pair = ipmap_find_key(map, mock_2pred_szt, D0);
	assert_ptr_equal(k_pair.key, 1);
	assert_ptr_equal(k_pair.val, V1);

	ipmap_free(map);
}

static void ipmap_find_val__matches(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	// skip 0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// get 1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, true);

	const struct IPmapPair kv_pair = ipmap_find_val(map, mock_2pred, D0);
	assert_ptr_equal(kv_pair.key, 1);
	assert_ptr_equal(kv_pair.val, V1);

	ipmap_free(map);
}

static void ipmap_find__no_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_int_value(mock_3pred_szt_ptr, i, 0);
	expect_ptr(mock_3pred_szt_ptr, ptr, V0);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	// skip 1
	expect_int_value(mock_3pred_szt_ptr, i, 1);
	expect_ptr(mock_3pred_szt_ptr, ptr, V1);
	expect_ptr(mock_3pred_szt_ptr, data, D0);
	will_return(mock_3pred_szt_ptr, false);

	const struct IPmapPair kv_pair = ipmap_find(map, mock_3pred_szt_ptr, D0);
	assert_int_equal(kv_pair.key, 0);
	assert_nul(kv_pair.val);

	ipmap_free(map);
}

static void ipmap_find_key__no_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_int_value(mock_2pred_szt, i, 0);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	// skip 1
	expect_int_value(mock_2pred_szt, i, 1);
	expect_ptr(mock_2pred_szt, data, D0);
	will_return(mock_2pred_szt, false);

	const struct IPmapPair k_pair = ipmap_find_key(map, mock_2pred_szt, D0);
	assert_int_equal(k_pair.key, 0);
	assert_nul(k_pair.val);

	ipmap_free(map);
}

static void ipmap_find_val__no_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	// skip 0
	expect_ptr(mock_2pred, ptr, V0);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	// skip 1
	expect_ptr(mock_2pred, ptr, V1);
	expect_ptr(mock_2pred, data, D0);
	will_return(mock_2pred, false);

	const struct IPmapPair v_pair = ipmap_find_val(map, mock_2pred, D0);
	assert_int_equal(v_pair.key, 0);
	assert_nul(v_pair.val);

	ipmap_free(map);
}

static void ipmap_find__null_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	const struct IPmapPair kv_pair = ipmap_find(map, NULL, D0);
	assert_int_equal(kv_pair.key, 0);
	assert_nul(kv_pair.val);

	ipmap_free(map);
}

static void ipmap_find_key__null_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	const struct IPmapPair k_pair = ipmap_find_key(map, NULL, D0);
	assert_int_equal(k_pair.key, 0);
	assert_nul(k_pair.val);

	ipmap_free(map);
}

static void ipmap_find_val__null_match(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));

	const struct IPmapPair v_pair = ipmap_find_val(map, NULL, D0);
	assert_int_equal(v_pair.key, 0);
	assert_nul(v_pair.val);

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

static void ipmap_get__key_removed(void **state) {

	const struct IPmap *actual = ipmap_init();
	assert_nul(ipmap_put(actual, 0, V0));

	int *removed_key = (int*)actual->ppmap->keys[0];
	actual->ppmap->keys[0] = NULL;

	assert_nul(ipmap_get(actual, 0));

	free(removed_key);
	ipmap_free(actual);
}

static void ipmap_put_free__(void **state) {
	const struct IPmap *map = ipmap_init();

	const char *val = strdup("val");

	assert_nul(ipmap_put(map, 0, val));

	assert_false(ipmap_put_free(map, 1, V1));

	assert_true(ipmap_put_free(map, 0, V0));

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

	assert_nul(ipmap_put(map, 0, V0));
	assert_nul(ipmap_put(map, 1, V1));
	assert_nul(ipmap_put(map, 2, V2));

	const struct IPmap *from = ipmap_init();

	assert_nul(ipmap_put(from, 1, V1));
	assert_nul(ipmap_put(from, 3, V3));

	const struct IPmap *expected = ipmap_init();

	assert_nul(ipmap_put(expected, 0, V0));
	assert_nul(ipmap_put(expected, 2, V2));

	assert_int_equal(ipmap_remove_all(map, from), 1);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(from);
	ipmap_free(expected);
}

static void ipmap_remove_all_free__(void **state) {
	const struct IPmapParams params = { .free_val = mock_free, };
	const struct IPmap *map = ipmap_init_with(params);

	assert_nul(ipmap_put(map, 0, V0));

	const struct IPmap *from = ipmap_init();

	assert_nul(ipmap_put(from, 0, V0));

	expect_ptr(mock_free, ptr, V0);

	assert_int_equal(ipmap_remove_all_free(map, from), 1);

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
			ipmap_it_remove(it);
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

	ipmap_it_remove(it);
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
			ipmap_it_remove_free(it);
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

	ipmap_it_remove_free(it);
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

static void ipmap_vals_pslist__many(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, V2);

	struct Pslist *list = ipmap_vals_pslist(map);

	assert_int_equal(pslist_length(list), 3);
	assert_ptr_equal(pslist_at(list, 0), V0);
	assert_nul(pslist_at(list, 1));
	assert_ptr_equal(pslist_at(list, 2), V2);

	pslist_free(&list);
	ipmap_free(map);
}

static void ipmap_vals_pslist_clone__many(void **state) {
	const struct IPmapParams params = {
		.allow_null_val = true,
		.clone_val = (fn_clone)clone_strdup,
	};
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, "0");
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, "2");

	struct Pslist *list = ipmap_vals_pslist_clone(map);

	assert_int_equal(pslist_length(list), 3);
	assert_str_equal(pslist_at(list, 0), "0");
	assert_nul(pslist_at(list, 1));
	assert_str_equal(pslist_at(list, 2), "2");

	pslist_free_vals(&list, NULL);
	ipmap_free(map);
}

static void ipmap_vals_pset__many(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, V2);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);
	pset_add(expected, V2);

	const struct Pset *actual = ipmap_vals_pset(map);

	assert_pset_equal(actual, expected);

	ipmap_free(map);
	pset_free(expected);
	pset_free(actual);
}

static void ipmap_vals_pset_clone__many(void **state) {
	const struct IPmapParams params = { .clone_val = mock_clone, };
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, V0);

	const struct Pset *expected = pset_init();
	pset_add(expected, V0);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Pset *actual = ipmap_vals_pset_clone(map);

	assert_pset_equal(actual, expected);

	ipmap_free(map);
	pset_free(expected);
	pset_free(actual);
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

static void ipmap__null_inputs(void **state) {
	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_clone(NULL));
	assert_nul(ipmap_clone_deep(NULL));
	ipmap_free(NULL);
	ipmap_free_vals(NULL);
	ipmap_it_free(NULL);
	assert_false(ipmap_get(NULL, 0));
	assert_false(ipmap_contains_key(NULL, 0));
	assert_false(ipmap_contains_val(NULL, 0));
	assert_nul(ipmap_it(NULL));
	assert_nul(ipmap_filter_it(NULL, NULL, NULL));
	assert_nul(ipmap_filter_it(map, NULL, NULL));
	assert_nul(ipmap_key_filter_it(NULL, NULL, NULL));
	assert_nul(ipmap_key_filter_it(map, NULL, NULL));
	assert_nul(ipmap_val_filter_it(NULL, NULL, NULL));
	assert_nul(ipmap_val_filter_it(map, NULL, NULL));
	assert_nul(ipmap_it_next(NULL));
	ipmap_find(NULL, NULL, NULL);
	ipmap_find(map, NULL, NULL);
	ipmap_find_key(NULL, NULL, NULL);
	ipmap_find_key(map, NULL, NULL);
	ipmap_find_val(NULL, NULL, NULL);
	ipmap_find_val(map, NULL, NULL);
	assert_false(ipmap_put(NULL, 0, NULL));
	assert_nul(ipmap_put_if_absent(NULL, 0, NULL));
	assert_nul(ipmap_put_if_absent(map, 0, NULL));
	assert_false(ipmap_put_free(NULL, 0, NULL));
	assert_int_equal(ipmap_put_many(NULL, NULL), 0);
	assert_nul(ipmap_remove(NULL, 0));
	assert_nul(ipmap_remove(map, 0));
	assert_false(ipmap_remove_free(NULL, 0));
	assert_false(ipmap_remove_free(map, 0));
	ipmap_it_remove(NULL);
	assert_int_equal(ipmap_remove_all(NULL, NULL), 0);
	assert_int_equal(ipmap_remove_all(map, NULL), 0);
	assert_int_equal(ipmap_remove_all(NULL, map), 0);
	assert_int_equal(ipmap_remove_all_free(NULL, NULL), 0);
	assert_int_equal(ipmap_remove_all_free(map, NULL), 0);
	assert_int_equal(ipmap_remove_all_free(NULL, map), 0);
	assert_int_equal(ipmap_put_all(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all(map, NULL), 0);
	assert_int_equal(ipmap_put_all_free(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_free(map, NULL), 0);
	assert_int_equal(ipmap_put_all_clone(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_clone(map, NULL), 0);
	assert_int_equal(ipmap_put_all_clone_free(NULL, NULL), 0);
	assert_int_equal(ipmap_put_all_clone_free(map, NULL), 0);
	assert_false(ipmap_equal(NULL, NULL));
	assert_false(ipmap_equal(map, NULL));
	assert_nul(ipmap_vals_pslist(NULL));
	assert_nul(ipmap_vals_pslist_clone(NULL));
	assert_nul(ipmap_vals_pset(NULL));
	assert_nul(ipmap_vals_pset_clone(NULL));
	assert_nul(ipmap_str(NULL));
	assert_int_equal(ipmap_size(NULL), 0);

	ipmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ipmap_put_get_remove),

		TEST(ipmap_free_vals__),

		TEST(ipmap_it__many),
		TEST(ipmap_it__empty),

		TEST(ipmap_it_free__partial),

		TEST(ipmap_it_next__partial),

		TEST(ipmap_filter_it__many),
		TEST(ipmap_key_filter_it__many),
		TEST(ipmap_val_filter_it__many),

		TEST(ipmap_filter_it__none),
		TEST(ipmap_filter_it_val__none),
		TEST(ipmap_filter_it_key__none),

		TEST(ipmap_filter_it__empty),
		TEST(ipmap_filter_it_val__empty),
		TEST(ipmap_filter_it_key__empty),

		TEST(ipmap_find__matches),
		TEST(ipmap_find_key__matches),
		TEST(ipmap_find_val__matches),

		TEST(ipmap_find__no_match),
		TEST(ipmap_find_key__no_match),
		TEST(ipmap_find_val__no_match),

		TEST(ipmap_find__null_match),
		TEST(ipmap_find_key__null_match),
		TEST(ipmap_find_val__null_match),

		TEST(ipmap_equal__),
		TEST(ipmap_equal__key_removed),

		TEST(ipmap_contains_key__),

		TEST(ipmap_contains_val__),

		TEST(ipmap_get__key_removed),

		TEST(ipmap_put_free__),

		TEST(ipmap_put_if_absent__),

		TEST(ipmap_put_all__variants),

		TEST(ipmap_put_many__many),
		TEST(ipmap_put_many__no_keyvals),

		TEST(ipmap_remove_free__),

		TEST(ipmap_remove_all__),

		TEST(ipmap_remove_all_free__),

		TEST(ipmap_it_remove__many),
		TEST(ipmap_it_remove__partial),

		TEST(ipmap_it_remove_free__many),
		TEST(ipmap_it_remove_free__partial),

		TEST(ipmap_str__),

		TEST(ipmap_vals_pslist__many),
		TEST(ipmap_vals_pslist_clone__many),

		TEST(ipmap_vals_pset__many),
		TEST(ipmap_vals_pset_clone__many),

		TEST(ipmap_clone__many),
		TEST(ipmap_clone__params),

		TEST(ipmap_clone_deep__clone_val),
		TEST(ipmap_clone_deep__no_clone_val),

		TEST(ipmap__null_inputs),
	};

	return RUN(tests);
}

