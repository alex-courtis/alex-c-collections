#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "stable.h"
#include "../src/stable.c"

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

void mock_free_val(const void *val) {
	check_expected(val);
}

void stable_init__size(void **state) {
	const struct STable *tab = stable_init(5, 50, false);

	assert_non_nul(tab);

	assert_int_equal(stable_size(tab), 0);
	assert_int_equal(tab->capacity, 5);
	assert_int_equal(tab->grow, 50);

	stable_free(tab);
}

void stable_init__invalid(void **state) {
	const struct STable *tab = stable_init(0, 0, false);

	assert_nul(tab);
}

void stable_free_vals__null(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *val = strdup("0");

	stable_put(tab, "a", val);

	assert_int_equal(stable_size(tab), 1);

	// not much we can do here but valgrind
	stable_free_vals(tab, NULL);
}

void stable_free_vals__free_val(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *vals[] = { "0", "1", };

	expect_value(mock_free_val, val, "0");
	expect_value(mock_free_val, val, "1");

	stable_put(tab, "a", vals[0]);
	stable_put(tab, "b", vals[1]);

	assert_int_equal(stable_size(tab), 2);

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

	char *vals[] = { "0", "1", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));

	assert_int_equal(stable_size(tab), 2);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");

	stable_free(tab);
}

void stable_put__overwrite(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	char *initial[] = { "0", "1", "2", "3", };
	assert_nul(stable_put(tab, "a", initial[0]));
	assert_nul(stable_put(tab, "b", initial[1]));
	assert_nul(stable_put(tab, "c", initial[2]));
	assert_nul(stable_put(tab, "d", initial[3]));

	char *new[] = { "10", "13", };
	assert_str_equal(stable_put(tab, "b", new[0]), "1");
	assert_str_equal(stable_put(tab, "d", new[1]), "3");

	assert_int_equal(stable_size(tab), 4);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "10");
	assert_str_equal(stable_get(tab, "c"), "2");
	assert_str_equal(stable_get(tab, "d"), "13");

	stable_free(tab);
}

void stable_put__null_val(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	char *vals[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, NULL, NULL));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, "c", vals[2]));
	assert_int_equal(stable_size(tab), 2);

	assert_str_equal(stable_get(tab, "a"), vals[0]);
	assert_nul(stable_get(tab, "b"));
	assert_str_equal(stable_get(tab, "c"), vals[2]);

	stable_free(tab);
}

void stable_put__null_key(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	char *vals[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_int_equal(stable_size(tab), 1);

	assert_nul(stable_put(tab, "b", NULL));
	assert_int_equal(stable_size(tab), 2);

	assert_nul(stable_put(tab, "c", vals[2]));
	assert_int_equal(stable_size(tab), 3);

	assert_str_equal(stable_get(tab, "a"), vals[0]);
	assert_nul(stable_get(tab, "b"));
	assert_str_equal(stable_get(tab, "c"), vals[2]);

	stable_free(tab);
}

void stable_put__null_overwrite(void **state) {
	const struct STable *tab = stable_init(5, 5, false);

	char *zero = "0";
	assert_nul(stable_put(tab, "a", zero));
	assert_str_equal(stable_get(tab, "a"), "0");

	assert_str_equal(stable_put(tab, "a", NULL), "0");
	assert_int_equal(stable_size(tab), 1);
	assert_nul(stable_get(tab, "a"));

	stable_free(tab);
}

void stable_put__grow(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *initial[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", initial[0]));
	assert_nul(stable_put(tab, "b", initial[1]));
	assert_nul(stable_put(tab, "c", initial[2]));

	assert_int_equal(stable_size(tab), 3);
	assert_int_equal(tab->capacity, 3);

	char *grow[] = { "3", "4", "5", };
	assert_nul(stable_put(tab, "d", grow[0]));
	assert_int_equal(stable_size(tab), 4);
	assert_int_equal(tab->capacity, 8);

	assert_nul(stable_put(tab, "e", grow[1]));
	assert_nul(stable_put(tab, "f", grow[2]));

	assert_int_equal(stable_size(tab), 6);
	assert_int_equal(tab->capacity, 8);

	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");
	assert_str_equal(stable_get(tab, "c"), "2");

	assert_str_equal(stable_get(tab, "d"), "3");
	assert_str_equal(stable_get(tab, "e"), "4");
	assert_str_equal(stable_get(tab, "f"), "5");

	stable_free(tab);
}

void stable_iter__empty(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	assert_nul(stable_iter(tab));

	stable_free(tab);
}

void stable_iter__vals(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	char *vals[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));
	assert_nul(stable_put(tab, "c", vals[2]));

	// a 0
	const struct STableIter *iter = stable_iter(tab);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "a");
	assert_str_equal(iter->val, "0");

	// b 1
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "b");
	assert_str_equal(iter->val, "1");

	// c 2
	iter = stable_next(iter);
	assert_non_nul(iter);
	assert_str_equal(iter->key, "c");
	assert_str_equal(iter->val, "2");

	// end
	iter = stable_next(iter);
	assert_nul(iter);

	stable_free(tab);
}

