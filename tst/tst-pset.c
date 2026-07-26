#include "assert-pset.h"
#include "asserts.h"
#include "data.h"
#include "expects.h"
#include "mock-fn.h"
#include "tst.h"
#include "util-col.h"

#include <cmocka.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "plist.h"
#include "str.h"

#include "pset.h"

struct Pset {
	const struct PsetParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static const char *starts_with_a_or_null(const char* const key) {
	return key && *key == 'a' ? strdup(key) : NULL;
}

static void pset_init__defaults(void **state) {
	const struct Pset *set = pset_init();

	assert_non_nul(set);

	assert_int_equal(set->size, 0);
	assert_int_equal(set->capacity, 10);

	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		pset_add(set, &v[i]);

	assert_int_equal(set->size, 25);
	assert_int_equal(set->capacity, 30);

	pset_free(set);
}

static void pset_clone__null(void **state) {
	assert_nul(pset_clone(NULL));
}

static void pset_clone__empty(void **state) {
	const struct Pset *set = pset_init();

	const struct Pset *clone = pset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(clone->size, 0);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone__params__constructor(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.str_val = mock_str,
		.initial = 3,
		.grow  = 4,
	});

	const struct Pset *clone = pset_clone(set);

	assert_non_nul(clone);

	assert_int_equal(clone->size, 0);
	assert_int_equal(clone->capacity, 3);
	assert_int_equal(clone->params.grow, 4);
	assert_ptr_equal(clone->params.equal_val, mock_equal);
	assert_ptr_equal(clone->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->params.clone_val, mock_clone);
	assert_ptr_equal(clone->params.str_val, mock_str);

	pset_free(set);
	pset_free(clone);
}

static void pset_clone__val_ptr(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	const struct Pset *clone = pset_clone(set);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->vals[1], V1);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone__alloc_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	assert_true(pset_add(set, V0));

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	const struct Pset *clone = pset_clone(set);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 1);
	assert_ptr_equal(clone->vals[0], V0);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__null(void **state) {
	assert_nul(pset_clone_deep(NULL));
}

static void pset_clone_deep__clone_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .clone_val = mock_clone, });
	pset_add_many(set, V0, V1, NULL);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V3, void*);
	const struct Pset *clone = pset_clone_deep(set);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->vals[0], V2);
	assert_ptr_equal(clone->vals[1], V3);

	pset_free(clone);
	pset_free(set);
}

static void pset_clone_deep__no_clone_val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_nul(pset_clone_deep(set));

	pset_free(set);
}

static void pset_clone_deep__alloc_val_overrides_clone_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .clone_val = mock_clone, .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	pset_add(set, V0);

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);

	const struct Pset *clone = pset_clone_deep(set);

	assert_non_nul(clone);
	assert_int_equal(clone->size, 1);
	assert_ptr_equal(clone->vals[0], V0);

	pset_free(clone);
	pset_free(set);
}

static void pset_free__null(void **state) {
	pset_free(NULL);
}

static void pset_free__empty(void **state) {
	const struct Pset *set = pset_init();

	pset_free(set);
}

static void pset_free_vals__null(void **state) {
	pset_free_vals(NULL);
}

static void pset_free_vals__empty(void **state) {
	const struct Pset *set = pset_init();

	pset_free_vals(set);
}

static void pset_free_vals__null_free_val(void **state) {
	const struct Pset *set = pset_init();

	const char *val = strdup("val will be freed");

	pset_add(set, val);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], val);

	pset_free_vals(set);
}

static void pset_free_vals__missing_val(void **state) {
	const struct Pset *set = pset_init();

	char *val = strdup("will not be freed");

	pset_add(set, val);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], val);

	set->vals[0] = NULL;

	pset_free_vals(set);
	free(val);
}

static void pset_free_vals__free_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	pset_free_vals(set);
}

static void pset_it_free__null(void **state) {
	pset_it_free(NULL);
}

static void pset_it_free__incomplete(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	pset_it_free(it);
}

