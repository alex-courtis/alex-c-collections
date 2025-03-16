#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "stable.h"

int before_all(void **state) {
	return 0;
}

int after_all(void **state) {
	return 0;
}

int before_each(void **state) {
	return 0;
}

int after_each(void **state) {
	return 0;
}

void mock_free_val(const void* const val) {
	check_expected(val);
}

void stable_init__size(void **state) {
	const struct STable *tab = stable_init(5, 50, false);

	assert_non_nul(tab);

	assert_int_equal(stable_size(tab), 0);
	assert_int_equal(stable_capacity(tab), 5);

	stable_free_vals(tab, NULL);
}

void stable_init__invalid(void **state) {
	const struct STable *tab = stable_init(0, 0, false);

	stable_free_vals(tab, NULL);
}

void stable_free_vals__null(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *key = strdup("a");
	char *val = strdup("0");

	stable_put(tab, key, val);

	assert_int_equal(stable_size(tab), 1);

	// not much we can do here but valgrind
	stable_free_vals(tab, NULL);
	free(key);
}

void stable_free_vals__free_val(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *vals[] = { "0", "1", };

	expect_value(mock_free_val, val, "0");
	expect_value(mock_free_val, val, "1");

	stable_put(tab, "a", vals[0]);
	stable_put(tab, "b", vals[1]);
	stable_put(tab, "c", NULL);

	assert_int_equal(stable_size(tab), 3);

	stable_free_vals(tab, mock_free_val);
}

void free_val_stable(const void *val) {
	stable_free_vals(val, mock_free_val);
}

void stable_free_vals__free_val_reentrant(void **state) {
	const struct STable *outer = stable_init(3, 5, false);
	const struct STable *inner1 = stable_init(3, 5, false);
	const struct STable *inner2 = stable_init(3, 5, false);

	char *vals[] = { "11", "12", "21", "22", };

	expect_value(mock_free_val, val, "11");
	expect_value(mock_free_val, val, "12");
	expect_value(mock_free_val, val, "21");
	expect_value(mock_free_val, val, "22");

	stable_put(inner1, "a1", vals[0]);
	stable_put(inner1, "b1", vals[1]);
	stable_put(inner2, "a2", vals[2]);
	stable_put(inner2, "b2", vals[3]);

	stable_put(outer, "a", (void*)inner1);
	stable_put(outer, "b", (void*)inner2);

	assert_int_equal(stable_size(outer), 2);

	stable_free_vals(outer, free_val_stable);
}

void stable_put__new(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));

	assert_int_equal(stable_size(tab), 2);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");

	stable_free_vals(tab, NULL);
}

void stable_put__overwrite(void **state) {
	const struct STable *tab = stable_init(5, 5, false);
	char *replaced;

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));
	assert_nul(stable_put(tab, "c", strdup("2")));
	assert_nul(stable_put(tab, "d", strdup("3")));

	replaced = (char*)stable_put(tab, "b", strdup("10"));
	assert_str_equal(replaced, "1");
	free(replaced);

	replaced = (char*)stable_put(tab, "d", strdup("13"));
	assert_str_equal(replaced, "3");
	free(replaced);

	assert_int_equal(stable_size(tab), 4);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "10");
	assert_str_equal(stable_get(tab, "c"), "2");
	assert_str_equal(stable_get(tab, "d"), "13");

	stable_free_vals(tab, NULL);
}

void stable_put__null_key(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, NULL, NULL));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, "c", strdup("2")));
	assert_int_equal(stable_size(tab), 2);

	assert_str_equal(stable_get(tab, "a"), "0");
	assert_nul(stable_get(tab, "b"));
	assert_str_equal(stable_get(tab, "c"), "2");

	stable_free_vals(tab, NULL);
}

void stable_put__null_val(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, "b", NULL));
	assert_int_equal(stable_size(tab), 2);

	assert_nul(stable_put(tab, "c", strdup("2")));
	assert_int_equal(stable_size(tab), 3);

	assert_str_equal(stable_get(tab, "a"), "0");
	assert_nul(stable_get(tab, "b"));
	assert_str_equal(stable_get(tab, "c"), "2");

	stable_free_vals(tab, NULL);
}

