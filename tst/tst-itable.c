#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"

#include "itable.h"

/*
   diff -u \
   <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' tst/tst-itable.c) \
   <(sed -e 's/ptable/xtable/g ; s/PTable/XTable/g' tst/tst-ptable.c)
   */

static uint64_t KEYS[] = { 1, 2, 3, 4, 5, 6 };

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

void itable_init__size(void **state) {
	const struct ITable *tab = itable_init(5, 50);

	assert_non_nul(tab);

	assert_int_equal(itable_size(tab), 0);
	assert_int_equal(itable_capacity(tab), 5);

	itable_free_vals(tab, NULL);
}

void itable_init__invalid(void **state) {
	const struct ITable *tab = itable_init(0, 0);

	assert_nul(tab);
}

void itable_free_vals__null(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *val = strdup("0");

	itable_put(tab, KEYS[0], val);

	assert_int_equal(itable_size(tab), 1);

	// not much we can do here but valgrind
	itable_free_vals(tab, NULL);
}

void itable_free_vals__free_val(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *vals[] = { "0", "1", NULL, };

	expect_value(mock_free_val, val, "0");
	expect_value(mock_free_val, val, "1");

	itable_put(tab, KEYS[0], vals[0]);
	itable_put(tab, KEYS[1], vals[1]);
	itable_put(tab, KEYS[2], vals[2]);

	assert_int_equal(itable_size(tab), 3);

	itable_free_vals(tab, mock_free_val);
}

void free_val_itable(const void *val) {
	itable_free_vals(val, mock_free_val);
}

void itable_free_vals__free_val_reentrant(void **state) {
	const struct ITable *outer = itable_init(3, 5);
	const struct ITable *inner1 = itable_init(3, 5);
	const struct ITable *inner2 = itable_init(3, 5);

	char *vals[] = { "11", "12", "21", "22", };

	expect_value(mock_free_val, val, "11");
	expect_value(mock_free_val, val, "12");
	expect_value(mock_free_val, val, "21");
	expect_value(mock_free_val, val, "22");

	itable_put(inner1, KEYS[0], vals[0]);
	itable_put(inner1, KEYS[1], vals[1]);
	itable_put(inner2, KEYS[0], vals[2]);
	itable_put(inner2, KEYS[1], vals[3]);

	itable_put(outer, KEYS[0], (void*)inner1);
	itable_put(outer, KEYS[1], (void*)inner2);

	assert_int_equal(itable_size(outer), 2);

	itable_free_vals(outer, free_val_itable);
}

void itable_put__new(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));

	assert_int_equal(itable_size(tab), 2);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "1");

	itable_free_vals(tab, NULL);
}

void itable_put__overwrite(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	char *replaced = NULL;

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], strdup("2")));
	assert_nul(itable_put(tab, KEYS[3], strdup("3")));

	replaced = (char*)itable_put(tab, KEYS[1], strdup("10"));
	assert_str_equal(replaced, "1");
	free(replaced);

	replaced = (char*)itable_put(tab, KEYS[3], strdup("13"));
	assert_str_equal(replaced, "3");
	free(replaced);

	assert_int_equal(itable_size(tab), 4);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "10");
	assert_str_equal(itable_get(tab, KEYS[2]), "2");
	assert_str_equal(itable_get(tab, KEYS[3]), "13");

	itable_free_vals(tab, NULL);
}

void itable_put__null(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_int_equal(itable_size(tab), 1);

	assert_nul(itable_put(tab, KEYS[1], NULL));
	assert_int_equal(itable_size(tab), 2);

	assert_nul(itable_put(tab, KEYS[2], strdup("2")));
	assert_int_equal(itable_size(tab), 3);

	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_nul(itable_get(tab, KEYS[1]));
	assert_str_equal(itable_get(tab, KEYS[2]), "2");

	itable_free_vals(tab, NULL);
}

