#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "slist.h"

#include "itable.h"

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


void itable_keys_slist__empty(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	assert_nul(itable_keys_slist(tab));

	itable_free(tab);
}

void itable_keys_slist__many(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	itable_put(tab, 1, NULL);
	itable_put(tab, 2, NULL);

	struct SList *list = itable_keys_slist(tab);

	assert_int_equal(slist_length(list), 2);
	assert_int_equal(*(uint64_t*)slist_at(list, 0), 1);
	assert_int_equal(*(uint64_t*)slist_at(list, 1), 2);

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
	itable_free_vals(tab, NULL);
}

void itable_str__string_vals(void **state) {
	const struct ITable *tab = itable_init(3, 5);

	itable_put(tab, -1, strdup("1"));
	itable_put(tab, 20, NULL);
	itable_put(tab, 30, strdup("3"));

	char *str = itable_str(tab);
	assert_str_equal(str,
			"18446744073709551615 = 1\n"
			"20 = (null)\n"
			"30 = 3"
			);

	free(str);
	itable_free_vals(tab, NULL);
}

int main(void) {
	const struct CMUnitTest tests[] = {

		// other cases covered by tst-ptable

		TEST(itable_keys_slist__empty),
		TEST(itable_keys_slist__many),

		TEST(itable_str__null),
		TEST(itable_str__empty),
		TEST(itable_str__string_vals),
	};

	return RUN(tests);
}

