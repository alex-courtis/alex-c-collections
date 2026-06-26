#include "tst.h"
#include "asserts.h"
#include "assert-slist.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"

#include "slist.h"
#include "str.h"

#include "data/words-sorted.c"
#include "data/words-unsorted.c"

static char* str_first(const void *val) {
	return strndup(val, 1);
}

static bool test_contains_x(const void* const val) {
	if (strcmp("x", val) == 0) {
		return true;
	}
	return false;
}

static bool test_false(const void* const val) {
	return false;
}

static bool less_than_int(const void *a, const void *b) {
	if (a && b)
		return (*(int*)a < *(int*)b);
	else if (a && !b)
		return true;
	else
		return false;
}

static void slist_free_vals__many(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	expect_str(mock_free, val, "0");
	expect_str(mock_free, val, "1");
	expect_str(mock_free, val, "2");

	slist_free_vals(&list, mock_free);

	assert_nul(list);
}

static void slist_remove__every(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	// mid
	struct SList *i = list->nex;
	assert_str_equal(i->val, "1");
	assert_str_equal(slist_remove(&list, &i), "1");
	assert_nul(i);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 2);

	// first
	i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(slist_remove(&list, &i), "0");
	assert_nul(i);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 1);

	// last
	i = list;
	assert_str_equal(i->val, "2");
	assert_str_equal(slist_remove(&list, &i), "2");
	assert_nul(i);

	assert_nul(list);
	assert_int_equal(slist_length(list), 0);

	slist_free(&list);
}

static void slist_remove__inexistent(void **state) {
	struct SList *list = NULL;

	slist_append(&list, strdup("0"));

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 1);

	assert_nul(slist_remove(&list, NULL));

	struct SList *other = NULL;
	slist_append(&list, strdup("x"));

	assert_nul(slist_remove(&list, &other));

	slist_free_vals(&list, NULL);
	slist_free_vals(&other, NULL);
}

static void slist_remove_all__some(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);
	slist_append(&list, vals[3]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 4);

	slist_remove_all(&list, (fn_equal)equal_strcmp, "x");

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 2);

	const struct SList *i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(slist_at(list, 0), "0");

	i = i->nex;
	assert_non_nul(i);
	assert_str_equal(i->val, "2");
	assert_str_equal(slist_at(list, 1), "2");

	i = i->nex;
	assert_nul(i);

	slist_free(&list);
}

static void slist_remove_all_free__some(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);
	slist_append(&list, vals[3]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 4);

	expect_str(mock_free, val, "x");
	expect_str(mock_free, val, "x");

	slist_remove_all_free(&list, (fn_equal)equal_strcmp, "x", mock_free);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 2);

	const struct SList *i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(slist_at(list, 0), "0");

	i = i->nex;
	assert_non_nul(i);
	assert_str_equal(i->val, "2");
	assert_str_equal(slist_at(list, 1), "2");

	i = i->nex;
	assert_nul(i);

	slist_free(&list);
}

static void slist_find__no(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	const void *val = slist_find_val(list, test_false);
	assert_nul(val);

	const struct SList *i = slist_find(list, test_false);
	assert_nul(i);

	assert_nul(slist_find(list, NULL));

	slist_free(&list);
}

static void slist_find__yes(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	const void *val = slist_find_val(list, test_contains_x);
	assert_non_nul(val);
	assert_str_equal(val, "x");

	const struct SList *i = slist_find(list, test_contains_x);
	assert_non_nul(i);
	assert_str_equal(i->val, "x");

	slist_free(&list);
}

static void slist_find_equal_val__no(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	const void *val = slist_find_equal_val(list, (fn_equal)equal_strcmp, "x");
	assert_nul(val);

	const struct SList *i = slist_find_equal(list, (fn_equal)equal_strcmp, "x");
	assert_nul(i);

	slist_free(&list);
}

static void slist_find_equal_val__yes(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(slist_length(list), 3);

	const void *val = slist_find_equal_val(list, (fn_equal)equal_strcmp, "1");
	assert_non_nul(val);
	assert_str_equal(val, "1");

	const struct SList *i = slist_find_equal(list, (fn_equal)equal_strcmp, "1");
	assert_non_nul(i);
	assert_str_equal(i->val, "1");

	i = slist_find_equal(list, NULL, vals[1]);
	assert_non_nul(i);
	assert_str_equal(i->val, "1");

	slist_free(&list);
}

static void slist_equal__empty_lhs(void **state) {
	struct SList *rhs = NULL;

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_not_equal(NULL, rhs, (fn_equal)equal_strcmp, NULL);

	assert_slist_not_equal(NULL, rhs, NULL, NULL);

	slist_free(&rhs);
}

