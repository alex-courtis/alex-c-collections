#include "assert-ssmap.h"
#include "asserts.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "slist.h"
#include "ppmap.h"

#include "ssmap.h"















// ssmap_put_many(set, "a", "0", "b", "1", "c", "2", "d", "3", "e", "4", NULL);














#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#include "data/words-sorted.c"
#include "data/words-unsorted.c"
#pragma GCC diagnostic pop // "-Wunused-variable"

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

static void ssmap_clone__params__constructor(void **state) {
	assert_nul(ssmap_clone(NULL));

	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, .case_insensitive_val = true, .initial = 99, .grow = 1, });
	ssmap_put_many(set, "a", "0", "b", "1", NULL);

	const struct SSmap *clone = ssmap_clone(set);

	assert_non_nul(clone);
	assert_int_equal(clone->ppmap->size, 2);
	assert_int_equal(clone->ppmap->capacity, 99);
	assert_int_equal(clone->params.grow, 1);
	assert_ptr_equal(clone->ppmap->params.equal_val, equal_strcasecmp);

	assert_ptr_equal(clone->params.case_insensitive_key, true);
	assert_ptr_equal(clone->params.case_insensitive_val, true);
	assert_ptr_equal(clone->params.initial, 99);
	assert_ptr_equal(clone->params.grow, 1);

	assert_ssmap_equal(set, clone);

	ssmap_free(set);
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

	const struct SSmap *set = ssmap_init();

	assert_false(ssmap_contains_key(set, "x"));

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	assert_true(ssmap_contains_key(set, "b"));

	assert_false(ssmap_contains_key(set, "x"));

	ssmap_free(set);
}

static void ssmap_contains_key__case_insensitive(void **state) {
	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(set, "a", "0", NULL);

	assert_true(ssmap_contains_key(set, "A"));

	ssmap_free(set);
}

static void ssmap_contains_val__(void **state) {
	assert_false(ssmap_contains_val(NULL, "9"));

	const struct SSmap *set = ssmap_init();

	assert_false(ssmap_contains_val(set, "9"));

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	assert_true(ssmap_contains_val(set, "1"));

	assert_false(ssmap_contains_val(set, "9"));

	ssmap_free(set);
}

static void ssmap_contains_val__case_insensitive(void **state) {
	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });
	ssmap_put_many(set, "a", "aa", NULL);

	assert_true(ssmap_contains_val(set, "AA"));

	ssmap_free(set);
}

static void ssmap_at__(void **state) {
	assert_nul(ssmap_at(NULL, 0).key);
	assert_nul(ssmap_at(NULL, 0).val);

	const struct SSmap *set = ssmap_init();

	assert_nul(ssmap_at(set, 0).key);
	assert_nul(ssmap_at(set, 0).val);

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	assert_str_equal(ssmap_at(set, 1).key, "b");
	assert_str_equal(ssmap_at(set, 1).val, "1");

	assert_nul(ssmap_at(set, 3).key);
	assert_nul(ssmap_at(set, 3).val);

	ssmap_free(set);
}

static void ssmap_find__(void **state) {
	assert_nul(ssmap_find(NULL, (struct SSmapFilter){ 0 }).key);
	assert_nul(ssmap_find(NULL, (struct SSmapFilter){ 0 }).val);

	const struct SSmap *set = ssmap_init();

	assert_nul(ssmap_find(set, (struct SSmapFilter){ 0 }).key);
	assert_nul(ssmap_find(set, (struct SSmapFilter){ 0 }).val);

	ssmap_put_many(set, "a", "x0", "b", "x1", "c", "a2", "d", "x3", NULL);

	assert_str_equal(ssmap_find(set, (struct SSmapFilter){ 0 }).key, "a");
	assert_str_equal(ssmap_find(set, (struct SSmapFilter){ 0 }).val, "x0");

    struct SSmapPair pair = ssmap_find(set, (struct SSmapFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_str_equal(pair.key, "c");
	assert_str_equal(pair.val, "a2");

    pair = ssmap_find(set, (struct SSmapFilter){ .key_data = match_starts_with_a, .data = "x", });

	assert_str_equal(pair.key, "a");
	assert_str_equal(pair.val, "x0");

	ssmap_free(set);
}

static void ssmap_it__(void **state) {
	assert_nul(ssmap_it(NULL));

	const struct SSmap *set = ssmap_init();

	assert_nul(ssmap_it(set));

	ssmap_put_many(set, "a", "0", "b", "1", NULL);

	const struct SSmapIt *it = ssmap_it(set);

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "0");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "1");

	assert_nul(ssmap_it_next(it));

	ssmap_free(set);
}