void itable_put__null_overwrite(void **state) {
	const struct ITable *tab = itable_init(5, 5);

	char *zero = "0";
	assert_nul(itable_put(tab, KEYS[0], zero));

	assert_str_equal(itable_get(tab, KEYS[0]), "0");

	assert_str_equal(itable_put(tab, KEYS[0], NULL), "0");

	assert_int_equal(itable_size(tab), 1);
	assert_nul(itable_get(tab, KEYS[0]));

	itable_free_vals(tab, NULL);
}

void itable_put__grow(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], strdup("2")));

	assert_int_equal(itable_size(tab), 3);
	assert_int_equal(itable_capacity(tab), 3);

	assert_nul(itable_put(tab, KEYS[3], strdup("3")));
	assert_int_equal(itable_size(tab), 4);
	assert_int_equal(itable_capacity(tab), 8);

	assert_nul(itable_put(tab, KEYS[4], strdup("4")));
	assert_nul(itable_put(tab, KEYS[5], strdup("5")));

	assert_int_equal(itable_size(tab), 6);
	assert_int_equal(itable_capacity(tab), 8);

	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "1");
	assert_str_equal(itable_get(tab, KEYS[2]), "2");

	assert_str_equal(itable_get(tab, KEYS[3]), "3");
	assert_str_equal(itable_get(tab, KEYS[4]), "4");
	assert_str_equal(itable_get(tab, KEYS[5]), "5");

	itable_free_vals(tab, NULL);
}

void itable_iter__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_iter(tab));

	itable_free_vals(tab, NULL);
}

void itable_iter__free(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));

	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[0]);
	assert_str_equal(iter->val, "0");

	// not much we can do here but valgrind
	itable_iter_free(iter);

	itable_free_vals(tab, NULL);
}

void itable_iter__vals(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_put(tab, KEYS[0], NULL));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], NULL));
	assert_nul(itable_put(tab, KEYS[3], strdup("3")));
	assert_nul(itable_put(tab, KEYS[4], NULL));

	assert_int_equal(itable_size(tab), 5);

	// zero
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[0]);
	assert_nul(iter->val);

	// one
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[1]);
	assert_str_equal(iter->val, "1");

	// two
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[2]);
	assert_nul(iter->val);

	// three
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[3]);
	assert_str_equal(iter->val, "3");

	// four
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[4]);
	assert_nul(iter->val);

	// end
	iter = itable_next(iter);
	assert_nul(iter);

	itable_free_vals(tab, NULL);
}

void itable_iter__removed(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *removed = NULL;

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], strdup("2")));
	assert_nul(itable_put(tab, KEYS[3], strdup("3")));
	assert_nul(itable_put(tab, KEYS[4], strdup("4")));

	removed = (char*)itable_remove(tab, KEYS[0]);
	assert_str_equal(removed, "0");
	free(removed);

	removed = (char*)itable_remove(tab, KEYS[2]);
	assert_str_equal(removed, "2");
	free(removed);

	removed = (char*)itable_remove(tab, KEYS[4]);
	assert_str_equal(removed, "4");
	free(removed);

	assert_int_equal(itable_size(tab), 2);

	// one
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[1]);
	assert_str_equal(iter->val, "1");

	// three
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[3]);
	assert_str_equal(iter->val, "3");

	// end
	iter = itable_next(iter);
	assert_nul(iter);

	itable_free_vals(tab, NULL);
}

void itable_put__again(void **state) {
	const struct ITable *tab = itable_init(3, 5);
	char *removed;

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));

	assert_int_equal(itable_size(tab), 2);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "1");

	// remove zero
	removed = (char*)itable_remove(tab, KEYS[0]);
	assert_str_equal(removed, "0");
	free(removed);

	assert_int_equal(itable_size(tab), 1);
	assert_nul(itable_get(tab, KEYS[0]));

	// put zero again afterwards
	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_int_equal(itable_size(tab), 2);

	// one
	const struct ITableIter *iter = itable_iter(tab);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[1]);
	assert_str_equal(iter->val, "1");

	// zero moved later
	iter = itable_next(iter);
	assert_non_nul(iter);
	assert_int_equal(iter->key, KEYS[0]);
	assert_str_equal(iter->val, "0");

	// end
	iter = itable_next(iter);
	assert_nul(iter);

	itable_free_vals(tab, NULL);
}