static void pset_contains__null(void **state) {
	assert_false(pset_contains(NULL, V0));
}

static void pset_contains__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_false(pset_contains(set, V0));

	pset_free(set);
}

static void pset_contains__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_true(pset_contains(set, V0));

	assert_false(pset_contains(set, NULL));

	assert_false(pset_contains(set, V2));

	pset_free(set);
}

static void pset_contains__equal_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .equal_val = mock_equal, });
	pset_add_many(set, V0, NULL);

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V0); will_return(mock_equal, true);
	assert_true(pset_contains(set, V0));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V1); will_return(mock_equal, false);
	assert_false(pset_contains(set, V1));

	pset_free(set);
}

static void pset_at__null(void **state) {
	assert_nul(pset_at(NULL, 0));
}

static void pset_at__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_at(set, 0));
	assert_nul(pset_at(set, 123));

	pset_free(set);
}

static void pset_at__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	assert_ptr_equal(pset_at(set, 0), V0);
	assert_ptr_equal(pset_at(set, 1), V1);
	assert_ptr_equal(pset_at(set, 2), V2);
	assert_nul(pset_at(set, 3));

	pset_free(set);
}

static void pset_find__null(void **state) {
	assert_nul(pset_find(NULL, (struct PsetFilter){ 0 }));
}

static void pset_find__set_empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_find(set, (struct PsetFilter){ 0 }));

	pset_free(set);
}

static void pset_find__filter_empty(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	assert_ptr_equal(pset_find(set, (struct PsetFilter){ 0 }), V0);

	pset_free(set);
}

static void pset_find__val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	// skip V0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);
	// get V1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	assert_ptr_equal(pset_find(set, (struct PsetFilter){ .val = mock_pred_p, }), V1);

	pset_free(set);
}

static void pset_find__val_data(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);
	// get V1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	assert_ptr_equal(pset_find(set, (struct PsetFilter){ .val_data = mock_pred_p_p, .data = D0, }), V1);

	pset_free(set);
}

static void pset_find__no_match(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);
	// get V1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	assert_nul(pset_find(set, (struct PsetFilter){ .val_data = mock_pred_p_p, .data = D0, }));

	pset_free(set);
}

static void pset_it__null(void **state) {
	assert_nul(pset_it(NULL));
}

static void pset_it__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_it(set));

	pset_free(set);
}

static void pset_it__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	const struct PsetIt *it = pset_it(set);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = pset_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = pset_it_next(it);
	assert_nul(it);

	pset_free(set);
}

static void pset_it_next__null(void **state) {
	assert_nul(pset_it_next(NULL));
}

static void pset_it_next__incomplete(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	assert_nul(pset_it_next(it));
}

static void pset_filter_it__null(void **state) {
	assert_nul(pset_filter_it(NULL, (struct PsetFilter){ 0 }));
}

static void pset_filter_it__set_empty(void **state) {
	const struct Pset *set = pset_init();

	assert_nul(pset_filter_it(set, (struct PsetFilter){ 0 }));

	pset_free(set);
}

static void pset_filter_it__filter_empty(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_filter_it(set, (struct PsetFilter){ 0 });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	pset_it_free(it);
	pset_free(set);
}

static void pset_filter_it__val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);

	// get 1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	const struct PsetIt *it = pset_filter_it(set, (struct PsetFilter){ .val = mock_pred_p, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	pset_it_free(it);
	pset_free(set);
}

static void pset_filter_it__val_data(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	const struct PsetIt *it = pset_filter_it(set, (struct PsetFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	pset_it_free(it);
	pset_free(set);
}

static void pset_filter_it__no_match(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// skip 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	const struct PsetIt *it = pset_filter_it(set, (struct PsetFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_nul(it);

	pset_free(set);
}

static void pset_add__null(void **state) {
	assert_false(pset_add(NULL, NULL));
}

static void pset_add__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V1);

	assert_false(pset_add(set, V1));

	pset_free(set);
}