static void slist_equal__empty_rhs(void **state) {
	struct SList *lhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	assert_slist_not_equal(lhs, NULL, (fn_equal)equal_strcmp, NULL);

	assert_slist_not_equal(lhs, NULL, NULL, NULL);

	slist_free(&lhs);
}

static void slist_equal__equal(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_equal__not_equal_start(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "x", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_not_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);
	assert_slist_not_equal(lhs, rhs, NULL, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_equal__not_equal_mid(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "x", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_not_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);
	assert_slist_not_equal(lhs, rhs, NULL, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_equal__not_equal_end(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", "x", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_not_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);
	assert_slist_not_equal(lhs, rhs, NULL, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_equal__not_equal_lhs_size(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_slist_not_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);
	assert_slist_not_equal(lhs, rhs, NULL, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_equal__not_equal_rhs_size(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);

	assert_slist_not_equal(lhs, rhs, (fn_equal)equal_strcmp, NULL);
	assert_slist_not_equal(lhs, rhs, NULL, NULL);

	slist_free(&lhs);
	slist_free(&rhs);
}

static void slist_sort__empty(void **state) {
	struct SList *from = NULL;

	const struct SList *to = slist_sort(from, less_than_int);
	assert_nul(to);
}

static void slist_sort__one(void **state) {
	struct SList *from = NULL;

	slist_append(&from, strdup("only"));

	struct SList *to = slist_sort(from, mock_less_than);

	assert_non_nul(to);

	assert_nul(slist_sort(from, NULL));

	slist_free_vals(&from, NULL);
	slist_free(&to);
}

static void slist_sort__vals(void **state) {
	struct SList *from = NULL;

	int vals[] = { 3, 2, 5, 4, 1, 0 };
	slist_append(&from, NULL);
	slist_append(&from, &vals[0]);
	slist_append(&from, NULL);
	slist_append(&from, &vals[1]);
	slist_append(&from, &vals[2]);
	slist_append(&from, NULL);
	slist_append(&from, &vals[3]);
	slist_append(&from, &vals[4]);
	slist_append(&from, &vals[5]);
	slist_append(&from, NULL);

	struct SList *to = slist_sort(from, less_than_int);
	assert_non_nul(to);

	assert_int_equal(slist_length(to), 10);

	// first 6 are integers
	for (int i = 0; i < 6; i++) {
		assert_non_nul(slist_at(to, i));
		assert_int_equal(*(int*)slist_at(to, i), i);
	}

	// remaining 4 are null
	for (int i = 6; i < 10; i++) {
		assert_nul(slist_at(to, i));
	}

	slist_free(&from);
	slist_free(&to);
}

static void slist_sort__words(void **state) {
	struct SList *from = NULL;
	for (size_t i = 0; i < sizeof(words_unsorted) / sizeof(words_unsorted[0]); i++) {
		slist_append(&from, (void*)words_unsorted[i]);
	}

	struct SList *actual = slist_sort(from, (fn_equal)less_than_strcmp);
	assert_non_nul(actual);

	assert_int_equal(slist_length(actual), slist_length(from));

	struct SList *expected = NULL;
	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++) {
		slist_append(&expected, (void*)words_sorted[i]);
	}

	assert_slist_equal(actual, expected, (fn_equal)equal_strcmp, NULL);

	slist_free(&from);
	slist_free(&actual);
	slist_free(&expected);
}

static void slist_move__empty(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	slist_move(&to, &from, (fn_equal)equal_strcmp, "x");
	assert_nul(to);

	slist_move(&to, NULL, (fn_equal)equal_strcmp, "x");
	assert_nul(to);

	slist_move(&to, &from, NULL, "x");
	assert_nul(to);

	slist_move(NULL, NULL, NULL, "x");

	assert_nul(from);
}

static void slist_move__empty_to(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&from, vals[0]);
	slist_append(&from, vals[1]);
	slist_append(&from, vals[2]);

	slist_move(&to, &from, (fn_equal)equal_strcmp, "x");

	assert_int_equal(slist_length(to), 0);
	assert_int_equal(slist_length(from), 3);

	slist_free(&from);
}

static void slist_move__empty_from(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&to, vals[0]);
	slist_append(&to, vals[1]);
	slist_append(&to, vals[2]);

	slist_move(&to, &from, (fn_equal)equal_strcmp, "x");

	assert_int_equal(slist_length(to), 3);
	assert_int_equal(slist_length(from), 0);

	slist_free(&to);
}

static void slist_move__no_match(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *to_vals[] = { "0", "1", "2", };
	slist_append(&to, to_vals[0]);
	slist_append(&to, to_vals[1]);
	slist_append(&to, to_vals[2]);

	void *from_vals[] = { "0", "1", "2", };
	slist_append(&from, from_vals[0]);
	slist_append(&from, from_vals[1]);
	slist_append(&from, from_vals[2]);

	slist_move(&to, &from, (fn_equal)equal_strcmp, "x");

	assert_int_equal(slist_length(to), 3);
	assert_int_equal(slist_length(from), 3);

	slist_free(&to);
	slist_free(&from);
}

static void slist_move__many(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *to_vals[] = { "0", "1", "2", };
	slist_append(&to, to_vals[0]);
	slist_append(&to, to_vals[1]);
	slist_append(&to, to_vals[2]);

	void *from_vals[] = { "x0", "1", "x2", "3", "x4", };
	slist_append(&from, from_vals[0]);
	slist_append(&from, from_vals[1]);
	slist_append(&from, from_vals[2]);
	slist_append(&from, from_vals[3]);
	slist_append(&from, from_vals[4]);

	slist_move(&to, &from, (fn_equal)equal_strstr, "x");

	// values moved
	assert_int_equal(slist_length(to), 6);
	assert_str_equal(slist_at(to, 0), "0");
	assert_str_equal(slist_at(to, 1), "1");
	assert_str_equal(slist_at(to, 2), "2");
	assert_str_equal(slist_at(to, 3), "x0");
	assert_str_equal(slist_at(to, 4), "x2");
	assert_str_equal(slist_at(to, 5), "x4");

	// values remaining
	assert_int_equal(slist_length(from), 2);
	assert_str_equal(slist_at(from, 0), "1");
	assert_str_equal(slist_at(from, 1), "3");

	slist_free(&to);
	slist_free(&from);
}

static void slist_move__all(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *to_vals[] = { "0", "1", };
	slist_append(&to, to_vals[0]);
	slist_append(&to, to_vals[1]);

	void *from_vals[] = { "x0", "x1", };
	slist_append(&from, from_vals[0]);
	slist_append(&from, from_vals[1]);

	slist_move(&to, &from, (fn_equal)equal_strstr, "x");

	// values moved
	assert_int_equal(slist_length(to), 4);
	assert_str_equal(slist_at(to, 0), "0");
	assert_str_equal(slist_at(to, 1), "1");
	assert_str_equal(slist_at(to, 2), "x0");
	assert_str_equal(slist_at(to, 3), "x1");

	// values remaining
	assert_int_equal(slist_length(from), 0);

	slist_free(&to);
	slist_free(&from);
}

static void slist_at__inexistent(void **state) {
	struct SList *to = NULL;

	slist_append(&to, strdup("0"));

	assert_nul(slist_at(to, 2));

	slist_free_vals(&to, NULL);
}

static void slist_clone__deep_empty(void **state) {
	assert_nul(slist_clone(NULL, clone_strdup));
}

static void slist_clone__deep_vals(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);

	struct SList *cloned = slist_clone(list, clone_strdup);

	assert_non_nul(cloned);

	assert_str_equal(slist_at(cloned, 0), "0");
	assert_str_equal(slist_at(cloned, 1), "1");

	slist_free(&list);
	slist_free_vals(&cloned, NULL);
}

static void slist_clone__shallow_empty(void **state) {
	assert_nul(slist_clone(NULL, NULL));
}

static void slist_clone__shallow_vals(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);

	struct SList *cloned = slist_clone(list, NULL);

	assert_non_nul(cloned);

	assert_str_equal(slist_at(cloned, 0), "0");
	assert_str_equal(slist_at(cloned, 1), "1");

	slist_free(&list);
	slist_free(&cloned);
}

