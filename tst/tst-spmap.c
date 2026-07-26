#include "assert-plist.h"
#include "assert-slist.h"
#include "assert-spmap.h"
#include "assert-sset.h"
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
#include "slist.h"
#include "sset.h"
#include "str.h"

#include "spmap.h"

struct PPmap {
	const struct PPmapParams params;
	const void **keys;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Slist {
	const struct SlistParams params;
	const struct Plist *plist;
};

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

struct SPmap {
	const struct SPmapParams params;
	const struct PPmap *ppmap;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void spmap_clone__(void **state) {
	assert_nul(spmap_clone(NULL));

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "a", V0, "b", V1, NULL);

	const struct SPmap *clone = spmap_clone(map);

	assert_spmap_equal(map, clone);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V1, NULL);

	assert_spmap_equal(clone, expected);

	spmap_free(map);
	spmap_free(clone);
	spmap_free(expected);
}

static void spmap_clone__params__constructor(void **state) {
	assert_nul(spmap_clone(NULL));

	struct SPmapParams params = {
		.case_insensitive_key = true,
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.free_val = mock_free,
		.clone_val = mock_clone ,
		.str_val = mock_str,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1, };
	const struct SPmap *map = spmap_init_with(params);

	const struct SPmap *clone = spmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->ppmap->size, 0);
	assert_int_equal(clone->ppmap->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(clone->ppmap->params.equal_val, mock_equal);
	assert_ptr_equal(clone->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(clone->ppmap->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->ppmap->params.free_key, free);
	assert_ptr_equal(clone->ppmap->params.free_val, mock_free);
	assert_ptr_equal(clone->ppmap->params.clone_val, mock_clone);
	assert_ptr_equal(clone->ppmap->params.str_key, str_or_null);
	assert_ptr_equal(clone->ppmap->params.str_val, mock_str);
	assert_true(clone->ppmap->params.allow_null_val);

	assert_ptr_equal(clone->params.equal_val, mock_equal);
	assert_ptr_equal(clone->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->params.free_val, mock_free);
	assert_ptr_equal(clone->params.clone_val, mock_clone);
	assert_ptr_equal(clone->params.str_val, mock_str);
	assert_ptr_equal(clone->params.case_insensitive_key, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	spmap_free(map);
	spmap_free(clone);
}

static void spmap_clone_deep__(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .clone_val = mock_clone, });
	spmap_put_many(map, "a", V0, "b", V1, NULL);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);

	const struct SPmap *clone = spmap_clone_deep(map);

	assert_spmap_equal(map, clone);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V1, NULL);

	assert_spmap_equal(clone, expected);

	spmap_free(map);
	spmap_free(expected);
	spmap_free(clone);
}

static void spmap_free__(void **state) {
	spmap_free(NULL);
}

static void spmap_free_vals__(void **state) {
	spmap_free_vals(NULL);

	const char *val = strdup("no double free");

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .allow_null_val = true, });
	spmap_put_many(map, "a", val, "b", NULL, "c", val, NULL);

	spmap_free_vals(map);
}

static void spmap_it_free__(void **state) {
	spmap_it_free(NULL);

	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	spmap_it_free(it);
}

static void spmap_contains_key__(void **state) {
	assert_false(spmap_contains_key(NULL, "x"));

	const struct SPmap *map = spmap_init();

	assert_false(spmap_contains_key(map, "x"));

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_true(spmap_contains_key(map, "b"));

	assert_false(spmap_contains_key(map, "x"));

	spmap_free(map);
}

static void spmap_contains_key__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });
	spmap_put_many(map, "a", V0, NULL);

	assert_true(spmap_contains_key(map, "A"));

	spmap_free(map);
}

static void spmap_contains_val__(void **state) {
	assert_false(spmap_contains_val(NULL, V5));

	const struct SPmap *map = spmap_init();

	assert_false(spmap_contains_val(map, V0));

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_true(spmap_contains_val(map, V1));

	assert_false(spmap_contains_val(map, V5));

	spmap_free(map);
}