static void pset_add__null_val(void **state) {
	const struct Pset *set = pset_init();

	assert_false(pset_add(set, NULL));

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_add__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, NULL);

	assert_false(pset_add(set, V0));

	assert_int_equal(set->size, 1);

	pset_free(set);
}

static void pset_add__equal_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .equal_val = mock_equal, });

	assert_true(pset_add(set, V0));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V1); will_return(mock_equal, false);
	assert_true(pset_add(set, V1));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V0); will_return(mock_equal, true);
	assert_false(pset_add(set, V0));

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V1);

	pset_free(set);
}

static void pset_add__alloc_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	assert_true(pset_add(set, V0));

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V0);

	pset_free(set);
}

static void pset_add__alloc_val_returned_null(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, NULL, void*);
	assert_false(pset_add(set, V0));

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_add__grow(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .initial = 2, .grow = 5 });
	pset_add_many(set, V0, V1, NULL);

	assert_int_equal(set->size, 2);
	assert_int_equal(set->capacity, 2);
	assert_int_equal(set->params.grow, 5);

	assert_true(pset_add(set, V2));
	assert_int_equal(set->size, 3);
	assert_int_equal(set->capacity, 7);

	assert_true(pset_add(set, V3));
	assert_int_equal(set->size, 4);
	assert_int_equal(set->capacity, 7);

	assert_true(pset_add(set, V4));
	assert_true(pset_add(set, V5));
	assert_int_equal(set->size, 6);
	assert_int_equal(set->capacity, 7);

	pset_free(set);
}

static void pset_add_all__null(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_add_all(NULL, NULL), 0);
	assert_int_equal(pset_add_all(NULL, set), 0);
	assert_int_equal(pset_add_all(set, NULL), 0);

	pset_free(set);
}

static void pset_add_all__duplicates(void **state) {
	const struct Pset *from = pset_init();
	pset_add_many(from, V1, V2, NULL);

	const struct Pset *to = pset_init();
	pset_add_many(to, V0, V1, NULL);

	assert_int_equal(pset_add_all(to, from), 1);

	assert_int_equal(to->size, 3);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);
	assert_ptr_equal(to->vals[2], V2);

	pset_free(to);
	pset_free(from);
}

static void pset_add_all__alloc_val(void **state) {
	const struct Pset *from = pset_init();
	pset_add_many(from, V1, NULL);

	const struct Pset *to = pset_init_with((struct PsetParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	assert_true(pset_add(to, V0));

	expect_ptr(mock_alloc, ptr, V1); will_return_ptr_type(mock_alloc, V1, void*);
	assert_int_equal(pset_add_all(to, from), 1);

	assert_int_equal(to->size, 2);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);

	pset_free(to);
	pset_free(from);
}

static void pset_add_all_clone__null(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_add_all_clone(NULL, NULL), 0);
	assert_int_equal(pset_add_all_clone(NULL, set), 0);
	assert_int_equal(pset_add_all_clone(set, NULL), 0);

	pset_free(set);
}

static void pset_add_all_clone__clone_val(void **state) {
	const struct Pset *from = pset_init();
	pset_add_many(from, V1, V2, NULL);

	const struct Pset *to = pset_init_with((struct PsetParams){ .clone_val = mock_clone, });
	pset_add_many(to, V0, V1, NULL);

	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V2, void*);
	assert_int_equal(pset_add_all_clone(to, from), 1);

	assert_int_equal(to->size, 3);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);
	assert_ptr_equal(to->vals[2], V2);

	pset_free(to);
	pset_free(from);
}

static void pset_add_all_clone__no_clone_val(void **state) {
	const struct Pset *from = pset_init();
	pset_add_many(from, V1, NULL);

	const struct Pset *to = pset_init();
	pset_add_many(to, V0, NULL);

	assert_int_equal(pset_add_all_clone(to, from), 0);

	assert_int_equal(to->size, 1);
	assert_ptr_equal(to->vals[0], V0);

	pset_free(to);
	pset_free(from);
}