void itable_remove__existing(void **state) {
	const struct ITable *tab = itable_init(3, 5);
	char *removed;

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], strdup("2")));

	assert_int_equal(itable_size(tab), 3);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "1");
	assert_str_equal(itable_get(tab, KEYS[2]), "2");

	// 1
	removed = (char*)itable_remove(tab, KEYS[1]);
	assert_str_equal(removed, "1");
	free(removed);
	assert_int_equal(itable_size(tab), 2);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_nul(itable_get(tab, KEYS[1]));
	assert_str_equal(itable_get(tab, KEYS[2]), "2");

	// 2
	removed = (char*)itable_remove(tab, KEYS[2]);
	assert_str_equal(removed, "2");
	free(removed);
	assert_int_equal(itable_size(tab), 1);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_nul(itable_get(tab, KEYS[1]));
	assert_nul(itable_get(tab, KEYS[2]));

	// 0
	removed = (char*)itable_remove(tab, KEYS[0]);
	assert_str_equal(removed, "0");
	free(removed);
	assert_int_equal(itable_size(tab), 0);
	assert_nul(itable_get(tab, KEYS[0]));
	assert_nul(itable_get(tab, KEYS[1]));
	assert_nul(itable_get(tab, KEYS[2]));

	itable_free_vals(tab, NULL);
}

void itable_remove__inexistent(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_put(tab, KEYS[0], strdup("0")));
	assert_nul(itable_put(tab, KEYS[1], strdup("1")));
	assert_nul(itable_put(tab, KEYS[2], strdup("2")));

	assert_int_equal(itable_size(tab), 3);
	assert_str_equal(itable_get(tab, KEYS[0]), "0");
	assert_str_equal(itable_get(tab, KEYS[1]), "1");
	assert_str_equal(itable_get(tab, KEYS[2]), "2");

	// 1
	assert_nul(itable_remove(tab, KEYS[3]));
	assert_int_equal(itable_size(tab), 3);

	itable_free_vals(tab, NULL);
}

void itable_equal__length_different(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	assert_nul(itable_put(a, KEYS[0], strdup("0")));
	assert_nul(itable_put(a, KEYS[1], strdup("1")));

	assert_nul(itable_put(b, KEYS[1], strdup("11")));

	assert_false(itable_equal(a, b, NULL));

	itable_free_vals(a, NULL);
	itable_free_vals(b, NULL);
}

void itable_equal__keys_different(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	assert_nul(itable_put(a, KEYS[0], NULL));
	assert_nul(itable_put(a, KEYS[1], NULL));

	assert_nul(itable_put(b, KEYS[0], NULL));
	assert_nul(itable_put(b, KEYS[2], NULL));

	assert_false(itable_equal(a, b, NULL));

	itable_free_vals(a, NULL);
	itable_free(b);
}

void itable_equal__pointers_ok(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_nul(itable_put(a, KEYS[0], vals[0]));
	assert_nul(itable_put(a, KEYS[1], vals[1]));
	assert_nul(itable_put(a, KEYS[2], vals[2]));

	assert_nul(itable_put(b, KEYS[0], vals[0]));
	assert_nul(itable_put(b, KEYS[1], vals[1]));
	assert_nul(itable_put(b, KEYS[2], vals[2]));

	assert_true(itable_equal(a, b, NULL));

	itable_free(a);
	itable_free_vals(b, NULL);
}

void itable_equal__pointers_different(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	void *vals[] = { strdup("0"), strdup("1"), strdup("2"), };

	assert_nul(itable_put(a, KEYS[0], vals[0]));
	assert_nul(itable_put(a, KEYS[1], vals[1]));
	assert_nul(itable_put(a, KEYS[2], vals[2]));

	assert_nul(itable_put(b, KEYS[0], vals[0]));
	assert_nul(itable_put(b, KEYS[1], vals[0]));
	assert_nul(itable_put(b, KEYS[2], vals[0]));

	assert_false(itable_equal(a, b, NULL));

	itable_free_vals(a, NULL);
	itable_free(b);
}

