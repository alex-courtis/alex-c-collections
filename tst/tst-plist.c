#include "assert-plist.h"
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
#include "str.h"

#include "plist.h"

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

static const char *starts_with_a_or_null(const char* const key) {
	return key && *key == 'a' ? strdup(key) : NULL;
}

static void plist_init__defaults(void **state) {
	const struct Plist *list = plist_init();

	assert_non_nul(list);

	assert_int_equal(list->size, 0);
	assert_int_equal(list->capacity, 10);

	size_t v[25] = { 0 };
	for (size_t i = 0; i < 25; i++)
		plist_append(list, &v[i]);

	assert_int_equal(list->size, 25);
	assert_int_equal(list->capacity, 30);

	plist_free(list);
}

static void plist_clone__null(void **state) {
	assert_nul(plist_clone(NULL));
}

static void plist_clone__empty(void **state) {
	const struct Plist *list = plist_init();

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(clone->size, 0);

	plist_free(list);
	plist_free(clone);
}

static void plist_clone__params__constructor(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.str_val = mock_str,
		.initial = 3,
		.grow  = 4,
	});

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(clone->size, 0);
	assert_int_equal(clone->capacity, 3);
	assert_int_equal(clone->params.grow, 4);
	assert_ptr_equal(clone->params.equal_val, mock_equal);
	assert_ptr_equal(clone->params.alloc_val, mock_alloc);
	assert_ptr_equal(clone->params.clone_val, mock_clone);
	assert_ptr_equal(clone->params.str_val, mock_str);

	plist_free(list);
	plist_free(clone);
}

static void plist_clone__val_ptr(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->vals[1], V1);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone__alloc_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	plist_append(list, V0);

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	const struct Plist *clone = plist_clone(list);

	assert_int_equal(clone->size, 1);
	assert_ptr_equal(clone->vals[0], V0);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(clone->size, 3);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->vals[1], NULL);
	assert_ptr_equal(clone->vals[2], V2);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__null(void **state) {
	assert_nul(plist_clone_deep(NULL));
}

static void plist_clone_deep__clone_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .clone_val = mock_clone, });
	plist_append_many(list, V0, V1, NULL);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V3, void*);
	const struct Plist *clone = plist_clone_deep(list);

	assert_int_equal(clone->size, 2);
	assert_ptr_equal(clone->vals[0], V2);
	assert_ptr_equal(clone->vals[1], V3);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__no_clone_val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_nul(plist_clone_deep(list));

	plist_free(list);
}

static void plist_clone_deep__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .clone_val = mock_clone, .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);
	plist_append(list, V3);

	expect_ptr(mock_clone, ptr, V0); will_return_ptr_type(mock_clone, V0, void*);
	expect_ptr(mock_clone, ptr, NULL); will_return_ptr_type(mock_clone, V1, void*);
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, V3); will_return_ptr_type(mock_clone, V3, void*);
	const struct Plist *clone = plist_clone_deep(list);

	assert_int_equal(clone->size, 4);
	assert_ptr_equal(clone->vals[0], V0);
	assert_ptr_equal(clone->vals[1], V1);
	assert_ptr_equal(clone->vals[2], NULL);
	assert_ptr_equal(clone->vals[3], V3);

	plist_free(clone);
	plist_free(list);
}
static void plist_free__null(void **state) {
	plist_free(NULL);
}

static void plist_free__empty(void **state) {
	const struct Plist *list = plist_init();

	plist_free(list);
}

static void plist_free_vals__null(void **state) {
	plist_free_vals(NULL);
}

static void plist_free_vals__empty(void **state) {
	const struct Plist *list = plist_init();

	plist_free_vals(list);
}

static void plist_free_vals__null_free_val(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("val will be freed");

	plist_append(list, val);

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], val);

	plist_free_vals(list);
}

static void plist_free_vals__missing_val(void **state) {
	const struct Plist *list = plist_init();

	char *val = strdup("will not be freed");

	plist_append(list, val);

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], val);

	list->vals[0] = NULL;

	plist_free_vals(list);
	free(val);
}

static void plist_free_vals__free_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V0, V0, V0, V0, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	plist_free_vals(list);
}

static void plist_free_vals__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V2);

	plist_free_vals(list);
}

static void plist_it_free__null(void **state) {
	plist_it_free(NULL);
}

static void plist_it_free__incomplete(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	plist_it_free(it);
}

static void plist_contains__null(void **state) {
	assert_false(plist_contains(NULL, V0));
}

static void plist_contains__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_false(plist_contains(list, V0));

	plist_free(list);
}

static void plist_contains__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_true(plist_contains(list, V0));

	assert_false(plist_contains(list, NULL));

	assert_false(plist_contains(list, V2));

	plist_free(list);
}

static void plist_contains__equal_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .equal_val = mock_equal, });
	plist_append_many(list, V0, NULL);

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V0); will_return(mock_equal, true);

	assert_true(plist_contains(list, V0));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V1); will_return(mock_equal, false);

	assert_false(plist_contains(list, V1));

	plist_free(list);
}

static void plist_contains__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_true(plist_contains(list, V0));
	assert_true(plist_contains(list, NULL));
	assert_true(plist_contains(list, V2));

	plist_free(list);
}