static void spmap_get__(void **state) {
	assert_nul(spmap_get(NULL, "x"));

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_get(map, "x"));

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_ptr_equal(spmap_get(map, "b"), V1);

	assert_nul(spmap_get(map, "x"));

	spmap_free(map);
}

static void spmap_get__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_ptr_equal(spmap_get(map, "B"), V1);

	spmap_free(map);
}

static void spmap_first_key__(void **state) {
	assert_nul(spmap_first_key(NULL, V0));

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_first_key(map, V5));

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_str_equal(spmap_first_key(map, V1), "b");

	assert_nul(spmap_first_key(map, "9"));

	spmap_free(map);
}

static void spmap_at__(void **state) {
	assert_nul(spmap_at(NULL, 0).key);
	assert_nul(spmap_at(NULL, 0).val);

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_at(map, 0).key);
	assert_nul(spmap_at(map, 0).val);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_str_equal(spmap_at(map, 1).key, "b");
	assert_ptr_equal(spmap_at(map, 1).val, V1);

	assert_nul(spmap_at(map, 3).key);
	assert_nul(spmap_at(map, 3).val);

	spmap_free(map);
}

static void spmap_find__(void **state) {
	assert_nul(spmap_find(NULL, (struct SPmapFilter){ 0 }).key);
	assert_nul(spmap_find(NULL, (struct SPmapFilter){ 0 }).val);

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_find(map, (struct SPmapFilter){ 0 }).key);
	assert_nul(spmap_find(map, (struct SPmapFilter){ 0 }).val);

	spmap_put_many(map, "b", V0, "a", V1, "c", V2, "d", V3, NULL);

	assert_str_equal(spmap_find(map, (struct SPmapFilter){ 0 }).key, "b");
	assert_ptr_equal(spmap_find(map, (struct SPmapFilter){ 0 }).val, V0);

	struct SPmapPair pair = spmap_find(map, (struct SPmapFilter){ .key_data = match_starts_with_a, .data = V5, });

	assert_str_equal(pair.key, "a");
	assert_ptr_equal(pair.val, V1);

	pair = spmap_find(map, (struct SPmapFilter){ .val_data = equal_ptr, .data = V3, });

	assert_str_equal(pair.key, "d");
	assert_ptr_equal(pair.val, V3);

	spmap_free(map);
}

static void spmap_it__(void **state) {
	assert_nul(spmap_it(NULL));

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_it(map));

	spmap_put_many(map, "a", V0, "b", V1, NULL);

	const struct SPmapIt *it = spmap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_ptr_equal(it->val, V0);

	it = spmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_ptr_equal(it->val, V1);

	assert_nul(spmap_it_next(it));

	spmap_free(map);
}

static void spmap_filter_it__(void **state) {
	assert_nul(spmap_filter_it(NULL, (struct SPmapFilter){ 0 }));

	const struct SPmap *map = spmap_init();

	assert_nul(spmap_filter_it(map, (struct SPmapFilter){ 0 }));

	spmap_put_many(map, "b0", V0, "b1", V1, "a2", V2, "a3", V3, NULL);

	const struct SPmapIt *it = spmap_filter_it(map, (struct SPmapFilter){ .key_data = match_starts_with_a, .data = V5, });

	assert_non_nul(it);
	assert_str_equal(it->key, "a2");
	assert_ptr_equal(it->val, V2);

	it = spmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "a3");
	assert_ptr_equal(it->val, V3);

	assert_nul(spmap_it_next(it));

	it = spmap_filter_it(map, (struct SPmapFilter){ .val_data = equal_ptr, .data = V3, });

	assert_non_nul(it);
	assert_str_equal(it->key, "a3");
	assert_ptr_equal(it->val, V3);

	assert_nul(spmap_it_next(it));

	spmap_free(map);
}

static void spmap_it_next__(void **state) {
	assert_nul(spmap_it_next(NULL));

	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	assert_nul(spmap_it_next(it));
}

