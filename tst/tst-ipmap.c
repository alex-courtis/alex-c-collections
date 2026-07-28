#include "assert-ipmap.h"
#include "assert-plist.h"
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
#include "plist.h"
#include "ppmap.h"
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

struct Plist {
	const struct PlistParams params;
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

struct IPmap {
	const struct IPmapParams params;
	const struct PPmap *ppmap;
};

static bool gt_9(const size_t i, const void* const b) {
	return i > 9;
}

static void ipmap_clone__(void **state) {
	assert_nul(ipmap_clone(NULL));

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	const struct IPmap *clone = ipmap_clone(map);

	assert_ipmap_equal_ordered(map, clone);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V1);

	assert_ipmap_equal_ordered(clone, expected);

	ipmap_free(map);
	ipmap_free(clone);
	ipmap_free(expected);
}

static void ipmap_clone__params__constructor(void **state) {
	assert_nul(ipmap_clone(NULL));

	struct IPmapParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone ,
		.str_val = mock_str,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1, };
	const struct IPmap *map = ipmap_init_with(params);

	const struct IPmap *clone = ipmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->ppmap->size, 0);
	assert_int_equal(clone->ppmap->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->ppmap->params.equal_key, equal_stp);
	assert_ptr_equal(clone->ppmap->params.equal_val, mock_equal);
	assert_ptr_equal(clone->ppmap->params.alloc_key, clone_size_t_ptr);
	assert_ptr_equal(clone->ppmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->ppmap->params.free_key, free);
	assert_ptr_equal(clone->ppmap->params.free_val, mock_free);
	assert_ptr_equal(clone->ppmap->params.clone_val, mock_clone);
	assert_ptr_equal(clone->ppmap->params.str_key, str_size_t_ptr);
	assert_ptr_equal(clone->ppmap->params.str_val, mock_str);
	assert_true(clone->ppmap->params.allow_null_val);

	assert_ptr_equal(clone->params.equal_val, mock_equal);
	assert_ptr_equal(clone->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->params.free_val, mock_free);
	assert_ptr_equal(clone->params.clone_val, mock_clone);
	assert_ptr_equal(clone->params.str_val, mock_str);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	ipmap_free(map);
	ipmap_free(clone);
}

static void ipmap_clone_deep__(void **state) {
	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);

	const struct IPmap *clone = ipmap_clone_deep(map);

	assert_ipmap_equal_ordered(map, clone);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V1);

	assert_ipmap_equal_ordered(clone, expected);

	ipmap_free(map);
	ipmap_free(expected);
	ipmap_free(clone);
}

static void ipmap_free__(void **state) {
	ipmap_free(NULL);
}

static void ipmap_free_vals__(void **state) {
	ipmap_free_vals(NULL);

	const char *val = strdup("no double free");

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });
	ipmap_put(map, 0, val);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, val);

	ipmap_free_vals(map);
}

static void ipmap_it_free__(void **state) {
	ipmap_it_free(NULL);

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	ipmap_it_free(it);
}

static void ipmap_contains_key__(void **state) {
	assert_false(ipmap_contains_key(NULL, 1));

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_contains_key(map, 1));

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	assert_true(ipmap_contains_key(map, 0));
	assert_true(ipmap_contains_key(map, 1));

	assert_false(ipmap_contains_key(map, 3));

	ipmap_free(map);
}

static void ipmap_contains_val__(void **state) {
	assert_false(ipmap_contains_val(NULL, V5));

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_contains_val(map, V0));

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	assert_true(ipmap_contains_val(map, V1));

	assert_false(ipmap_contains_val(map, V5));

	ipmap_free(map);
}

static void ipmap_get__(void **state) {
	assert_nul(ipmap_get(NULL, 99));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_get(map, 99));

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	assert_ptr_equal(ipmap_get(map, 2), V2);

	assert_nul(ipmap_get(map, 99));

	ipmap_free(map);
}

static void ipmap_first_key__(void **state) {
	assert_false(ipmap_first_key(NULL, NULL, V0));

	size_t i = 99;
	assert_false(ipmap_first_key(&i, NULL, V0));
	assert_int_equal(i, 0);

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_first_key(NULL, map, V5));

	size_t j = 99;
	assert_false(ipmap_first_key(&j, map, V5));
	assert_int_equal(j, 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	size_t k = 99;
	assert_true(ipmap_first_key(&k, map, V2));
	assert_int_equal(k, 2);

	size_t l = 99;
	assert_false(ipmap_first_key(&l, map, V5));
	assert_int_equal(l, 0);

	size_t m = 99;
	assert_false(ipmap_first_key(&m, NULL, V5));
	assert_int_equal(m, 0);

	ipmap_free(map);
}