void stable_iter__removed(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	void *vals[] = { "0", "1", "2", "3", "4", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));
	assert_nul(stable_put(tab, "c", vals[2]));
	assert_nul(stable_put(tab, "d", vals[3]));
	assert_nul(stable_put(tab, "e", vals[4]));

	assert_str_equal(stable_remove(tab, "a"), "0");
	assert_str_equal(stable_remove(tab, "c"), "2");
	assert_str_equal(stable_remove(tab, "e"), "4");

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

	stable_free(tab);
}

void stable_put__again(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	void *vals[] = { "0", "1", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));

	assert_int_equal(stable_size(tab), 2);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");

	// remove a 0
	assert_str_equal(stable_remove(tab, "a"), "0");

	assert_int_equal(stable_size(tab), 1);
	assert_nul(stable_get(tab, "a"));

	// put a 0
	assert_nul(stable_put(tab, "a", vals[0]));
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

	stable_free(tab);
}

void stable_put__case_insensitive(void **state) {
	const struct STable *tab = stable_init(5, 5, true);

	assert_nul(stable_put(tab, "a", "1"));

	assert_str_equal(stable_get(tab, "a"), "1");
	assert_str_equal(stable_get(tab, "A"), "1");

	assert_str_equal(stable_put(tab, "a", "2"), "1");

	assert_str_equal(stable_put(tab, "A", "3"), "2");

	stable_free(tab);
}

void stable_remove__case_insensitive(void **state) {
	const struct STable *tab = stable_init(5, 5, true);

	assert_nul(stable_put(tab, "a", "1"));

	assert_str_equal(stable_remove(tab, "A"), "1");

	assert_nul(stable_put(tab, "B", "2"));

	assert_str_equal(stable_remove(tab, "b"), "2");

	stable_free(tab);
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

	stable_free(tab);
}

void stable_remove__inexistent(void **state) {
	const struct STable *tab = stable_init(3, 5, false);

	void *vals[] = { "0", "1", "2", };
	assert_nul(stable_put(tab, "a", vals[0]));
	assert_nul(stable_put(tab, "b", vals[1]));
	assert_nul(stable_put(tab, "c", vals[2]));

	assert_int_equal(stable_size(tab), 3);
	assert_str_equal(stable_get(tab, "a"), "0");
	assert_str_equal(stable_get(tab, "b"), "1");
	assert_str_equal(stable_get(tab, "c"), "2");

	// x
	assert_nul(stable_remove(tab, "x"));
	assert_int_equal(stable_size(tab), 3);

	stable_free(tab);
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
		TEST(stable_iter__vals),
		TEST(stable_iter__removed),

		TEST(stable_put__again),

		TEST(stable_put__case_insensitive),
		TEST(stable_remove__case_insensitive),

		TEST(stable_remove__existing),
		TEST(stable_remove__inexistent),
	};

	return RUN(tests);
}