static void spmap_put__(void **state) {
	assert_false(spmap_put(NULL, "a", V0));

	const struct SPmap *map = spmap_init();

	assert_false(spmap_put(map, "a", V0));

	assert_true(spmap_put(map, "a", V1));

	assert_int_equal(spmap_size(map), 1);

	assert_ptr_equal(spmap_get(map, "a"), V1);

	spmap_free(map);
}

static void spmap_put__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });

	assert_false(spmap_put(map, "a", V0));

	assert_true(spmap_put(map, "A", V1));

	assert_ptr_equal(spmap_get(map, "a"), V1);

	spmap_free(map);
}

static void spmap_put_if_absent__(void **state) {
	assert_false(spmap_put_if_absent(NULL, "a", V0));

	const struct SPmap *map = spmap_init();

	assert_false(spmap_put_if_absent(map, "a", V0));

	assert_true(spmap_put_if_absent(map, "a", V1));

	assert_int_equal(spmap_size(map), 1);

	assert_ptr_equal(spmap_get(map, "a"), V0);

	assert_false(spmap_put_if_absent(map, "b", V2));

	assert_ptr_equal(spmap_get(map, "b"), V2);

	spmap_free(map);
}

static void spmap_put_if_absent__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });

	assert_false(spmap_put_if_absent(map, "a", V0));

	assert_true(spmap_put_if_absent(map, "A", V1));

	assert_ptr_equal(spmap_get(map, "a"), V0);

	spmap_free(map);
}

static void spmap_put_free__(void **state) {
	assert_false(spmap_put_free(NULL, "a", V0));

	const struct SPmap *map = spmap_init();

	assert_false(spmap_put_free(map, "a", strdup("to be freed")));

	assert_true(spmap_put_free(map, "a", V1));

	assert_int_equal(spmap_size(map), 1);

	assert_ptr_equal(spmap_get(map, "a"), V1);

	spmap_free(map);
}

static void spmap_put_all__(void **state) {
	assert_int_equal(spmap_put_all(NULL, NULL), 0);

	const struct SPmap *map = spmap_init();

	assert_int_equal(spmap_put_all(NULL, map), 0);
	assert_int_equal(spmap_put_all(map, NULL), 0);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	const struct SPmap *from = spmap_init();

	spmap_put_many(from, "a", V0, "c", V4, "d", V5, NULL);

	assert_int_equal(spmap_put_all(map, from), 2);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V1, "c", V4, "d", V5, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(from);
	spmap_free(map);
}

static void spmap_put_all__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	const struct SPmap *from = spmap_init();

	spmap_put_many(from, "A", V0, "C", V4, "D", V5, NULL);

	assert_int_equal(spmap_put_all(map, from), 2);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V1, "c", V4, "d", V5, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(from);
	spmap_free(map);
}

static void spmap_put_all_free__(void **state) {
	assert_int_equal(spmap_put_all_free(NULL, NULL), 0);

	const struct SPmap *map = spmap_init();

	assert_int_equal(spmap_put_all_free(NULL, map), 0);
	assert_int_equal(spmap_put_all_free(map, NULL), 0);

	spmap_put_many(map, "a", strdup("V0"), "b", V1, "c", strdup("V2"), NULL);

	const struct SPmap *from = spmap_init();

	spmap_put_many(from, "a", V0, "c", V4, "d", V5, NULL);

	assert_int_equal(spmap_put_all_free(map, from), 2);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V1, "c", V4, "d", V5, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(from);
	spmap_free(map);
}