static void plist_index_of__null(void **state) {
	size_t i = 99;

	assert_false(plist_index_of(&i, NULL, V0));

	assert_int_equal(i, 0);
}

static void plist_index_of__empty(void **state) {
	const struct Plist *list = plist_init();

	size_t i = 10;
	assert_false(plist_index_of(&i, list, V0));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_index_of__no_list(void **state) {
	const struct Plist *list = plist_init();

	size_t i = 10;
	assert_false(plist_index_of(&i, list, V0));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_index_of__no_allow_null_val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V1));
	assert_int_equal(i, 1);

	assert_false(plist_index_of(&i, list, NULL));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_index_of__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V2));
	assert_int_equal(i, 2);

	assert_true(plist_index_of(&i, list, NULL));
	assert_int_equal(i, 1);

	plist_free(list);
}

static void plist_index_of__no_ptr(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_true(plist_index_of(NULL, list, V1));

	plist_free(list);
}

static void plist_index_of__equal_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .equal_val = mock_equal, });
	plist_append_many(list, V0, V1, NULL);

	expect_ptr_count(mock_equal, a, V0, 1); expect_ptr_count(mock_equal, b, V2, 1); will_return_int_count(mock_equal, true, 1);

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V2));
	assert_int_equal(i, 0);

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V3); will_return(mock_equal, false);

	expect_ptr(mock_equal, a, V1); expect_ptr(mock_equal, b, V3); will_return(mock_equal, true);

	assert_true(plist_index_of(&i, list, V3));
	assert_int_equal(i, 1);

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V4); will_return(mock_equal, false);

	expect_ptr(mock_equal, a, V1); expect_ptr(mock_equal, b, V4); will_return(mock_equal, false);

	assert_false(plist_index_of(&i, list, V4));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_at__null(void **state) {
	assert_nul(plist_at(NULL, 0));
}

static void plist_at__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_at(list, 0));
	assert_nul(plist_at(list, 123));

	plist_free(list);
}

static void plist_at__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_nul(plist_at(list, 2));

	plist_free(list);
}

static void plist_at__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_find__null(void **state) {
	assert_nul(plist_find(NULL, (struct PlistFilter){ 0 }));
}

static void plist_find__list_empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_find(list, (struct PlistFilter){ 0 }));

	plist_free(list);
}

static void plist_find__filter_empty(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	assert_ptr_equal(plist_find(list, (struct PlistFilter){ 0 }), V0);

	plist_free(list);
}

static void plist_find__val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	// skip V0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);
	// get V1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	assert_ptr_equal(plist_find(list, (struct PlistFilter){ .val = mock_pred_p, }), V1);

	plist_free(list);
}

static void plist_find__val_data(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);
	// get V1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	assert_ptr_equal(plist_find(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, }), V1);

	plist_free(list);
}

static void plist_find__no_match(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);
	// get V1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	assert_nul(plist_find(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, }));

	plist_free(list);
}

static void plist_it_start__null(void **state) {
	assert_nul(plist_it_start(NULL));
}

static void plist_it_start__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_it_start(list));

	plist_free(list);
}

static void plist_it_start__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	const struct PlistIt *it = plist_it_start(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_next(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_it_end__null(void **state) {
	assert_nul(plist_it_end(NULL));
}

static void plist_it_end__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_it_end(list));

	plist_free(list);
}

static void plist_it_end__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	const struct PlistIt *it = plist_it_end(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_prev(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_it_next__null(void **state) {
	assert_nul(plist_it_next(NULL));
}

static void plist_it_next__incomplete(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_next(it));
}

static void plist_it_prev__null(void **state) {
	assert_nul(plist_it_prev(NULL));
}

static void plist_it_prev__incomplete(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_prev(it));
}

