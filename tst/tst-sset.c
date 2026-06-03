#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>

#include "slist.h"

#include "sset.h"

/*
   diff -u \
   <(sed -e 's/pset/xtable/g ; s/pset/XTable/g' tst/tst-pset.c) \
   <(sed -e 's/sset/xtable/g ; s/sset/XTable/g' tst/tst-sset.c)
   */

static int before_all(void **state) {
	return 0;
}

static int after_all(void **state) {
	return 0;
}

static int before_each(void **state) {
	return 0;
}

static int after_each(void **state) {
	return 0;
}

static void sset_init__size(void **state) {
	const struct SSet *set = sset_init_with(5, 50, false);

	assert_non_nul(set);

	assert_int_equal(sset_size(set), 0);

	assert_int_equal(sset_capacity(set), 5);

	sset_free(set);
}

static void sset_init__invalid(void **state) {
	const struct SSet *set = sset_init_with(0, 0, false);

	assert_nul(set);
}

static void sset_free__ok(void **state) {
	const struct SSet *set = sset_init();

	char *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__new(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__existing(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	assert_false(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	sset_free(set);
}

static void sset_add__null(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", };
	assert_true(sset_add(set, vals[0]));

	assert_int_equal(sset_size(set), 1);

	assert_false(sset_contains(set, NULL));
	assert_false(sset_add(set, NULL));
	assert_false(sset_contains(set, NULL));

	assert_int_equal(sset_size(set), 1);

	sset_free(set);
}

static void sset_add__case_insensitive(void **state) {
	const struct SSet *set = sset_init_with(5, 5, true);

	void *vals[] = { "a", "A", };
	assert_true(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 1);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_add__grow(void **state) {
	const struct SSet *set = sset_init_with(2, 5, false);

	void *initial[] = { "0", "1", };
	assert_true(sset_add(set, initial[0]));
	assert_true(sset_add(set, initial[1]));

	assert_int_equal(sset_size(set), 2);
	assert_int_equal(sset_capacity(set), 2);

	assert_true(sset_contains(set, initial[0]));
	assert_true(sset_contains(set, initial[1]));

	void *grow[] = { "2", "3", };
	assert_true(sset_add(set, grow[0]));
	assert_int_equal(sset_size(set), 3);
	assert_int_equal(sset_capacity(set), 7);
	assert_true(sset_contains(set, grow[0]));

	assert_true(sset_add(set, grow[1]));
	assert_int_equal(sset_size(set), 4);
	assert_int_equal(sset_capacity(set), 7);
	assert_true(sset_contains(set, grow[1]));

	void *subsequent[] = { "4", "5", };
	assert_true(sset_add(set, subsequent[0]));
	assert_true(sset_add(set, subsequent[1]));
	assert_int_equal(sset_size(set), 6);
	assert_int_equal(sset_capacity(set), 7);

	assert_true(sset_contains(set, subsequent[0]));
	assert_true(sset_contains(set, subsequent[1]));

	sset_free(set);
}

static void sset_remove__existing(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);


	void *vals[] = { "0", "2", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	// 0
	assert_true(sset_remove(set, vals[0]));

	assert_int_equal(sset_size(set), 1);
	assert_false(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	// 1
	assert_true(sset_remove(set, vals[1]));

	assert_int_equal(sset_size(set), 0);
	assert_false(sset_contains(set, vals[0]));
	assert_false(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_remove__inexistent(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	const void *inexistent = "inexistent";
	assert_false(sset_remove(set, inexistent));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));

	sset_free(set);
}

static void sset_remove__case_insensitive(void **state) {
	const struct SSet *set = sset_init_with(5, 5, true);

	void *vals[] = { "a", "A", "b", };
	assert_true(sset_add(set, vals[0]));
	assert_false(sset_add(set, vals[1]));
	assert_true(sset_add(set, vals[2]));

	assert_int_equal(sset_size(set), 2);
	assert_true(sset_contains(set, vals[0]));
	assert_true(sset_contains(set, vals[1]));
	assert_true(sset_contains(set, vals[2]));

	assert_true(sset_remove(set, vals[0]));
	assert_false(sset_remove(set, vals[1]));

	assert_int_equal(sset_size(set), 1);
	assert_true(sset_contains(set, vals[2]));

	sset_free(set);
}

static void sset_iter__empty(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	assert_int_equal(sset_size(set), 0);

	assert_nul(sset_iter(set));

	sset_free(set);
}

static void sset_iter__free(void **state) {
	const struct SSet *set = sset_init_with(3, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "0");

	sset_iter_free(iter);

	sset_free(set);
}


static void sset_iter__vals(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "0");

	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "1");

	iter = sset_iter_next(iter);
	assert_nul(iter);

	sset_free(set);
}

static void sset_iter__cleared(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));

	assert_int_equal(sset_size(set), 2);

	sset_remove(set, vals[0]);
	sset_remove(set, vals[1]);

	assert_int_equal(sset_size(set), 0);

	assert_nul(sset_iter(set));

	sset_free(set);
}

static void sset_add__again(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", "2", "3", };
	assert_true(sset_add(set, vals[0]));
	assert_true(sset_add(set, vals[1]));
	assert_true(sset_add(set, vals[2]));
	assert_true(sset_add(set, vals[3]));

	assert_int_equal(sset_size(set), 4);

	// remove 1
	assert_true(sset_remove(set, vals[1]));
	assert_int_equal(sset_size(set), 3);

	// put 1 again afterwards
	assert_true(sset_add(set, vals[1]));
	assert_int_equal(sset_size(set), 4);

	// 0
	const struct SSetIter *iter = sset_iter(set);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "0");

	// 2
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "2");

	// 3
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "3");

	// 0 moved later
	iter = sset_iter_next(iter);
	assert_non_nul(iter);
	assert_str_equal(sset_iter_val(iter), "1");

	// end
	iter = sset_iter_next(iter);
	assert_nul(iter);

	sset_free(set);
}

