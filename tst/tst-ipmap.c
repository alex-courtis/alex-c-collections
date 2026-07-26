#include "assert-ipmap.h"
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

struct Plist {
	const struct PlistParams params;
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
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	const struct IPmap *clone = ipmap_clone(map);

	assert_ipmap_equal(map, clone);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	assert_ipmap_equal(clone, expected);

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
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V2, void*);

	const struct IPmap *clone = ipmap_clone_deep(map);

	assert_ipmap_equal(map, clone);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	assert_ipmap_equal(clone, expected);

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
	ipmap_put_many(map, (size_t)1, val, (size_t)2, NULL, (size_t)3, val, 0);

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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	assert_true(ipmap_contains_key(map, 1));

	assert_false(ipmap_contains_key(map, 0));

	ipmap_put(map, 0, V3);

	assert_true(ipmap_contains_key(map, 0));

	assert_false(ipmap_contains_key(map, 99));

	ipmap_free(map);
}

static void ipmap_contains_val__(void **state) {
	assert_false(ipmap_contains_val(NULL, V5));

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_contains_val(map, V0));

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	assert_true(ipmap_contains_val(map, V1));

	assert_false(ipmap_contains_val(map, V5));

	ipmap_free(map);
}

static void ipmap_get__(void **state) {
	assert_nul(ipmap_get(NULL, 99));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_get(map, 99));

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	assert_int_equal(ipmap_at(map, 1).key, 2);
	assert_ptr_equal(ipmap_at(map, 1).val, V2);

	assert_int_equal(ipmap_at(map, 3).key, 0);
	assert_nul(ipmap_at(map, 3).val);

	ipmap_free(map);
}

static void ipmap_find__(void **state) {
	assert_int_equal(ipmap_find(NULL, (struct IPmapFilter){ 0 }).key, 0);
	assert_nul(ipmap_find(NULL, (struct IPmapFilter){ 0 }).val);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).key, 0);
	assert_nul(ipmap_find(map, (struct IPmapFilter){ 0 }).val);

	ipmap_put_many(map, (size_t)1, V1, (size_t)98, V2, (size_t)99, V3, (size_t)0);

	assert_int_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).key, 1);
	assert_ptr_equal(ipmap_find(map, (struct IPmapFilter){ 0 }).val, V1);

	struct IPmapPair pair = ipmap_find(map, (struct IPmapFilter){ .key_data = gt_9, .data = V5, });

	assert_int_equal(pair.key, 98);
	assert_ptr_equal(pair.val, V2);

	pair = ipmap_find(map, (struct IPmapFilter){ .val_data = equal_ptr, .data = V3, });

	assert_int_equal(pair.key, 99);
	assert_ptr_equal(pair.val, V3);

	ipmap_free(map);
}

static void ipmap_it__(void **state) {
	assert_nul(ipmap_it(NULL));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_it(map));

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	const struct IPmapIt *it = ipmap_it(map);

	assert_non_nul(it);
	assert_int_equal(it->key, 1);
	assert_ptr_equal(it->val, V1);

	it = ipmap_it_next(it);
	assert_non_nul(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	assert_nul(ipmap_it_next(it));

	ipmap_free(map);
}

static void ipmap_filter_it__(void **state) {
	assert_nul(ipmap_filter_it(NULL, (struct IPmapFilter){ 0 }));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_filter_it(map, (struct IPmapFilter){ 0 }));

	ipmap_put_many(map, (size_t)91, V1, (size_t)2, V2, (size_t)93, V3, (size_t)0);

	const struct IPmapIt *it = ipmap_filter_it(map, (struct IPmapFilter){ .key_data = gt_9, .data = V5, });

	assert_non_nul(it);
	assert_int_equal(it->key, 91);
	assert_ptr_equal(it->val, V1);

	it = ipmap_it_next(it);
	assert_non_nul(it);
	assert_int_equal(it->key, 93);
	assert_ptr_equal(it->val, V3);

	assert_nul(ipmap_it_next(it));

	it = ipmap_filter_it(map, (struct IPmapFilter){ .val_data = equal_ptr, .data = V2, });

	assert_non_nul(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	assert_nul(ipmap_it_next(it));

	ipmap_free(map);
}

static void ipmap_it_next__(void **state) {
	assert_nul(ipmap_it_next(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_next(it));
}

static void ipmap_put__(void **state) {
	assert_nul(ipmap_put(NULL, (size_t)1, V1));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put(map, (size_t)1, V1));

	assert_ptr_equal(ipmap_put(map, (size_t)1, V2), V1);

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 1), V2);

	assert_nul(ipmap_put(map, (size_t)0, V0));

	assert_ptr_equal(ipmap_get(map, 0), V0);

	ipmap_free(map);
}

static void ipmap_put_if_absent__(void **state) {
	assert_nul(ipmap_put_if_absent(NULL, (size_t)1, V1));

	const struct IPmap *map = ipmap_init();

	assert_nul(ipmap_put_if_absent(map, (size_t)1, V1));

	assert_ptr_equal(ipmap_put_if_absent(map, (size_t)1, V2), V1);

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 1), V1);

	assert_nul(ipmap_put_if_absent(map, (size_t)2, V2));

	assert_ptr_equal(ipmap_get(map, 2), V2);

	ipmap_free(map);
}

