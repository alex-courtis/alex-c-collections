#include "assert-slist.h"
#include "assert-sset.h"
#include "assert-ssmap.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "ppmap.h"
#include "slist.h"
#include "sset.h"

#include "ssmap.h"

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

struct SSmap {
	const struct SSmapParams params;
	const struct PPmap *ppmap;
};

static bool match_starts_with_a(const char* const a, const void* const b) {
	return *a == 'a';
}

static void ssmap_clone__(void **state) {
	assert_nul(ssmap_clone(NULL));

	const struct SSmap *map = ssmap_init();
	ssmap_put_many(map, "a", "0", "b", "1", NULL);

	const struct SSmap *clone = ssmap_clone(map);

	assert_ssmap_equal_ordered(map, clone);

	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", "b", "1", NULL);

	assert_ssmap_equal_ordered(clone, expected);

	ssmap_free(map);
	ssmap_free(clone);
	ssmap_free(expected);
}

static void ssmap_clone__params__constructor(void **state) {
	struct SSmapParams params = {
		.case_insensitive_key = true,
		.case_insensitive_val = true,
		.allow_null_val = true,
		.initial = 99,
		.grow = 1,
	};
	const struct SSmap *map = ssmap_init_with(params);

	const struct SSmap *clone = ssmap_clone(map);

	assert_non_nul(clone);
	assert_int_equal(clone->ppmap->size, 0);
	assert_int_equal(clone->ppmap->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->ppmap->params.equal_key, equal_strcasecmp);
	assert_ptr_equal(clone->ppmap->params.equal_val, equal_strcasecmp);
	assert_ptr_equal(clone->ppmap->params.alloc_key, clone_strdup);
	assert_ptr_equal(clone->ppmap->params.alloc_val, clone_strdup);
	assert_ptr_equal(clone->ppmap->params.free_key, free);
	assert_ptr_equal(clone->ppmap->params.free_val, free);
	assert_ptr_equal(clone->ppmap->params.clone_val, NULL);
	assert_ptr_equal(clone->ppmap->params.str_key, str_or_null);
	assert_ptr_equal(clone->ppmap->params.str_val, str_or_null);
	assert_true(clone->ppmap->params.allow_null_val);

	assert_ptr_equal(clone->params.case_insensitive_key, true);
	assert_ptr_equal(clone->params.case_insensitive_val, true);
	assert_ptr_equal(clone->params.allow_null_val, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	ssmap_free(map);
	ssmap_free(clone);
}

static void ssmap_free__(void **state) {
	ssmap_free(NULL);
}

static void ssmap_it_free__(void **state) {
	ssmap_it_free(NULL);

	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	ssmap_it_free(it);
}

static void ssmap_contains_key__(void **state) {
	assert_false(ssmap_contains_key(NULL, "x"));

	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_contains_key(map, "x"));

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_true(ssmap_contains_key(map, "b"));

	assert_false(ssmap_contains_key(map, "x"));

	ssmap_free(map);
}

static void ssmap_contains_key__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(map, "a", "0", NULL);

	assert_true(ssmap_contains_key(map, "A"));

	ssmap_free(map);
}

static void ssmap_contains_val__(void **state) {
	assert_false(ssmap_contains_val(NULL, "9"));

	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_contains_val(map, "9"));

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_true(ssmap_contains_val(map, "1"));

	assert_false(ssmap_contains_val(map, "9"));

	ssmap_free(map);
}

static void ssmap_contains_val__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });
	ssmap_put_many(map, "a", "aa", NULL);

	assert_true(ssmap_contains_val(map, "AA"));

	ssmap_free(map);
}

static void ssmap_get__(void **state) {
	assert_nul(ssmap_get(NULL, "x"));

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_get(map, "x"));

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_str_equal(ssmap_get(map, "b"), "1");

	assert_nul(ssmap_get(map, "x"));

	ssmap_free(map);
}

static void ssmap_get__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_str_equal(ssmap_get(map, "B"), "1");

	ssmap_free(map);
}

static void ssmap_first_key__(void **state) {
	assert_nul(ssmap_first_key(NULL, "0"));

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_first_key(map, "x"));

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_str_equal(ssmap_first_key(map, "1"), "b");

	assert_nul(ssmap_first_key(map, "9"));

	ssmap_free(map);
}

static void ssmap_first_key__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });

	ssmap_put_many(map, "a", "AA", "b", "BB", "c", "CC", NULL);

	assert_str_equal(ssmap_first_key(map, "BB"), "b");

	ssmap_free(map);
}

static void ssmap_at__(void **state) {
	assert_nul(ssmap_at(NULL, 0).key);
	assert_nul(ssmap_at(NULL, 0).val);

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_at(map, 0).key);
	assert_nul(ssmap_at(map, 0).val);

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_str_equal(ssmap_at(map, 1).key, "b");
	assert_str_equal(ssmap_at(map, 1).val, "1");

	assert_nul(ssmap_at(map, 3).key);
	assert_nul(ssmap_at(map, 3).val);

	ssmap_free(map);
}

