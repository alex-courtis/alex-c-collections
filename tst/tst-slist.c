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

		// TODO
		// slist_equal
		// slist_sort
		// slist_move
	};

	return RUN(tests);
}

