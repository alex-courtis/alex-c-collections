#include "tst.h"
#include "asserts.h"

#include <cmocka.h>
#include <stdint.h>
#include <stdio.h>

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
	assert_int_equal((uint64_t)slist_at(list, 0), 1);
	assert_int_equal((uint64_t)slist_at(list, 1), 2);

	slist_free(&list);
	itable_free_vals(tab, NULL);
}

int main(void) {
	const struct CMUnitTest tests[] = {

		// other cases covered by tst-ptable

		TEST(itable_keys_slist__empty),
		TEST(itable_keys_slist__many),
	};

	return RUN(tests);
}