static void ipmap_at__(void **state) {
	assert_int_equal(ipmap_at(NULL, 0).key, 0);
	assert_nul(ipmap_at(NULL, 0).val);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_at(NULL, 0).key, 0);
	assert_nul(ipmap_at(map, 0).val);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	assert_int_equal(ipmap_at(map, 1).key, 1);
	assert_ptr_equal(ipmap_at(map, 1).val, V1);

	assert_int_equal(ipmap_at(map, 3).key, 0);
	assert_nul(ipmap_at(map, 3).val);

	ipmap_free(map);
}

static void ipmap_find__empty_filter(void **state) {
	assert_int_equal(ipmap_find(NULL, (struct IPmapFilter){ 0 }).key, 0);
	assert_nul(ipmap_find(NULL, (struct IPmapFilter){ 0 }).val);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).key, 0);
	assert_nul(ipmap_find(map, (struct IPmapFilter){ 0 }).val);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 98, V1);
	ipmap_put(map, 99, V2);

	assert_int_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).key, 0);
	assert_ptr_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).val, V0);

	ipmap_free(map);
}

static void ipmap_find__variants(void **state) {
	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	// key
	expect_int_value(mock_pred_i, i, 0); will_return(mock_pred_i, false);
	expect_int_value(mock_pred_i, i, 1); will_return(mock_pred_i, true);

	struct IPmapPair pair = ipmap_find(map, (struct IPmapFilter){ .key = mock_pred_i, });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	// key_data
	expect_int_value(mock_pred_i_p, i, 0); expect_string(mock_pred_i_p, p, "x"); will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 1); expect_string(mock_pred_i_p, p, "x"); will_return(mock_pred_i_p, true);

	pair = ipmap_find(map, (struct IPmapFilter){ .key_data = mock_pred_i_p, .data = "x", });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	// val
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	pair = ipmap_find(map, (struct IPmapFilter){ .val = mock_pred_p, });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	// val_data
	expect_ptr(mock_pred_p_p, p1, V0); expect_string(mock_pred_p_p, p2, "x"); will_return(mock_pred_p_p, false);
	expect_ptr(mock_pred_p_p, p1, V1); expect_string(mock_pred_p_p, p2, "x"); will_return(mock_pred_p_p, true);

	pair = ipmap_find(map, (struct IPmapFilter){ .val_data = mock_pred_p_p, .data = "x", });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	// key_val
	expect_int_value(mock_pred_i_p, i, 0); expect_ptr(mock_pred_i_p, p, V0); will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 1); expect_ptr(mock_pred_i_p, p, V1); will_return(mock_pred_i_p, true);

	pair = ipmap_find(map, (struct IPmapFilter){ .key_val = mock_pred_i_p, });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	// key_val_data
	expect_int_value(mock_pred_i_p_p, i, 0); expect_ptr(mock_pred_i_p_p, p1, V0); expect_ptr(mock_pred_i_p_p, p2, "x"); will_return(mock_pred_i_p_p, false);
	expect_int_value(mock_pred_i_p_p, i, 1); expect_ptr(mock_pred_i_p_p, p1, V1); expect_ptr(mock_pred_i_p_p, p2, "x"); will_return(mock_pred_i_p_p, true);

	pair = ipmap_find(map, (struct IPmapFilter){ .key_val_data = mock_pred_i_p_p, .data = "x", });
	assert_int_equal(pair.key, 1);
	assert_ptr_equal(pair.val, V1);

	ipmap_free(map);
}

static void ipmap_find__some_block(void **state) {
	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	// key blocks
	expect_int_value(mock_pred_i, i, 0); will_return(mock_pred_i, false);

	// key passes, val blocks
	expect_int_value(mock_pred_i, i, 1); will_return(mock_pred_i, true);
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, false);

	// both pass
	expect_int_value(mock_pred_i, i, 2); will_return(mock_pred_i, true);
	expect_ptr(mock_pred_p, p, V2); will_return(mock_pred_p, true);

	const struct IPmapPair pair = ipmap_find(map, (struct IPmapFilter){ .key = mock_pred_i, .val = mock_pred_p, });
	assert_int_equal(pair.key, 2);
	assert_int_equal(pair.val, V2);

	ipmap_free(map);
}