void stable_put__null_overwrite(void **state) {
	const struct STable *tab = stable_init(5, 5, false);
	char *replaced = NULL;

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_str_equal(stable_get(tab, "a"), "0");

	replaced = (char*)stable_put(tab, "a", NULL);
	assert_str_equal(replaced, "0");
	free(replaced);

	assert_int_equal(stable_size(tab), 1);
	assert_nul(stable_get(tab, "a"));

	stable_free_vals(tab, NULL);
}

void stable_put__grow(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));
	assert_nul(stable_put(tab, "c", strdup("2")));

	assert_int_equal(stable_size(tab), 3);
	assert_int_equal(stable_capacity(tab), 3);

	assert_nul(stable_put(tab, "d", strdup("3")));
	assert_int_equal(stable_size(tab), 4);
	assert_int_equal(stable_capacity(tab), 8);

	assert_nul(stable_put(tab, "e", strdup("4")));
	assert_nul(stable_put(tab, "f", strdup("5")));

	assert_int_equal(stable_size(tab), 6);
	assert_int_equal(stable_capacity(tab), 8);

	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");
	assert_str_equal(stable_get(tab, "c"), "2");

	assert_str_equal(stable_get(tab, "d"), "3");
	assert_str_equal(stable_get(tab, "e"), "4");
	assert_str_equal(stable_get(tab, "f"), "5");

	stable_free_vals(tab, NULL);
}

void stable_iter__empty(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_iter(tab));

	stable_free_vals(tab, NULL);
}

void stable_iter__free(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));

	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);
	assert_str_equal(iter->val, "0");

	// not much we can do here but valgrind
	stable_iter_free(iter);

	stable_free_vals(tab, NULL);
}

void stable_iter__vals(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_put(tab, "a", NULL));
	assert_nul(stable_put(tab, "b", strdup("1")));
	assert_nul(stable_put(tab, "c", NULL));
	assert_nul(stable_put(tab, "d", strdup("3")));
	assert_nul(stable_put(tab, "e", NULL));

	assert_int_equal(stable_size(tab), 5);

	// a NULL
	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_nul(iter->val);

	// b 1
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_str_equal(iter->val, "1");

	// c NULL
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "c");
	assert_nul(iter->val);

	// d 3
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "d");
	assert_str_equal(iter->val, "3");

	// e NULL
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "e");
	assert_nul(iter->val);

	// end
	iter = stable_next(iter);
	assert_nul(iter);

	stable_free_vals(tab, NULL);
}

void stable_iter__removed(void **state) {
	const struct STable *tab = stable_init(3, 5, false);
	char *replaced = NULL;

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));
	assert_nul(stable_put(tab, "c", strdup("2")));
	assert_nul(stable_put(tab, "d", strdup("3")));
	assert_nul(stable_put(tab, "e", strdup("4")));

	replaced = (char*)stable_remove(tab, "a");
	assert_str_equal(replaced, "0");
	free(replaced);

	replaced = (char*)stable_remove(tab, "c");
	assert_str_equal(replaced, "2");
	free(replaced);

	replaced = (char*)stable_remove(tab, "e");
	assert_str_equal(replaced, "4");
	free(replaced);

	assert_int_equal(stable_size(tab), 2);

	// b 1
	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_str_equal(iter->val, "1");

	// d 3
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "d");
	assert_str_equal(iter->val, "3");

	// end
	iter = stable_next(iter);
	assert_nul(iter);

	stable_free_vals(tab, NULL);
}

void stable_put__again(void **state) {
	const struct STable *tab = stable_init(3, 5, false);
	char *removed = NULL;

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));

	assert_int_equal(stable_size(tab), 2);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");

	// remove a 0
	removed = (char*)stable_remove(tab, "a");
	assert_str_equal(removed, "0");
	free(removed);

	assert_int_equal(stable_size(tab), 1);
	assert_nul(stable_get(tab, "a"));

	// put a 0
	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_int_equal(stable_size(tab), 2);

	// b 1
	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_str_equal(iter->val, "1");

	// a 0 moved later
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_str_equal(iter->val, "0");

	// end
	iter = stable_next(iter);
	assert_nul(iter);

	stable_free_vals(tab, NULL);
}