static void plist_it_next__prev(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_prev(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_it_prev__next(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_next(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_filter_it_start__null(void **state) {
	assert_nul(plist_filter_it_start(NULL, (struct PlistFilter){ 0 }));
}

static void plist_filter_it_start__list_empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_filter_it_start(list, (struct PlistFilter){ 0 }));

	plist_free(list);
}

static void plist_filter_it_start__filter_empty(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_filter_it_start(list, (struct PlistFilter){ 0 });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_start__val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, false);

	// get 1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, true);

	const struct PlistIt *it = plist_filter_it_start(list, (struct PlistFilter){ .val = mock_pred_p, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_start__val_data(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	const struct PlistIt *it = plist_filter_it_start(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_start__no_match(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// skip 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	const struct PlistIt *it = plist_filter_it_start(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_nul(it);

	plist_free(list);
}

static void plist_filter_it_end__null(void **state) {
	assert_nul(plist_filter_it_end(NULL, (struct PlistFilter){ 0 }));
}

static void plist_filter_it_end__list_empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_filter_it_end(list, (struct PlistFilter){ 0 }));

	plist_free(list);
}

static void plist_filter_it_end__filter_empty(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_filter_it_end(list, (struct PlistFilter){ 0 });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_end__val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 1
	expect_ptr(mock_pred_p, p, V1); will_return(mock_pred_p, false);

	// get 0
	expect_ptr(mock_pred_p, p, V0); will_return(mock_pred_p, true);

	const struct PlistIt *it = plist_filter_it_end(list, (struct PlistFilter){ .val = mock_pred_p, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_end__val_data(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// get 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, true);

	const struct PlistIt *it = plist_filter_it_end(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);
	plist_free(list);
}

static void plist_filter_it_end__no_match(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	// skip 1
	expect_ptr(mock_pred_p_p, p1, V1); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0); expect_ptr(mock_pred_p_p, p2, D0); will_return(mock_pred_p_p, false);

	const struct PlistIt *it = plist_filter_it_end(list, (struct PlistFilter){ .val_data = mock_pred_p_p, .data = D0, });

	assert_nul(it);

	plist_free(list);
}

static void plist_insert__null(void **state) {
	assert_false(plist_insert(NULL, 0, NULL));
}

static void plist_insert__start(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_insert(list, 0, V1));
	assert_true(plist_insert(list, 0, V0));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	plist_free(list);
}

static void plist_insert__end(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_insert(list, 0, V0));
	assert_true(plist_insert(list, 1, V1));
	assert_true(plist_insert(list, 999, V2));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_insert__no_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_false(plist_insert(list, 0, NULL));

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_insert__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	assert_true(plist_insert(list, 0, NULL));

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], NULL);

	plist_free(list);
}

static void plist_insert__alloc_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	assert_true(plist_insert(list, 0, V0));

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], V0);

	plist_free(list);
}

static void plist_insert__alloc_val_returned_null(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, NULL, void*);
	assert_false(plist_insert(list, 0, V0));

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_insert__grow(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .initial = 2, .grow = 5 });
	plist_append_many(list, V0, V1, NULL);

	assert_int_equal(list->size, 2);
	assert_int_equal(list->capacity, 2);
	assert_int_equal(list->params.grow, 5);

	assert_true(plist_insert(list, 0, V2));
	assert_int_equal(list->size, 3);
	assert_int_equal(list->capacity, 7);

	assert_true(plist_insert(list, 0, V3));
	assert_int_equal(list->size, 4);
	assert_int_equal(list->capacity, 7);

	assert_true(plist_insert(list, 0, V4));
	assert_true(plist_insert(list, 0, V5));
	assert_int_equal(list->size, 6);
	assert_int_equal(list->capacity, 7);

	plist_free(list);
}

static void plist_append__null(void **state) {
	assert_false(plist_append(NULL, V0));
}

static void plist_append__no_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_false(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	plist_free(list);
}

static void plist_append__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], NULL);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_prepend__null(void **state) {
	assert_false(plist_prepend(NULL, V0));
}

static void plist_prepend__no_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_prepend(list, V0));
	assert_false(plist_prepend(list, NULL));
	assert_true(plist_prepend(list, V2));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V2);
	assert_ptr_equal(list->vals[1], V0);

	plist_free(list);
}

static void plist_prepend__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	assert_true(plist_prepend(list, V0));
	assert_true(plist_prepend(list, NULL));
	assert_true(plist_prepend(list, V2));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V2);
	assert_ptr_equal(list->vals[1], NULL);
	assert_ptr_equal(list->vals[2], V0);

	plist_free(list);
}

static void plist_replace__null(void **state) {
	assert_false(plist_replace(NULL, 1, V0));
}

static void plist_replace__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	assert_ptr_equal(plist_replace(list, 1, V3), V1);

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V3);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_replace__beyond_end(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	assert_nul(plist_replace(list, 3, V3));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_replace__no_allow_null_val(void **state) {
	const struct Plist *list = plist_init();

	plist_append(list, V0);

	assert_nul(plist_replace(list, 0, NULL));

	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_replace__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_nul(plist_replace(list, 1, V3));
	assert_ptr_equal(plist_replace(list, 2, NULL), V2);

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V3);
	assert_ptr_equal(list->vals[2], NULL);

	plist_free(list);
}

static void plist_replace__alloc_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	plist_append(list, V0);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_ptr_equal(plist_replace(list, 0, V1), V0);

	assert_ptr_equal(plist_at(list, 0), V1);

	expect_ptr(mock_alloc, ptr, V2);
	will_return_ptr_type(mock_alloc, V5, void*);

	assert_ptr_equal(plist_replace(list, 0, V2), V1);

	expect_ptr(mock_alloc, ptr, NULL);
	will_return_ptr_type(mock_alloc, V4, void*);

	assert_ptr_equal(plist_replace(list, 0, NULL), V5);

	assert_ptr_equal(plist_at(list, 0), V4);

	plist_free(list);
}

static void plist_replace__alloc_val_returned_null(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	plist_append(list, V0);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(plist_replace(list, 0, V1));

	assert_int_equal(plist_size(list), 1);

	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_replace__alloc_val__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, .allow_null_val = true, });

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	plist_append(list, V0);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_ptr_equal(plist_replace(list, 0, V1), V0);

	assert_int_equal(plist_size(list), 1);

	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_replace_free__null(void **state) {
	assert_false(plist_replace_free(NULL, 1, V0));
}