static void spmap_put_all_clone__(void **state) {
	assert_int_equal(spmap_put_all_clone(NULL, NULL), 0);

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(spmap_put_all_clone(NULL, map), 0);
	assert_int_equal(spmap_put_all_clone(map, NULL), 0);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	const struct SPmap *from = spmap_init();

	spmap_put_many(from, "b", V4, "d", V5, NULL);

	// NOP, no clone_val
	assert_int_equal(spmap_put_all_clone(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(spmap_put_all_clone(map, from), 1);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V4, "c", V2, "d", V5, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(from);
	spmap_free(map);
}

static void spmap_put_all_clone_free__(void **state) {
	assert_int_equal(spmap_put_all_clone_free(NULL, NULL), 0);

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .clone_val = mock_clone, });

	assert_int_equal(spmap_put_all_clone_free(NULL, map), 0);
	assert_int_equal(spmap_put_all_clone_free(map, NULL), 0);

	spmap_put_many(map, "a", V0, "b", strdup("V4"), "c", V2, NULL);

	const struct SPmap *from = spmap_init();

	spmap_put_many(from, "b", V4, "d", V5, NULL);

	// NOP, no clone_val
	assert_int_equal(spmap_put_all_clone_free(from, map), 0);

	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V5); will_return_ptr_type(mock_clone, V5, void*);

	assert_int_equal(spmap_put_all_clone_free(map, from), 1);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "b", V4, "c", V2, "d", V5, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(from);
	spmap_free(map);
}

static void spmap_remove__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "B", V1, NULL);

	assert_false(spmap_remove(NULL, "x"));

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "A", V0, "B", V1, NULL);

	assert_true(spmap_remove(map, "A"));

	assert_false(spmap_remove(map, NULL));

	assert_false(spmap_remove(map, "x"));

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(map);
}

static void spmap_remove__case_insensitive(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "B", V1, NULL);

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });
	spmap_put_many(map, "A", V0, "B", V1, NULL);

	assert_true(spmap_remove(map, "a"));

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(map);
}

static void spmap_remove_free__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "B", V1, NULL);

	assert_false(spmap_remove_free(NULL, "x"));

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "A", strdup("V0"), "B", V1, NULL);

	assert_true(spmap_remove_free(map, "A"));

	assert_false(spmap_remove_free(map, NULL));

	assert_false(spmap_remove_free(map, "x"));

	assert_spmap_equal(map, expected);

	spmap_free(expected);
	spmap_free(map);
}

static void spmap_remove_all__(void **state) {
	assert_int_equal(spmap_remove_all(NULL), 0);

	const struct SPmap *map = spmap_init();

	assert_int_equal(spmap_remove_all(map), 0);

	spmap_put_many(map, "a", V0, "b", V1, NULL);

	assert_int_equal(spmap_remove_all(map), 2);

	assert_int_equal(spmap_size(map), 0);

	spmap_free(map);
}

static void spmap_remove_all_free__(void **state) {
	assert_int_equal(spmap_remove_all_free(NULL), 0);

	const struct SPmap *map = spmap_init();

	assert_int_equal(spmap_remove_all_free(map), 0);

	spmap_put_many(map, "a", strdup("V0"), "b", strdup("V0"), NULL);

	assert_int_equal(spmap_remove_all_free(map), 2);

	assert_int_equal(spmap_size(map), 0);

	spmap_free(map);
}

static void spmap_remove_in__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "b", V1, NULL);

	assert_int_equal(spmap_remove_in(NULL, NULL), 0);

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_int_equal(spmap_remove_in(map, NULL), 0);

	assert_int_equal(spmap_remove_in(NULL, map), 0);

	const struct SPmap *in = spmap_init();
	spmap_put_many(in, "a", V0, "c", V2, "d", V4, NULL);

	assert_int_equal(spmap_remove_in(map, in), 2);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(in);
	spmap_free(expected);
}

static void spmap_remove_in__case_insensitive(void **state) {
	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });
	spmap_put_many(map, "A", V0, "B", V1, NULL);

	const struct SPmap *in = spmap_init();
	spmap_put_many(in, "B", V1, "C", V2, NULL);

	assert_int_equal(spmap_remove_in(map, in), 1);

	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, NULL);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(expected);
	spmap_free(in);
}

static void spmap_remove_in_free__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "b", V1, NULL);

	assert_int_equal(spmap_remove_in_free(NULL, NULL), 0);

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "a", strdup("V0"), "b", V1, "c", strdup("V1"), NULL);

	assert_int_equal(spmap_remove_in_free(map, NULL), 0);

	assert_int_equal(spmap_remove_in_free(NULL, map), 0);

	const struct SPmap *in = spmap_init();
	spmap_put_many(in, "a", V0, "c", V2, "d", V4, NULL);

	assert_int_equal(spmap_remove_in_free(map, in), 2);

	assert_spmap_equal(map, expected);

	spmap_free(map);
	spmap_free(in);
	spmap_free(expected);
}