static void ipmap_put_free__(void **state) {
	assert_false(ipmap_put_free(NULL, (size_t)1, V1));

	const struct IPmap *map = ipmap_init();

	assert_false(ipmap_put_free(map, (size_t)1, strdup("to be freed")));

	assert_true(ipmap_put_free(map, (size_t)1, V1));

	assert_int_equal(ipmap_size(map), 1);

	assert_ptr_equal(ipmap_get(map, 1), V1);

	ipmap_free(map);
}

static void ipmap_put_all__(void **state) {
	assert_int_equal(ipmap_put_all(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_put_all(NULL, map), 0);
	assert_int_equal(ipmap_put_all(map, NULL), 0);

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	const struct IPmap *from = ipmap_init();

	ipmap_put_many(from, (size_t)1, V1, (size_t)3, V4, (size_t)4, V5, (size_t)0);

	assert_int_equal(ipmap_put_all(map, from), 2);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V2, (size_t)3, V4, (size_t)4, V5, (size_t)0);

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_free__(void **state) {
	assert_int_equal(ipmap_put_all_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_put_all_free(NULL, map), 0);
	assert_int_equal(ipmap_put_all_free(map, NULL), 0);

	ipmap_put_many(map, (size_t)1, strdup("V0"), (size_t)2, V2, (size_t)3, strdup("V2"), (size_t)0);

	const struct IPmap *from = ipmap_init();

	ipmap_put_many(from, (size_t)1, V1, (size_t)3, V4, (size_t)4, V5, (size_t)0);

	assert_int_equal(ipmap_put_all_free(map, from), 2);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V2, (size_t)3, V4, (size_t)4, V5, (size_t)0);

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_clone__(void **state) {
	assert_int_equal(ipmap_put_all_clone(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(ipmap_put_all_clone(NULL, map), 0);
	assert_int_equal(ipmap_put_all_clone(map, NULL), 0);

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	const struct IPmap *from = ipmap_init();

	ipmap_put_many(from, (size_t)2, V4, (size_t)4, V5, (size_t)0);

	// NOP, no clone_val
	assert_int_equal(ipmap_put_all_clone(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ipmap_put_all_clone(map, from), 1);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V4, (size_t)3, V3, (size_t)4, V5, (size_t)0);

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_put_all_clone_free__(void **state) {
	assert_int_equal(ipmap_put_all_clone_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(ipmap_put_all_clone_free(NULL, map), 0);
	assert_int_equal(ipmap_put_all_clone_free(map, NULL), 0);

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, strdup("V4"), (size_t)3, V3, (size_t)0);

	const struct IPmap *from = ipmap_init();

	ipmap_put_many(from, (size_t)2, V4, (size_t)4, V5, (size_t)0);

	// NOP, no clone_val
	assert_int_equal(ipmap_put_all_clone_free(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(ipmap_put_all_clone_free(map, from), 1);

	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)2, V4, (size_t)3, V3, (size_t)4, V5, (size_t)0);

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(from);
	ipmap_free(map);
}

static void ipmap_remove__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)2, V2, (size_t)0);

	assert_nul(ipmap_remove(NULL, 99));

	const struct IPmap *map = ipmap_init();
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	assert_ptr_equal(ipmap_remove(map, 1), V1);

	assert_nul(ipmap_remove(map, 99));

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_remove_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)2, V2, (size_t)0);

	assert_false(ipmap_remove_free(NULL, 99));

	const struct IPmap *map = ipmap_init();
	ipmap_put_many(map, (size_t)1, strdup("V1"), (size_t)2, V2, (size_t)0);

	assert_true(ipmap_remove_free(map, 1));

	assert_false(ipmap_remove_free(map, 99));

	assert_ipmap_equal(map, expected);

	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_remove_all__(void **state) {
	assert_int_equal(ipmap_remove_all(NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_remove_all(map), 0);

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	assert_int_equal(ipmap_remove_all(map), 2);

	assert_int_equal(ipmap_size(map), 0);

	ipmap_free(map);
}

static void ipmap_remove_all_free__(void **state) {
	assert_int_equal(ipmap_remove_all_free(NULL), 0);

	const struct IPmap *map = ipmap_init();

	assert_int_equal(ipmap_remove_all_free(map), 0);

	ipmap_put_many(map, (size_t)1, strdup("V0"), (size_t)2, strdup("V0"), (size_t)0);

	assert_int_equal(ipmap_remove_all_free(map), 2);

	assert_int_equal(ipmap_size(map), 0);

	ipmap_free(map);
}

static void ipmap_remove_in__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)2, V2, (size_t)0);

	assert_int_equal(ipmap_remove_in(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	assert_int_equal(ipmap_remove_in(map, NULL), 0);

	assert_int_equal(ipmap_remove_in(NULL, map), 0);

	const struct IPmap *in = ipmap_init();
	ipmap_put_many(in, (size_t)1, V1, (size_t)3, V3, (size_t)4, V4, (size_t)0);

	assert_int_equal(ipmap_remove_in(map, in), 2);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(in);
	ipmap_free(expected);
}

static void ipmap_remove_in_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)2, V2, (size_t)0);

	assert_int_equal(ipmap_remove_in_free(NULL, NULL), 0);

	const struct IPmap *map = ipmap_init();
	ipmap_put_many(map, (size_t)1, strdup("V0"), (size_t)2, V2, (size_t)3, strdup("V1"), (size_t)0);

	assert_int_equal(ipmap_remove_in_free(map, NULL), 0);

	assert_int_equal(ipmap_remove_in_free(NULL, map), 0);

	const struct IPmap *in = ipmap_init();
	ipmap_put_many(in, (size_t)1, V1, (size_t)3, V3, (size_t)4, V4, (size_t)0);

	assert_int_equal(ipmap_remove_in_free(map, in), 2);

	assert_ipmap_equal(map, expected);

	ipmap_free(map);
	ipmap_free(in);
	ipmap_free(expected);
}

