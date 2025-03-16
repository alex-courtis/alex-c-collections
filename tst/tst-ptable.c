#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "ptable.h"

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

void ptable_init__size(void **state) {
	const struct PTable *tab = ptable_init(5, 50);

	assert_non_nul(tab);

	assert_int_equal(ptable_size(tab), 0);
	assert_int_equal(ptable_capacity(tab), 5);

	ptable_free_vals(tab, NULL);
}

void ptable_init__invalid(void **state) {
	const struct PTable *tab = ptable_init(0, 0);

	assert_nul(tab);
}

void ptable_free_vals__null(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *key = strdup("a");
	char *val = strdup("0");

	ptable_put(tab, key, val);

	assert_int_equal(ptable_size(tab), 1);

	// not much we can do here but valgrind
	ptable_free_vals(tab, NULL);
	free(key);
}

void ptable_free_vals__free_val(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };
	char *vals[] = { "0", "1", NULL, };

	expect_value(mock_free_val, val, "0");
	expect_value(mock_free_val, val, "1");

	ptable_put(tab, keys[0], vals[0]);
	ptable_put(tab, keys[1], vals[1]);
	ptable_put(tab, keys[2], vals[2]);

	assert_int_equal(ptable_size(tab), 3);

	ptable_free_vals(tab, mock_free_val);
}

void free_val_ptable(const void *val) {
	ptable_free_vals(val, mock_free_val);
}

void ptable_free_vals__free_val_reentrant(void **state) {
	const struct PTable *outer = ptable_init(3, 5);
	const struct PTable *inner1 = ptable_init(3, 5);
	const struct PTable *inner2 = ptable_init(3, 5);

	char *keys[] = { "a", "b", };
	char *vals[] = { "11", "12", "21", "22", };

	expect_value(mock_free_val, val, "11");
	expect_value(mock_free_val, val, "12");
	expect_value(mock_free_val, val, "21");
	expect_value(mock_free_val, val, "22");

	ptable_put(inner1, keys[0], vals[0]);
	ptable_put(inner1, keys[1], vals[1]);
	ptable_put(inner2, keys[0], vals[2]);
	ptable_put(inner2, keys[1], vals[3]);

	ptable_put(outer, keys[0], (void*)inner1);
	ptable_put(outer, keys[1], (void*)inner2);

	assert_int_equal(ptable_size(outer), 2);

	ptable_free_vals(outer, free_val_ptable);
}

void ptable_put__new(void **state) {
	const struct PTable *tab = ptable_init(5, 5);

	char *keys[] = { "a", "b", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));

	assert_int_equal(ptable_size(tab), 2);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "1");

	ptable_free_vals(tab, NULL);
}

void ptable_put__overwrite(void **state) {
	const struct PTable *tab = ptable_init(5, 5);

	char *replaced = NULL;
	char *keys[] = { "a", "b", "c", "d", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], strdup("2")));
	assert_nul(ptable_put(tab, keys[3], strdup("3")));

	replaced = (char*)ptable_put(tab, keys[1], strdup("10"));
	assert_str_equal(replaced, "1");
	free(replaced);

	replaced = (char*)ptable_put(tab, keys[3], strdup("13"));
	assert_str_equal(replaced, "3");
	free(replaced);

	assert_int_equal(ptable_size(tab), 4);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "10");
	assert_str_equal(ptable_get(tab, keys[2]), "2");
	assert_str_equal(ptable_get(tab, keys[3]), "13");

	ptable_free_vals(tab, NULL);
}

void ptable_put__null(void **state) {
	const struct PTable *tab = ptable_init(5, 5);

	char *keys[] = { "a", "b", "c", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_int_equal(ptable_size(tab), 1);

	assert_nul(ptable_put(tab, keys[1], NULL));
	assert_int_equal(ptable_size(tab), 2);

	assert_nul(ptable_put(tab, keys[2], strdup("2")));
	assert_int_equal(ptable_size(tab), 3);

	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_nul(ptable_get(tab, keys[1]));
	assert_str_equal(ptable_get(tab, keys[2]), "2");

	ptable_free_vals(tab, NULL);
}

void ptable_put__null_overwrite(void **state) {
	const struct PTable *tab = ptable_init(5, 5);

	char *zero = "0";
	assert_nul(ptable_put(tab, 0, zero));

	assert_str_equal(ptable_get(tab, 0), "0");

	assert_str_equal(ptable_put(tab, 0, NULL), "0");

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, 0));

	ptable_free_vals(tab, NULL);
}