static void pset_remove__null(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove(NULL, NULL), 0);
	assert_int_equal(pset_remove(set, NULL), 0);

	pset_free(set);
}

static void pset_remove__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove(set, V0), 0);

	pset_free(set);
}

static void pset_remove__null_val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, NULL);

	assert_int_equal(pset_remove(set, NULL), 0);

	pset_free(set);
}

static void pset_remove__exists(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_ptr_equal(pset_remove(set, V0), V0);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V1);

	assert_ptr_equal(pset_remove(set, V1), V1);

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove__inexistent(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_nul(pset_remove(set, V2));

	assert_int_equal(set->size, 2);

	pset_free(set);
}

static void pset_remove__equal_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .equal_val = mock_equal, });
	assert_true(pset_add(set, V0));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V0); will_return(mock_equal, true);
	assert_ptr_equal(pset_remove(set, V0), V0);

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove_free__null(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove_free(NULL, NULL), 0);
	assert_int_equal(pset_remove_free(set, NULL), 0);

	pset_free(set);
}

static void pset_remove_free__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove_free(set, V0), 0);

	pset_free(set);
}

static void pset_remove_free__free_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0);
	assert_true(pset_remove_free(set, V0));

	assert_false(pset_remove_free(set, NULL));

	assert_false(pset_remove_free(set, V2));

	assert_int_equal(set->size, 1);

	pset_free(set);
}

static void pset_remove_free__free(void **state) {
	const struct Pset *set = pset_init();

	const char *val = strdup("should be freed");

	assert_true(pset_add(set, val));

	assert_true(pset_remove_free(set, val));

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove_all__null(void **state) {
	assert_int_equal(pset_remove_all(NULL), 0);
}

static void pset_remove_all__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove_all(set), 0);

	pset_free(set);
}

static void pset_remove_all__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_int_equal(pset_remove_all(set), 2);

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove_all_free__null(void **state) {
	assert_int_equal(pset_remove_all_free(NULL), 0);
}

static void pset_remove_all_free__empty(void **state) {
	const struct Pset *set = pset_init();

	assert_int_equal(pset_remove_all_free(set), 0);

	pset_free(set);
}

static void pset_remove_all_free__no_free_val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, strdup("to free 0"), strdup("to free 1"), NULL);

	assert_int_equal(pset_remove_all_free(set), 2);

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove_all_free__free_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0); expect_ptr(mock_free, ptr, V1);
	assert_int_equal(pset_remove_all_free(set), 2);

	assert_int_equal(set->size, 0);

	pset_free(set);
}

static void pset_remove_in__null(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V2, V3, NULL);

	const struct Pset *in = pset_init();
	pset_add_many(in, V0, V1, NULL);

	assert_int_equal(pset_remove_in(NULL, NULL), 0);
	assert_int_equal(pset_remove_in(NULL, in), 0);
	assert_int_equal(pset_remove_in(set, NULL), 0);

	assert_int_equal(set->size, 2);

	pset_free(in);
	pset_free(set);
}

static void pset_remove_in__empty(void **state) {
	const struct Pset *set = pset_init();

	const struct Pset *in = pset_init();
	pset_add_many(in, V0, V1, NULL);

	assert_int_equal(pset_remove_in(set, in), 0);

	assert_int_equal(set->size, 0);

	pset_free(set);
	pset_free(in);
}

static void pset_remove_in__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	const struct Pset *from = pset_init();
	pset_add_many(from, V0, V2, NULL);

	assert_int_equal(pset_remove_in(set, from), 1);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V1);

	pset_free(set);
	pset_free(from);
}

static void pset_remove_in_free__null(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V2, V3, NULL);

	const struct Pset *in = pset_init();
	pset_add_many(in, V0, V1, NULL);

	assert_int_equal(pset_remove_in_free(NULL, NULL), 0);
	assert_int_equal(pset_remove_in_free(NULL, in), 0);
	assert_int_equal(pset_remove_in_free(set, NULL), 0);

	assert_int_equal(set->size, 2);

	pset_free(in);
	pset_free(set);
}