static void ssmap_find__(void **state) {
	assert_nul(ssmap_find(NULL, (struct SSmapFilter){ 0 }).key);
	assert_nul(ssmap_find(NULL, (struct SSmapFilter){ 0 }).val);

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_find(map, (struct SSmapFilter){ 0 }).key);
	assert_nul(ssmap_find(map, (struct SSmapFilter){ 0 }).val);

	ssmap_put_many(map, "a", "x0", "b", "x1", "c", "a2", "d", "x3", NULL);

	assert_str_equal(ssmap_find(map, (struct SSmapFilter){ 0 }).key, "a");
	assert_str_equal(ssmap_find(map, (struct SSmapFilter){ 0 }).val, "x0");

	struct SSmapPair pair = ssmap_find(map, (struct SSmapFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_str_equal(pair.key, "c");
	assert_str_equal(pair.val, "a2");

	pair = ssmap_find(map, (struct SSmapFilter){ .key_data = match_starts_with_a, .data = "x", });

	assert_str_equal(pair.key, "a");
	assert_str_equal(pair.val, "x0");

	ssmap_free(map);
}

static void ssmap_it__(void **state) {
	assert_nul(ssmap_it(NULL));

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_it(map));

	ssmap_put_many(map, "a", "0", "b", "1", NULL);

	const struct SSmapIt *it = ssmap_it(map);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "0");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "1");

	assert_nul(ssmap_it_next(it));

	ssmap_free(map);
}

static void ssmap_filter_it__(void **state) {
	assert_nul(ssmap_filter_it(NULL, (struct SSmapFilter){ 0 }));

	const struct SSmap *map = ssmap_init();

	assert_nul(ssmap_filter_it(map, (struct SSmapFilter){ 0 }));

	ssmap_put_many(map, "a", "x0", "b", "a1", "c", "a2", "d", "x3", NULL);

	const struct SSmapIt *it = ssmap_filter_it(map, (struct SSmapFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "a1");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "c");
	assert_str_equal(it->val, "a2");

	assert_nul(ssmap_it_next(it));

	it = ssmap_filter_it(map, (struct SSmapFilter){ .key_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "x0");

	assert_nul(ssmap_it_next(it));

	ssmap_free(map);
}

static void ssmap_it_next__(void **state) {
	assert_nul(ssmap_it_next(NULL));

	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	assert_nul(ssmap_it_next(it));
}

static void ssmap_put__(void **state) {
	assert_false(ssmap_put(NULL, "a", "0"));

	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put(map, "a", "0"));

	assert_true(ssmap_put(map, "a", "1"));

	assert_int_equal(ssmap_size(map), 1);

	assert_str_equal(ssmap_get(map, "a"), "1");

	ssmap_free(map);
}

static void ssmap_put__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });

	assert_false(ssmap_put(map, "a", "0"));

	assert_true(ssmap_put(map, "A", "1"));

	assert_str_equal(ssmap_get(map, "a"), "1");

	ssmap_free(map);
}

static void ssmap_put_if_absent__(void **state) {
	assert_false(ssmap_put_if_absent(NULL, "a", "0"));

	const struct SSmap *map = ssmap_init();

	assert_false(ssmap_put_if_absent(map, "a", "0"));

	assert_true(ssmap_put_if_absent(map, "a", "1"));

	assert_int_equal(ssmap_size(map), 1);

	assert_str_equal(ssmap_get(map, "a"), "0");

	assert_false(ssmap_put_if_absent(map, "b", "2"));

	assert_str_equal(ssmap_get(map, "b"), "2");

	ssmap_free(map);
}

static void ssmap_put_if_absent__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });

	assert_false(ssmap_put_if_absent(map, "a", "0"));

	assert_true(ssmap_put_if_absent(map, "A", "1"));

	assert_str_equal(ssmap_get(map, "a"), "0");

	ssmap_free(map);
}

static void ssmap_put_all__(void **state) {
	assert_int_equal(ssmap_put_all(NULL, NULL), 0);

	const struct SSmap *map = ssmap_init();

	assert_int_equal(ssmap_put_all(NULL, map), 0);
	assert_int_equal(ssmap_put_all(map, NULL), 0);

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	const struct SSmap *from = ssmap_init();

	ssmap_put_many(from, "a", "0", "c", "20", "d", "3", NULL);

	assert_int_equal(ssmap_put_all(map, from), 2);

	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", "b", "1", "c", "20", "d", "3", NULL);

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(expected);
	ssmap_free(from);
	ssmap_free(map);
}