void itable_equal__comparison_ok(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	assert_nul(itable_put(a, KEYS[0], strdup("1")));

	assert_nul(itable_put(b, KEYS[0], strdup("1")));

	assert_true(itable_equal(a, b, fn_comp_equals_strcmp));

	itable_free_vals(a, NULL);
	itable_free_vals(b, NULL);
}

void itable_equal__comparison_different(void **state) {
	const struct ITable *a = itable_init(3, 5);
	const struct ITable *b = itable_init(3, 5);

	assert_nul(itable_put(a, KEYS[0], strdup("0")));

	assert_nul(itable_put(b, KEYS[0], strdup("1")));

	assert_false(itable_equal(a, b, fn_comp_equals_strcmp));

	itable_free_vals(a, NULL);
	itable_free_vals(b, NULL);
}

void itable_keys_slist__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_keys_slist(tab));

	itable_free_vals(tab, NULL);
}

void itable_keys_slist__many(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	itable_put(tab, KEYS[0], strdup("0"));
	itable_put(tab, KEYS[1], strdup("1"));

	struct SList *list = itable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_int_equal(*(uint64_t*)slist_at(list, 0), 1);
	assert_int_equal(*(uint64_t*)slist_at(list, 1), 2);

	slist_free(&list);
	itable_free_vals(tab, NULL);
}

void itable_vals_slist__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_vals_slist(tab));

	itable_free_vals(tab, NULL);
}

void itable_vals_slist__many(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	itable_put(tab, KEYS[0], strdup("1"));
	itable_put(tab, KEYS[1], NULL);
	itable_put(tab, KEYS[2], strdup("3"));

	struct SList *list = itable_vals_slist(tab);

	assert_int_equal(slist_length(list), 3);
	assert_str_equal(slist_at(list, 0), "1");
	assert_nul(slist_at(list, 1));
	assert_str_equal(slist_at(list, 2), "3");

	slist_free(&list);
	itable_free_vals(tab, NULL);
}

void itable_str__null(void **state) {
	assert_nul(itable_str(NULL));
}

void itable_str__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *str = itable_str(tab);
	assert_str_equal(str, "");

	free(str);
	itable_free(tab);
}

void itable_str__string_vals(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	char *vals[] = { "1", NULL, "3", };

	itable_put(tab, KEYS[0], vals[0]);
	itable_put(tab, KEYS[1], vals[1]);
	itable_put(tab, KEYS[2], vals[2]);

	char expected[2048];
	snprintf(expected, sizeof(expected),
			"%"PRIu64" = 1\n"
			"%"PRIu64" = (null)\n"
			"%"PRIu64" = 3",
			KEYS[0],
			KEYS[1],
			KEYS[2]
			);

	char *actual = itable_str(tab);
	assert_str_equal(expected, actual);

	free(actual);
	itable_free(tab);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(itable_init__size),
		TEST(itable_init__invalid),

		TEST(itable_free_vals__null),
		TEST(itable_free_vals__free_val),
		TEST(itable_free_vals__free_val_reentrant),

		TEST(itable_put__new),
		TEST(itable_put__overwrite),
		TEST(itable_put__null),
		TEST(itable_put__null_overwrite),
		TEST(itable_put__grow),

		TEST(itable_iter__empty),
		TEST(itable_iter__free),
		TEST(itable_iter__vals),
		TEST(itable_iter__removed),

		TEST(itable_put__again),

		TEST(itable_remove__existing),
		TEST(itable_remove__inexistent),

		TEST(itable_equal__length_different),
		TEST(itable_equal__keys_different),
		TEST(itable_equal__pointers_ok),
		TEST(itable_equal__pointers_different),
		TEST(itable_equal__comparison_ok),
		TEST(itable_equal__comparison_different),

		TEST(itable_keys_slist__empty),
		TEST(itable_keys_slist__many),

		TEST(itable_vals_slist__empty),
		TEST(itable_vals_slist__many),

		TEST(itable_str__null),
		TEST(itable_str__empty),
		TEST(itable_str__string_vals),
	};

	return RUN(tests);
}

