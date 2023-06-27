#include "tst.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "slist.h"

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

void mock_free_val(void *val) {
	check_expected(val);
}

bool x_test(const void *data) {
	if (strcmp("x", data) == 0) {
		return true;
	}
	return false;
}

bool false_test(const void *data) {
	return false;
}

bool before_int(const void *a, const void *b) {
	if (!a && !b)
		return false;
	else if (a && !b)
		return true;
	else if (!a && b)
		return false;
	else {
		return (*(int*)a < *(int*)b);
	}
}

bool predicate_strstr(const void *val, const void *data) {
	if (!val || !data) {
		return false;
	}
	return strstr(val, data);
}

void slist_free_vals__many(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	expect_string(mock_free_val, val, "0");
	expect_string(mock_free_val, val, "1");
	expect_string(mock_free_val, val, "2");

	slist_free_vals(&list, mock_free_val);

	assert_null(list);
}

void slist_remove__every(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	// mid
	struct SList *i = list->nex;
	assert_string_equal(i->val, "1");
	assert_string_equal(slist_remove(&list, &i), "1");
	assert_null(i);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 2);

	// first
	i = list;
	assert_string_equal(i->val, "0");
	assert_string_equal(slist_remove(&list, &i), "0");
	assert_null(i);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 1);

	// last
	i = list;
	assert_string_equal(i->val, "2");
	assert_string_equal(slist_remove(&list, &i), "2");
	assert_null(i);

	assert_null(list);
	assert_int_equal(slist_length(list), 0);

	slist_free(&list);
}

void slist_remove_all__some(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);
	slist_append(&list, vals[3]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 4);

	slist_remove_all(&list, slist_predicate_strcmp, "x");

	assert_non_null(list);
	assert_int_equal(slist_length(list), 2);

	struct SList *i = list;
	assert_string_equal(i->val, "0");
	assert_string_equal(slist_at(list, 0), "0");

	i = i->nex;
	assert_non_null(i);
	assert_string_equal(i->val, "2");
	assert_string_equal(slist_at(list, 1), "2");

	i = i->nex;
	assert_null(i);

	slist_free(&list);
}

void slist_remove_all_free__some(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", "x", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);
	slist_append(&list, vals[3]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 4);

	expect_string(mock_free_val, val, "x");
	expect_string(mock_free_val, val, "x");

	slist_remove_all_free(&list, slist_predicate_strcmp, "x", mock_free_val);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 2);

	struct SList *i = list;
	assert_string_equal(i->val, "0");
	assert_string_equal(slist_at(list, 0), "0");

	i = i->nex;
	assert_non_null(i);
	assert_string_equal(i->val, "2");
	assert_string_equal(slist_at(list, 1), "2");

	i = i->nex;
	assert_null(i);

	slist_free(&list);
}

void slist_find__no(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	void *val = slist_find_val(list, false_test);
	assert_null(val);

	slist_free(&list);
}

void slist_find__yes(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "x", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	void *val = slist_find_val(list, x_test);
	assert_non_null(val);
	assert_string_equal(val, "x");

	slist_free(&list);
}

void slist_find_equal_val__no(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	void *val = slist_find_equal_val(list, slist_predicate_strcmp, "x");
	assert_null(val);

	slist_free(&list);
}

void slist_find_equal_val__yes(void **state) {
	struct SList *list = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&list, vals[0]);
	slist_append(&list, vals[1]);
	slist_append(&list, vals[2]);

	assert_non_null(list);
	assert_int_equal(slist_length(list), 3);

	void *val = slist_find_equal_val(list, slist_predicate_strcmp, "1");
	assert_non_null(val);
	assert_string_equal(val, "1");

	slist_free(&list);
}

void slist_equal__empty_lhs(void **state) {
	struct SList *rhs = NULL;

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_false(slist_equal(NULL, rhs, slist_predicate_strcmp));

	slist_free(&rhs);
}

void slist_equal__empty_rhs(void **state) {
	struct SList *lhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	assert_false(slist_equal(lhs, NULL, slist_predicate_strcmp));

	slist_free(&lhs);
}