void stable_put__case_insensitive(void **state) {
	const struct STable *tab = stable_init(5, 5, true);
	char *replaced = NULL;

	assert_nul(stable_put(tab, "a", strdup("1")));

	assert_str_equal(stable_get(tab, "a"), "1");
	assert_str_equal(stable_get(tab, "A"), "1");

	replaced = (char*)stable_put(tab, "a", strdup("2"));
	assert_str_equal(replaced, "1");
	free(replaced);

	replaced = (char*)stable_put(tab, "A", strdup("3"));
	assert_str_equal(replaced, "2");
	free(replaced);

	stable_free_vals(tab, NULL);
}

void stable_remove__case_insensitive(void **state) {
	const struct STable *tab = stable_init(5, 5, true);

	assert_nul(stable_put(tab, "a", "1"));

	assert_str_equal(stable_remove(tab, "A"), "1");

	assert_nul(stable_put(tab, "B", "2"));

	assert_str_equal(stable_remove(tab, "b"), "2");

	stable_free_vals(tab, NULL);
}

void stable_remove__existing(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	void *vals[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));
	assert_nul(stable_put(tab, "c", vals[2]));

	assert_int_equal(stable_size(tab), 3);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");
	assert_str_equal(stable_get(tab, "c"), "2");

	// b 1
	assert_str_equal(stable_remove(tab, "b"), "1");
	assert_int_equal(stable_size(tab), 2);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_nul(stable_get(tab, "b"));
	assert_str_equal(stable_get(tab, "c"), "2");

	// c 2
	assert_str_equal(stable_remove(tab, "c"), "2");
	assert_int_equal(stable_size(tab), 1);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_nul(stable_get(tab, "b"));
	assert_nul(stable_get(tab, "c"));

	// a 0
	assert_str_equal(stable_remove(tab, "a"), "0");
	assert_int_equal(stable_size(tab), 0);
	assert_nul(stable_get(tab, "a"));
	assert_nul(stable_get(tab, "b"));
	assert_nul(stable_get(tab, "c"));

	stable_free_vals(tab, NULL);
}

void stable_remove__inexistent(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_put(tab, "a", strdup("0")));
	assert_nul(stable_put(tab, "b", strdup("1")));
	assert_nul(stable_put(tab, "c", strdup("2")));

	assert_int_equal(stable_size(tab), 3);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");
	assert_str_equal(stable_get(tab, "c"), "2");

	// x
	assert_nul(stable_remove(tab, "x"));
	assert_int_equal(stable_size(tab), 3);

	stable_free_vals(tab, NULL);
}

void stable_equal__length_different(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	assert_nul(stable_put(a, "a", strdup("1")));
	assert_nul(stable_put(a, "b", strdup("2")));

	assert_nul(stable_put(b, "a", strdup("1")));

	assert_false(stable_equal(a, b, NULL));

	stable_free_vals(a, NULL);
	stable_free_vals(b, NULL);
}

void stable_equal__keys_different(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	void *vals[] = { strdup("0"), strdup("1"), };

	assert_nul(stable_put(a, "a", vals[0]));
	assert_nul(stable_put(a, "b", vals[1]));

	assert_nul(stable_put(b, "a", vals[0]));
	assert_nul(stable_put(b, "B", vals[1]));

	assert_false(stable_equal(a, b, NULL));

	stable_free(a);
	stable_free_vals(b, NULL);
}

void stable_equal__keys_insensitive_a(void **state) {
	const struct STable *a = stable_init(3, 5, true);
	const struct STable *b = stable_init(3, 5, false);

	void *vals[] = { strdup("0"), };

	assert_nul(stable_put(a, "a", vals[0]));

	assert_nul(stable_put(b, "A", vals[0]));

	assert_true(stable_equal(a, b, fn_comp_equals_strcmp));

	stable_free_vals(a, NULL);
	stable_free(b);
}

void stable_equal__keys_insensitive_b(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, true);

	void *vals[] = { strdup("0"), };

	assert_nul(stable_put(a, "a", vals[0]));

	assert_nul(stable_put(b, "A", vals[0]));

	assert_true(stable_equal(a, b, fn_comp_equals_strcmp));

	stable_free_vals(a, NULL);
	stable_free(b);
}

void stable_equal__pointers_ok(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };
	assert_nul(stable_put(a, "a", vals[0]));
	assert_nul(stable_put(a, "b", vals[1]));
	assert_nul(stable_put(a, "c", vals[2]));

	assert_nul(stable_put(b, "a", vals[0]));
	assert_nul(stable_put(b, "b", vals[1]));
	assert_nul(stable_put(b, "c", vals[2]));

	assert_true(stable_equal(a, b, NULL));

	stable_free(a);
	stable_free_vals(b, NULL);
}