static void ssmap_put_all__case_insensitive(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	const struct SSmap *from = ssmap_init();

	ssmap_put_many(from, "A", "0", "C", "20", "D", "3", NULL);

	assert_int_equal(ssmap_put_all(map, from), 2);

	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", "b", "1", "c", "20", "d", "3", NULL);

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(expected);
	ssmap_free(from);
	ssmap_free(map);
}

static void ssmap_remove__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "B", "1", NULL);

	assert_false(ssmap_remove(NULL, "x"));

	const struct SSmap *map = ssmap_init();
	ssmap_put_many(map, "A", "0", "B", "1", NULL);

	assert_true(ssmap_remove(map, "A"));

	assert_false(ssmap_remove(map, NULL));

	assert_false(ssmap_remove(map, "x"));

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(expected);
	ssmap_free(map);
}

static void ssmap_remove__case_insensitive_key(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "B", "1", NULL);

	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(map, "A", "0", "B", "1", NULL);

	assert_true(ssmap_remove(map, "a"));

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(expected);
	ssmap_free(map);
}

static void ssmap_remove_all__(void **state) {
	assert_int_equal(ssmap_remove_all(NULL), 0);

	const struct SSmap *map = ssmap_init();

	assert_int_equal(ssmap_remove_all(map), 0);

	ssmap_put_many(map, "a", "0", "b", "1", NULL);

	assert_int_equal(ssmap_remove_all(map), 2);

	assert_int_equal(ssmap_size(map), 0);

	ssmap_free(map);
}

static void ssmap_remove_in__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "b", "1", NULL);

	assert_int_equal(ssmap_remove_in(NULL, NULL), 0);

	const struct SSmap *map = ssmap_init();
	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_int_equal(ssmap_remove_in(map, NULL), 0);

	assert_int_equal(ssmap_remove_in(NULL, map), 0);

	const struct SSmap *in = ssmap_init();
	ssmap_put_many(in, "a", "0", "c", "2", "d", "3", NULL);

	assert_int_equal(ssmap_remove_in(map, in), 2);

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(map);
	ssmap_free(in);
	ssmap_free(expected);
}

static void ssmap_remove_in__case_insensitive_key(void **state) {
	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(map, "A", "0", "B", "1", NULL);

	const struct SSmap *in = ssmap_init();
	ssmap_put_many(in, "B", "1", "C", "2", NULL);

	assert_int_equal(ssmap_remove_in(map, in), 1);

	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", NULL);

	assert_ssmap_equal_ordered(map, expected);

	ssmap_free(map);
	ssmap_free(expected);
	ssmap_free(in);
}

static void ssmap_it_remove__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", "c", "2", "d", "3", "e", "4", NULL);

	assert_false(ssmap_it_remove(NULL));

	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	assert_false(ssmap_it_remove(it));

	const struct SSmap *map = ssmap_init();
	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", "d", "3", "e", "4", NULL);

	it = ssmap_it(map);
	it = ssmap_it_next(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "1");

	assert_true(ssmap_it_remove(it));

	assert_false(ssmap_contains_key(map, "b"));

	it = ssmap_it_next(it);
	assert_str_equal(it->key, "c");
	assert_str_equal(it->val, "2");

	assert_ssmap_equal_ordered(map, expected);

	ssmap_it_free(it);
	ssmap_free(expected);
	ssmap_free(map);
}