static void plist_replace_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, strdup("to free"), V2, NULL);

	assert_true(plist_replace_free(list, 1, V3));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V3);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_replace_free__free_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	expect_ptr(mock_free, ptr, V1);

	assert_true(plist_replace_free(list, 1, V3));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V3);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_replace_free__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, .allow_null_val = true, });

	plist_append(list, V0);
	plist_append(list, V1);
	plist_append(list, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_true(plist_replace_free(list, 0, NULL));
	assert_true(plist_replace_free(list, 1, V3));
	assert_true(plist_replace_free(list, 2, V4));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], NULL);
	assert_ptr_equal(list->vals[1], V3);
	assert_ptr_equal(list->vals[2], V4);

	plist_free(list);
}

static void plist_replace_free__beyond_end(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free,  });
	plist_append_many(list, V0, V1, V2, NULL);

	assert_false(plist_replace_free(list, 3, V3));

	assert_int_equal(list->size, 3);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);
	assert_ptr_equal(list->vals[2], V2);

	plist_free(list);
}

static void plist_append_all__null(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_append_all(NULL, NULL), 0);
	assert_int_equal(plist_append_all(NULL, list), 0);
	assert_int_equal(plist_append_all(list, NULL), 0);

	plist_free(list);
}

static void plist_append_all__duplicates(void **state) {
	const struct Plist *from = plist_init();
	plist_append_many(from, V1, V2, NULL);

	const struct Plist *to = plist_init();
	plist_append_many(to, V0, V1, NULL);

	assert_int_equal(plist_append_all(to, from), 2);

	assert_int_equal(to->size, 4);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);
	assert_ptr_equal(to->vals[2], V1);
	assert_ptr_equal(to->vals[3], V2);

	plist_free(to);
	plist_free(from);
}

static void plist_append_all__alloc_val(void **state) {
	const struct Plist *from = plist_init();
	plist_append_many(from, V1, NULL);

	const struct Plist *to = plist_init_with((struct PlistParams){ .alloc_val = mock_alloc, });

	expect_ptr(mock_alloc, ptr, V0); will_return_ptr_type(mock_alloc, V0, void*);
	assert_true(plist_append(to, V0));

	expect_ptr(mock_alloc, ptr, V1); will_return_ptr_type(mock_alloc, V1, void*);
	assert_int_equal(plist_append_all(to, from), 1);

	assert_int_equal(to->size, 2);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);

	plist_free(to);
	plist_free(from);
}

static void plist_append_all__allow_null_val(void **state) {
	const struct Plist *from = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(from, V3);
	plist_append(from, NULL);
	plist_append(from, V5);

	const struct Plist *to = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(to, V0);
	plist_append(to, NULL);
	plist_append(to, V2);

	assert_int_equal(plist_append_all(to, from), 3);

	assert_int_equal(to->size, 6);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], NULL);
	assert_ptr_equal(to->vals[2], V2);
	assert_ptr_equal(to->vals[3], V3);
	assert_ptr_equal(to->vals[4], NULL);
	assert_ptr_equal(to->vals[5], V5);

	plist_free(to);
	plist_free(from);
}

static void plist_append_all_clone__null(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_append_all_clone(NULL, NULL), 0);
	assert_int_equal(plist_append_all_clone(NULL, list), 0);
	assert_int_equal(plist_append_all_clone(list, NULL), 0);

	plist_free(list);
}

static void plist_append_all_clone__clone_val(void **state) {
	const struct Plist *from = plist_init();
	plist_append_many(from, V1, V2, V3, NULL);

	const struct Plist *to = plist_init_with((struct PlistParams){ .clone_val = mock_clone, });
	plist_append_many(to, V0, V1, NULL);

	// V1 ok
	expect_ptr(mock_clone, ptr, V1); will_return_ptr_type(mock_clone, V1, void*);

	// V2 ok
	expect_ptr(mock_clone, ptr, V2); will_return_ptr_type(mock_clone, V2, void*);

	// V3 alloc fail
	expect_ptr(mock_clone, ptr, V3); will_return_ptr_type(mock_clone, NULL, void*);

	assert_int_equal(plist_append_all_clone(to, from), 2);

	assert_int_equal(to->size, 4);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], V1);
	assert_ptr_equal(to->vals[2], V1);
	assert_ptr_equal(to->vals[3], V2);

	plist_free(to);
	plist_free(from);
}

static void plist_append_all_clone__no_clone_val(void **state) {
	const struct Plist *from = plist_init();
	plist_append_many(from, V1, NULL);

	const struct Plist *to = plist_init();
	plist_append_many(to, V0, NULL);

	assert_int_equal(plist_append_all_clone(to, from), 0);

	assert_int_equal(to->size, 1);
	assert_ptr_equal(to->vals[0], V0);

	plist_free(to);
	plist_free(from);
}