static void spmap_it_remove__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "c", V2, "d", V3, "e", V4, NULL);

	assert_false(spmap_it_remove(NULL));

	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	assert_false(spmap_it_remove(it));

	const struct SPmap *map = spmap_init();
	spmap_put_many(map, "a", V0, "b", V1, "c", V2, "d", V3, "e", V4, NULL);

	it = spmap_it(map);
	it = spmap_it_next(it);
	assert_str_equal(it->key, "b");
	assert_ptr_equal(it->val, V1);

	assert_true(spmap_it_remove(it));

	assert_false(spmap_contains_key(map, "b"));

	it = spmap_it_next(it);
	assert_str_equal(it->key, "c");
	assert_ptr_equal(it->val, V2);

	assert_spmap_equal(map, expected);

	spmap_it_free(it);
	spmap_free(expected);
	spmap_free(map);
}

static void spmap_it_remove_free__(void **state) {
	const struct SPmap *expected = spmap_init();
	spmap_put_many(expected, "a", V0, "c", V2, "d", V3, "e", V4, NULL);

	assert_false(spmap_it_remove_free(NULL));

	const struct SPmapIt *it = calloc(1, sizeof(struct SPmapIt));

	assert_false(spmap_it_remove_free(it));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .free_val = mock_free, });
	spmap_put_many(map, "a", V0, "b", V1, "c", V2, "d", V3, "e", V4, NULL);

	it = spmap_it(map);
	it = spmap_it_next(it);
	assert_str_equal(it->key, "b");
	assert_ptr_equal(it->val, V1);

	expect_ptr(mock_free, ptr, V1);

	assert_true(spmap_it_remove_free(it));

	assert_false(spmap_contains_key(map, "b"));

	it = spmap_it_next(it);
	assert_str_equal(it->key, "c");
	assert_ptr_equal(it->val, V2);

	assert_spmap_equal(map, expected);

	spmap_it_free(it);
	spmap_free(expected);
	spmap_free(map);
}

static void spmap_equal__(void **state) {
	assert_false(spmap_equal(NULL, NULL));

	const struct SPmap *a = spmap_init();

	assert_false(spmap_equal(a, NULL));
	assert_false(spmap_equal(NULL, a));

	const struct SPmap *b = spmap_init();

	assert_spmap_equal(a, b);

	spmap_put_many(a, "a", V0, NULL);

	assert_spmap_not_equal(a, b);

	spmap_put_many(b, "a", V0, NULL);

	assert_spmap_equal(a, b);

	spmap_free(a);
	spmap_free(b);
}

static void spmap_equal__case_insensitive(void **state) {
	const struct SPmap *a = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, });
	spmap_put_many(a, "a", V0, "b", V1, "c", V2, NULL);

	const struct SPmap *b = spmap_init();
	spmap_put_many(b, "a", V0, "B", V1, "c", V2, NULL);

	assert_spmap_equal(a, b);

	spmap_free(a);
	spmap_free(b);
}

static void spmap_keys_slist__(void **state) {
	assert_nul(spmap_keys_slist(NULL));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Slist *list = spmap_keys_slist(map);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	list = spmap_keys_slist(map);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", "b", "c", NULL);

	assert_slist_equal(list, expected);

	slist_free(list);
	slist_free(expected);
	spmap_free(map);
}

static void spmap_keys_sset__(void **state) {
	assert_nul(spmap_keys_sset(NULL));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Sset *set = spmap_keys_sset(map);
	assert_int_equal(sset_size(set), 0);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 2);
	assert_int_equal(set->params.grow, 1);

	sset_free(set);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	set = spmap_keys_sset(map);

	const struct Sset *expected = sset_init();
	sset_add_many(expected, "A", "b", "c", NULL);

	assert_sset_equal(set, expected);

	sset_free(set);
	sset_free(expected);
	spmap_free(map);
}