void ptable_put__grow(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", "d", "e", "f", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], strdup("2")));

	assert_int_equal(ptable_size(tab), 3);
	assert_int_equal(ptable_capacity(tab), 3);

	assert_nul(ptable_put(tab, keys[3], strdup("3")));
	assert_int_equal(ptable_size(tab), 4);
	assert_int_equal(ptable_capacity(tab), 8);

	assert_nul(ptable_put(tab, keys[4], strdup("4")));
	assert_nul(ptable_put(tab, keys[5], strdup("5")));

	assert_int_equal(ptable_size(tab), 6);
	assert_int_equal(ptable_capacity(tab), 8);

	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "1");
	assert_str_equal(ptable_get(tab, keys[2]), "2");

	assert_str_equal(ptable_get(tab, keys[3]), "3");
	assert_str_equal(ptable_get(tab, keys[4]), "4");
	assert_str_equal(ptable_get(tab, keys[5]), "5");

	ptable_free_vals(tab, NULL);
}

void ptable_iter__empty(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	assert_nul(ptable_iter(tab));

	ptable_free_vals(tab, NULL);
}

void ptable_iter__vals(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", "d", "e", };

	assert_nul(ptable_put(tab, keys[0], NULL));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], NULL));
	assert_nul(ptable_put(tab, keys[3], strdup("3")));
	assert_nul(ptable_put(tab, keys[4], NULL));

	assert_int_equal(ptable_size(tab), 5);

	// zero
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[0]);
	assert_nul(iter->val);

	// one
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[1]);
	assert_str_equal(iter->val, "1");

	// two
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[2]);
	assert_nul(iter->val);

	// three
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[3]);
	assert_str_equal(iter->val, "3");

	// four
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[4]);
	assert_nul(iter->val);

	// end
	iter = ptable_next(iter);
	assert_nul(iter);

	ptable_free_vals(tab, NULL);
}

void ptable_iter__removed(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", "d", "e", };
	char *removed = NULL;

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], strdup("2")));
	assert_nul(ptable_put(tab, keys[3], strdup("3")));
	assert_nul(ptable_put(tab, keys[4], strdup("4")));

	removed = (char*)ptable_remove(tab, keys[0]);
	assert_str_equal(removed, "0");
	free(removed);

	removed = (char*)ptable_remove(tab, keys[2]);
	assert_str_equal(removed, "2");
	free(removed);

	removed = (char*)ptable_remove(tab, keys[4]);
	assert_str_equal(removed, "4");
	free(removed);

	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[1]);
	assert_str_equal(iter->val, "1");

	// three
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[3]);
	assert_str_equal(iter->val, "3");

	// end
	iter = ptable_next(iter);
	assert_nul(iter);

	ptable_free_vals(tab, NULL);
}

void ptable_put__again(void **state) {
	const struct PTable *tab = ptable_init(3, 5);
	char *removed;

	char *keys[] = { "a", "b", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));

	assert_int_equal(ptable_size(tab), 2);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "1");

	// remove zero
	removed = (char*)ptable_remove(tab, keys[0]);
	assert_str_equal(removed, "0");
	free(removed);

	assert_int_equal(ptable_size(tab), 1);
	assert_nul(ptable_get(tab, keys[0]));

	// put zero again afterwards
	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_int_equal(ptable_size(tab), 2);

	// one
	const struct PTableIter *iter = ptable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[1]);
	assert_str_equal(iter->val, "1");

	// zero moved later
	iter = ptable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, keys[0]);
	assert_str_equal(iter->val, "0");

	// end
	iter = ptable_next(iter);
	assert_nul(iter);

	ptable_free_vals(tab, NULL);
}

