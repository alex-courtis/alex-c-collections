#include "tst.h"
#include "asserts.h"
#include "assert-pslist.h"
#include "expects.h"
#include "mock-fn.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"

#include "pslist.h"
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

static void pslist_free_vals__many(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	expect_str(mock_free, ptr, "0");
	expect_str(mock_free, ptr, "1");
	expect_str(mock_free, ptr, "2");

	pslist_free_vals(&list, mock_free);

	assert_nul(list);
}

static void pslist_remove__every(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	// mid
	struct Pslist *i = list->nex;
	assert_str_equal(i->val, "1");
	assert_str_equal(pslist_remove(&list, &i), "1");
	assert_nul(i);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 2);

	// first
	i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(pslist_remove(&list, &i), "0");
	assert_nul(i);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 1);

	// last
	i = list;
	assert_str_equal(i->val, "2");
	assert_str_equal(pslist_remove(&list, &i), "2");
	assert_nul(i);

	assert_nul(list);
	assert_int_equal(pslist_length(list), 0);

	pslist_free(&list);
}

static void pslist_remove__inexistent(void **state) {
	struct Pslist *list = NULL;

	pslist_append(&list, strdup("0"));

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 1);

	assert_nul(pslist_remove(&list, NULL));

	struct Pslist *other = NULL;
	pslist_append(&list, strdup("x"));

	assert_nul(pslist_remove(&list, &other));

	pslist_free_vals(&list, NULL);
	pslist_free_vals(&other, NULL);
}

static void pslist_remove_all__some(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);
	pslist_append(&list, vals[3]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 4);

	pslist_remove_from(&list, (fn_2pred)equal_strcmp, "x");

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 2);

	const struct Pslist *i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(pslist_at(list, 0), "0");

	i = i->nex;
	assert_non_nul(i);
	assert_str_equal(i->val, "2");
	assert_str_equal(pslist_at(list, 1), "2");

	i = i->nex;
	assert_nul(i);

	pslist_free(&list);
}

static void pslist_remove_all_free__some(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);
	pslist_append(&list, vals[3]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 4);

	expect_str(mock_free, ptr, "x");
	expect_str(mock_free, ptr, "x");

	pslist_remove_all_free(&list, (fn_2pred)equal_strcmp, "x", mock_free);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 2);

	const struct Pslist *i = list;
	assert_str_equal(i->val, "0");
	assert_str_equal(pslist_at(list, 0), "0");

	i = i->nex;
	assert_non_nul(i);
	assert_str_equal(i->val, "2");
	assert_str_equal(pslist_at(list, 1), "2");

	i = i->nex;
	assert_nul(i);

	pslist_free(&list);
}

static void pslist_find__no(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	const void *val = pslist_find_val(list, test_false);
	assert_nul(val);

	const struct Pslist *i = pslist_find(list, test_false);
	assert_nul(i);

	assert_nul(pslist_find(list, NULL));

	pslist_free(&list);
}

static void pslist_find__yes(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "x", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	const void *val = pslist_find_val(list, test_contains_x);
	assert_non_nul(val);
	assert_str_equal(val, "x");

	const struct Pslist *i = pslist_find(list, test_contains_x);
	assert_non_nul(i);
	assert_str_equal(i->val, "x");

	pslist_free(&list);
}

static void pslist_find_equal_val__no(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	const void *val = pslist_find_equal_val(list, (fn_2pred)equal_strcmp, "x");
	assert_nul(val);

	const struct Pslist *i = pslist_find_equal(list, (fn_2pred)equal_strcmp, "x");
	assert_nul(i);

	pslist_free(&list);
}

static void pslist_find_equal_val__yes(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);

	assert_non_nul(list);
	assert_int_equal(pslist_length(list), 3);

	const void *val = pslist_find_equal_val(list, (fn_2pred)equal_strcmp, "1");
	assert_non_nul(val);
	assert_str_equal(val, "1");

	const struct Pslist *i = pslist_find_equal(list, (fn_2pred)equal_strcmp, "1");
	assert_non_nul(i);
	assert_str_equal(i->val, "1");

	i = pslist_find_equal(list, NULL, vals[1]);
	assert_non_nul(i);
	assert_str_equal(i->val, "1");

	pslist_free(&list);
}