static void pset_remove_in_free__empty(void **state) {
	const struct Pset *set = pset_init();

	const struct Pset *in = pset_init();
	pset_add_many(in, V0, V1, NULL);

	assert_int_equal(pset_remove_in_free(set, in), 0);

	assert_int_equal(set->size, 0);

	pset_free(set);
	pset_free(in);
}

static void pset_remove_in_free__no_free_val(void **state) {
	const char *val = strdup("should be freed");

	const struct Pset *set = pset_init();
	pset_add_many(set, val, V1, NULL);

	const struct Pset *from = pset_init();
	pset_add_many(from, val, V2, NULL);

	assert_int_equal(pset_remove_in_free(set, from), 1);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V1);

	pset_free(set);
	pset_free(from);
}

static void pset_remove_in_free__free_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, NULL);

	const struct Pset *from = pset_init();
	assert_true(pset_add(from, V1));
	assert_true(pset_add(from, V2));

	expect_ptr(mock_free, ptr, V1);
	assert_int_equal(pset_remove_in_free(set, from), 1);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V0);

	pset_free(set);
	pset_free(from);
}

static void pset_it_remove__null(void **state) {
	assert_nul(pset_it_remove(NULL));
}

static void pset_it_remove__incomplete(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	assert_nul(pset_it_remove(it));;
}

static void pset_it_remove__start(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);

	assert_ptr_equal(pset_it_remove(it), V0);

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V1);
	assert_ptr_equal(set->vals[1], V2);

	it = pset_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	pset_it_free(it);
	pset_free(set);
}

static void pset_it_remove__mid(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);
	it = pset_it_next(it);

	assert_ptr_equal(pset_it_remove(it), V1);

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V2);

	it = pset_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V2);

	pset_it_free(it);
	pset_free(set);
}

static void pset_it_remove__end(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);
	it = pset_it_next(it);
	it = pset_it_next(it);

	assert_ptr_equal(pset_it_remove(it), V2);

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V1);

	assert_nul(pset_it_next(it));

	pset_free(set);
}

static void pset_it_remove__all(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		assert_non_nul(pset_it_remove(it));
	}

	assert_int_equal(set->size, 0);
	assert_int_equal(iterations, 3);

	pset_free(set);
}

static void pset_it_remove_free__null(void **state) {
	assert_false(pset_it_remove_free(NULL));
}

static void pset_it_remove_free__incomplete(void **state) {
	const struct PsetIt *it = calloc(1, sizeof(struct PsetIt));

	assert_false(pset_it_remove_free(it));;
}

static void pset_it_remove_free__start(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);

	expect_ptr(mock_free, ptr, V0);
	assert_true(pset_it_remove_free(it));

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V1);
	assert_ptr_equal(set->vals[1], V2);

	it = pset_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	pset_it_free(it);
	pset_free(set);
}

static void pset_it_remove_free__mid(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);
	it = pset_it_next(it);

	expect_ptr(mock_free, ptr, V1);
	assert_true(pset_it_remove_free(it));

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V2);

	it = pset_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V2);

	pset_it_free(it);
	pset_free(set);
}

static void pset_it_remove_free__end(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, V2, NULL);

	const struct PsetIt *it = pset_it(set);
	it = pset_it_next(it);
	it = pset_it_next(it);

	expect_ptr(mock_free, ptr, V2);
	assert_true(pset_it_remove_free(it));

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V1);

	assert_nul(pset_it_next(it));

	pset_free(set);
}

static void pset_it_remove_free__all(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .free_val = mock_free, });
	pset_add_many(set, V0, V1, V2, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V2);

	size_t iterations = 0;
	for (const struct PsetIt *it = pset_it(set); it; it = pset_it_next(it)) {
		iterations++;
		assert_true(pset_it_remove_free(it));
	}

	assert_int_equal(set->size, 0);
	assert_int_equal(iterations, 3);

	pset_free(set);
}