static void ssmap_filter_it__(void **state) {
	assert_nul(ssmap_filter_it(NULL, (struct SSmapFilter){ 0 }));

	const struct SSmap *set = ssmap_init();

	assert_nul(ssmap_filter_it(set, (struct SSmapFilter){ 0 }));

	ssmap_put_many(set, "a", "x0", "b", "a1", "c", "a2", "d", "x3", NULL);

	const struct SSmapIt *it = ssmap_filter_it(set, (struct SSmapFilter){ .val_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "a1");

	it = ssmap_it_next(it);
	assert_non_nul(it);
	assert_str_equal(it->key, "c");
	assert_str_equal(it->val, "a2");

	assert_nul(ssmap_it_next(it));

	it = ssmap_filter_it(set, (struct SSmapFilter){ .key_data = match_starts_with_a, .data = "x", });

	assert_non_nul(it);
	assert_str_equal(it->key, "a");
	assert_str_equal(it->val, "x0");

	assert_nul(ssmap_it_next(it));

	ssmap_free(set);
}

static void ssmap_it_next__(void **state) {
	assert_nul(ssmap_it_next(NULL));

	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	assert_nul(ssmap_it_next(it));
}

static void ssmap_remove__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "B", "1", NULL);

	assert_false(ssmap_remove(NULL, "x"));

	const struct SSmap *set = ssmap_init();
	ssmap_put_many(set, "A", "0", "B", "1", NULL);

	assert_true(ssmap_remove(set, "A"));

	assert_false(ssmap_remove(set, NULL));

	assert_false(ssmap_remove(set, "x"));

	assert_ssmap_equal(set, expected);

	ssmap_free(expected);
	ssmap_free(set);
}

static void ssmap_remove__case_insensitive_key(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "B", "1", NULL);

	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(set, "A", "0", "B", "1", NULL);

	assert_true(ssmap_remove(set, "a"));

	assert_ssmap_equal(set, expected);

	ssmap_free(expected);
	ssmap_free(set);
}

static void ssmap_remove_all__(void **state) {
	assert_int_equal(ssmap_remove_all(NULL), 0);

	const struct SSmap *set = ssmap_init();

	assert_int_equal(ssmap_remove_all(set), 0);

	ssmap_put_many(set, "a", "0", "b", "1", NULL);

	assert_int_equal(ssmap_remove_all(set), 2);

	assert_int_equal(ssmap_size(set), 0);

	ssmap_free(set);
}

static void ssmap_remove_in__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "b", "1", NULL);

	assert_int_equal(ssmap_remove_in(NULL, NULL), 0);

	const struct SSmap *set = ssmap_init();
	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	assert_int_equal(ssmap_remove_in(set, NULL), 0);

	assert_int_equal(ssmap_remove_in(NULL, set), 0);

	const struct SSmap *in = ssmap_init();
	ssmap_put_many(in, "a", "0", "c", "2", "d", "3", NULL);

	assert_int_equal(ssmap_remove_in(set, in), 2);

	assert_ssmap_equal(set, expected);

	ssmap_free(set);
	ssmap_free(in);
	ssmap_free(expected);
}

static void ssmap_remove_in__case_insensitive_key(void **state) {
	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(set, "A", "0", "B", "1", NULL);

	const struct SSmap *in = ssmap_init();
	ssmap_put_many(in, "B", "1", "C", "2", NULL);

	assert_int_equal(ssmap_remove_in(set, in), 1);

	ssmap_free(set);
	ssmap_free(in);
}

static void ssmap_it_remove__(void **state) {
	const struct SSmap *expected = ssmap_init();
	ssmap_put_many(expected, "a", "0", "c", "2", "d", "3", "e", "4", NULL);

	assert_false(ssmap_it_remove(NULL));

	const struct SSmapIt *it = calloc(1, sizeof(struct SSmapIt));

	assert_false(ssmap_it_remove(it));

	const struct SSmap *set = ssmap_init();
	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", "d", "3", "e", "4", NULL);

	it = ssmap_it(set);
	it = ssmap_it_next(it);
	assert_str_equal(it->key, "b");
	assert_str_equal(it->val, "1");

	assert_true(ssmap_it_remove(it));

	assert_false(ssmap_contains_key(set, "b"));

	it = ssmap_it_next(it);
	assert_str_equal(it->key, "c");
	assert_str_equal(it->val, "2");

	assert_ssmap_equal(set, expected);

	ssmap_it_free(it);
	ssmap_free(expected);
	ssmap_free(set);
}

