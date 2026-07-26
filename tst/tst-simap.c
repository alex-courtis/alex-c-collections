#include "assert-simap.h"
#include "assert-slist.h"
#include "assert-sset.h"
#include "asserts.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "ppmap.h"
#include "slist.h"
#include "sset.h"

#include "simap.h"

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

struct Sset {
	const struct SsetParams params;
	const struct Pset *pset;
};

struct SImap {
	const struct SImapParams params;
	const struct PPmap *ppmap;
};

static bool match_key_a_val_lt_100(const char* const key, const size_t val, const void* const data) {
	return *key == 'a' && val < 100;
}

static bool starts_with_a(const char* const key) {
	return *key == 'a';
}

static bool ne_1(const size_t i) {
	return i != 1;
}

static void simap_clone__(void **state) {
	assert_nul(simap_clone(NULL));

	const struct SImap *map = simap_init();
	simap_put_many(map, "a", 0, "b", 1, NULL);

	const struct SImap *clone = simap_clone(map);

	assert_simap_equal(map, clone);

	const struct SImap *expected = simap_init();
	simap_put_many(expected, "a", 0, "b", 1, NULL);

	assert_simap_equal(clone, expected);

	simap_free(map);
	simap_free(clone);
	simap_free(expected);
}

static void simap_clone__params__constructor(void **state) {
	assert_nul(simap_clone(NULL));

	struct SImapParams params = {
		.case_insensitive_key = true,
		.initial = 99,
		.grow = 1, };
	const struct SImap *map = simap_init_with(params);

	const struct SImap *clone = simap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->ppmap->size, 0);
	assert_int_equal(clone->ppmap->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(clone->ppmap->params.equal_val, equal_stp);
	assert_ptr_equal(clone->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(clone->ppmap->params.alloc_val, clone_size_t_ptr);
	assert_ptr_equal(clone->ppmap->params.free_key, free);
	assert_ptr_equal(clone->ppmap->params.free_val, free);
	assert_ptr_equal(clone->ppmap->params.str_key, str_or_null);
	assert_ptr_equal(clone->ppmap->params.str_val, str_size_t_ptr);
	assert_false(clone->ppmap->params.allow_null_val);

	assert_ptr_equal(clone->params.case_insensitive_key, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	simap_free(map);
	simap_free(clone);
}

static void simap_free__(void **state) {
	simap_free(NULL);
}

static void simap_it_free__(void **state) {
	simap_it_free(NULL);

	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	simap_it_free(it);
}

static void simap_contains_key__(void **state) {
	assert_false(simap_contains_key(NULL, "x"));

	const struct SImap *map = simap_init();

	assert_false(simap_contains_key(map, "x"));

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_true(simap_contains_key(map, "b"));

	assert_false(simap_contains_key(map, "x"));

	simap_free(map);
}

static void simap_contains_key__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });
	simap_put_many(map, "a", 0, NULL);

	assert_true(simap_contains_key(map, "A"));

	simap_free(map);
}

static void simap_contains_val__(void **state) {
	assert_false(simap_contains_val(NULL, 5));

	const struct SImap *map = simap_init();

	assert_false(simap_contains_val(map, 0));

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_true(simap_contains_val(map, 1));

	assert_false(simap_contains_val(map, 5));

	simap_free(map);
}

static void simap_get__(void **state) {
	assert_int_equal(simap_get(NULL, "x"), 0);

	const struct SImap *map = simap_init();

	assert_int_equal(simap_get(map, "x"), 0);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_int_equal(simap_get(map, "b"), 1);

	assert_int_equal(simap_get(map, "x"), 0);

	simap_free(map);
}

static void simap_get__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_int_equal(simap_get(map, "B"), 1);

	simap_free(map);
}

static void simap_get_ptr__(void **state) {
	assert_false(simap_get_ptr(NULL, NULL, "x"));

	size_t i = 99;
	assert_false(simap_get_ptr(&i, NULL, "x"));
	assert_int_equal(i, 0);

	const struct SImap *map = simap_init();

	assert_false(simap_get_ptr(NULL, map, "x"));

	size_t j = 99;
	assert_false(simap_get_ptr(&j, map, "x"));
	assert_int_equal(j, 0);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	size_t k = 99;
	assert_true(simap_get_ptr(&k, map, "b"));
	assert_int_equal(k, 1);

	size_t l = 99;
	assert_false(simap_get_ptr(&l, map, "x"));
	assert_int_equal(l, 0);

	simap_free(map);
}