void slist_equal__equal(void **state) {
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

	assert_true(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_equal__not_equal_start(void **state) {
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

	assert_false(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_equal__not_equal_mid(void **state) {
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

	assert_false(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_equal__not_equal_end(void **state) {
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

	assert_false(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_equal__not_equal_lhs_size(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);

	void *rvals[] = { "0", "1", "2", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);
	slist_append(&rhs, rvals[2]);

	assert_false(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_equal__not_equal_rhs_size(void **state) {
	struct SList *lhs = NULL;
	struct SList *rhs = NULL;

	void *lvals[] = { "0", "1", "2", };
	slist_append(&lhs, lvals[0]);
	slist_append(&lhs, lvals[1]);
	slist_append(&lhs, lvals[2]);

	void *rvals[] = { "0", "1", };
	slist_append(&rhs, rvals[0]);
	slist_append(&rhs, rvals[1]);

	assert_false(slist_equal(lhs, rhs, slist_predicate_strcmp));

	slist_free(&lhs);
	slist_free(&rhs);
}

void slist_sort__empty(void **state) {
	struct SList *from = NULL;

	struct SList *to = slist_sort(from, before_int);
	assert_null(to);
}

void slist_sort__vals(void **state) {
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

	struct SList *to = slist_sort(from, before_int);
	assert_non_null(to);

	assert_int_equal(slist_length(to), 10);

	// first 6 are integers
	for (int i = 0; i < 6; i++) {
		assert_non_null(slist_at(to, i));
		assert_int_equal(*(int*)slist_at(to, i), i);
	}

	// remaining 4 are null
	for (int i = 6; i < 10; i++) {
		assert_null(slist_at(to, i));
	}

	slist_free(&from);
	slist_free(&to);
}

void slist_move__empty(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	slist_move(&to, &from, slist_predicate_strcmp, "x");

	assert_null(to);
	assert_null(from);
}

void slist_move__empty_to(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&from, vals[0]);
	slist_append(&from, vals[1]);
	slist_append(&from, vals[2]);

	slist_move(&to, &from, slist_predicate_strcmp, "x");

	assert_int_equal(slist_length(to), 0);
	assert_int_equal(slist_length(from), 3);

	slist_free(&from);
}

void slist_move__empty_from(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *vals[] = { "0", "1", "2", };
	slist_append(&to, vals[0]);
	slist_append(&to, vals[1]);
	slist_append(&to, vals[2]);

	slist_move(&to, &from, slist_predicate_strcmp, "x");

	assert_int_equal(slist_length(to), 3);
	assert_int_equal(slist_length(from), 0);

	slist_free(&to);
}

void slist_move__no_match(void **state) {
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

	slist_move(&to, &from, slist_predicate_strcmp, "x");

	assert_int_equal(slist_length(to), 3);
	assert_int_equal(slist_length(from), 3);

	slist_free(&to);
	slist_free(&from);
}

void slist_move__many(void **state) {
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

	slist_move(&to, &from, predicate_strstr, "x");

	// values moved
	assert_int_equal(slist_length(to), 6);
	assert_string_equal(slist_at(to, 0), "0");
	assert_string_equal(slist_at(to, 1), "1");
	assert_string_equal(slist_at(to, 2), "2");
	assert_string_equal(slist_at(to, 3), "x0");
	assert_string_equal(slist_at(to, 4), "x2");
	assert_string_equal(slist_at(to, 5), "x4");

	// values remaining
	assert_int_equal(slist_length(from), 2);
	assert_string_equal(slist_at(from, 0), "1");
	assert_string_equal(slist_at(from, 1), "3");

	slist_free(&to);
	slist_free(&from);
}

void slist_move__all(void **state) {
	struct SList *to = NULL;
	struct SList *from = NULL;

	void *to_vals[] = { "0", "1", };
	slist_append(&to, to_vals[0]);
	slist_append(&to, to_vals[1]);

	void *from_vals[] = { "x0", "x1", };
	slist_append(&from, from_vals[0]);
	slist_append(&from, from_vals[1]);

	slist_move(&to, &from, predicate_strstr, "x");

	// values moved
	assert_int_equal(slist_length(to), 4);
	assert_string_equal(slist_at(to, 0), "0");
	assert_string_equal(slist_at(to, 1), "1");
	assert_string_equal(slist_at(to, 2), "x0");
	assert_string_equal(slist_at(to, 3), "x1");

	// values remaining
	assert_int_equal(slist_length(from), 0);

	slist_free(&to);
	slist_free(&from);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(slist_free_vals__many),

		TEST(slist_remove__every),

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
		TEST(slist_sort__vals),

		TEST(slist_move__empty),
		TEST(slist_move__empty_to),
		TEST(slist_move__empty_from),
		TEST(slist_move__no_match),
		TEST(slist_move__many),
		TEST(slist_move__all),
	};

	return RUN(tests);
}