static void ipmap_it_remove__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)3, V3, (size_t)4, V3, (size_t)5, V4, (size_t)0);

	assert_nul(ipmap_it_remove(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_nul(ipmap_it_remove(it));

	const struct IPmap *map = ipmap_init();
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)4, V3, (size_t)5, V4, (size_t)0);

	it = ipmap_it(map);
	it = ipmap_it_next(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	assert_ptr_equal(ipmap_it_remove(it), V2);

	assert_false(ipmap_contains_key(map, 2));

	it = ipmap_it_next(it);
	assert_int_equal(it->key, 3);
	assert_ptr_equal(it->val, V3);

	assert_ipmap_equal(map, expected);

	ipmap_it_free(it);
	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_it_remove_free__(void **state) {
	const struct IPmap *expected = ipmap_init();
	ipmap_put_many(expected, (size_t)1, V1, (size_t)3, V3, (size_t)4, V3, (size_t)5, V4, (size_t)0);

	assert_false(ipmap_it_remove_free(NULL));

	const struct IPmapIt *it = calloc(1, sizeof(struct IPmapIt));

	assert_false(ipmap_it_remove_free(it));

	const struct IPmap *map = ipmap_init_with((struct IPmapParams){ .free_val = mock_free, });
	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)4, V3, (size_t)5, V4, (size_t)0);

	it = ipmap_it(map);
	it = ipmap_it_next(it);
	assert_int_equal(it->key, 2);
	assert_ptr_equal(it->val, V2);

	expect_ptr(mock_free, ptr, V2);

	assert_true(ipmap_it_remove_free(it));

	assert_false(ipmap_contains_key(map, 2));

	it = ipmap_it_next(it);
	assert_int_equal(it->key, 3);
	assert_ptr_equal(it->val, V3);

	assert_ipmap_equal(map, expected);

	ipmap_it_free(it);
	ipmap_free(expected);
	ipmap_free(map);
}

static void ipmap_equal__(void **state) {
	assert_false(ipmap_equal(NULL, NULL));

	const struct IPmap *a = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_false(ipmap_equal(a, NULL));
	assert_false(ipmap_equal(NULL, a));

	const struct IPmap *b = ipmap_init_with((struct IPmapParams){ .allow_null_val = true, });

	assert_ipmap_equal(a, b);

	ipmap_put_many(a, (size_t)1, V1, (size_t)2, NULL, (size_t)3, V3, (size_t)0);
	ipmap_put(a, (size_t)0, V5);

	assert_ipmap_not_equal(a, b);

	ipmap_put_many(b, (size_t)1, V1, (size_t)2, NULL, (size_t)3, V3, (size_t)0);

	assert_ipmap_not_equal(a, b);

	ipmap_put(b, (size_t)0, V5);

	assert_ipmap_equal(a, b);

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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, NULL, (size_t)3, V3, (size_t)0);

	list = ipmap_vals_plist(map);

	const struct Plist *expected = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(expected, V1);
	plist_append(expected, NULL);
	plist_append(expected, V3);

	assert_plist_equal(list, expected);

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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)0);

	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V5, void*);

	list = ipmap_vals_plist_clone(map);

	const struct Plist *expected = plist_init();
	plist_append_many(expected, V4, V5, NULL);

	assert_plist_equal(list, expected);

	plist_free(list);
	plist_free(expected);
	ipmap_free(map);
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

	ipmap_put_many(map, (size_t)1, V1, (size_t)2, V2, (size_t)3, V3, (size_t)0);

	assert_int_equal(ipmap_size(map), 3);

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

		TEST(ipmap_find__),

		TEST(ipmap_it__),

		// TODO comprehensive
		TEST(ipmap_filter_it__),

		TEST(ipmap_it_next__),

		TEST(ipmap_put__),

		TEST(ipmap_put_if_absent__),

		TEST(ipmap_put_free__),

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

		TEST(ipmap_vals_plist__),

		TEST(ipmap_vals_plist_clone__),

		TEST(ipmap_str__),

		TEST(ipmap_size__),
	};

	return RUN(tests);
}