static void simap_get_ptr__case_sensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });
	simap_put_many(map, "A", 0, "b", 1, "C", 2, NULL);

	size_t i = 99;
	assert_true(simap_get_ptr(&i, map, "B"));
	assert_int_equal(i, 1);

	size_t j = 99;
	assert_true(simap_get_ptr(&j, map, "c"));
	assert_int_equal(j, 2);

	simap_free(map);
}

static void simap_first_key__(void **state) {
	assert_nul(simap_first_key(NULL, 0));

	const struct SImap *map = simap_init();

	assert_nul(simap_first_key(map, 5));

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_str_equal(simap_first_key(map, 1), "b");

	assert_nul(simap_first_key(map, 3));

	simap_free(map);
}

static void simap_at__(void **state) {
	assert_nul(simap_at(NULL, 0).key);
	assert_int_equal(simap_at(NULL, 0).val, 0);

	const struct SImap *map = simap_init();

	assert_nul(simap_at(map, 0).key);
	assert_int_equal(simap_at(NULL, 0).val, 0);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_str_equal(simap_at(map, 1).key, "b");
	assert_int_equal(simap_at(map, 1).val, 1);

	assert_nul(simap_at(map, 3).key);
	assert_int_equal(simap_at(map, 3).val, 0);

	simap_free(map);
}

static void simap_find__empty_filter(void **state) {
	assert_nul(simap_find(NULL, (struct SImapFilter){ 0 }).key);
	assert_int_equal(simap_find(NULL, (struct SImapFilter){ 0 }).val, 0);

	const struct SImap *map = simap_init();

	assert_nul(simap_find(map, (struct SImapFilter){ 0 }).key);
	assert_int_equal(simap_find(map, (struct SImapFilter){ 0 }).val, 0);

	simap_put_many(map, "b", 0, "a", 1, "c", 2, "d", 3, NULL);

	assert_str_equal(simap_find(map, (struct SImapFilter){ 0 }).key, "b");
	assert_int_equal(simap_find(map, (struct SImapFilter){ 0 }).val, 0);

	simap_free(map);
}

static void simap_find__variants(void **state) {
	const struct SImap *map = simap_init();
	simap_put_many(map, "0", 10, "1", 11, "2", 12, NULL);

	// key
	expect_string(mock_pred_s, s, "0"); will_return(mock_pred_s, false);
	expect_string(mock_pred_s, s, "1"); will_return(mock_pred_s, true);

	const struct SImapPair pair_k = simap_find(map, (struct SImapFilter){ .key = mock_pred_s, .data = "x", });
	assert_str_equal(pair_k.key, "1");
	assert_int_equal(pair_k.val, 11);

	// key_data
	expect_string(mock_pred_s_p, s, "0"); expect_string(mock_pred_s_p, p, "x"); will_return(mock_pred_s_p, false);
	expect_string(mock_pred_s_p, s, "1"); expect_string(mock_pred_s_p, p, "x"); will_return(mock_pred_s_p, true);

	const struct SImapPair pair_kd = simap_find(map, (struct SImapFilter){ .key_data = mock_pred_s_p, .data = "x", });
	assert_str_equal(pair_kd.key, "1");
	assert_int_equal(pair_kd.val, 11);

	// val
	expect_int_value(mock_pred_i, i, 10); will_return(mock_pred_i, false);
	expect_int_value(mock_pred_i, i, 11); will_return(mock_pred_i, true);

	const struct SImapPair pair_v = simap_find(map, (struct SImapFilter){ .val = mock_pred_i, .data = "x", });
	assert_str_equal(pair_v.key, "1");
	assert_int_equal(pair_v.val, 11);

	// val_data
	expect_int_value(mock_pred_i_p, i, 10); expect_string(mock_pred_i_p, p, "x"); will_return(mock_pred_i_p, false);
	expect_int_value(mock_pred_i_p, i, 11); expect_string(mock_pred_i_p, p, "x"); will_return(mock_pred_i_p, true);

	const struct SImapPair pair_vd = simap_find(map, (struct SImapFilter){ .val_data = mock_pred_i_p, .data = "x", });
	assert_str_equal(pair_vd.key, "1");
	assert_int_equal(pair_vd.val, 11);

	// key_val
	expect_string(mock_pred_s_i, s, "0"); expect_int_value(mock_pred_s_i, i, 10); will_return(mock_pred_s_i, false);
	expect_string(mock_pred_s_i, s, "1"); expect_int_value(mock_pred_s_i, i, 11); will_return(mock_pred_s_i, true);

	const struct SImapPair pair_kv = simap_find(map, (struct SImapFilter){ .key_val = mock_pred_s_i, .data = "x", });
	assert_str_equal(pair_kv.key, "1");
	assert_int_equal(pair_kv.val, 11);

	// key_val_data
	expect_string(mock_pred_s_i_p, s, "0"); expect_int_value(mock_pred_s_i_p, i, 10); expect_string(mock_pred_s_i_p, p, "x"); will_return(mock_pred_s_i_p, false);
	expect_string(mock_pred_s_i_p, s, "1"); expect_int_value(mock_pred_s_i_p, i, 11); expect_string(mock_pred_s_i_p, p, "x"); will_return(mock_pred_s_i_p, true);

	const struct SImapPair pair_kvd = simap_find(map, (struct SImapFilter){ .key_val_data = mock_pred_s_i_p, .data = "x", });
	assert_str_equal(pair_kvd.key, "1");
	assert_int_equal(pair_kvd.val, 11);

	simap_free(map);
}