static void slist_str__null(void **state) {
	assert_nul(slist_str(NULL, NULL));
}

static void slist_str__string_vals(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", NULL, "3", };

	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);
	slist_append(&list, vals[3]);

	char *actual = slist_str(list, NULL);

	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n"
			"(null)\n"
			"%p\n",
			vals[0],
			vals[1],
			vals[3]
			);

	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	slist_free(&list);
}

static void slist_str__fn_str(void **state) {
	struct SList *list = NULL;

	slist_append(&list, "zero");
	slist_append(&list, "one");
	slist_append(&list, "two");
	slist_append(&list, NULL);

	char *str = slist_str(list, str_first);
	assert_str_equal(str,
			"z\n"
			"o\n"
			"t\n"
			"(null)\n"
			);

	free(str);
	slist_free(&list);
}

static void slist_xor_free__empty_lists(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, clone_strdup);

	assert_int_equal(slist_length(list1), 0);
}

static void slist_xor_free__first_list_empty(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;
	struct SList *expected = NULL;

	slist_append(&list2, strdup("item1"));
	slist_append(&list2, strdup("item2"));

	slist_append(&expected, strdup("item1"));
	slist_append(&expected, strdup("item2"));

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, clone_strdup);

	assert_slist_equal(list1, expected, (fn_equal)equal_strcmp, NULL);

	slist_free_vals(&list1, NULL);
	slist_free_vals(&list2, NULL);
	slist_free_vals(&expected, NULL);
}