static void sset_equal__length_different(void **state) {
	const struct SSet *a = sset_init_with(5, 5, false);
	const struct SSet *b = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };

	assert_true(sset_add(a, vals[0]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[1]));

	assert_false(sset_equal(a, b));

	sset_free(a);
	sset_free(b);
}

static void sset_equal__comparison_ok(void **state) {
	const struct SSet *a = sset_init_with(5, 5, false);
	const struct SSet *b = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", };
	assert_true(sset_add(a, vals[0]));
	assert_true(sset_add(a, vals[1]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[1]));

	assert_true(sset_equal(a, b));

	sset_free(a);
	sset_free(b);
}

static void sset_equal__comparison_different(void **state) {
	const struct SSet *a = sset_init_with(5, 5, false);
	const struct SSet *b = sset_init_with(5, 5, false);

	void *vals[] = { "0", "1", "2", };

	assert_true(sset_add(a, vals[0]));
	assert_true(sset_add(a, vals[1]));

	assert_true(sset_add(b, vals[0]));
	assert_true(sset_add(b, vals[2]));

	assert_false(sset_equal(a, b));

	sset_free(a);
	sset_free(b);
}

static void sset_equal__case_insensitive(void **state) {
	const struct SSet *a = sset_init_with(5, 5, true);
	const struct SSet *b = sset_init_with(5, 5, false);

	assert_true(sset_add(a, "a"));
	assert_true(sset_add(a, "B"));

	assert_true(sset_add(b, "A"));
	assert_true(sset_add(b, "b"));

	assert_true(sset_equal(a, b));
	assert_false(sset_equal(b, a));

	sset_free(a);
	sset_free(b);
}

static void sset_vals_slist__empty(void **state) {
	const struct SSet *set = sset_init_with(3, 5, false);

	assert_nul(sset_vals_slist(set));

	sset_free(set);
}

static void sset_vals_slist__many(void **state) {
	const struct SSet *tab = sset_init_with(3, 5, false);

	void *vals[] = { "0", "1", };

	assert_true(sset_add(tab, vals[0]));
	assert_true(sset_add(tab, vals[1]));

	struct SList *list = sset_vals_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_str_equal(slist_at(list, 0), "0");
	assert_str_equal(slist_at(list, 1), "1");

	slist_free(&list);
	sset_free(tab);
}

static void sset_str__null(void **state) {
	assert_nul(sset_str(NULL));
}

static void sset_str__empty(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	char *str = sset_str(set);
	assert_str_equal(str, "");

	free(str);
	sset_free(set);
}

static void sset_str__string_vals(void **state) {
	const struct SSet *set = sset_init_with(5, 5, false);

	assert_true(sset_add(set, "ONE"));
	assert_true(sset_add(set, "TWO"));
	assert_true(sset_add(set, "THREE"));

	char *str = sset_str(set);
	assert_str_equal(str,
			"ONE\n"
			"TWO\n"
			"THREE\n"
			);

	free(str);
	sset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(sset_init__size),
		TEST(sset_init__invalid),

		TEST(sset_free__ok),

		TEST(sset_add__new),
		TEST(sset_add__existing),
		TEST(sset_add__null),
		TEST(sset_add__grow),
		TEST(sset_add__case_insensitive),

		TEST(sset_remove__existing),
		TEST(sset_remove__inexistent),
		TEST(sset_remove__case_insensitive),

		TEST(sset_iter__empty),
		TEST(sset_iter__free),
		TEST(sset_iter__vals),
		TEST(sset_iter__cleared),

		TEST(sset_add__again),

		TEST(sset_equal__length_different),
		TEST(sset_equal__comparison_ok),
		TEST(sset_equal__comparison_different),
		TEST(sset_equal__case_insensitive),

		TEST(sset_vals_slist__empty),
		TEST(sset_vals_slist__many),

		TEST(sset_str__null),
		TEST(sset_str__empty),
		TEST(sset_str__string_vals),
	};

	return RUN(tests);
}