static void pset_sort__null(void **state) {
	pset_sort(NULL, mock_less_than);
}

static void pset_sort__no_less_than(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	pset_sort(set, NULL);

	assert_int_equal(set->size, 2);
	assert_ptr_equal(set->vals[0], V0);
	assert_ptr_equal(set->vals[1], V1);

	pset_free(set);
}

static void pset_sort__empty(void **state) {
	const struct Pset *actual = pset_init();

	pset_sort(actual, mock_less_than);

	pset_free(actual);
}

static void pset_sort__one(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, NULL);

	pset_sort(set, mock_less_than);

	assert_int_equal(set->size, 1);
	assert_ptr_equal(set->vals[0], V0);

	pset_free(set);
}

static void pset_sort__many(void **state) {
	const char *v[6] = { "a", "b", "c", "d", "e", "f" };

	const struct Pset *actual = pset_init();
	pset_add_many(actual, v[2], v[0], v[3], v[5], v[1], v[4], NULL);

	const struct Pset *expected = pset_init();
	pset_add_many(expected, v[0], v[1], v[2], v[3], v[4], v[5], NULL);

	pset_sort(actual, (fn_less_than)less_than_strcmp);

	assert_pset_equal_ordered(actual, expected);

	pset_free(actual);
	pset_free(expected);
}

static void pset_equal__null(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_pset_not_equal(NULL, NULL);
	assert_pset_not_equal(set, NULL);
	assert_pset_not_equal(NULL, set);

	pset_free(set);
}

static void pset_equal__length_different(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V0, NULL);
	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V1, NULL);

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__val_pointers_ok(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V1, V0, NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V1, NULL);

	assert_pset_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__val_pointers_different(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V0, V1, NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V2, NULL);

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal__equal_val_ok(void **state) {
	const struct Pset *a = pset_init_with((struct PsetParams){ .equal_val = (fn_equal)equal_strcmp, });
	pset_add_many(a, "a", "b", NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, "b", "a", NULL);

	assert_pset_equal(a, b);

	pset_free(b);
	pset_free(a);
}

static void pset_equal__equal_val_different(void **state) {
	const struct Pset *a = pset_init_with((struct PsetParams){ .equal_val = (fn_equal)equal_strcmp, });
	pset_add_many(a, "a", "b", NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, "a", "c", NULL);

	assert_pset_not_equal(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal_ordered__null(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, NULL);

	assert_pset_not_equal_ordered(NULL, NULL);
	assert_pset_not_equal_ordered(set, NULL);
	assert_pset_not_equal_ordered(NULL, set);

	pset_free(set);
}

static void pset_equal_ordered__length_different(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V0, NULL);
	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V1, NULL);

	assert_pset_not_equal_ordered(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal_ordered__val_pointers_ok(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V0, V1, NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V1, NULL);

	assert_pset_equal_ordered(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal_ordered__val_pointers_different(void **state) {
	const struct Pset *a = pset_init();
	pset_add_many(a, V0, V1, NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, V0, V2, NULL);

	assert_pset_not_equal_ordered(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_equal_ordered__equal_val_ok(void **state) {
	const struct Pset *a = pset_init_with((struct PsetParams){ .equal_val = (fn_equal)equal_strcmp, });
	pset_add_many(a, "a", "b", NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, "a", "b", NULL);

	assert_pset_equal_ordered(a, b);

	pset_free(b);
	pset_free(a);
}

static void pset_equal_ordered__equal_val_different(void **state) {
	const struct Pset *a = pset_init_with((struct PsetParams){ .equal_val = (fn_equal)equal_strcmp, });
	pset_add_many(a, "a", "b", NULL);

	const struct Pset *b = pset_init();
	pset_add_many(b, "a", "c", NULL);

	assert_pset_not_equal_ordered(a, b);

	pset_free(a);
	pset_free(b);
}

static void pset_plist__null(void **state) {
	assert_nul(pset_plist(NULL));
}

static void pset_plist__empty(void **state) {
	const struct Pset *set = pset_init();

	const struct Plist *list = pset_plist(set);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 0);

	pset_free(set);
	plist_free(list);
}

static void pset_plist__many(void **state) {
	const struct Pset *set = pset_init();

	assert_true(pset_add(set, V0));
	assert_true(pset_add(set, V1));

	const struct Plist *list = pset_plist(set);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);

	plist_free(list);
	pset_free(set);
}

static void pset_plist__alloc_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(pset_add(set, V0));

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *list = pset_plist(set);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
	pset_free(set);
}

static void pset_plist_clone__null(void **state) {
	assert_nul(pset_plist_clone(NULL));
}

static void pset_plist_clone__empty(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .clone_val = mock_clone, });

	const struct Plist *list = pset_plist_clone(set);

	assert_non_nul(list);
	assert_int_equal(plist_size(list), 0);

	pset_free(set);
	plist_free(list);
}