static void ssmap_equal__(void **state) {
	assert_false(ssmap_equal(NULL, NULL));

	const struct SSmap *a = ssmap_init();

	assert_false(ssmap_equal(a, NULL));
	assert_false(ssmap_equal(NULL, a));

	const struct SSmap *b = ssmap_init();

	assert_true(ssmap_equal(a, a));

	ssmap_put_many(a, "a", "0", NULL);

	assert_false(ssmap_equal(a, b));

	ssmap_put_many(b, "a", "0", NULL);

	assert_true(ssmap_equal(a, b));

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal__case_insensitive_key(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, });
	ssmap_put_many(a, "a", "0", "b", "1", "c", "2", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "a", "0", "B", "1", "c", "2", NULL);

	assert_true(ssmap_equal(a, b));

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_equal__case_insensitive_val(void **state) {
	const struct SSmap *a = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, });
	ssmap_put_many(a, "a", "AA", "b", "1", "c", "cc", NULL);

	const struct SSmap *b = ssmap_init();
	ssmap_put_many(b, "a", "aa", "b", "1", "c", "CC", NULL);

	assert_true(ssmap_equal(a, b));

	ssmap_free(a);
	ssmap_free(b);
}

static void ssmap_keys_slist__(void **state) {
	assert_nul(ssmap_keys_slist(NULL));

	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_key = true, .initial = 2, .grow = 1, });

	const struct Slist *list = ssmap_keys_slist(set);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	list = ssmap_keys_slist(set);

	assert_int_equal(slist_size(list), 3);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");
	assert_str_equal(slist_at(list, 2), "c");

	slist_free(list);
	ssmap_free(set);
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

	assert_int_equal(sset_size(set), 3);
	assert_str_equal(sset_at(set, 0), "a");
	assert_str_equal(sset_at(set, 1), "b");
	assert_str_equal(sset_at(set, 2), "c");

	sset_free(set);
	ssmap_free(map);
}

static void ssmap_vals_slist__(void **state) {
	assert_nul(ssmap_vals_slist(NULL));

	const struct SSmap *set = ssmap_init_with((struct SSmapParams){ .case_insensitive_val = true, .initial = 2, .grow = 1, });

	const struct Slist *list = ssmap_vals_slist(set);
	assert_int_equal(slist_size(list), 0);

	assert_true(list->params.case_insensitive);
	assert_int_equal(list->params.initial, 2);
	assert_int_equal(list->params.grow, 1);

	slist_free(list);

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	list = ssmap_vals_slist(set);

	assert_int_equal(slist_size(list), 3);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");
	assert_str_equal(slist_at(list, 2), "2");

	slist_free(list);
	ssmap_free(set);
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

	const struct SSmap *set = ssmap_init();

	assert_int_equal(ssmap_size(set), 0);

	ssmap_put_many(set, "a", "0", "b", "1", "c", "2", NULL);

	assert_int_equal(ssmap_size(set), 3);

	ssmap_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ssmap_clone__params__constructor),

		TEST(ssmap_free__),

		TEST(ssmap_it_free__),

		TEST(ssmap_contains_key__),
		TEST(ssmap_contains_key__case_insensitive),

		TEST(ssmap_contains_val__),
		TEST(ssmap_contains_val__case_insensitive),

		// TEST(ssmap_get__),
		// TEST(ssmap_get__case_insensitive),

		// TEST(ssmap_first_key__),
		// TEST(ssmap_first_key__case_insensitive),

		TEST(ssmap_at__),

		TEST(ssmap_find__),

		TEST(ssmap_it__),

		TEST(ssmap_filter_it__),

		TEST(ssmap_it_next__),

		// TEST(ssmap_put__),
		// TEST(ssmap_put__case_insensitive),

		// TEST(ssmap_put_if_absent__),
		// TEST(ssmap_put_if_absent__case_insensitive),

		// TEST(ssmap_put_all__),
		// TEST(ssmap_put_all__case_insensitive),

		TEST(ssmap_remove__),
		TEST(ssmap_remove__case_insensitive_key),

		TEST(ssmap_remove_all__),

		TEST(ssmap_remove_in__),
		TEST(ssmap_remove_in__case_insensitive_key),

		TEST(ssmap_it_remove__),

		TEST(ssmap_equal__),
		TEST(ssmap_equal__case_insensitive_key),
		TEST(ssmap_equal__case_insensitive_val),

		TEST(ssmap_keys_slist__),

		TEST(ssmap_keys_sset__),

		TEST(ssmap_vals_slist__),

		TEST(ssmap_str__),

		TEST(ssmap_size__),
	};

	return RUN(tests);
}