static void simap_find__some_block(void **state) {
	const struct SImap *map = simap_init();

	simap_put_many(map, "b0", 0, "a1", 1, "a2", 2, NULL);

	const struct SImapPair pair = simap_find(map, (struct SImapFilter){ .key = starts_with_a, .val = ne_1, });
	assert_str_equal(pair.key, "a2");
	assert_int_equal(pair.val, 2);

	simap_free(map);
}

static void simap_find__all_block(void **state) {
	const struct SImap *map = simap_init();
	simap_put_many(map, "a0", 1, "a1", 1, "a2", 1, NULL);

	const struct SImapPair pair = simap_find(map, (struct SImapFilter){ .key = starts_with_a, .val = ne_1, });
	assert_nul(pair.key);
	assert_int_equal(pair.val, 0);

	simap_free(map);
}

static void simap_find__none_block(void **state) {
	const struct SImap *map = simap_init();
	simap_put_many(map, "a0", 0, "a1", 0, "a2", 0, NULL);

	const struct SImapPair pair = simap_find(map, (struct SImapFilter){ .key = starts_with_a, .val = ne_1, });
	assert_str_equal(pair.key, "a0");
	assert_int_equal(pair.val, 0);

	simap_free(map);
}

static void simap_it__(void **state) {
	assert_nul(simap_it(NULL));

	const struct SImap *map = simap_init();

	assert_nul(simap_it(map));

	simap_put_many(map, "a", 0, "b", 1, NULL);

	const struct SImapIt *it = simap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_int_equal(it->val, 0);

	it = simap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_int_equal(it->val, 1);

	assert_nul(simap_it_next(it));

	simap_free(map);
}

static void simap_filter_it__(void **state) {
	assert_nul(simap_filter_it(NULL, (struct SImapFilter){ 0 }));

	const struct SImap *map = simap_init();

	assert_nul(simap_filter_it(map, (struct SImapFilter){ 0 }));

	simap_put_many(map,
			"ak0", 100,
			"ak1", 11,
			"bk2", 12,
			"ak3", 13,
			"ak4", 101,
			NULL);

	const struct SImapFilter filter = { .key_val_data = match_key_a_val_lt_100, };
	const struct SImapIt *it = simap_filter_it(map, filter);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak1");
	assert_int_equal(it->val, 11);

	it = simap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "ak3");
	assert_int_equal(it->val, 13);

	assert_nul(simap_it_next(it));

	simap_free(map);
}

static void simap_it_next__(void **state) {
	assert_nul(simap_it_next(NULL));

	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	assert_nul(simap_it_next(it));
}

static void simap_put__(void **state) {
	assert_false(simap_put(NULL, "a", 0));

	const struct SImap *map = simap_init();

	assert_false(simap_put(map, "a", 0));

	assert_true(simap_put(map, "a", 1));

	assert_int_equal(simap_size(map), 1);

	assert_int_equal(simap_get(map, "a"), 1);

	simap_free(map);
}

static void simap_put__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });

	assert_false(simap_put(map, "a", 0));

	assert_true(simap_put(map, "A", 1));

	assert_int_equal(simap_get(map, "a"), 1);

	simap_free(map);
}

static void simap_put_if_absent__(void **state) {
	assert_false(simap_put_if_absent(NULL, "a", 0));

	const struct SImap *map = simap_init();

	assert_false(simap_put_if_absent(map, "a", 0));

	assert_true(simap_put_if_absent(map, "a", 1));

	assert_int_equal(simap_size(map), 1);

	assert_int_equal(simap_get(map, "a"), 0);

	assert_false(simap_put_if_absent(map, "b", 2));

	assert_int_equal(simap_get(map, "b"), 2);

	simap_free(map);
}