static void pset_plist_clone__missing_clone_val(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, NULL);

	assert_nul(pset_plist_clone(set));

	pset_free(set);
}

static void pset_plist_clone__clone_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .clone_val = mock_clone, });
	pset_add_many(set, V0, NULL);

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Plist *list = pset_plist_clone(set);

	plist_free(list);
	pset_free(set);
}

static void pset_str__null(void **state) {
	assert_nul(pset_str(NULL));
}

static void pset_str__empty(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .str_val = mock_str, });

	char *str = pset_str(set);
	assert_str_equal(str, "");

	free(str);
	pset_free(set);
}

static void pset_str__pointers(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	const void **v = set->vals;
	v[1] = NULL;

	char *expected = sprintf_alloc(
			"%p\n"
			"(null)\n"
			"%p\n",
			(void*)V0,
			(void*)V2
			);

	char *actual = pset_str(set);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	pset_free(set);
}

static void pset_str__str_val(void **state) {
	const struct Pset *set = pset_init_with((struct PsetParams){ .str_val = (fn_str)starts_with_a_or_null, });
	pset_add_many(set, "a1", "b2", "a3", NULL);

	char *str = pset_str(set);
	assert_str_equal(str,
			"a1\n"
			"(null)\n"
			"a3\n"
			);

	free(str);
	pset_free(set);
}

static void pset_size__null(void **state) {
	assert_int_equal(pset_size(NULL), 0);
}