static void ipmap_find__all_block(void **state) {
	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	// key blocks
	expect_any_count(mock_pred_i, i, 3); will_return_int_count(mock_pred_i, false, 3);

	const struct IPmapPair pair = ipmap_find(map, (struct IPmapFilter){ .key = mock_pred_i, .val = mock_pred_p, });
	assert_int_equal(pair.key, 0);
	assert_nul(pair.val);

	ipmap_free(map);
}

static void ipmap_find__none_block(void **state) {
	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	expect_any(mock_pred_i, i); will_return_int(mock_pred_i, true);
	expect_any(mock_pred_p, p); will_return_int(mock_pred_p, true);

	const struct IPmapPair pair = ipmap_find(map, (struct IPmapFilter){ .key = mock_pred_i, .val = mock_pred_p, });
	assert_int_equal(pair.key, 0);
	assert_ptr_equal(pair.val, V0);

	ipmap_free(map);
}

static void ipmap_it__(void **state) {
	assert_nul(ipmap_it(NULL));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_it(map));

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	const struct IPmapIt *it = ipmap_it(map);

	assert_non_nul(it);
	assert_int_equal(it->key, 0);
	assert_ptr_equal(it->val, V0);

	it = ipmap_it_next(it);
	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	assert_nul(ipmap_it_next(it));

	ipmap_free(map);
}

static void ipmap_filter_it__(void **state) {
	assert_nul(ipmap_filter_it(NULL, (struct IPmapFilter){ 0 }));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_filter_it(map, (struct IPmapFilter){ 0 }));

	ipmap_put(map, 90, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 92, V2);

	const struct IPmapIt *it = ipmap_filter_it(map, (struct IPmapFilter){ .key_data = gt_9, .data = D0, });

	assert_non_nul(it);
	assert_int_equal(it->key, 90);
	assert_ptr_equal(it->val, V0);

	it = ipmap_it_next(it);
	assert_non_nul(it);
	assert_int_equal(it->key, 92);
	assert_ptr_equal(it->val, V2);

	assert_nul(ipmap_it_next(it));

	it = ipmap_filter_it(map, (struct IPmapFilter){ .val_data = equal_ptr, .data = V1, });

	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	assert_nul(ipmap_it_next(it));

	ipmap_free(map);
}

static void ipmap_it_next__(void **state) {
	assert_nul(ipmap_it_next(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_next(it));
}

static void ipmap_put__(void **state) {
	ipmap_put(NULL, 0, V0);

	const struct IPmap *map = ipmap_init();

	ipmap_put(map, 0, V0);

	assert_ptr_equal(ipmap_put(map, 0, V1), V0);

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 0), V1);

	ipmap_put(map, 1, V1);

	assert_ptr_equal(ipmap_get(map, 1), V1);

	ipmap_free(map);
}

static void ipmap_put_free__(void **state) {
	assert_false(ipmap_put_free(NULL, 0, V0));

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_put_free(map, 0, strdup("to be freed")));

	assert_true(ipmap_put_free(map, 0, V0));

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 0), V0);

	ipmap_free(map);
}

static void ipmap_put_clone__(void **state) {
	ipmap_put_clone(NULL, 0, V0);

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);

	assert_nul(ipmap_put_clone(map, 0, V4));
	assert_ptr_equal(ipmap_get(map, 0), V0);

	ipmap_free(map);

	map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V0, void*);
	assert_nul(ipmap_put_clone(map, 0, V0));

	assert_ptr_equal(ipmap_get(map, 0), V0);

	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);
	assert_ptr_equal(ipmap_put_clone(map, 0, V1), V0);;

	assert_ptr_equal(ipmap_get(map, 0), V1);

	ipmap_free(map);
}

static void ipmap_put_clone_free__(void **state) {
	ipmap_put_clone_free(NULL, 0, V0);

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);

	assert_false(ipmap_put_clone_free(map, 0, V4));
	assert_ptr_equal(ipmap_get(map, 0), V0);

	ipmap_free(map);

	map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, .free_val = mock_free, });

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V0, void*);
	assert_false(ipmap_put_clone_free(map, 0, V0));

	assert_ptr_equal(ipmap_get(map, 0), V0);

	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_free, ptr, V0);

	assert_true(ipmap_put_clone_free(map, 0, V1));

	assert_ptr_equal(ipmap_get(map, 0), V1);

	ipmap_free(map);
}