static void simap_put_if_absent__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });

	assert_false(simap_put_if_absent(map, "a", 0));

	assert_true(simap_put_if_absent(map, "A", 1));

	assert_int_equal(simap_get(map, "a"), 0);

	simap_free(map);
}

static void simap_put_all__(void **state) {
	assert_int_equal(simap_put_all(NULL, NULL), 0);

	const struct SImap *map = simap_init();

	assert_int_equal(simap_put_all(NULL, map), 0);
	assert_int_equal(simap_put_all(map, NULL), 0);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	const struct SImap *from = simap_init();

	simap_put_many(from, "a", 0, "c", 4, "d", 5, NULL);

	assert_int_equal(simap_put_all(map, from), 2);

	const struct SImap *expected = simap_init();
	simap_put_many(expected, "a", 0, "b", 1, "c", 4, "d", 5, NULL);

	assert_simap_equal(map, expected);

	simap_free(expected);
	simap_free(from);
	simap_free(map);
}

static void simap_put_all__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	const struct SImap *from = simap_init();

	simap_put_many(from, "A", 0, "C", 4, "D", 5, NULL);

	assert_int_equal(simap_put_all(map, from), 2);

	const struct SImap *expected = simap_init();
	simap_put_many(expected, "a", 0, "b", 1, "c", 4, "d", 5, NULL);

	assert_simap_equal(map, expected);

	simap_free(expected);
	simap_free(from);
	simap_free(map);
}

static void simap_remove__(void **state) {
	const struct SImap *expected = simap_init();
	simap_put_many(expected, "B", 1, NULL);

	assert_false(simap_remove(NULL, "x"));

	const struct SImap *map = simap_init();
	simap_put_many(map, "A", 0, "B", 1, NULL);

	assert_true(simap_remove(map, "A"));

	assert_false(simap_remove(map, NULL));

	assert_false(simap_remove(map, "x"));

	assert_simap_equal(map, expected);

	simap_free(expected);
	simap_free(map);
}

static void simap_remove__case_insensitive(void **state) {
	const struct SImap *expected = simap_init();
	simap_put_many(expected, "B", 1, NULL);

	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });
	simap_put_many(map, "A", 0, "B", 1, NULL);

	assert_true(simap_remove(map, "a"));

	assert_simap_equal(map, expected);

	simap_free(expected);
	simap_free(map);
}

static void simap_remove_all__(void **state) {
	assert_int_equal(simap_remove_all(NULL), 0);

	const struct SImap *map = simap_init();

	assert_int_equal(simap_remove_all(map), 0);

	simap_put_many(map, "a", 0, "b", 1, NULL);

	assert_int_equal(simap_remove_all(map), 2);

	assert_int_equal(simap_size(map), 0);

	simap_free(map);
}

static void simap_remove_in__(void **state) {
	const struct SImap *expected = simap_init();
	simap_put_many(expected, "b", 1, NULL);

	assert_int_equal(simap_remove_in(NULL, NULL), 0);

	const struct SImap *map = simap_init();
	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_int_equal(simap_remove_in(map, NULL), 0);

	assert_int_equal(simap_remove_in(NULL, map), 0);

	const struct SImap *in = simap_init();
	simap_put_many(in, "a", 0, "c", 2, "d", 4, NULL);

	assert_int_equal(simap_remove_in(map, in), 2);

	assert_simap_equal(map, expected);

	simap_free(map);
	simap_free(in);
	simap_free(expected);
}

static void simap_remove_in__case_insensitive(void **state) {
	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });
	simap_put_many(map, "A", 0, "B", 1, NULL);

	const struct SImap *in = simap_init();
	simap_put_many(in, "B", 1, "C", 2, NULL);

	assert_int_equal(simap_remove_in(map, in), 1);

	const struct SImap *expected = simap_init();
	simap_put_many(expected, "a", 0, NULL);

	assert_simap_equal(map, expected);

	simap_free(map);
	simap_free(expected);
	simap_free(in);
}

static void simap_it_remove__(void **state) {
	const struct SImap *expected = simap_init();
	simap_put_many(expected, "a", 0, "c", 2, "d", 3, "e", 4, NULL);

	assert_false(simap_it_remove(NULL));

	const struct SImapIt *it = calloc(1, sizeof(struct SImapIt));

	assert_false(simap_it_remove(it));

	const struct SImap *map = simap_init();
	simap_put_many(map, "a", 0, "b", 1, "c", 2, "d", 3, "e", 4, NULL);

	it = simap_it(map);
	it = simap_it_next(it);
	assert_str_equal(it->key, "b");
	assert_int_equal(it->val, 1);

	assert_true(simap_it_remove(it));

	assert_false(simap_contains_key(map, "b"));

	it = simap_it_next(it);
	assert_str_equal(it->key, "c");
	assert_int_equal(it->val, 2);

	assert_simap_equal(map, expected);

	simap_it_free(it);
	simap_free(expected);
	simap_free(map);
}