void ptable_remove__existing(void **state) {
	const struct PTable *tab = ptable_init(3, 5);
	char *removed;

	char *keys[] = { "a", "b", "c", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], strdup("2")));

	assert_int_equal(ptable_size(tab), 3);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "1");
	assert_str_equal(ptable_get(tab, keys[2]), "2");

	// 1
	removed = (char*)ptable_remove(tab, keys[1]);
	assert_str_equal(removed, "1");
	free(removed);
	assert_int_equal(ptable_size(tab), 2);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_nul(ptable_get(tab, keys[1]));
	assert_str_equal(ptable_get(tab, keys[2]), "2");

	// 2
	removed = (char*)ptable_remove(tab, keys[2]);
	assert_str_equal(removed, "2");
	free(removed);
	assert_int_equal(ptable_size(tab), 1);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_nul(ptable_get(tab, keys[1]));
	assert_nul(ptable_get(tab, keys[2]));

	// 0
	removed = (char*)ptable_remove(tab, keys[0]);
	assert_str_equal(removed, "0");
	free(removed);
	assert_int_equal(ptable_size(tab), 0);
	assert_nul(ptable_get(tab, keys[0]));
	assert_nul(ptable_get(tab, keys[1]));
	assert_nul(ptable_get(tab, keys[2]));

	ptable_free_vals(tab, NULL);
}

void ptable_remove__inexistent(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", "d", };

	assert_nul(ptable_put(tab, keys[0], strdup("0")));
	assert_nul(ptable_put(tab, keys[1], strdup("1")));
	assert_nul(ptable_put(tab, keys[2], strdup("2")));

	assert_int_equal(ptable_size(tab), 3);
	assert_str_equal(ptable_get(tab, keys[0]), "0");
	assert_str_equal(ptable_get(tab, keys[1]), "1");
	assert_str_equal(ptable_get(tab, keys[2]), "2");

	// 1
	assert_nul(ptable_remove(tab, keys[3]));
	assert_int_equal(ptable_size(tab), 3);

	ptable_free_vals(tab, NULL);
}

void ptable_equal__length_different(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", "b", };

	assert_nul(ptable_put(a, keys[0], strdup("0")));
	assert_nul(ptable_put(a, keys[1], strdup("1")));

	assert_nul(ptable_put(b, keys[1], strdup("11")));

	assert_false(ptable_equal(a, b, NULL));

	ptable_free_vals(a, NULL);
	ptable_free_vals(b, NULL);
}

void ptable_equal__keys_different(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };

	assert_nul(ptable_put(a, keys[0], NULL));
	assert_nul(ptable_put(a, keys[1], NULL));

	assert_nul(ptable_put(b, keys[0], NULL));
	assert_nul(ptable_put(b, keys[2], NULL));

	assert_false(ptable_equal(a, b, NULL));

	ptable_free_vals(a, NULL);
	ptable_free(b);
}

void ptable_equal__pointers_ok(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };
	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_nul(ptable_put(a, keys[0], vals[0]));
	assert_nul(ptable_put(a, keys[1], vals[1]));
	assert_nul(ptable_put(a, keys[2], vals[2]));

	assert_nul(ptable_put(b, keys[0], vals[0]));
	assert_nul(ptable_put(b, keys[1], vals[1]));
	assert_nul(ptable_put(b, keys[2], vals[2]));

	assert_true(ptable_equal(a, b, NULL));

	ptable_free(a);
	ptable_free_vals(b, NULL);
}

void ptable_equal__pointers_different(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };
	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_nul(ptable_put(a, keys[0], vals[0]));
	assert_nul(ptable_put(a, keys[1], vals[1]));
	assert_nul(ptable_put(a, keys[2], vals[2]));

	assert_nul(ptable_put(b, keys[0], vals[0]));
	assert_nul(ptable_put(b, keys[1], vals[0]));
	assert_nul(ptable_put(b, keys[2], vals[0]));

	assert_false(ptable_equal(a, b, NULL));

	ptable_free_vals(a, NULL);
	ptable_free(b);
}