static void pslist_equal__empty_lhs(void **state) {
	struct Pslist *rhs = NULL;

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_not_equal(NULL, rhs, (fn_2pred)equal_strcmp, NULL);

	assert_pslist_not_equal(NULL, rhs, NULL, NULL);

	pslist_free(&rhs);
}

static void pslist_equal__empty_rhs(void **state) {
	struct Pslist *lhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	assert_pslist_not_equal(lhs, NULL, (fn_2pred)equal_strcmp, NULL);

	assert_pslist_not_equal(lhs, NULL, NULL, NULL);

	pslist_free(&lhs);
}

static void pslist_equal__equal(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_equal__not_equal_start(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "x", "1", "2", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_not_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);
	assert_pslist_not_equal(lhs, rhs, NULL, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_equal__not_equal_mid(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "0", "x", "2", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_not_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);
	assert_pslist_not_equal(lhs, rhs, NULL, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_equal__not_equal_end(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "0", "1", "x", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_not_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);
	assert_pslist_not_equal(lhs, rhs, NULL, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_equal__not_equal_lhs_size(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "0", "1", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);

	void *rvals[] = { "0", "1", "2", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);
	pslist_append(&rhs, rvals[2]);

	assert_pslist_not_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);
	assert_pslist_not_equal(lhs, rhs, NULL, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_equal__not_equal_rhs_size(void **state) {
	struct Pslist *lhs = NULL;
	struct Pslist *rhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	pslist_append(&lhs, lvals[0]);
	pslist_append(&lhs, lvals[1]);
	pslist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", };
	pslist_append(&rhs, rvals[0]);
	pslist_append(&rhs, rvals[1]);

	assert_pslist_not_equal(lhs, rhs, (fn_2pred)equal_strcmp, NULL);
	assert_pslist_not_equal(lhs, rhs, NULL, NULL);

	pslist_free(&lhs);
	pslist_free(&rhs);
}

static void pslist_sort__empty(void **state) {
	struct Pslist *from = NULL;

	const struct Pslist *to = pslist_sort(from, less_than_int);
	assert_nul(to);
}

static void pslist_sort__one(void **state) {
	struct Pslist *from = NULL;

	pslist_append(&from, strdup("only"));

	struct Pslist *to = pslist_sort(from, mock_less_than);

	assert_non_nul(to);

	assert_nul(pslist_sort(from, NULL));

	pslist_free_vals(&from, NULL);
	pslist_free(&to);
}

static void pslist_sort__vals(void **state) {
	struct Pslist *from = NULL;

	int vals[] = { 3, 2, 5, 4, 1, 0 };
	pslist_append(&from, NULL);
	pslist_append(&from, &vals[0]);
	pslist_append(&from, NULL);
	pslist_append(&from, &vals[1]);
	pslist_append(&from, &vals[2]);
	pslist_append(&from, NULL);
	pslist_append(&from, &vals[3]);
	pslist_append(&from, &vals[4]);
	pslist_append(&from, &vals[5]);
	pslist_append(&from, NULL);

	struct Pslist *to = pslist_sort(from, less_than_int);
	assert_non_nul(to);

	assert_int_equal(pslist_length(to), 10);

	// first 6 are integers
	for (int i = 0; i < 6; i++) {
		assert_non_nul(pslist_at(to, i));
		assert_int_equal(*(int*)pslist_at(to, i), i);
	}

	// remaining 4 are null
	for (int i = 6; i < 10; i++) {
		assert_nul(pslist_at(to, i));
	}

	pslist_free(&from);
	pslist_free(&to);
}

static void pslist_sort__words(void **state) {
	struct Pslist *from = NULL;
	for (size_t i = 0; i < sizeof(words_unsorted) / sizeof(words_unsorted[0]); i++) {
		pslist_append(&from, (void*)words_unsorted[i]);
	}

	struct Pslist *actual = pslist_sort(from, (fn_2pred)less_than_strcmp);
	assert_non_nul(actual);

	assert_int_equal(pslist_length(actual), pslist_length(from));

	struct Pslist *expected = NULL;
	for (size_t i = 0; i < sizeof(words_sorted) / sizeof(words_sorted[0]); i++) {
		pslist_append(&expected, (void*)words_sorted[i]);
	}

	assert_pslist_equal(actual, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free(&from);
	pslist_free(&actual);
	pslist_free(&expected);
}

static void pslist_move__empty(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	pslist_move(&to, &from, (fn_2pred)equal_strcmp, "x");
	assert_nul(to);

	pslist_move(&to, NULL, (fn_2pred)equal_strcmp, "x");
	assert_nul(to);

	pslist_move(&to, &from, NULL, "x");
	assert_nul(to);

	pslist_move(NULL, NULL, NULL, "x");

	assert_nul(from);
}

static void pslist_move__empty_to(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&from, vals[0]);
	pslist_append(&from, vals[1]);
	pslist_append(&from, vals[2]);

	pslist_move(&to, &from, (fn_2pred)equal_strcmp, "x");

	assert_int_equal(pslist_length(to), 0);
	assert_int_equal(pslist_length(from), 3);

	pslist_free(&from);
}

static void pslist_move__empty_from(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	void *vals[] = { "0", "1", "2", };
	pslist_append(&to, vals[0]);
	pslist_append(&to, vals[1]);
	pslist_append(&to, vals[2]);

	pslist_move(&to, &from, (fn_2pred)equal_strcmp, "x");

	assert_int_equal(pslist_length(to), 3);
	assert_int_equal(pslist_length(from), 0);

	pslist_free(&to);
}

static void pslist_move__no_match(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	void *to_vals[] = { "0", "1", "2", };
	pslist_append(&to, to_vals[0]);
	pslist_append(&to, to_vals[1]);
	pslist_append(&to, to_vals[2]);

	void *from_vals[] = { "0", "1", "2", };
	pslist_append(&from, from_vals[0]);
	pslist_append(&from, from_vals[1]);
	pslist_append(&from, from_vals[2]);

	pslist_move(&to, &from, (fn_2pred)equal_strcmp, "x");

	assert_int_equal(pslist_length(to), 3);
	assert_int_equal(pslist_length(from), 3);

	pslist_free(&to);
	pslist_free(&from);
}

static void pslist_move__many(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	void *to_vals[] = { "0", "1", "2", };
	pslist_append(&to, to_vals[0]);
	pslist_append(&to, to_vals[1]);
	pslist_append(&to, to_vals[2]);

	void *from_vals[] = { "x0", "1", "x2", "3", "x4", };
	pslist_append(&from, from_vals[0]);
	pslist_append(&from, from_vals[1]);
	pslist_append(&from, from_vals[2]);
	pslist_append(&from, from_vals[3]);
	pslist_append(&from, from_vals[4]);

	pslist_move(&to, &from, (fn_2pred)equal_strstr, "x");

	// values moved
	assert_int_equal(pslist_length(to), 6);
	assert_str_equal(pslist_at(to, 0), "0");
	assert_str_equal(pslist_at(to, 1), "1");
	assert_str_equal(pslist_at(to, 2), "2");
	assert_str_equal(pslist_at(to, 3), "x0");
	assert_str_equal(pslist_at(to, 4), "x2");
	assert_str_equal(pslist_at(to, 5), "x4");

	// values remaining
	assert_int_equal(pslist_length(from), 2);
	assert_str_equal(pslist_at(from, 0), "1");
	assert_str_equal(pslist_at(from, 1), "3");

	pslist_free(&to);
	pslist_free(&from);
}

static void pslist_move__all(void **state) {
	struct Pslist *to = NULL;
	struct Pslist *from = NULL;

	void *to_vals[] = { "0", "1", };
	pslist_append(&to, to_vals[0]);
	pslist_append(&to, to_vals[1]);

	void *from_vals[] = { "x0", "x1", };
	pslist_append(&from, from_vals[0]);
	pslist_append(&from, from_vals[1]);

	pslist_move(&to, &from, (fn_2pred)equal_strstr, "x");

	// values moved
	assert_int_equal(pslist_length(to), 4);
	assert_str_equal(pslist_at(to, 0), "0");
	assert_str_equal(pslist_at(to, 1), "1");
	assert_str_equal(pslist_at(to, 2), "x0");
	assert_str_equal(pslist_at(to, 3), "x1");

	// values remaining
	assert_int_equal(pslist_length(from), 0);

	pslist_free(&to);
	pslist_free(&from);
}

static void pslist_at__inexistent(void **state) {
	struct Pslist *to = NULL;

	pslist_append(&to, strdup("0"));

	assert_nul(pslist_at(to, 2));

	pslist_free_vals(&to, NULL);
}

static void pslist_clone__deep_empty(void **state) {
	assert_nul(pslist_clone(NULL, (fn_clone)clone_strdup));
}

static void pslist_clone__deep_vals(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);

	struct Pslist *cloned = pslist_clone(list, (fn_clone)clone_strdup);

	assert_non_nul(cloned);

	assert_str_equal(pslist_at(cloned, 0), "0");
	assert_str_equal(pslist_at(cloned, 1), "1");

	pslist_free(&list);
	pslist_free_vals(&cloned, NULL);
}

static void pslist_clone__shallow_empty(void **state) {
	assert_nul(pslist_clone(NULL, NULL));
}

static void pslist_clone__shallow_vals(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", };
	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);

	struct Pslist *cloned = pslist_clone(list, NULL);

	assert_non_nul(cloned);

	assert_str_equal(pslist_at(cloned, 0), "0");
	assert_str_equal(pslist_at(cloned, 1), "1");

	pslist_free(&list);
	pslist_free(&cloned);
}

static void pslist_str__null(void **state) {
	assert_nul(pslist_str(NULL, NULL));
}

static void pslist_str__string_vals(void **state) {
	struct Pslist *list = NULL;

	void *vals[] = { "0", "1", NULL, "3", };

	pslist_append(&list, vals[0]);
	pslist_append(&list, vals[1]);
	pslist_append(&list, vals[2]);
	pslist_append(&list, vals[3]);

	char *actual = pslist_str(list, NULL);

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
	pslist_free(&list);
}

static void pslist_str__fn_str(void **state) {
	struct Pslist *list = NULL;

	pslist_append(&list, "zero");
	pslist_append(&list, "one");
	pslist_append(&list, "two");
	pslist_append(&list, NULL);

	char *str = pslist_str(list, str_first);
	assert_str_equal(str,
			"z\n"
			"o\n"
			"t\n"
			"(null)\n"
			);

	free(str);
	pslist_free(&list);
}

static void pslist_xor_free__empty_lists(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, (fn_clone)clone_strdup);

	assert_int_equal(pslist_length(list1), 0);
}

static void pslist_xor_free__first_list_empty(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;
	struct Pslist *expected = NULL;

	pslist_append(&list2, strdup("item1"));
	pslist_append(&list2, strdup("item2"));

	pslist_append(&expected, strdup("item1"));
	pslist_append(&expected, strdup("item2"));

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, (fn_clone)clone_strdup);

	assert_pslist_equal(list1, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free_vals(&list1, NULL);
	pslist_free_vals(&list2, NULL);
	pslist_free_vals(&expected, NULL);
}

static void pslist_xor_free__second_list_empty(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;
	struct Pslist *expected = NULL;

	pslist_append(&list1, strdup("item1"));
	pslist_append(&list1, strdup("item2"));

	pslist_append(&expected, strdup("item1"));
	pslist_append(&expected, strdup("item2"));

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, (fn_clone)clone_strdup);

	assert_pslist_equal(list1, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free_vals(&list1, NULL);
	pslist_free_vals(&list2, NULL);
	pslist_free_vals(&expected, NULL);
}

static void pslist_xor_free__toggle_items(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;
	struct Pslist *expected = NULL;

	pslist_append(&list1, strdup("item1"));
	pslist_append(&list1, strdup("item2"));
	pslist_append(&list1, strdup("item3"));

	pslist_append(&list2, strdup("item2"));
	pslist_append(&list2, strdup("item4"));

	pslist_append(&expected, strdup("item1"));
	pslist_append(&expected, strdup("item3"));
	pslist_append(&expected, strdup("item4"));

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, (fn_clone)clone_strdup);

	assert_pslist_equal(list1, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free_vals(&list1, NULL);
	pslist_free_vals(&list2, NULL);
	pslist_free_vals(&expected, NULL);
}

static void pslist_xor_free__duplicate_items(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;
	struct Pslist *expected = NULL;

	pslist_append(&list1, strdup("item1"));
	pslist_append(&list1, strdup("item1")); // duplicate
	pslist_append(&list1, strdup("item2"));

	pslist_append(&list2, strdup("item1"));

	pslist_append(&expected, strdup("item2"));

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, (fn_clone)clone_strdup);

	assert_pslist_equal(list1, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free_vals(&list1, NULL);
	pslist_free_vals(&list2, NULL);
	pslist_free_vals(&expected, NULL);
}

static void pslist_xor_free__vals(void **state) {
	struct Pslist *list1 = NULL;
	struct Pslist *list2 = NULL;
	struct Pslist *expected = NULL;

	char *val = "item1";

	pslist_append(&list2, val);

	pslist_append(&expected, val);

	pslist_xor_free(&list1, list2, (fn_2pred)equal_strcmp, NULL, NULL);

	assert_pslist_equal(list1, expected, (fn_2pred)equal_strcmp, NULL);

	pslist_free(&list1);
	pslist_free(&list2);
	pslist_free(&expected);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pslist_free_vals__many),

		TEST(pslist_remove__every),
		TEST(pslist_remove__inexistent),

		TEST(pslist_remove_all__some),
		TEST(pslist_remove_all_free__some),

		TEST(pslist_find__no),
		TEST(pslist_find__yes),

		TEST(pslist_find_equal_val__no),
		TEST(pslist_find_equal_val__yes),

		TEST(pslist_equal__empty_lhs),
		TEST(pslist_equal__empty_rhs),
		TEST(pslist_equal__equal),
		TEST(pslist_equal__not_equal_start),
		TEST(pslist_equal__not_equal_mid),
		TEST(pslist_equal__not_equal_end),
		TEST(pslist_equal__not_equal_lhs_size),
		TEST(pslist_equal__not_equal_rhs_size),

		TEST(pslist_sort__empty),
		TEST(pslist_sort__one),
		TEST(pslist_sort__vals),
		TEST(pslist_sort__words),

		TEST(pslist_move__empty),
		TEST(pslist_move__empty_to),
		TEST(pslist_move__empty_from),
		TEST(pslist_move__no_match),
		TEST(pslist_move__many),
		TEST(pslist_move__all),

		TEST(pslist_at__inexistent),

		TEST(pslist_clone__deep_empty),
		TEST(pslist_clone__deep_vals),

		TEST(pslist_clone__shallow_empty),
		TEST(pslist_clone__shallow_vals),

		TEST(pslist_str__null),
		TEST(pslist_str__string_vals),
		TEST(pslist_str__fn_str),

		TEST(pslist_xor_free__empty_lists),
		TEST(pslist_xor_free__first_list_empty),
		TEST(pslist_xor_free__second_list_empty),
		TEST(pslist_xor_free__toggle_items),
		TEST(pslist_xor_free__duplicate_items),
		TEST(pslist_xor_free__vals),
	};

	return RUN(tests);
}