static void plist_append_all_clone__allow_null_val(void **state) {
	const struct Plist *from = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(from, NULL);
	plist_append(from, V4);

	const struct Plist *to = plist_init_with((struct PlistParams){ .allow_null_val = true, .clone_val = mock_clone, });
	plist_append(to, V0);
	plist_append(to, NULL);
	plist_append(to, V2);

	// NULL ok
	expect_ptr(mock_clone, ptr, NULL); will_return_ptr_type(mock_clone, V3, void*);

	// V4 ok
	expect_ptr(mock_clone, ptr, V4); will_return_ptr_type(mock_clone, V4, void*);

	assert_int_equal(plist_append_all_clone(to, from), 2);

	assert_int_equal(to->size, 5);
	assert_ptr_equal(to->vals[0], V0);
	assert_ptr_equal(to->vals[1], NULL);
	assert_ptr_equal(to->vals[2], V2);
	assert_ptr_equal(to->vals[3], V3);
	assert_ptr_equal(to->vals[4], V4);

	plist_free(to);
	plist_free(from);
}

static void plist_remove__null(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove(NULL, NULL), 0);
	assert_int_equal(plist_remove(list, NULL), 0);

	plist_free(list);
}

static void plist_remove__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove(list, V0), 0);

	plist_free(list);
}

static void plist_remove__null_val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, NULL);

	assert_int_equal(plist_remove(list, NULL), 0);

	plist_free(list);
}

static void plist_remove__exists(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_ptr_equal(plist_remove(list, V0), V0);

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], V1);

	assert_ptr_equal(plist_remove(list, V1), V1);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove__inexistent(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_nul(plist_remove(list, V2));

	assert_int_equal(list->size, 2);

	plist_free(list);
}

static void plist_remove__equal_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .equal_val = mock_equal, });
	assert_true(plist_append(list, V0));

	expect_ptr(mock_equal, a, V0); expect_ptr(mock_equal, b, V0); will_return(mock_equal, true);
	assert_ptr_equal(plist_remove(list, V0), V0);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_nul(plist_remove(list, NULL));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	plist_free(list);
}

static void plist_remove_free__null(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_free(NULL, NULL), 0);
	assert_int_equal(plist_remove_free(list, NULL), 0);

	plist_free(list);
}

static void plist_remove_free__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_free(list, V0), 0);

	plist_free(list);
}

static void plist_remove_free__free_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0);
	assert_true(plist_remove_free(list, V0));

	assert_false(plist_remove_free(list, NULL));

	assert_false(plist_remove_free(list, V2));

	assert_int_equal(list->size, 1);

	plist_free(list);
}

static void plist_remove_free__free(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("should be freed");

	assert_true(plist_append(list, val));

	assert_true(plist_remove_free(list, val));

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove_free__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_true(plist_remove_free(list, NULL));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	plist_free(list);
}

static void plist_remove_at__null(void **state) {
	assert_nul(plist_remove_at(NULL, 1));
}

static void plist_remove_at__existing(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	assert_ptr_equal(plist_remove_at(list, 0), V0);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V1);
	assert_ptr_equal(list->vals[1], V2);

	assert_ptr_equal(plist_remove_at(list, 1), V2);

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], V1);

	plist_free(list);
}

static void plist_remove_at__inexistent(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_nul(plist_remove_at(list, 999));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	plist_free(list);
}

static void plist_remove_at__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });

	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_nul(plist_remove_at(list, 1));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	plist_free(list);
}

static void plist_remove_at_free__null(void **state) {
	assert_false(plist_remove_at_free(NULL, 1));
}

static void plist_remove_at_free__free_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	expect_ptr_count(mock_free, ptr, V1, 1);

	assert_true(plist_remove_at_free(list, 1));

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V2);

	expect_ptr_count(mock_free, ptr, V2, 1);

	assert_true(plist_remove_at_free(list, 1));

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	expect_ptr_count(mock_free, ptr, V0, 1);

	assert_true(plist_remove_at_free(list, 0));

	assert_false(plist_remove_at_free(list, 0));

	plist_free(list);
}

static void plist_remove_at_free__free(void **state) {
	const struct Plist *list = plist_init();

	const char *val0 = strdup("0");
	const char *val1 = strdup("1");

	plist_append_many(list, val0, val1, NULL);

	assert_true(plist_remove_at_free(list, 1));

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), val0);

	assert_false(plist_remove_at_free(list, 1));

	plist_free_vals(list);
}

static void plist_remove_at_free__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, .allow_null_val = true, });

	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	expect_ptr(mock_free, ptr, V2);

	assert_true(plist_remove_at_free(list, 2));

	// no mock_free

	assert_true(plist_remove_at_free(list, 1));

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_remove_all__null(void **state) {
	assert_int_equal(plist_remove_all(NULL), 0);
}

static void plist_remove_all__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all(list), 0);

	plist_free(list);
}

static void plist_remove_all__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_int_equal(plist_remove_all(list), 2);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove_all__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	assert_int_equal(plist_remove_all(list), 3);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove_all_free__null(void **state) {
	assert_int_equal(plist_remove_all_free(NULL), 0);
}

static void plist_remove_all_free__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all_free(list), 0);

	plist_free(list);
}

static void plist_remove_all_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, strdup("to free 0"), strdup("to free 1"), NULL);

	assert_int_equal(plist_remove_all_free(list), 2);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove_all_free__free_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, NULL);

	expect_ptr(mock_free, ptr, V0); expect_ptr(mock_free, ptr, V1);
	assert_int_equal(plist_remove_all_free(list), 2);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_remove_all_free__allow_null_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	expect_ptr(mock_free, ptr, V0); expect_ptr(mock_free, ptr, V2);
	assert_int_equal(plist_remove_all_free(list), 3);

	assert_int_equal(list->size, 0);

	plist_free(list);
}