static void spmap_vals_plist__(void **state) {
	assert_nul(spmap_vals_plist(NULL));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .str_val = mock_str, .initial = 2, .grow = 1, });

	const struct Plist *list = spmap_vals_plist(map);
	assert_int_equal(plist_size(list), 0);

	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);
	assert_ptr_equal(list->params.str_val, mock_str);

	plist_free(list);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	list = spmap_vals_plist(map);

	const struct Plist *expected = plist_init();
	plist_append_many(expected, V0, V1, V2, NULL);

	assert_plist_equal_ordered(list, expected);

	plist_free(list);
	plist_free(expected);
	spmap_free(map);
}

static void spmap_vals_plist_clone__(void **state) {
	assert_nul(spmap_vals_plist_clone(NULL));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .clone_val = mock_clone, });

	const struct Plist *list = spmap_vals_plist_clone(map);
	assert_int_equal(plist_size(list), 0);

	plist_free(list);

	spmap_put_many(map, "a", V0, "b", V1, NULL);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V4, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V5, void*);

	list = spmap_vals_plist_clone(map);

	const struct Plist *expected = plist_init();
	plist_append_many(expected, V4, V5, NULL);

	assert_plist_equal_ordered(list, expected);

	plist_free(list);
	plist_free(expected);
	spmap_free(map);
}

static void spmap_str__(void **state) {
	assert_nul(spmap_str(NULL));

	const struct SPmap *map = spmap_init_with((struct SPmapParams){ .allow_null_val = true, });
	spmap_put_many(map, "a", V0, "b", NULL, "c", V2, NULL);

	char *expected = sprintf_alloc(
			"a = %p\n"
			"b = (null)\n"
			"c = %p\n",
			(void*)V0,
			(void*)V2
			);

	char *actual = spmap_str(map);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	spmap_free(map);
}

static void spmap_size__(void **state) {
	assert_int_equal(spmap_size(NULL), 0);

	const struct SPmap *map = spmap_init();

	assert_int_equal(spmap_size(map), 0);

	spmap_put_many(map, "a", V0, "b", V1, "c", V2, NULL);

	assert_int_equal(spmap_size(map), 3);

	spmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(spmap_clone__),
		TEST(spmap_clone__params__constructor),

		TEST(spmap_clone_deep__),

		TEST(spmap_free__),

		TEST(spmap_free_vals__),

		TEST(spmap_it_free__),

		TEST(spmap_contains_key__),
		TEST(spmap_contains_key__case_insensitive),

		TEST(spmap_contains_val__),

		TEST(spmap_get__),
		TEST(spmap_get__case_insensitive),

		TEST(spmap_first_key__),

		TEST(spmap_at__),

		TEST(spmap_find__),

		TEST(spmap_it__),

		TEST(spmap_filter_it__),

		TEST(spmap_it_next__),

		TEST(spmap_put__),
		TEST(spmap_put__case_insensitive),

		TEST(spmap_put_if_absent__),
		TEST(spmap_put_if_absent__case_insensitive),

		TEST(spmap_put_free__),

		TEST(spmap_put_all__),
		TEST(spmap_put_all__case_insensitive),

		TEST(spmap_put_all_free__),

		TEST(spmap_put_all_clone__),

		TEST(spmap_put_all_clone_free__),

		TEST(spmap_remove__),
		TEST(spmap_remove__case_insensitive),

		TEST(spmap_remove_free__),

		TEST(spmap_remove_all__),

		TEST(spmap_remove_all_free__),

		TEST(spmap_remove_in__),
		TEST(spmap_remove_in__case_insensitive),

		TEST(spmap_remove_in_free__),

		TEST(spmap_it_remove__),

		TEST(spmap_it_remove_free__),

		TEST(spmap_equal__),
		TEST(spmap_equal__case_insensitive),

		TEST(spmap_keys_slist__),

		TEST(spmap_keys_sset__),

		TEST(spmap_vals_plist__),

		TEST(spmap_vals_plist_clone__),

		TEST(spmap_str__),

		TEST(spmap_size__),
	};

	return RUN(tests);
}