static void pset_size__present(void **state) {
	const struct Pset *set = pset_init();
	pset_add_many(set, V0, V1, V2, NULL);

	assert_int_equal(pset_size(set), 3);

	pset_free(set);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(pset_init__defaults),

		TEST(pset_clone__null),
		TEST(pset_clone__empty),
		TEST(pset_clone__params__constructor),
		TEST(pset_clone__val_ptr),
		TEST(pset_clone__alloc_val),

		TEST(pset_clone_deep__null),
		TEST(pset_clone_deep__clone_val),
		TEST(pset_clone_deep__no_clone_val),
		TEST(pset_clone_deep__alloc_val_overrides_clone_val),

		TEST(pset_free__null),
		TEST(pset_free__empty),

		TEST(pset_free_vals__null),
		TEST(pset_free_vals__empty),
		TEST(pset_free_vals__null_free_val),
		TEST(pset_free_vals__missing_val),
		TEST(pset_free_vals__free_val),

		TEST(pset_it_free__null),
		TEST(pset_it_free__incomplete),

		TEST(pset_contains__null),
		TEST(pset_contains__empty),
		TEST(pset_contains__present),
		TEST(pset_contains__equal_val),

		TEST(pset_at__null),
		TEST(pset_at__empty),
		TEST(pset_at__present),

		TEST(pset_find__null),
		TEST(pset_find__set_empty),
		TEST(pset_find__filter_empty),
		TEST(pset_find__val),
		TEST(pset_find__val_data),
		TEST(pset_find__no_match),

		TEST(pset_it__null),
		TEST(pset_it__empty),
		TEST(pset_it__present),

		TEST(pset_it_next__null),
		TEST(pset_it_next__incomplete),

		TEST(pset_filter_it__null),
		TEST(pset_filter_it__set_empty),
		TEST(pset_filter_it__filter_empty),
		TEST(pset_filter_it__val),
		TEST(pset_filter_it__val_data),
		TEST(pset_filter_it__no_match),

		TEST(pset_add__null),
		TEST(pset_add__empty),
		TEST(pset_add__null_val),
		TEST(pset_add__present),
		TEST(pset_add__equal_val),
		TEST(pset_add__alloc_val),
		TEST(pset_add__alloc_val_returned_null),
		TEST(pset_add__grow),

		TEST(pset_add_all__null),
		TEST(pset_add_all__duplicates),
		TEST(pset_add_all__alloc_val),

		TEST(pset_add_all_clone__null),
		TEST(pset_add_all_clone__clone_val),
		TEST(pset_add_all_clone__no_clone_val),

		TEST(pset_remove__null),
		TEST(pset_remove__empty),
		TEST(pset_remove__null_val),
		TEST(pset_remove__exists),
		TEST(pset_remove__inexistent),
		TEST(pset_remove__equal_val),

		TEST(pset_remove_free__null),
		TEST(pset_remove_free__empty),
		TEST(pset_remove_free__free_val),
		TEST(pset_remove_free__free),

		TEST(pset_remove_all__null),
		TEST(pset_remove_all__empty),
		TEST(pset_remove_all__present),

		TEST(pset_remove_all_free__null),
		TEST(pset_remove_all_free__empty),
		TEST(pset_remove_all_free__no_free_val),
		TEST(pset_remove_all_free__free_val),

		TEST(pset_remove_in__null),
		TEST(pset_remove_in__empty),
		TEST(pset_remove_in__present),

		TEST(pset_remove_in_free__null),
		TEST(pset_remove_in_free__empty),
		TEST(pset_remove_in_free__no_free_val),
		TEST(pset_remove_in_free__free_val),

		TEST(pset_it_remove__null),
		TEST(pset_it_remove__incomplete),
		TEST(pset_it_remove__start),
		TEST(pset_it_remove__mid),
		TEST(pset_it_remove__end),
		TEST(pset_it_remove__all),

		TEST(pset_it_remove_free__null),
		TEST(pset_it_remove_free__incomplete),
		TEST(pset_it_remove_free__start),
		TEST(pset_it_remove_free__mid),
		TEST(pset_it_remove_free__end),
		TEST(pset_it_remove_free__all),

		TEST(pset_sort__null),
		TEST(pset_sort__no_less_than),
		TEST(pset_sort__empty),
		TEST(pset_sort__one),
		TEST(pset_sort__many),

		TEST(pset_equal__null),
		TEST(pset_equal__length_different),
		TEST(pset_equal__val_pointers_ok),
		TEST(pset_equal__val_pointers_different),
		TEST(pset_equal__equal_val_ok),
		TEST(pset_equal__equal_val_different),

		TEST(pset_equal_ordered__null),
		TEST(pset_equal_ordered__length_different),
		TEST(pset_equal_ordered__val_pointers_ok),
		TEST(pset_equal_ordered__val_pointers_different),
		TEST(pset_equal_ordered__equal_val_ok),
		TEST(pset_equal_ordered__equal_val_different),

		TEST(pset_plist__null),
		TEST(pset_plist__empty),
		TEST(pset_plist__many),
		TEST(pset_plist__alloc_val),

		TEST(pset_plist_clone__null),
		TEST(pset_plist_clone__empty),
		TEST(pset_plist_clone__clone_val),
		TEST(pset_plist_clone__missing_clone_val),

		TEST(pset_str__null),
		TEST(pset_str__empty),
		TEST(pset_str__pointers),
		TEST(pset_str__str_val),

		TEST(pset_size__null),
		TEST(pset_size__present),
	};

	return RUN(tests);
}