static void plist_it_remove__null(void **state) {
	assert_nul(plist_it_remove(NULL));
}

static void plist_it_remove__incomplete(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_remove(it));;
}

static void plist_it_remove__forwards_first(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);

	assert_ptr_equal(plist_it_remove(it), V0);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V1);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove__backwards_first(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);

	assert_ptr_equal(plist_it_remove(it), V2);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	it = plist_it_prev(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove__forwards_mid(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);
	it = plist_it_next(it);

	assert_ptr_equal(plist_it_remove(it), V1);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V2);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove__backwards_mid(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);
	it = plist_it_prev(it);

	assert_ptr_equal(plist_it_remove(it), V1);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_prev(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V0);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove__forwards_last(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);
	it = plist_it_next(it);
	it = plist_it_next(it);

	assert_ptr_equal(plist_it_remove(it), V2);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	assert_nul(plist_it_next(it));

	plist_free(list);
}

static void plist_it_remove__backwards_last(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);
	it = plist_it_prev(it);
	it = plist_it_prev(it);

	assert_ptr_equal(plist_it_remove(it), V0);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V1);
	assert_ptr_equal(list->vals[1], V2);

	assert_nul(plist_it_prev(it));

	plist_free(list);
}

static void plist_it_remove__forwards_all(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(list->size, 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove__backwards_all(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_end(list); it; it = plist_it_prev(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(list->size, 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove_free__null(void **state) {
	assert_false(plist_it_remove_free(NULL));
}

static void plist_it_remove_free__incomplete(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_false(plist_it_remove_free(it));;
}

static void plist_it_remove_free__forwards_first(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);

	expect_ptr(mock_free, ptr, V0);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V1);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove_free__backwards_first(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);

	expect_ptr(mock_free, ptr, V2);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	it = plist_it_prev(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V1);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove_free__forwards_mid(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);
	it = plist_it_next(it);

	expect_ptr(mock_free, ptr, V1);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_next(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V2);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove_free__backwards_mid(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);
	it = plist_it_prev(it);

	expect_ptr(mock_free, ptr, V1);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V2);

	it = plist_it_prev(it);
	assert_non_nul(it);

	assert_ptr_equal(it->val, V0);

	plist_it_free(it);
	plist_free(list);
}

static void plist_it_remove_free__forwards_last(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_start(list);
	it = plist_it_next(it);
	it = plist_it_next(it);

	expect_ptr(mock_free, ptr, V2);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	assert_nul(plist_it_next(it));

	plist_free(list);
}

static void plist_it_remove_free__backwards_last(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	const struct PlistIt *it = plist_it_end(list);
	it = plist_it_prev(it);
	it = plist_it_prev(it);

	expect_ptr(mock_free, ptr, V0);
	assert_true(plist_it_remove_free(it));

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V1);
	assert_ptr_equal(list->vals[1], V2);

	assert_nul(plist_it_prev(it));

	plist_free(list);
}

static void plist_it_remove_free__forwards_all(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V2);

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		iterations++;
		assert_true(plist_it_remove_free(it));
	}

	assert_int_equal(list->size, 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove_free__backwards_all(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .free_val = mock_free, });
	plist_append_many(list, V0, V1, V2, NULL);

	expect_ptr(mock_free, ptr, V2);
	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V0);

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_end(list); it; it = plist_it_prev(it)) {
		iterations++;
		assert_true(plist_it_remove_free(it));
	}

	assert_int_equal(list->size, 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_sort__null(void **state) {
	plist_sort(NULL, mock_less_than);
}

static void plist_sort__no_less_than(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	plist_sort(list, NULL);

	assert_int_equal(list->size, 2);
	assert_ptr_equal(list->vals[0], V0);
	assert_ptr_equal(list->vals[1], V1);

	plist_free(list);
}

static void plist_sort__empty(void **state) {
	const struct Plist *actual = plist_init();

	plist_sort(actual, mock_less_than);

	plist_free(actual);
}

static void plist_sort__one(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, NULL);

	plist_sort(list, mock_less_than);

	assert_int_equal(list->size, 1);
	assert_ptr_equal(list->vals[0], V0);

	plist_free(list);
}