static void ipmap_put_if_absent__(void **state) {
	assert_nul(ipmap_put_if_absent(NULL, 0, V0));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put_if_absent(map, 0, V0));

	assert_ptr_equal(ipmap_put_if_absent(map, 0, V5), V0);

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 0), V0);

	assert_nul(ipmap_put_if_absent(map, 1, V1));

	assert_ptr_equal(ipmap_get(map, 1), V1);

	ipmap_free(map);
}

static void ipmap_put_all__(void **state) {
	assert_int_equal(ipmap_put_all(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_put_all(NULL, map), 0);
	assert_int_equal(ipmap_put_all(map, NULL), 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	const struct IPmap *from = ipmap_init();

	ipmap_put(from, 0, V0);
	ipmap_put(from, 1, V4);
	ipmap_put(from, 3, V5);

	assert_int_equal(ipmap_put_all(map, from), 2);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V4);
	ipmap_put(expected, 2, V2);
	ipmap_put(expected, 3, V5);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_free__(void **state) {
	assert_int_equal(ipmap_put_all_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_put_all_free(NULL, map), 0);
	assert_int_equal(ipmap_put_all_free(map, NULL), 0);

	ipmap_put(map, 0, strdup("V0"));
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, strdup("V2"));

	const struct IPmap *from = ipmap_init();

	ipmap_put(from, 0, V0);
	ipmap_put(from, 2, V4);
	ipmap_put(from, 3, V5);

	assert_int_equal(ipmap_put_all_free(map, from), 2);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V1);
	ipmap_put(expected, 2, V4);
	ipmap_put(expected, 3, V5);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_clone__(void **state) {
	assert_int_equal(ipmap_put_all_clone(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(ipmap_put_all_clone(NULL, map), 0);
	assert_int_equal(ipmap_put_all_clone(map, NULL), 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	const struct IPmap *from = ipmap_init();

	ipmap_put(from, 2, V4);
	ipmap_put(from, 3, V5);

	// NOP, no clone_val
	assert_int_equal(ipmap_put_all_clone(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ipmap_put_all_clone(map, from), 1);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V1);
	ipmap_put(expected, 2, V4);
	ipmap_put(expected, 3, V5);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_clone_free__(void **state) {
	assert_int_equal(ipmap_put_all_clone_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(ipmap_put_all_clone_free(NULL, map), 0);
	assert_int_equal(ipmap_put_all_clone_free(map, NULL), 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, strdup("V1"));
	ipmap_put(map, 2, V2);

	const struct IPmap *from = ipmap_init();

	ipmap_put(from, 1, V4);
	ipmap_put(from, 3, V5);

	// NOP, no clone_val
	assert_int_equal(ipmap_put_all_clone_free(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ipmap_put_all_clone_free(map, from), 1);

	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V4);
	ipmap_put(expected, 2, V2);
	ipmap_put(expected, 3, V5);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_remove__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);

	assert_nul(ipmap_remove(NULL, 99));

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	assert_ptr_equal(ipmap_remove(map, 1), V1);

	assert_nul(ipmap_remove(map, 99));

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_remove_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 1, V1);

	assert_false(ipmap_remove_free(NULL, 99));

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, strdup("V0"));
	ipmap_put(map, 1, V1);

	assert_true(ipmap_remove_free(map, 0));

	assert_false(ipmap_remove_free(map, 99));

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_remove_all__(void **state) {
	assert_int_equal(ipmap_remove_all(NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_remove_all(map), 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	assert_int_equal(ipmap_remove_all(map), 2);

	assert_int_equal(ipmap_size(map), 0);

	ipmap_free(map);
}

static void ipmap_remove_all_free__(void **state) {
	assert_int_equal(ipmap_remove_all_free(NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_remove_all_free(map), 0);

	ipmap_put(map, 0, strdup("to be freed"));
	ipmap_put(map, 1, strdup("to be freed"));

	assert_int_equal(ipmap_remove_all_free(map), 2);

	assert_int_equal(ipmap_size(map), 0);

	ipmap_free(map);
}

static void ipmap_remove_in__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 1, V1);

	assert_int_equal(ipmap_remove_in(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);

	assert_int_equal(ipmap_remove_in(map, NULL), 0);

	assert_int_equal(ipmap_remove_in(NULL, map), 0);

	const struct IPmap *in = ipmap_init();
	ipmap_put(in, 0, V0);
	ipmap_put(in, 2, V2);

	assert_int_equal(ipmap_remove_in(map, in), 2);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(map);
	ipmap_free(in);
	ipmap_free(expected);
}

static void ipmap_remove_in_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 1, V1);

	assert_int_equal(ipmap_remove_in_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, strdup("V0"));
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, strdup("V2"));

	assert_int_equal(ipmap_remove_in_free(map, NULL), 0);

	assert_int_equal(ipmap_remove_in_free(NULL, map), 0);

	const struct IPmap *in = ipmap_init();
	ipmap_put(in, 0, V0);
	ipmap_put(in, 2, V2);

	assert_int_equal(ipmap_remove_in_free(map, in), 2);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(map);
	ipmap_free(in);
	ipmap_free(expected);
}

static void ipmap_it_remove__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 2, V2);
	ipmap_put(expected, 3, V3);

	assert_nul(ipmap_it_remove(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_remove(it));

	const struct IPmap *map = ipmap_init();
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);
	ipmap_put(map, 3, V3);

	it = ipmap_it(map);
	it = ipmap_it_next(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	assert_ptr_equal(ipmap_it_remove(it), V1);

	assert_false(ipmap_contains_key(map, 1));

	it = ipmap_it_next(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	assert_ipmap_equal_ordered(map, expected);

	ipmap_it_free(it);
	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_it_remove_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put(expected, 0, V0);
	ipmap_put(expected, 1, V1);
	ipmap_put(expected, 3, V3);

	assert_false(ipmap_it_remove_free(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_false(ipmap_it_remove_free(it));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .free_val = mock_free, });
	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);
	ipmap_put(map, 2, V2);
	ipmap_put(map, 3, V3);

	it = ipmap_it(map);
	it = ipmap_it_next(it);
	it = ipmap_it_next(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	expect_ptr(mock_free, ptr, V2);

	assert_true(ipmap_it_remove_free(it));

	assert_false(ipmap_contains_key(map, 2));

	it = ipmap_it_next(it);
	assert_int_equal(it->key, 3);
	assert_ptr_equal(it->val, V3);

	assert_nul(ipmap_it_next(it));

	assert_ipmap_equal_ordered(map, expected);

	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_equal__(void **state) {
	assert_ipmap_not_equal(NULL, NULL);

	const struct IPmap *a = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_ipmap_not_equal(a, NULL);
	assert_ipmap_not_equal(NULL, a);

	const struct IPmap *b = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_ipmap_equal(a, b);

	ipmap_put(a, 2, V2);
	ipmap_put(a, 1, V1);
	ipmap_put(a, 0, V0);

	assert_ipmap_not_equal(a, b);

	ipmap_put(b, 0, V0);
	ipmap_put(b, 1, V1);

	assert_ipmap_not_equal(a, b);

	ipmap_put(b, 2, V2);

	assert_ipmap_equal(a, b);

	ipmap_free(a);
	ipmap_free(b);
}

static void ipmap_equal_ordered__(void **state) {
	assert_ipmap_not_equal_ordered(NULL, NULL);

	const struct IPmap *a = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_ipmap_not_equal_ordered(a, NULL);
	assert_ipmap_not_equal_ordered(NULL, a);

	const struct IPmap *b = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_ipmap_equal_ordered(a, b);

	ipmap_put(a, 0, V0);
	ipmap_put(a, 1, V1);
	ipmap_put(a, 2, V2);

	assert_ipmap_not_equal_ordered(a, b);

	ipmap_put(b, 0, V0);
	ipmap_put(b, 1, V1);

	assert_ipmap_not_equal_ordered(a, b);

	ipmap_put(b, 2, V2);

	assert_ipmap_equal_ordered(a, b);

	ipmap_free(a);
	ipmap_free(b);
}

static void ipmap_vals_plist__(void **state) {
	assert_nul(ipmap_vals_plist(NULL));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, .initial = 2, .grow = 1, });

	const struct Plist *list = ipmap_vals_plist(map);
	assert_int_equal(plist_size(list), 0);

	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);
	assert_true(list->params.allow_null_val);

	plist_free(list);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, V2);

	list = ipmap_vals_plist(map);

	const struct Plist *expected = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(expected, V0);
	plist_append(expected, NULL);
	plist_append(expected, V2);

	assert_plist_equal_ordered(list, expected);

	plist_free(list);
	plist_free(expected);
	ipmap_free(map);
}