void stable_equal__pointers_different(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };
	assert_nul(stable_put(a, "a", vals[0]));
	assert_nul(stable_put(a, "b", vals[1]));
	assert_nul(stable_put(a, "c", vals[2]));

	assert_nul(stable_put(b, "a", vals[0]));
	assert_nul(stable_put(b, "b", vals[0]));
	assert_nul(stable_put(b, "c", vals[0]));

	assert_false(stable_equal(a, b, NULL));

	stable_free_vals(a, NULL);
	stable_free(b);
}

void stable_equal__comparison_ok(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	assert_nul(stable_put(a, "a", strdup("1")));

	assert_nul(stable_put(b, "a", strdup("1")));

	assert_true(stable_equal(a, b, fn_comp_equals_strcmp));

	stable_free_vals(a, NULL);
	stable_free_vals(b, NULL);
}

void stable_equal__comparison_different(void **state) {
	const struct STable *a = stable_init(3, 5, false);
	const struct STable *b = stable_init(3, 5, false);

	assert_nul(stable_put(a, "a", strdup("0")));

	assert_nul(stable_put(b, "a", strdup("1")));

	assert_false(stable_equal(a, b, fn_comp_equals_strcmp));

	stable_free_vals(a, NULL);
	stable_free_vals(b, NULL);
}

void stable_keys_slist__empty(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_keys_slist(tab));

	stable_free_vals(tab, NULL);
}

void stable_keys_slist__many(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	stable_put(tab, "a", strdup("1"));
	stable_put(tab, "b", strdup("2"));

	struct SList *list = stable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "a");
	assert_str_equal(slist_at(list, 1), "b");

	slist_free(&list);
	stable_free_vals(tab, NULL);
}

void stable_vals_slist__empty(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_vals_slist(tab));

	stable_free_vals(tab, NULL);
}

void stable_vals_slist__many(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	stable_put(tab, "a", strdup("1"));
	stable_put(tab, "b", NULL);
	stable_put(tab, "c", strdup("3"));

	struct SList *list = stable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "1");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "3");

	slist_free(&list);
	stable_free_vals(tab, NULL);
}

void stable_str__null(void **state) {
	assert_nul(stable_str(NULL));
}

void stable_str__empty(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *str = stable_str(tab);
	assert_str_equal(str, "");

	free(str);
	stable_free_vals(tab, NULL);
}

void stable_str__string_vals(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	stable_put(tab, "a", strdup("1"));
	stable_put(tab, "b", NULL);
	stable_put(tab, "c", strdup("3"));

	char *str = stable_str(tab);
	assert_str_equal(str,
			"a = 1\n"
			"b = (null)\n"
			"c = 3"
			);

	free(str);
	stable_free_vals(tab, NULL);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(stable_init__size),
		TEST(stable_init__invalid),

		TEST(stable_free_vals__null),
		TEST(stable_free_vals__free_val),
		TEST(stable_free_vals__free_val_reentrant),

		TEST(stable_put__new),
		TEST(stable_put__overwrite),
		TEST(stable_put__null_val),
		TEST(stable_put__null_key),
		TEST(stable_put__null_overwrite),
		TEST(stable_put__grow),

		TEST(stable_iter__empty),
		TEST(stable_iter__free),
		TEST(stable_iter__vals),
		TEST(stable_iter__removed),

		TEST(stable_put__again),

		TEST(stable_put__case_insensitive),
		TEST(stable_remove__case_insensitive),

		TEST(stable_remove__existing),
		TEST(stable_remove__inexistent),

		TEST(stable_equal__length_different),
		TEST(stable_equal__keys_different),
		TEST(stable_equal__keys_insensitive_a),
		TEST(stable_equal__keys_insensitive_b),
		TEST(stable_equal__pointers_ok),
		TEST(stable_equal__pointers_different),
		TEST(stable_equal__comparison_ok),
		TEST(stable_equal__comparison_different),

		TEST(stable_keys_slist__empty),
		TEST(stable_keys_slist__many),

		TEST(stable_vals_slist__empty),
		TEST(stable_vals_slist__many),

		TEST(stable_str__null),
		TEST(stable_str__empty),
		TEST(stable_str__string_vals),
	};

	return RUN(tests);
}