static void plist_sort__many(void **state) {
	const char *v[6] = { "a", "b", "c", "d", "e", "f" };

	const struct Plist *expected = plist_init();
	plist_append_many(expected, v[0], v[1], v[2], v[3], v[4], v[5], NULL);

	const struct Plist *actual = plist_init();
	plist_append_many(actual, v[2], v[0], v[3], v[5], v[1], v[4], NULL);

	plist_sort(actual, (fn_less_than)less_than_strcmp);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_equal__null(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	assert_false(plist_equal(NULL, NULL));
	assert_false(plist_equal(list, NULL));
	assert_false(plist_equal(NULL, list));

	plist_free(list);
}

static void plist_equal__length_different(void **state) {
	const struct Plist *a = plist_init();
	plist_append_many(a, V0, NULL);
	const struct Plist *b = plist_init();
	plist_append_many(b, V0, V1, NULL);

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_ok(void **state) {
	const struct Plist *a = plist_init();
	plist_append_many(a, V0, V1, NULL);

	const struct Plist *b = plist_init();
	plist_append_many(b, V0, V1, NULL);

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_different(void **state) {
	const struct Plist *a = plist_init();
	plist_append_many(a, V0, V1, NULL);

	const struct Plist *b = plist_init();
	plist_append_many(b, V0, V2, NULL);

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val_ok(void **state) {
	const struct Plist *a = plist_init_with((struct PlistParams){ .equal_val = (fn_equal)equal_strcmp, });
	plist_append_many(a, "a", "b", NULL);

	const struct Plist *b = plist_init();
	plist_append_many(b, "a", "b", NULL);

	assert_plist_equal(a, b);

	plist_free(b);
	plist_free(a);
}

static void plist_equal__equal_val_different(void **state) {
	const struct Plist *a = plist_init_with((struct PlistParams){ .equal_val = (fn_equal)equal_strcmp, });
	plist_append_many(a, "a", "b", NULL);

	const struct Plist *b = plist_init();
	plist_append_many(b, "a", "c", NULL);

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_str__null(void **state) {
	assert_nul(plist_str(NULL));
}

static void plist_str__empty(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .str_val = mock_str, });

	char *str = plist_str(list);
	assert_str_equal(str, "");

	free(str);
	plist_free(list);
}

static void plist_str__pointers(void **state) {
	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n",
			V0,
			V1
			);

	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, NULL);

	char *actual = plist_str(list);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	plist_free(list);
}

static void plist_str__allow_null_val(void **state) {
	char *expected = sprintf_alloc(
			"%p\n"
			"(null)\n"
			"%p\n",
			V0,
			V2
			);

	const struct Plist *list = plist_init_with((struct PlistParams){ .allow_null_val = true, });
	plist_append(list, V0);
	plist_append(list, NULL);
	plist_append(list, V2);

	char *actual = plist_str(list);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	plist_free(list);
}

static void plist_str__str_val(void **state) {
	const struct Plist *list = plist_init_with((struct PlistParams){ .str_val = (fn_str)starts_with_a_or_null, });
	plist_append_many(list, "a1", "b2", "a3", NULL);

	char *str = plist_str(list);
	assert_str_equal(str,
			"a1\n"
			"(null)\n"
			"a3\n"
			);

	free(str);
	plist_free(list);
}

static void plist_size__null(void **state) {
	assert_int_equal(plist_size(NULL), 0);
}

static void plist_size__present(void **state) {
	const struct Plist *list = plist_init();
	plist_append_many(list, V0, V1, V2, NULL);

	assert_int_equal(plist_size(list), 3);

	plist_free(list);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(plist_init__defaults),

		TEST(plist_clone__null),
		TEST(plist_clone__empty),
		TEST(plist_clone__params__constructor),
		TEST(plist_clone__val_ptr),
		TEST(plist_clone__alloc_val),
		TEST(plist_clone__allow_null_val),

		TEST(plist_clone_deep__null),
		TEST(plist_clone_deep__clone_val),
		TEST(plist_clone_deep__no_clone_val),
		TEST(plist_clone_deep__allow_null_val),

		TEST(plist_free__null),
		TEST(plist_free__empty),

		TEST(plist_free_vals__null),
		TEST(plist_free_vals__empty),
		TEST(plist_free_vals__null_free_val),
		TEST(plist_free_vals__missing_val),
		TEST(plist_free_vals__free_val),
		TEST(plist_free_vals__allow_null_val),

		TEST(plist_it_free__null),
		TEST(plist_it_free__incomplete),

		TEST(plist_contains__null),
		TEST(plist_contains__empty),
		TEST(plist_contains__present),
		TEST(plist_contains__equal_val),
		TEST(plist_contains__allow_null_val),

		TEST(plist_index_of__null),
		TEST(plist_index_of__empty),
		TEST(plist_index_of__no_list),
		TEST(plist_index_of__allow_null_val),
		TEST(plist_index_of__no_allow_null_val),
		TEST(plist_index_of__no_ptr),
		TEST(plist_index_of__equal_val),

		TEST(plist_at__null),
		TEST(plist_at__empty),
		TEST(plist_at__present),
		TEST(plist_at__allow_null_val),

		TEST(plist_find__null),
		TEST(plist_find__list_empty),
		TEST(plist_find__filter_empty),
		TEST(plist_find__val),
		TEST(plist_find__val_data),
		TEST(plist_find__no_match),

		TEST(plist_it_start__null),
		TEST(plist_it_start__empty),
		TEST(plist_it_start__present),

		TEST(plist_it_end__null),
		TEST(plist_it_end__empty),
		TEST(plist_it_end__present),

		TEST(plist_it_next__null),
		TEST(plist_it_next__incomplete),

		TEST(plist_it_prev__null),
		TEST(plist_it_prev__incomplete),

		TEST(plist_it_next__prev),
		TEST(plist_it_prev__next),

		TEST(plist_filter_it_start__null),
		TEST(plist_filter_it_start__list_empty),
		TEST(plist_filter_it_start__filter_empty),
		TEST(plist_filter_it_start__val),
		TEST(plist_filter_it_start__val_data),
		TEST(plist_filter_it_start__no_match),

		TEST(plist_filter_it_end__null),
		TEST(plist_filter_it_end__list_empty),
		TEST(plist_filter_it_end__filter_empty),
		TEST(plist_filter_it_end__val),
		TEST(plist_filter_it_end__val_data),
		TEST(plist_filter_it_end__no_match),

		TEST(plist_insert__null),
		TEST(plist_insert__start),
		TEST(plist_insert__end),
		TEST(plist_insert__no_null_val),
		TEST(plist_insert__allow_null_val),
		TEST(plist_insert__alloc_val),
		TEST(plist_insert__alloc_val_returned_null),
		TEST(plist_insert__grow),

		TEST(plist_append__null),
		TEST(plist_append__no_null_val),
		TEST(plist_append__allow_null_val),

		TEST(plist_prepend__null),
		TEST(plist_prepend__no_null_val),
		TEST(plist_prepend__allow_null_val),

		TEST(plist_replace__null),
		TEST(plist_replace__present),
		TEST(plist_replace__beyond_end),
		TEST(plist_replace__no_allow_null_val),
		TEST(plist_replace__allow_null_val),
		TEST(plist_replace__alloc_val),
		TEST(plist_replace__alloc_val_returned_null),
		TEST(plist_replace__alloc_val__allow_null_val),

		TEST(plist_replace_free__null),
		TEST(plist_replace_free__no_free_val),
		TEST(plist_replace_free__free_val),
		TEST(plist_replace_free__allow_null_val),
		TEST(plist_replace_free__beyond_end),

		TEST(plist_append_all__null),
		TEST(plist_append_all__duplicates),
		TEST(plist_append_all__alloc_val),
		TEST(plist_append_all__allow_null_val),

		TEST(plist_append_all_clone__null),
		TEST(plist_append_all_clone__clone_val),
		TEST(plist_append_all_clone__no_clone_val),
		TEST(plist_append_all_clone__allow_null_val),

		TEST(plist_remove__null),
		TEST(plist_remove__empty),
		TEST(plist_remove__null_val),
		TEST(plist_remove__exists),
		TEST(plist_remove__inexistent),
		TEST(plist_remove__equal_val),
		TEST(plist_remove__allow_null_val),

		TEST(plist_remove_free__null),
		TEST(plist_remove_free__empty),
		TEST(plist_remove_free__free_val),
		TEST(plist_remove_free__free),
		TEST(plist_remove_free__allow_null_val),

		TEST(plist_remove_at__null),
		TEST(plist_remove_at__existing),
		TEST(plist_remove_at__inexistent),
		TEST(plist_remove_at__allow_null_val),

		TEST(plist_remove_at_free__null),
		TEST(plist_remove_at_free__free_val),
		TEST(plist_remove_at_free__free),
		TEST(plist_remove_at_free__allow_null_val),

		TEST(plist_remove_all__null),
		TEST(plist_remove_all__empty),
		TEST(plist_remove_all__present),
		TEST(plist_remove_all__allow_null_val),

		TEST(plist_remove_all_free__null),
		TEST(plist_remove_all_free__empty),
		TEST(plist_remove_all_free__no_free_val),
		TEST(plist_remove_all_free__free_val),
		TEST(plist_remove_all_free__allow_null_val),

		TEST(plist_it_remove__null),
		TEST(plist_it_remove__incomplete),

		TEST(plist_it_remove__forwards_first),
		TEST(plist_it_remove__backwards_first),

		TEST(plist_it_remove__forwards_mid),
		TEST(plist_it_remove__backwards_mid),

		TEST(plist_it_remove__forwards_last),
		TEST(plist_it_remove__backwards_last),

		TEST(plist_it_remove__forwards_all),
		TEST(plist_it_remove__backwards_all),

		TEST(plist_it_remove_free__null),
		TEST(plist_it_remove_free__incomplete),

		TEST(plist_it_remove_free__forwards_first),
		TEST(plist_it_remove_free__backwards_first),

		TEST(plist_it_remove_free__forwards_mid),
		TEST(plist_it_remove_free__backwards_mid),

		TEST(plist_it_remove_free__forwards_last),
		TEST(plist_it_remove_free__backwards_last),

		TEST(plist_it_remove_free__forwards_all),
		TEST(plist_it_remove_free__backwards_all),

		TEST(plist_sort__null),
		TEST(plist_sort__no_less_than),
		TEST(plist_sort__empty),
		TEST(plist_sort__one),
		TEST(plist_sort__many),

		TEST(plist_equal__null),
		TEST(plist_equal__length_different),
		TEST(plist_equal__val_pointers_ok),
		TEST(plist_equal__val_pointers_different),
		TEST(plist_equal__equal_val_ok),
		TEST(plist_equal__equal_val_different),

		TEST(plist_str__null),
		TEST(plist_str__empty),
		TEST(plist_str__pointers),
		TEST(plist_str__allow_null_val),
		TEST(plist_str__str_val),

		TEST(plist_size__null),
		TEST(plist_size__present),
	};

	return RUN(tests);
}