static void ipmap_vals_plist_clone__(void **state) {
	assert_nul(ipmap_vals_plist_clone(NULL));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	const struct Plist *list = ipmap_vals_plist_clone(map);
	assert_int_equal(plist_size(list), 0);

	plist_free(list);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V5, void*);

	list = ipmap_vals_plist_clone(map);

	const struct Plist *expected = plist_init();
	plist_append_many(expected, V4, V5, NULL);

	assert_plist_equal_ordered(list, expected);

	plist_free(list);
	plist_free(expected);
	ipmap_free(map);
}

static void ipmap_vals_pset__(void **state) {
	assert_nul(ipmap_vals_pset(NULL));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, .initial = 2, .grow = 1, });

	const struct Pset *set = ipmap_vals_pset(map);
	assert_int_equal(pset_size(set), 0);

	assert_int_equal(set->params.initial, 2);
	assert_int_equal(set->params.grow, 1);

	pset_free(set);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 2, V2);
	ipmap_put(map, 3, V2);

	set = ipmap_vals_pset(map);

	const struct Pset *expected = pset_init();
	pset_add_many(expected, V0, V2, NULL);

	assert_pset_equal_ordered(set, expected);

	pset_free(set);
	pset_free(expected);
	ipmap_free(map);
}

static void ipmap_vals_pset_clone__(void **state) {
	assert_nul(ipmap_vals_pset_clone(NULL));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	const struct Pset *set = ipmap_vals_pset_clone(map);
	assert_int_equal(pset_size(set), 0);

	pset_free(set);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V5, void*);

	set = ipmap_vals_pset_clone(map);

	const struct Pset *expected = pset_init();
	pset_add_many(expected, V4, V5, NULL);

	assert_pset_equal_ordered(set, expected);

	pset_free(set);
	pset_free(expected);
	ipmap_free(map);
}