void ptable_equal__comparison_ok(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", };

	assert_nul(ptable_put(a, keys[0], strdup("1")));

	assert_nul(ptable_put(b, keys[0], strdup("1")));

	assert_true(ptable_equal(a, b, fn_comp_equals_strcmp));

	ptable_free_vals(a, NULL);
	ptable_free_vals(b, NULL);
}

void ptable_equal__comparison_different(void **state) {
	const struct PTable *a = ptable_init(3, 5);
	const struct PTable *b = ptable_init(3, 5);

	char *keys[] = { "a", };

	assert_nul(ptable_put(a, keys[0], strdup("0")));

	assert_nul(ptable_put(b, keys[0], strdup("1")));

	assert_false(ptable_equal(a, b, fn_comp_equals_strcmp));

	ptable_free_vals(a, NULL);
	ptable_free_vals(b, NULL);
}

void ptable_keys_slist__empty(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	assert_nul(ptable_keys_slist(tab));

	ptable_free_vals(tab, NULL);
}

void ptable_keys_slist__many(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", };

	ptable_put(tab, keys[0], strdup("0"));
	ptable_put(tab, keys[1], strdup("1"));

	struct SList *list = ptable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_ptr_equal(slist_at(list, 0), keys[0]);
	assert_ptr_equal(slist_at(list, 1), keys[1]);

	slist_free(&list);
	ptable_free_vals(tab, NULL);
}

void ptable_vals_slist__empty(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	assert_nul(ptable_vals_slist(tab));

	ptable_free_vals(tab, NULL);
}

void ptable_vals_slist__many(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };

	ptable_put(tab, keys[0], strdup("1"));
	ptable_put(tab, keys[1], NULL);
	ptable_put(tab, keys[2], strdup("3"));

	struct SList *list = ptable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "1");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "3");

	slist_free(&list);
	ptable_free_vals(tab, NULL);
}

void ptable_str__null(void **state) {
	assert_nul(ptable_str(NULL));
}

void ptable_str__empty(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *str = ptable_str(tab);
	assert_str_equal(str, "");

	free(str);
	ptable_free(tab);
}

void ptable_str__string_vals(void **state) {
	const struct PTable *tab = ptable_init(3, 5);

	char *keys[] = { "a", "b", "c", };
	char *vals[] = { "1", NULL, "3", };

	ptable_put(tab, keys[0], vals[0]);
	ptable_put(tab, keys[1], vals[1]);
	ptable_put(tab, keys[2], vals[2]);

	char expected[2048];
	snprintf(expected, sizeof(expected),
			"%p = 1\n"
			"%p = (null)\n"
			"%p = 3",
			(void*)keys[0],
			(void*)keys[1],
			(void*)keys[2]
			);

	char *actual = ptable_str(tab);
	assert_str_equal(expected, actual);

	free(actual);
	ptable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(ptable_init__size),
		TEST(ptable_init__invalid),

		TEST(ptable_free_vals__null),
		TEST(ptable_free_vals__free_val),
		TEST(ptable_free_vals__free_val_reentrant),

		TEST(ptable_put__new),
		TEST(ptable_put__overwrite),
		TEST(ptable_put__null),
		TEST(ptable_put__null_overwrite),
		TEST(ptable_put__grow),

		TEST(ptable_iter__empty),
		TEST(ptable_iter__vals),
		TEST(ptable_iter__removed),

		TEST(ptable_put__again),

		TEST(ptable_remove__existing),
		TEST(ptable_remove__inexistent),

		TEST(ptable_equal__length_different),
		TEST(ptable_equal__keys_different),
		TEST(ptable_equal__pointers_ok),
		TEST(ptable_equal__pointers_different),
		TEST(ptable_equal__comparison_ok),
		TEST(ptable_equal__comparison_different),

		TEST(ptable_keys_slist__empty),
		TEST(ptable_keys_slist__many),

		TEST(ptable_vals_slist__empty),
		TEST(ptable_vals_slist__many),

		TEST(ptable_str__null),
		TEST(ptable_str__empty),
		TEST(ptable_str__string_vals),
	};

	return RUN(tests);
}