static void slist_xor_free__second_list_empty(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;
	struct SList *expected = NULL;

	slist_append(&list1, strdup("item1"));
	slist_append(&list1, strdup("item2"));

	slist_append(&expected, strdup("item1"));
	slist_append(&expected, strdup("item2"));

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, clone_strdup);

	assert_slist_equal(list1, expected, (fn_equal)equal_strcmp, NULL);

	slist_free_vals(&list1, NULL);
	slist_free_vals(&list2, NULL);
	slist_free_vals(&expected, NULL);
}

static void slist_xor_free__toggle_items(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;
	struct SList *expected = NULL;

	slist_append(&list1, strdup("item1"));
	slist_append(&list1, strdup("item2"));
	slist_append(&list1, strdup("item3"));

	slist_append(&list2, strdup("item2"));
	slist_append(&list2, strdup("item4"));

	slist_append(&expected, strdup("item1"));
	slist_append(&expected, strdup("item3"));
	slist_append(&expected, strdup("item4"));

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, clone_strdup);

	assert_slist_equal(list1, expected, (fn_equal)equal_strcmp, NULL);

	slist_free_vals(&list1, NULL);
	slist_free_vals(&list2, NULL);
	slist_free_vals(&expected, NULL);
}

static void slist_xor_free__duplicate_items(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;
	struct SList *expected = NULL;

	slist_append(&list1, strdup("item1"));
	slist_append(&list1, strdup("item1")); // duplicate
	slist_append(&list1, strdup("item2"));

	slist_append(&list2, strdup("item1"));

	slist_append(&expected, strdup("item2"));

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, clone_strdup);

	assert_slist_equal(list1, expected, (fn_equal)equal_strcmp, NULL);

	slist_free_vals(&list1, NULL);
	slist_free_vals(&list2, NULL);
	slist_free_vals(&expected, NULL);
}

static void slist_xor_free__vals(void **state) {
	struct SList *list1 = NULL;
	struct SList *list2 = NULL;
	struct SList *expected = NULL;

	char *val = "item1";

	slist_append(&list2, val);

	slist_append(&expected, val);

	slist_xor_free(&list1, list2, (fn_equal)equal_strcmp, NULL, NULL);

	assert_slist_equal(list1, expected, (fn_equal)equal_strcmp, NULL);

	slist_free(&list1);
	slist_free(&list2);
	slist_free(&expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(slist_free_vals__many),

		TEST(slist_remove__every),
		TEST(slist_remove__inexistent),

		TEST(slist_remove_all__some),
		TEST(slist_remove_all_free__some),

		TEST(slist_find__no),
		TEST(slist_find__yes),

		TEST(slist_find_equal_val__no),
		TEST(slist_find_equal_val__yes),

		TEST(slist_equal__empty_lhs),
		TEST(slist_equal__empty_rhs),
		TEST(slist_equal__equal),
		TEST(slist_equal__not_equal_start),
		TEST(slist_equal__not_equal_mid),
		TEST(slist_equal__not_equal_end),
		TEST(slist_equal__not_equal_lhs_size),
		TEST(slist_equal__not_equal_rhs_size),

		TEST(slist_sort__empty),
		TEST(slist_sort__one),
		TEST(slist_sort__vals),
		TEST(slist_sort__words),

		TEST(slist_move__empty),
		TEST(slist_move__empty_to),
		TEST(slist_move__empty_from),
		TEST(slist_move__no_match),
		TEST(slist_move__many),
		TEST(slist_move__all),

		TEST(slist_at__inexistent),

		TEST(slist_clone__deep_empty),
		TEST(slist_clone__deep_vals),

		TEST(slist_clone__shallow_empty),
		TEST(slist_clone__shallow_vals),

		TEST(slist_str__null),
		TEST(slist_str__string_vals),
		TEST(slist_str__fn_str),

		TEST(slist_xor_free__empty_lists),
		TEST(slist_xor_free__first_list_empty),
		TEST(slist_xor_free__second_list_empty),
		TEST(slist_xor_free__toggle_items),
		TEST(slist_xor_free__duplicate_items),
		TEST(slist_xor_free__vals),
	};

	return RUN(tests);
}