static void ipmap_str__(void **state) {
	const struct IPmapParams params = { .allow_null_val = true, };
	const struct IPmap *map = ipmap_init_with(params);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, NULL);
	ipmap_put(map, 999, V2);

	char *expected = sprintf_alloc(
			"0 = %p\n"
			"1 = (null)\n"
			"999 = %p\n",
			(void*)V0,
			(void*)V2
			);

	char *actual = ipmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	ipmap_free(map);

	assert_nul(ipmap_str(NULL));
}

static void ipmap_size__(void **state) {
	assert_int_equal(ipmap_size(NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_size(map), 0);

	ipmap_put(map, 0, V0);
	ipmap_put(map, 1, V1);

	assert_int_equal(ipmap_size(map), 2);

	ipmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ipmap_clone__),
		TEST(ipmap_clone__params__constructor),

		TEST(ipmap_clone_deep__),

		TEST(ipmap_free__),

		TEST(ipmap_free_vals__),

		TEST(ipmap_it_free__),

		TEST(ipmap_contains_key__),

		TEST(ipmap_contains_val__),

		TEST(ipmap_get__),

		TEST(ipmap_first_key__),

		TEST(ipmap_at__),

		TEST(ipmap_find__empty_filter),
		TEST(ipmap_find__variants),
		TEST(ipmap_find__some_block),
		TEST(ipmap_find__all_block),
		TEST(ipmap_find__none_block),

		TEST(ipmap_it__),

		TEST(ipmap_filter_it__),

		TEST(ipmap_it_next__),

		TEST(ipmap_put__),

		TEST(ipmap_put_clone__),

		TEST(ipmap_put_clone_free__),

		TEST(ipmap_put_free__),

		TEST(ipmap_put_if_absent__),

		TEST(ipmap_put_all__),

		TEST(ipmap_put_all_free__),

		TEST(ipmap_put_all_clone__),

		TEST(ipmap_put_all_clone_free__),

		TEST(ipmap_remove__),

		TEST(ipmap_remove_free__),

		TEST(ipmap_remove_all__),

		TEST(ipmap_remove_all_free__),

		TEST(ipmap_remove_in__),

		TEST(ipmap_remove_in_free__),

		TEST(ipmap_it_remove__),

		TEST(ipmap_it_remove_free__),

		TEST(ipmap_equal__),

		TEST(ipmap_equal_ordered__),

		TEST(ipmap_vals_plist__),

		TEST(ipmap_vals_plist_clone__),

		TEST(ipmap_vals_pset__),

		TEST(ipmap_vals_pset_clone__),

		TEST(ipmap_str__),

		TEST(ipmap_size__),
	};

	return RUN(tests);
}