static void ssmap_equal__(void **state) {
	assert_ssmap_not_equal(NULL, NULL);

	const struct SSmap *a = ssmap_init();

	assert_ssmap_not_equal(a, NULL);
	assert_ssmap_not_equal(NULL, a);

	const struct SSmap *b = ssmap_init();

	assert_ssmap_equal(a, b);

	ssmap_put_many(a, "a", "0", "b", "1", NULL);

	assert_ssmap_not_equal(a, b);

	ssmap_put_many(b, "b", "1", "a", "0", NULL);

	assert_ssmap_equal(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal__case_insensitive_key(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(a, "a", "0", "b", "1", "c", "2", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "c", "2", "a", "0", "B", "1", NULL);

	assert_ssmap_equal(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal__case_insensitive_val(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });
	ssmap_put_many(a, "a", "AA", "b", "1", "c", "cc", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "a", "aa", "c", "CC", "b", "1", NULL);

	assert_ssmap_equal(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal_ordered__(void **state) {
	assert_ssmap_not_equal_ordered(NULL, NULL);

	const struct SSmap *a = ssmap_init();

	assert_ssmap_not_equal_ordered(a, NULL);
	assert_ssmap_not_equal_ordered(NULL, a);

	const struct SSmap *b = ssmap_init();

	assert_ssmap_equal_ordered(a, b);

	ssmap_put_many(a, "a", "0", NULL);

	assert_ssmap_not_equal_ordered(a, b);

	ssmap_put_many(b, "a", "0", NULL);

	assert_ssmap_equal_ordered(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal_ordered__case_insensitive_key(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(a, "a", "0", "b", "1", "c", "2", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "a", "0", "B", "1", "c", "2", NULL);

	assert_ssmap_equal_ordered(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal_ordered__case_insensitive_val(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });
	ssmap_put_many(a, "a", "AA", "b", "1", "c", "cc", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "a", "aa", "b", "1", "c", "CC", NULL);

	assert_ssmap_equal_ordered(a, b);

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_keys_slist__(void **state) {
	assert_nul(ssmap_keys_slist(NULL));

	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Slist *list = ssmap_keys_slist(map);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	list = ssmap_keys_slist(map);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "a", "b", "c", NULL);

	assert_slist_equal(list, expected);

	slist_free(list);
	slist_free(expected);
	ssmap_free(map);
}

static void ssmap_keys_sset__(void **state) {
	assert_nul(ssmap_keys_sset(NULL));

	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Sset *set = ssmap_keys_sset(map);
	assert_int_equal(sset_size(set), 0);

	assert_true(set->params.case_insensitive);
	assert_int_equal(set->params.initial, 2);
	assert_int_equal(set->params.grow, 1);

	sset_free(set);

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	set = ssmap_keys_sset(map);

	const struct Sset *expected = sset_init();
	sset_add_many(expected, "A", "b", "c", NULL);

	assert_sset_equal_ordered(set, expected);

	sset_free(set);
	sset_free(expected);
	ssmap_free(map);
}

static void ssmap_vals_slist__(void **state) {
	assert_nul(ssmap_vals_slist(NULL));

	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, .initial = 2, .grow = 1, });

	const struct Slist *list = ssmap_vals_slist(map);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	ssmap_put_many(map, "a", "aa", "b", "1", "c", "2", NULL);

	list = ssmap_vals_slist(map);

	const struct Slist *expected = slist_init();
	slist_append_many(expected, "AA", "1", "2", NULL);

	assert_slist_equal(list, expected);

	slist_free(list);
	slist_free(expected);
	ssmap_free(map);
}

static void ssmap_str__(void **state) {
	assert_nul(ssmap_str(NULL));

	const struct SSmap *map = ssmap_init_with((struct SSmapParams){ .allow_null_val = true, });
	ssmap_put_many(map, "a", "aa", "b", NULL, "c", "cc", NULL);

	char *actual = ssmap_str(map);

	assert_str_equal(actual,
			"a = aa\n"
			"b = (null)\n"
			"c = cc\n"
			);

	free(actual);
	ssmap_free(map);
}

static void ssmap_size__(void **state) {
	assert_int_equal(ssmap_size(NULL), 0);

	const struct SSmap *map = ssmap_init();

	assert_int_equal(ssmap_size(map), 0);

	ssmap_put_many(map, "a", "0", "b", "1", "c", "2", NULL);

	assert_int_equal(ssmap_size(map), 3);

	ssmap_free(map);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ssmap_clone__),
		TEST(ssmap_clone__params__constructor),

		TEST(ssmap_free__),

		TEST(ssmap_it_free__),

		TEST(ssmap_contains_key__),
		TEST(ssmap_contains_key__case_insensitive),

		TEST(ssmap_contains_val__),
		TEST(ssmap_contains_val__case_insensitive),

		TEST(ssmap_get__),
		TEST(ssmap_get__case_insensitive),

		TEST(ssmap_first_key__),
		TEST(ssmap_first_key__case_insensitive),

		TEST(ssmap_at__),

		TEST(ssmap_find__),

		TEST(ssmap_it__),

		TEST(ssmap_filter_it__),

		TEST(ssmap_it_next__),

		TEST(ssmap_put__),
		TEST(ssmap_put__case_insensitive),

		TEST(ssmap_put_if_absent__),
		TEST(ssmap_put_if_absent__case_insensitive),

		TEST(ssmap_put_all__),
		TEST(ssmap_put_all__case_insensitive),

		TEST(ssmap_remove__),
		TEST(ssmap_remove__case_insensitive_key),

		TEST(ssmap_remove_all__),

		TEST(ssmap_remove_in__),
		TEST(ssmap_remove_in__case_insensitive_key),

		TEST(ssmap_it_remove__),

		TEST(ssmap_equal__),
		TEST(ssmap_equal__case_insensitive_key),
		TEST(ssmap_equal__case_insensitive_val),

		TEST(ssmap_equal_ordered__),
		TEST(ssmap_equal_ordered__case_insensitive_key),
		TEST(ssmap_equal_ordered__case_insensitive_val),

		TEST(ssmap_keys_slist__),

		TEST(ssmap_keys_sset__),

		TEST(ssmap_vals_slist__),

		TEST(ssmap_str__),

		TEST(ssmap_size__),
	};

	return RUN(tests);
}