static void simap_equal__(void **state) {
	assert_false(simap_equal(NULL, NULL));

	const struct SImap *a = simap_init();

	assert_false(simap_equal(a, NULL));
	assert_false(simap_equal(NULL, a));

	const struct SImap *b = simap_init();

	assert_simap_equal(a, b);

	simap_put_many(a, "a", 0, NULL);

	assert_simap_not_equal(a, b);

	simap_put_many(b, "a", 0, NULL);

	assert_simap_equal(a, b);

	simap_free(a);
	simap_free(b);
}

static void simap_equal__case_insensitive(void **state) {
	const struct SImap *a = simap_init_with((struct SImapParams){ .case_insensitive_key = true, });
	simap_put_many(a, "a", 0, "b", 1, "c", 2, NULL);

	const struct SImap *b = simap_init();
	simap_put_many(b, "a", 0, "B", 1, "c", 2, NULL);

	assert_simap_equal(a, b);

	simap_free(a);
	simap_free(b);
}

static void simap_keys_slist__(void **state) {
	assert_nul(simap_keys_slist(NULL));

	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Slist *list = simap_keys_slist(map);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	list = simap_keys_slist(map);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "A", "b", "c", NULL);

	assert_slist_equal(list, expected);

	slist_free(list);
	slist_free(expected);
	simap_free(map);
}

static void simap_keys_sset__(void **state) {
	assert_nul(simap_keys_sset(NULL));

	const struct SImap *map = simap_init_with((struct SImapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Sset *set = simap_keys_sset(map);
	assert_int_equal(sset_size(set), 0);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 2);
	assert_int_equal(set->params.grow, 1);

	sset_free(set);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	set = simap_keys_sset(map);

	const struct Sset *expected = sset_init();
	sset_add_many(expected, "A", "b", "c", NULL);

	assert_sset_equal_ordered(set, expected);

	sset_free(set);
	sset_free(expected);
	simap_free(map);
}

static void simap_str__(void **state) {
	assert_nul(simap_str(NULL));

	const struct SImap *map = simap_init();
	simap_put_many(map,
			"a", 10,
			"b", 11,
			"c", 12,
			"x", 0,
			NULL);

	char *actual = simap_str(map);

	assert_str_equal(actual,
			"a = 10\n"
			"b = 11\n"
			"c = 12\n"
			"x = 0\n"
			);

	free(actual);
	simap_free(map);
}

static void simap_size__(void **state) {
	assert_int_equal(simap_size(NULL), 0);

	const struct SImap *map = simap_init();

	assert_int_equal(simap_size(map), 0);

	simap_put_many(map, "a", 0, "b", 1, "c", 2, NULL);

	assert_int_equal(simap_size(map), 3);

	simap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(simap_clone__),
		TEST(simap_clone__params__constructor),

		TEST(simap_free__),

		TEST(simap_it_free__),

		TEST(simap_contains_key__),
		TEST(simap_contains_key__case_insensitive),

		TEST(simap_contains_val__),

		TEST(simap_get__),
		TEST(simap_get__case_insensitive),

		TEST(simap_get_ptr__),
		TEST(simap_get_ptr__case_sensitive),

		TEST(simap_first_key__),

		TEST(simap_at__),

		TEST(simap_find__empty_filter),
		TEST(simap_find__variants),
		TEST(simap_find__some_block),
		TEST(simap_find__all_block),
		TEST(simap_find__none_block),

		TEST(simap_it__),

		TEST(simap_filter_it__),

		TEST(simap_it_next__),

		TEST(simap_put__),
		TEST(simap_put__case_insensitive),

		TEST(simap_put_if_absent__),
		TEST(simap_put_if_absent__case_insensitive),

		TEST(simap_put_all__),
		TEST(simap_put_all__case_insensitive),

		TEST(simap_remove__),
		TEST(simap_remove__case_insensitive),

		TEST(simap_remove_all__),

		TEST(simap_remove_in__),
		TEST(simap_remove_in__case_insensitive),

		TEST(simap_it_remove__),

		TEST(simap_equal__),
		TEST(simap_equal__case_insensitive),

		TEST(simap_keys_slist__),

		TEST(simap_keys_sset__),

		TEST(simap_str__),

		TEST(simap_size__),
	};

	return RUN(tests);
}

