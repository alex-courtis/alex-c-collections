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

static void plist_clone__empty(void **state) {
	const struct Plist *list = plist_init();

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(plist_size(clone), 0);

	plist_free(list);
	plist_free(clone);
}

// also tests constructor
static void plist_clone__params(void **state) {
	const struct PlistParams params = {
		.equal_val = mock_equal,
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
		.allow_null_val = true,
		.initial = 3,
		.grow = 4,
	};
	const struct Plist *list = plist_init_with(params);

	const struct Plist *clone = plist_clone(list);

	assert_non_nul(clone);

	assert_int_equal(list->size, 0);
	assert_int_equal(list->capacity, 3);
	assert_int_equal(list->params.grow, 4);
	assert_true(list->params.allow_null_val);
	assert_ptr_equal(list->params.equal_val, mock_equal);
	assert_ptr_equal(list->params.alloc_val, mock_alloc);
	assert_ptr_equal(list->params.clone_val, mock_clone);

	plist_free(list);
	plist_free(clone);
}

static void plist_clone__many(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(plist_size(clone), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);
	assert_ptr_equal(plist_at(list, 2), V1);

	assert_plist_equal(list, clone);
	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 4);

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(plist_size(clone), 4);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V0);
	assert_ptr_equal(plist_at(list, 2), NULL);
	assert_ptr_equal(plist_at(list, 3), V1);

	assert_plist_equal(list, clone);
	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone__alloc_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.alloc_val = mock_alloc,
	};
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_append(list, V0));

	expect_ptr(mock_alloc, ptr, NULL);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_true(plist_append(list, NULL));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	expect_ptr(mock_alloc, ptr, NULL);
	will_return_ptr_type(mock_alloc, NULL, void*);

	const struct Plist *clone = plist_clone(list);

	assert_int_equal(plist_size(clone), 2);

	assert_ptr_equal(plist_at(clone, 0), V0);
	assert_ptr_equal(plist_at(clone, 1), NULL);

	assert_plist_equal(list, clone);
	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__clone_val(void **state) {
	const struct PlistParams params = { .clone_val = mock_clone, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));

	assert_true(plist_append(list, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V2, void*);
	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V3, void*);

	const struct Plist *clone = plist_clone_deep(list);

	assert_int_equal(plist_size(clone), 2);

	assert_ptr_equal(plist_at(clone, 0), V2);
	assert_ptr_equal(plist_at(clone, 1), V3);

	assert_plist_not_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_clone_deep__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.clone_val = mock_clone,
	};
	const struct Plist *from = plist_init_with(params);

	assert_true(plist_append(from, V0));
	assert_true(plist_append(from, NULL));
	assert_true(plist_append(from, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, NULL);
	will_return_ptr_type(mock_clone, NULL, void*);
	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V4, void*);

	const struct Plist *to = plist_clone_deep(from);

	assert_int_equal(plist_size(to), 3);

	assert_ptr_equal(plist_at(to, 0), NULL);
	assert_ptr_equal(plist_at(to, 1), NULL);
	assert_ptr_equal(plist_at(to, 2), V4);

	plist_free(from);
	plist_free(to);
}

static void plist_clone_deep__no_clone_val(void **state) {
	const struct Plist *from = plist_init();

	assert_true(plist_append(from, V0));

	assert_nul(plist_clone_deep(from));

	plist_free(from);
}

static void plist_clone_deep__alloc_val___clone_val(void **state) {
	const struct PlistParams params = {
		.alloc_val = mock_alloc,
		.clone_val = mock_clone,
	};
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_append(list, V0));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	const struct Plist *clone = plist_clone_deep(list);

	assert_int_equal(plist_size(clone), 1);

	assert_ptr_equal(plist_at(clone, 0), V0);

	assert_plist_equal(clone, list);

	plist_free(clone);
	plist_free(list);
}

static void plist_free_vals__null_free_val(void **state) {
	const struct Plist *list = plist_init();

	const char *val = strdup("0");

	plist_append(list, val);

	assert_int_equal(plist_size(list), 1);

	assert_ptr_equal(plist_at(list, 0), val);

	plist_free_vals(list);
}

static void plist_free_vals__missing_val(void **state) {
	const struct Plist *list = plist_init();

	char *val = strdup("0");

	plist_append(list, val);
	plist_append(list, val);

	assert_int_equal(plist_size(list), 2);

	list->vals[0] = NULL;
	list->vals[1] = NULL;

	plist_free_vals(list);
	free(val);
}

static void plist_free_vals__free_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 4);

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	plist_free_vals(list);
}

static void plist_insert__mid(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_true(plist_insert(list, 1, V3));
	assert_false(plist_insert(list, 1, NULL));

	assert_int_equal(plist_size(list), 4);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), V1);
	assert_ptr_equal(plist_at(list, 3), V2);

	plist_free(list);
}

static void plist_insert__mid__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_true(plist_insert(list, 2, V3));
	assert_true(plist_insert(list, 1, NULL));

	assert_int_equal(plist_size(list), 6);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);
	assert_ptr_equal(plist_at(list, 2), V1);
	assert_ptr_equal(plist_at(list, 3), V3);
	assert_ptr_equal(plist_at(list, 4), NULL);
	assert_ptr_equal(plist_at(list, 5), V2);

	plist_free(list);
}

static void plist_insert__start(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_true(plist_insert(list, 0, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V2);
	assert_ptr_equal(plist_at(list, 1), V0);
	assert_ptr_equal(plist_at(list, 2), V1);

	plist_free(list);
}

static void plist_insert__end(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_true(plist_insert(list, 2, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_insert__beyond_end(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_true(plist_insert(list, 999, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_insert__grow(void **state) {
	const struct PlistParams params = { .initial = 2, .grow = 5 };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));

	assert_int_equal(list->size, 2);
	assert_int_equal(list->capacity, 2);
	assert_int_equal(list->params.grow, 5);

	assert_true(plist_insert(list, 0, V0));
	assert_int_equal(list->size, 3);
	assert_int_equal(list->capacity, 7);

	assert_true(plist_insert(list, 1, V1));
	assert_int_equal(list->size, 4);
	assert_int_equal(list->capacity, 7);

	assert_true(plist_insert(list, 4, V4));
	assert_true(plist_insert(list, 5, V5));
	assert_int_equal(list->size, 6);
	assert_int_equal(list->capacity, 7);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);
	assert_ptr_equal(plist_at(list, 3), V3);
	assert_ptr_equal(plist_at(list, 4), V4);
	assert_ptr_equal(plist_at(list, 5), V5);

	plist_free(list);
}

static void plist_insert__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_insert(list, 0, V0));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_true(plist_insert(list, 0, V1));

	expect_ptr(mock_alloc, ptr, V2);
	will_return_ptr_type(mock_alloc, V3, void*);

	assert_true(plist_insert(list, 0, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V3);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V0);

	plist_free(list);
}

static void plist_insert__alloc_val__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.alloc_val = mock_alloc,
	};
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, NULL);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_true(plist_insert(list, 0, NULL));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_true(plist_insert(list, 0, V1));

	assert_int_equal(plist_size(list), 2);

	assert_ptr_equal(plist_at(list, 0), V1);
	assert_ptr_equal(plist_at(list, 1), NULL);

	plist_free(list);
}

static void plist_insert__alloc_val_returned_null(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_false(plist_insert(list, 0, V0));

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_insert__null(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_insert(list, 0, V0));

	assert_int_equal(plist_size(list), 1);

	assert_false(plist_insert(list, 0, NULL));

	assert_int_equal(plist_size(list), 1);

	plist_free(list);
}

static void plist_append__new(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_prepend__new(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_prepend(list, V0));
	assert_true(plist_prepend(list, V1));
	assert_true(plist_prepend(list, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_at(list, 0), V2);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V0);

	plist_free(list);
}

static void plist_replace__existing(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_ptr_equal(plist_replace(list, 1, V3), V1);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_replace__beyond_end(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_nul(plist_replace(list, 3, V3));

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_replace__no_allow_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));

	assert_nul(plist_replace(list, 0, NULL));

	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_replace__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_nul(plist_replace(list, 1, V3));
	assert_ptr_equal(plist_replace(list, 2, NULL), V2);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), NULL);

	plist_free(list);
}

static void plist_replace__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_append(list, V0));

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
	const struct PlistParams params = { .alloc_val = mock_alloc, };
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_append(list, V0));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_nul(plist_replace(list, 0, V1));

	assert_int_equal(plist_size(list), 1);

	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_replace__alloc_val__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.alloc_val = mock_alloc,
	};
	const struct Plist *list = plist_init_with(params);

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	assert_true(plist_append(list, V0));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_ptr_equal(plist_replace(list, 0, V1), V0);

	assert_int_equal(plist_size(list), 1);

	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_replace_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, strdup("to free")));
	assert_true(plist_append(list, V2));

	plist_replace_free(list, 1, V3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_replace_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	expect_ptr(mock_free, ptr, V1);

	plist_replace_free(list, 1, V3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_replace_free__allow_null_val(void **state) {
	const struct PlistParams params = {
		.free_val = mock_free,
		.allow_null_val = true,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	plist_replace_free(list, 0, NULL);
	plist_replace_free(list, 1, V3);
	plist_replace_free(list, 2, V4);

	assert_ptr_equal(plist_at(list, 0), NULL);
	assert_ptr_equal(plist_at(list, 1), V3);
	assert_ptr_equal(plist_at(list, 2), V4);

	plist_free(list);
}

static void plist_replace_free__beyond_end(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	plist_replace_free(list, 3, V3);

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_append_all__many(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_append(to, V0));
	assert_true(plist_append(to, V0));
	assert_true(plist_append(to, V1));

	assert_int_equal(plist_size(to), 3);

	const struct Plist *from = plist_init();
	assert_true(plist_append(from, V1));
	assert_true(plist_append(from, V2));

	assert_int_equal(plist_size(from), 2);

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V2));

	assert_int_equal(plist_size(expected), 5);

	assert_int_equal(plist_append_all(to, from), 2);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all__alloc_val(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *to = plist_init_with(params);
	assert_true(plist_append(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_append(from, V0));
	assert_true(plist_append(from, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, V1, void*);

	assert_int_equal(plist_append_all(to, from), 2);
	assert_int_equal(plist_size(to), 3);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all__alloc_val_returned_null(void **state) {
	const struct PlistParams params = { .alloc_val = mock_alloc, };

	expect_ptr(mock_alloc, ptr, V0);
	will_return_ptr_type(mock_alloc, V0, void*);

	const struct Plist *to = plist_init_with(params);
	assert_true(plist_append(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_append(from, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));

	expect_ptr(mock_alloc, ptr, V1);
	will_return_ptr_type(mock_alloc, NULL, void*);

	assert_int_equal(plist_append_all(to, from), 0);
	assert_int_equal(plist_size(to), 1);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all__to_allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
	};
	const struct Plist *to = plist_init_with(params);

	assert_true(plist_append(to, V0));

	const struct Plist *from = plist_init_with(params);
	assert_true(plist_append(from, V1));
	assert_true(plist_append(from, NULL));
	assert_true(plist_append(from, V3));

	const struct Plist *expected = plist_init_with(params);
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, NULL));
	assert_true(plist_append(expected, V3));

	assert_int_equal(plist_append_all(to, from), 3);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all__to_no_allow_null_val(void **state) {
	const struct Plist *to = plist_init();

	assert_true(plist_append(to, V0));

	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *from = plist_init_with(params);
	assert_true(plist_append(from, V1));
	assert_true(plist_append(from, NULL));
	assert_true(plist_append(from, V3));

	const struct Plist *expected = plist_init_with(params);
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V3));

	assert_int_equal(plist_append_all(to, from), 2);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all_clone__many(void **state) {
	const struct PlistParams params = { .clone_val = mock_clone, };
	const struct Plist *to = plist_init_with(params);

	assert_true(plist_append(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_append(from, V0));
	assert_true(plist_append(from, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));

	expect_ptr(mock_clone, ptr, V0);
	will_return_ptr_type(mock_clone, V0, void*);

	expect_ptr(mock_clone, ptr, V1);
	will_return_ptr_type(mock_clone, V1, void*);

	assert_int_equal(plist_append_all_clone(to, from), 2);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_all_clone__no_clone_val(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_append(to, V0));

	const struct Plist *from = plist_init();
	assert_true(plist_append(from, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));

	assert_int_equal(plist_append_all_clone(to, from), 0);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(from);
	plist_free(expected);
}

static void plist_append_many__many(void **state) {
	const struct Plist *to = plist_init();
	assert_true(plist_append(to, V0));
	assert_true(plist_append(to, V1));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V0));
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V2));

	assert_int_equal(plist_append_many(to, V0, V1, V2, NULL, NULL), 3);

	assert_plist_equal(to, expected);

	plist_free(to);
	plist_free(expected);
}

static void plist_append_many__no_vals(void **state) {
	const struct Plist *to = plist_init();

	assert_int_equal(plist_append_many(to, NULL, NULL), 0);

	plist_free(to);
}

static void plist_at__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_nul(plist_at(list, 0));
	assert_nul(plist_at(list, 123));

	plist_free(list);
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

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V1));

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V1));
	assert_int_equal(i, 1);

	assert_false(plist_index_of(&i, list, NULL));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_index_of__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V2));
	assert_int_equal(i, 2);

	assert_true(plist_index_of(&i, list, NULL));
	assert_int_equal(i, 1);

	plist_free(list);
}

static void plist_index_of__no_ptr(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_true(plist_index_of(NULL, list, V1));

	plist_free(list);
}

static void plist_index_of__equal_val(void **state) {
	const struct PlistParams params = { .equal_val = mock_equal, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	expect_ptr_count(mock_equal, a, V0, 1);
	expect_ptr_count(mock_equal, b, V2, 1);
	will_return_int_count(mock_equal, true, 1);

	size_t i = 10;
	assert_true(plist_index_of(&i, list, V2));
	assert_int_equal(i, 0);

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V3);
	will_return(mock_equal, false);

	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, V3);
	will_return(mock_equal, true);

	assert_true(plist_index_of(&i, list, V3));
	assert_int_equal(i, 1);

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V4);
	will_return(mock_equal, false);

	expect_ptr(mock_equal, a, V1);
	expect_ptr(mock_equal, b, V4);
	will_return(mock_equal, false);

	assert_false(plist_index_of(&i, list, V4));
	assert_int_equal(i, 0);

	plist_free(list);
}

static void plist_at__many(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);
	assert_ptr_equal(plist_at(list, 2), V1);
	assert_ptr_equal(plist_at(list, 3), V2);

	assert_nul(plist_at(list, 4));

	plist_free(list);
}

static void plist_at__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);
	assert_ptr_equal(plist_at(list, 2), V2);

	plist_free(list);
}

static void plist_contains__no_allow_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_true(plist_contains(list, V1));
	assert_false(plist_contains(list, V2));
	assert_false(plist_contains(list, NULL));

	plist_free(list);
}

static void plist_contains__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));

	assert_true(plist_contains(list, V0));
	assert_false(plist_contains(list, NULL));

	assert_true(plist_append(list, NULL));

	assert_true(plist_contains(list, NULL));

	plist_free(list);
}

static void plist_remove__no_allow_null_val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 3);

	// 0
	assert_ptr_equal(plist_remove(list, V0), V0);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);

	// was 2
	assert_ptr_equal(plist_remove(list, V1), V1);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	// NULL
	assert_null(plist_remove(list, NULL));

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_remove__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));

	assert_null(plist_remove(list, NULL));

	assert_int_equal(plist_size(list), 3);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);
	assert_ptr_equal(plist_at(list, 2), V1);

	plist_free(list);
}

static void plist_remove__inexistent(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 2);

	assert_nul(plist_remove(list, V2));

	assert_nul(plist_remove(list, NULL));

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);

	plist_free(list);
}

static void plist_remove__equal_val(void **state) {
	const struct PlistParams params = { .equal_val = mock_equal, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));

	expect_ptr(mock_equal, a, V0);
	expect_ptr(mock_equal, b, V0);
	will_return(mock_equal, true);

	assert_ptr_equal(plist_remove(list, V0), V0);

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_remove_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V2));

	expect_ptr_count(mock_free, ptr, V0, 1);

	plist_remove_free(list, V0);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V2);

	expect_ptr_count(mock_free, ptr, V2, 1);

	plist_remove_free(list, V2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	expect_ptr_count(mock_free, ptr, V0, 1);

	plist_remove_free(list, V0);

	plist_remove_free(list, V0);

	plist_free(list);
}

static void plist_remove_free__free(void **state) {
	const struct Plist *list = plist_init();

	const char *val0 = strdup("0");
	const char *val1 = strdup("1");

	assert_true(plist_append(list, val0));
	assert_true(plist_append(list, val1));

	plist_remove_free(list, val1);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), val0);

	plist_remove_free(list, val1);

	plist_free_vals(list);
}

static void plist_remove_free__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));

	expect_ptr(mock_free, ptr, V1);

	plist_remove_free(list, V1);

	// no mock_free

	plist_remove_free(list, NULL);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), NULL);

	plist_free(list);
}

static void plist_remove_at__existing(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_int_equal(plist_size(list), 3);

	assert_ptr_equal(plist_remove_at(list, 0), V0);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V1);
	assert_ptr_equal(plist_at(list, 1), V2);

	assert_ptr_equal(plist_remove_at(list, 1), V2);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V1);

	plist_free(list);
}

static void plist_remove_at__inexistent(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 2);

	assert_nul(plist_remove_at(list, 2));

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V1);


	plist_free(list);
}

static void plist_remove_at__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	assert_int_equal(plist_size(list), 3);

	assert_nul(plist_remove_at(list, 1));

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V2);

	plist_free(list);
}

static void plist_remove_at_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	assert_int_equal(plist_size(list), 3);

	expect_ptr_count(mock_free, ptr, V1, 1);

	plist_remove_at_free(list, 1);

	assert_int_equal(plist_size(list), 2);
	assert_ptr_equal(plist_at(list, 0), V0);
	assert_ptr_equal(plist_at(list, 1), V2);

	expect_ptr_count(mock_free, ptr, V2, 1);

	plist_remove_at_free(list, 1);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	expect_ptr_count(mock_free, ptr, V0, 1);

	plist_remove_at_free(list, 0);

	plist_remove_at_free(list, 0);

	plist_free(list);
}

static void plist_remove_at_free__free(void **state) {
	const struct Plist *list = plist_init();

	const char *val0 = strdup("0");
	const char *val1 = strdup("1");

	assert_true(plist_append(list, val0));
	assert_true(plist_append(list, val1));

	plist_remove_at_free(list, 1);

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), val0);

	plist_remove_free(list, val1);

	plist_free_vals(list);
}

static void plist_remove_at_free__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	expect_ptr(mock_free, ptr, V2);

	plist_remove_at_free(list, 2);

	// no mock_free

	assert_nul(plist_remove_at(list, 1));

	assert_int_equal(plist_size(list), 1);
	assert_ptr_equal(plist_at(list, 0), V0);

	plist_free(list);
}

static void plist_remove_all__many(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all(list), 0);

	assert_int_equal(plist_size(list), 0);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_remove_all(list), 3);

	assert_int_equal(plist_size(list), 0);
	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_all_free__no_free_val(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_remove_all_free(list), 0);

	const char *val0 = strdup("0");
	const char *val1 = strdup("1");
	const char *val2 = strdup("2");

	assert_true(plist_append(list, val0));
	assert_true(plist_append(list, val1));
	assert_true(plist_append(list, val1));
	assert_true(plist_append(list, val2));
	assert_true(plist_append(list, val1));
	assert_true(plist_append(list, val0));
	assert_true(plist_append(list, val2));

	assert_int_equal(plist_remove_all_free(list), 7);

	assert_int_equal(plist_size(list), 0);

	plist_free(list);
}

static void plist_remove_all_free__free_val(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_int_equal(plist_remove_all_free(list), 0);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V0));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(plist_remove_all_free(list), 4);

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_remove_all_free__allow_null_val(void **state) {
	const struct PlistParams params = {
		.free_val = mock_free,
		.allow_null_val = true,
	};
	const struct Plist *list = plist_init_with(params);

	assert_int_equal(plist_remove_all_free(list), 0);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V0));

	expect_ptr(mock_free, ptr, V0);
	expect_ptr(mock_free, ptr, V1);

	assert_int_equal(plist_remove_all_free(list), 5);

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_at(list, 0));

	plist_free(list);
}

static void plist_find__empty_filter(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	const struct PlistFilter filter = { 0 };

	assert_ptr_equal(plist_find(list, filter), V0);

	plist_free(list);
}

static void plist_find__empty_list(void **state) {
	const struct Plist *list = plist_init();

	const struct PlistFilter filter = {
		.val = mock_pred_p,
		.data = D0,
		.val_data = mock_pred_p_p,
	};

	assert_nul(plist_find(list, filter));

	plist_free(list);
}

static void plist_find__val(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	// skip V0
	expect_ptr(mock_pred_p, p, V0);
	will_return(mock_pred_p, false);

	// get V1
	expect_ptr(mock_pred_p, p, V1);
	will_return(mock_pred_p, true);

	const struct PlistFilter filter = { .val = mock_pred_p, };

	assert_ptr_equal(plist_find(list, filter), V1);

	plist_free(list);
}

static void plist_find__val__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	// skip V0
	expect_ptr(mock_pred_p, p, V0);
	will_return(mock_pred_p, false);

	// skip NULL
	expect_ptr(mock_pred_p, p, NULL);
	will_return(mock_pred_p, false);

	// get NULL
	expect_ptr(mock_pred_p, p, NULL);
	will_return(mock_pred_p, true);

	const struct PlistFilter filter = { .val = mock_pred_p, };

	assert_ptr_equal(plist_find(list, filter), NULL);

	plist_free(list);
}

static void plist_find__val_data(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get V1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };

	assert_ptr_equal(plist_find(list, filter), V1);

	plist_free(list);
}

static void plist_find__val_data__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	// skip V0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// skip NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };

	assert_ptr_equal(plist_find(list, filter), NULL);

	plist_free(list);
}

static void plist_it_start__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_it_start(list));

	plist_free(list);
}

static void plist_it_end__empty(void **state) {
	const struct Plist *list = plist_init();

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_it_end(list));

	plist_free(list);
}

static void plist_it__free(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	const struct PlistIt *it = plist_it_start(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);

	plist_free(list);
}

static void plist_it_start__direction_changes(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));

	assert_int_equal(plist_size(list), 4);

	const struct PlistIt *it = plist_it_start(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	it = plist_it_next(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_it_end__direction_changes(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));

	assert_int_equal(plist_size(list), 4);

	const struct PlistIt *it = plist_it_end(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	it = plist_it_prev(it);
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

static void plist_it_start__allow_null_vals(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, NULL));

	const struct PlistIt *it = plist_it_start(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	assert_nul(plist_it_next(it));

	plist_free(list);
}

static void plist_it_end__allow_null_vals(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, NULL));

	const struct PlistIt *it = plist_it_end(list);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, NULL);

	assert_nul(plist_it_prev(it));

	plist_free(list);
}

static void plist_it__cleared(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));

	assert_int_equal(plist_size(list), 2);

	assert_ptr_equal(plist_remove(list, V0), V0);
	assert_ptr_equal(plist_remove(list, V1), V1);

	assert_int_equal(plist_size(list), 0);

	assert_nul(plist_it_start(list));

	assert_nul(plist_it_end(list));

	plist_free(list);
}

static void plist_it_free__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	plist_it_free(it);
}

static void plist_it_next__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_next(it));
}

static void plist_it_prev__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	assert_nul(plist_it_prev(it));
}

static void plist_filter_it_start__empty_filter(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	const struct PlistFilter filter = { 0 };

	const struct PlistIt *it = plist_filter_it_start(list, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V0);

	plist_it_free(it);

	plist_free(list);
}

static void plist_filter_it_end__empty_filter(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	const struct PlistFilter filter = { 0 };

	const struct PlistIt *it = plist_filter_it_end(list, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V2);

	plist_it_free(it);

	plist_free(list);
}

static void plist_filter_it_start__empty_list(void **state) {
	const struct Plist *list = plist_init();

	const struct PlistFilter filter = {
		.val = mock_pred_p,
		.val_data = mock_pred_p_p,
		.data = D0,
	};
	assert_nul(plist_filter_it_start(list, filter));

	plist_free(list);
}

static void plist_filter_it_end__empty_list(void **state) {
	const struct Plist *list = plist_init();

	const struct PlistFilter filter = {
		.val = mock_pred_p,
		.val_data = mock_pred_p_p,
		.data = D0,
	};
	assert_nul(plist_filter_it_end(list, filter));

	plist_free(list);
}

static void plist_filter_it_start__direction_changes(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, V4));

	assert_int_equal(plist_size(list), 5);

	// skip 0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	const struct PlistIt *it = plist_filter_it_start(list, filter);

	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// skip 2
	expect_ptr(mock_pred_p_p, p1, V2);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 3
	expect_ptr(mock_pred_p_p, p1, V3);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// skip 2
	expect_ptr(mock_pred_p_p, p1, V2);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// skip 2
	expect_ptr(mock_pred_p_p, p1, V2);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 3
	expect_ptr(mock_pred_p_p, p1, V3);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// skip 4
	expect_ptr(mock_pred_p_p, p1, V4);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// done
	it = plist_it_next(it);
	assert_nul(it);

	plist_free(list);
}

static void plist_filter_it_start__allow_null_vals(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, NULL));

	// skip NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 1
	expect_ptr(mock_pred_p_p, p1, V1);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	const struct PlistIt *it = plist_filter_it_start(list, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V1);

	// get NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_next(it);
	assert_non_nul(it);
	assert_nul(it->val);

	plist_it_free(it);

	plist_free(list);
}

static void plist_filter_it_end__allow_null_vals(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, NULL));

	// skip NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	// get 3
	expect_ptr(mock_pred_p_p, p1, V3);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	const struct PlistIt *it = plist_filter_it_end(list, filter);
	assert_non_nul(it);
	assert_ptr_equal(it->val, V3);

	// get NULL
	expect_ptr(mock_pred_p_p, p1, NULL);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, true);

	it = plist_it_prev(it);
	assert_non_nul(it);
	assert_nul(it->val);

	plist_it_free(it);

	plist_free(list);
}

static void plist_filter_it_start__none(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));

	assert_int_equal(plist_size(list), 1);

	// skip K0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	assert_nul(plist_filter_it_start(list, filter));

	plist_free(list);
}

static void plist_filter_it_end__none(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));

	assert_int_equal(plist_size(list), 1);

	// skip K0
	expect_ptr(mock_pred_p_p, p1, V0);
	expect_ptr(mock_pred_p_p, p2, D0);
	will_return(mock_pred_p_p, false);

	const struct PlistFilter filter = { .val_data = mock_pred_p_p, .data = D0, };
	assert_nul(plist_filter_it_end(list, filter));

	plist_free(list);
}

static void plist_it_remove__forwards(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V1));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(plist_size(list), 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove__forwards__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(plist_size(list), 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove__backwards(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V1));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_end(list); it; it = plist_it_prev(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(plist_size(list), 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove__backwards__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	size_t iterations = 0;
	for (const struct PlistIt *it = plist_it_end(list); it; it = plist_it_prev(it)) {
		iterations++;
		plist_it_remove(it);
	}

	assert_int_equal(plist_size(list), 0);
	assert_int_equal(iterations, 3);

	plist_free(list);
}

static void plist_it_remove__partial(void **state) {
	const struct PlistIt *it = calloc(1, sizeof(struct PlistIt));

	plist_it_remove(it);
}

static void plist_it_remove_free__forwards(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		if (it->val == V1 || it->val == V3) {
			plist_it_remove_free(it);
		}
	}

	assert_int_equal(plist_size(list), 3);

	plist_free(list);
}

static void plist_it_remove_free__forwards__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		if (it->val == V1 || it->val == V3 || it->val == NULL) {
			plist_it_remove_free(it);
		}
	}

	assert_int_equal(plist_size(list), 2);

	plist_free(list);
}

static void plist_it_remove_free__backwards(void **state) {
	const struct PlistParams params = { .free_val = mock_free, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, V4));

	expect_ptr(mock_free, ptr, V3);
	expect_ptr(mock_free, ptr, V1);

	for (const struct PlistIt *it = plist_it_end(list); it; it = plist_it_prev(it)) {
		if (it->val == V1 || it->val == V3) {
			plist_it_remove_free(it);
		}
	}

	assert_int_equal(plist_size(list), 3);

	plist_free(list);
}

static void plist_it_remove_free__backwards__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.free_val = mock_free,
	};
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V3));
	assert_true(plist_append(list, V4));

	expect_ptr(mock_free, ptr, V1);
	expect_ptr(mock_free, ptr, V3);

	for (const struct PlistIt *it = plist_it_start(list); it; it = plist_it_next(it)) {
		if (it->val == V1 || it->val == V3 || it->val == NULL) {
			plist_it_remove_free(it);
		}
	}

	assert_int_equal(plist_size(list), 2);

	plist_free(list);
}


static void plist_sort__empty(void **state) {
	const struct Plist *actual = plist_init();
	const struct Plist *expected = plist_init();

	plist_sort(actual, mock_less_than);

	assert_int_equal(plist_size(actual), 0);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_sort__one(void **state) {
	const struct Plist *actual = plist_init();

	assert_true(plist_append(actual, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V0));

	plist_sort(actual, mock_less_than);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static bool test_less_than_int(const void* const a, const void* const b) {
	if (a && !b)
		return true;
	if (!a && b)
		return false;
	if (!a && !b)
		return false;

	return *(int*)a < *(int*)b;
}

static void plist_sort__many(void **state) {
	const struct Plist *actual = plist_init();

	const int v[6] = { 0, 1, 2, 3, 4, 5 };

	assert_true(plist_append(actual, &v[2]));
	assert_true(plist_append(actual, &v[0]));
	assert_true(plist_append(actual, &v[3]));
	assert_true(plist_append(actual, &v[5]));
	assert_true(plist_append(actual, &v[1]));
	assert_true(plist_append(actual, &v[4]));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, &v[0]));
	assert_true(plist_append(expected, &v[1]));
	assert_true(plist_append(expected, &v[2]));
	assert_true(plist_append(expected, &v[3]));
	assert_true(plist_append(expected, &v[4]));
	assert_true(plist_append(expected, &v[5]));

	plist_sort(actual, test_less_than_int);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_sort__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true };
	const struct Plist *actual = plist_init_with(params);

	const int v[6] = { 0, 1, 2, 3, 4, 5 };

	assert_true(plist_append(actual, NULL));
	assert_true(plist_append(actual, &v[2]));
	assert_true(plist_append(actual, &v[0]));
	assert_true(plist_append(actual, &v[3]));
	assert_true(plist_append(actual, NULL));
	assert_true(plist_append(actual, &v[5]));
	assert_true(plist_append(actual, &v[1]));
	assert_true(plist_append(actual, &v[4]));
	assert_true(plist_append(actual, NULL));

	const struct Plist *expected = plist_init_with(params);
	assert_true(plist_append(expected, &v[0]));
	assert_true(plist_append(expected, &v[1]));
	assert_true(plist_append(expected, &v[2]));
	assert_true(plist_append(expected, &v[3]));
	assert_true(plist_append(expected, &v[4]));
	assert_true(plist_append(expected, &v[5]));
	assert_true(plist_append(expected, NULL));
	assert_true(plist_append(expected, NULL));
	assert_true(plist_append(expected, NULL));

	plist_sort(actual, test_less_than_int);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_sort__no_less_than(void **state) {
	const struct Plist *actual = plist_init();

	assert_true(plist_append(actual, V1));
	assert_true(plist_append(actual, V0));

	const struct Plist *expected = plist_init();
	assert_true(plist_append(expected, V1));
	assert_true(plist_append(expected, V0));

	plist_sort(actual, NULL);

	assert_plist_equal(actual, expected);

	plist_free(actual);
	plist_free(expected);
}

static void plist_equal__length_different(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_append(a, V0));

	assert_true(plist_append(b, V0));
	assert_true(plist_append(b, V1));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_ok(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_append(a, V0));
	assert_true(plist_append(a, V1));
	assert_true(plist_append(a, V1));

	assert_plist_not_equal(a, NULL);

	assert_true(plist_append(b, V0));
	assert_true(plist_append(b, V1));
	assert_true(plist_append(b, V1));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers_different(void **state) {
	const struct Plist *a = plist_init();
	const struct Plist *b = plist_init();

	assert_true(plist_append(a, V0));
	assert_true(plist_append(a, V1));

	assert_true(plist_append(b, V0));
	assert_true(plist_append(b, V2));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__val_pointers__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };

	const struct Plist *a = plist_init_with(params);

	assert_true(plist_append(a, NULL));
	assert_true(plist_append(a, V0));
	assert_true(plist_append(a, V1));
	assert_true(plist_append(a, NULL));
	assert_true(plist_append(a, V1));
	assert_true(plist_append(a, NULL));

	assert_plist_not_equal(a, NULL);

	const struct Plist *b = plist_init_with(params);

	assert_plist_not_equal(a, b);

	assert_true(plist_append(b, NULL));
	assert_true(plist_append(b, V0));
	assert_true(plist_append(b, V1));
	assert_true(plist_append(b, NULL));
	assert_true(plist_append(b, V1));
	assert_true(plist_append(b, NULL));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val_ok(void **state) {
	const struct PlistParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Plist *a = plist_init_with(params);
	const struct Plist *b = plist_init_with(params);

	assert_true(plist_append(a, V0));
	assert_true(plist_append(a, V1));
	assert_true(plist_append(a, V1));

	assert_true(plist_append(b, V0));
	assert_true(plist_append(b, V1));
	assert_true(plist_append(b, V1));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val_different(void **state) {
	const struct PlistParams params = { .equal_val = (fn_equal)equal_strcmp, };
	const struct Plist *a = plist_init_with(params);
	const struct Plist *b = plist_init_with(params);

	assert_true(plist_append(a, "0"));
	assert_true(plist_append(a, "1"));

	assert_true(plist_append(b, "0"));
	assert_true(plist_append(b, "2"));

	assert_plist_not_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_equal__equal_val__allow_null_val(void **state) {
	const struct PlistParams params = {
		.allow_null_val = true,
		.equal_val = (fn_equal)equal_strcmp,
	};
	const struct Plist *a = plist_init_with(params);
	const struct Plist *b = plist_init_with(params);

	assert_true(plist_append(a, "0"));
	assert_true(plist_append(a, NULL));
	assert_true(plist_append(a, "2"));

	assert_true(plist_append(b, "0"));
	assert_true(plist_append(b, NULL));
	assert_true(plist_append(b, NULL));

	assert_plist_not_equal(a, b);

	assert_nul(plist_replace(b, 2, "2"));

	assert_plist_equal(a, b);

	plist_free(a);
	plist_free(b);
}

static void plist_str__empty(void **state) {
	const struct PlistParams params = { .str_val = mock_str, };
	const struct Plist *list = plist_init_with(params);

	char *str = plist_str(list);
	assert_str_equal(str, "");

	free(str);
	plist_free(list);
}

static void plist_str__pointers(void **state) {
	const struct Plist *list = plist_init();

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, V1));
	assert_true(plist_append(list, V2));

	char *expected = sprintf_alloc(
			"%p\n"
			"%p\n"
			"%p\n",
			V0,
			V1,
			V2
			);

	char *actual = plist_str(list);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	plist_free(list);
}

static void plist_str__pointers__allow_null_val(void **state) {
	const struct PlistParams params = { .allow_null_val = true, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, V0));
	assert_true(plist_append(list, NULL));
	assert_true(plist_append(list, V2));

	char *expected = sprintf_alloc(
			"%p\n"
			"(null)\n"
			"%p\n",
			V0,
			V2
			);

	char *actual = plist_str(list);
	assert_str_equal(actual, expected);

	free(actual);
	free(expected);
	plist_free(list);
}

static void plist_str__str_val(void **state) {
	const struct PlistParams params = { .str_val = (fn_str)starts_with_a_or_null, };
	const struct Plist *list = plist_init_with(params);

	assert_true(plist_append(list, "a"));
	assert_true(plist_append(list, "b"));

	char *str = plist_str(list);
	assert_str_equal(str,
			"a\n"
			"(null)\n"
			);

	free(str);
	plist_free(list);
}

static void plist__null_inputs(void **state) {
	const struct Plist *list = plist_init();
	const struct PlistFilter filter = { 0 };

	assert_int_equal(plist_append_all(NULL, NULL), 0);
	assert_int_equal(plist_append_all(list, NULL), 0);
	assert_int_equal(plist_append_all_clone(NULL, NULL), 0);
	assert_int_equal(plist_append_all_clone(list, NULL), 0);
	assert_nul(plist_remove_at(NULL, 0));
	plist_remove_at_free(NULL, 0);
	assert_nul(plist_clone_deep(NULL));
	assert_nul(plist_clone(NULL));
	plist_free(NULL);
	plist_free_vals(NULL);
	plist_it_free(NULL);
	assert_false(plist_contains(NULL, NULL));
	assert_false(plist_contains(list, NULL));
	assert_false(plist_index_of(NULL, NULL, NULL));
	assert_false(plist_index_of(NULL, list, NULL));
	assert_nul(plist_at(NULL, 0));
	assert_nul(plist_find(NULL, filter));
	assert_nul(plist_it_start(NULL));
	assert_nul(plist_it_end(NULL));
	assert_nul(plist_filter_it_start(NULL, filter));
	assert_nul(plist_filter_it_end(NULL, filter));
	assert_nul(plist_it_next(NULL));
	assert_nul(plist_it_prev(NULL));
	plist_it_remove(NULL);
	plist_it_remove_free(NULL);
	assert_false(plist_append(NULL, NULL));
	assert_false(plist_append(list, NULL));
	assert_false(plist_insert(NULL, 0, NULL));
	assert_false(plist_insert(list, 0, NULL));
	assert_false(plist_prepend(NULL, NULL));
	assert_false(plist_prepend(list, NULL));
	assert_nul(plist_replace(NULL, 0, NULL));
	plist_replace_free(NULL, 0, NULL);
	assert_int_equal(plist_append_many(NULL), 0);
	assert_int_equal(plist_append_many_v(NULL, NULL), 0);
	assert_nul(plist_remove(NULL, NULL));
	assert_nul(plist_remove(list, NULL));
	plist_remove_free(NULL, NULL);
	plist_remove_free(list, NULL);
	assert_int_equal(plist_remove_all(NULL), 0);
	assert_int_equal(plist_remove_all_free(NULL), 0);
	plist_sort(NULL, NULL);
	assert_false(plist_equal(NULL, NULL));
	assert_false(plist_equal(list, NULL));
	assert_nul(plist_str(NULL));
	assert_int_equal(plist_size(NULL), 0);

	plist_free(list);
}

int main(void) {
	const struct CMUnitTest tests[] = {
		TEST(plist_init__defaults),

		TEST(plist_clone__empty),
		TEST(plist_clone__params),
		TEST(plist_clone__many),
		TEST(plist_clone__allow_null_val),
		TEST(plist_clone__alloc_val),

		TEST(plist_clone_deep__clone_val),
		TEST(plist_clone_deep__no_clone_val),
		TEST(plist_clone_deep__allow_null_val),
		TEST(plist_clone_deep__alloc_val___clone_val),

		TEST(plist_free_vals__null_free_val),
		TEST(plist_free_vals__missing_val),
		TEST(plist_free_vals__free_val),

		TEST(plist_insert__mid),
		TEST(plist_insert__mid__allow_null_val),
		TEST(plist_insert__start),
		TEST(plist_insert__end),
		TEST(plist_insert__beyond_end),
		TEST(plist_insert__grow),
		TEST(plist_insert__alloc_val),
		TEST(plist_insert__alloc_val__allow_null_val),
		TEST(plist_insert__alloc_val_returned_null),
		TEST(plist_insert__null),

		TEST(plist_append__new),
		TEST(plist_prepend__new),

		TEST(plist_replace__existing),
		TEST(plist_replace__beyond_end),
		TEST(plist_replace__no_allow_null_val),
		TEST(plist_replace__allow_null_val),
		TEST(plist_replace__alloc_val),
		TEST(plist_replace__alloc_val_returned_null),
		TEST(plist_replace__alloc_val__allow_null_val),

		TEST(plist_replace_free__no_free_val),
		TEST(plist_replace_free__free_val),
		TEST(plist_replace_free__allow_null_val),
		TEST(plist_replace_free__beyond_end),

		TEST(plist_append_all__many),
		TEST(plist_append_all__alloc_val),
		TEST(plist_append_all__alloc_val_returned_null),
		TEST(plist_append_all__to_allow_null_val),
		TEST(plist_append_all__to_no_allow_null_val),

		TEST(plist_append_all_clone__many),
		TEST(plist_append_all_clone__no_clone_val),

		TEST(plist_append_many__many),
		TEST(plist_append_many__no_vals),

		TEST(plist_index_of__empty),
		TEST(plist_index_of__no_list),
		TEST(plist_index_of__allow_null_val),
		TEST(plist_index_of__no_allow_null_val),
		TEST(plist_index_of__no_ptr),
		TEST(plist_index_of__equal_val),

		TEST(plist_at__empty),
		TEST(plist_at__many),
		TEST(plist_at__allow_null_val),

		TEST(plist_contains__no_allow_null_val),
		TEST(plist_contains__allow_null_val),

		TEST(plist_remove__no_allow_null_val),
		TEST(plist_remove__allow_null_val),
		TEST(plist_remove__inexistent),
		TEST(plist_remove__equal_val),

		TEST(plist_remove_free__free_val),
		TEST(plist_remove_free__free),
		TEST(plist_remove_free__allow_null_val),

		TEST(plist_remove_at__existing),
		TEST(plist_remove_at__inexistent),
		TEST(plist_remove_at__allow_null_val),

		TEST(plist_remove_at_free__free_val),
		TEST(plist_remove_at_free__free),
		TEST(plist_remove_at_free__allow_null_val),

		TEST(plist_remove_all__many),

		TEST(plist_remove_all_free__no_free_val),
		TEST(plist_remove_all_free__free_val),
		TEST(plist_remove_all_free__allow_null_val),

		TEST(plist_find__empty_filter),
		TEST(plist_find__empty_list),

		TEST(plist_find__val),
		TEST(plist_find__val__allow_null_val),
		TEST(plist_find__val_data),
		TEST(plist_find__val_data__allow_null_val),

		TEST(plist_it_start__empty),
		TEST(plist_it_end__empty),

		TEST(plist_it__free),

		TEST(plist_it_start__direction_changes),
		TEST(plist_it_end__direction_changes),

		TEST(plist_it_start__allow_null_vals),
		TEST(plist_it_end__allow_null_vals),

		TEST(plist_it__cleared),

		TEST(plist_it_free__partial),

		TEST(plist_it_next__partial),
		TEST(plist_it_prev__partial),

		TEST(plist_filter_it_start__empty_filter),
		TEST(plist_filter_it_end__empty_filter),

		TEST(plist_filter_it_start__empty_list),
		TEST(plist_filter_it_end__empty_list),

		TEST(plist_filter_it_start__direction_changes),

		TEST(plist_filter_it_start__allow_null_vals),
		TEST(plist_filter_it_end__allow_null_vals),

		TEST(plist_filter_it_start__none),
		TEST(plist_filter_it_end__none),

		TEST(plist_it_remove__forwards),
		TEST(plist_it_remove__forwards__allow_null_val),
		TEST(plist_it_remove__backwards),
		TEST(plist_it_remove__backwards__allow_null_val),
		TEST(plist_it_remove__partial),

		TEST(plist_it_remove_free__forwards),
		TEST(plist_it_remove_free__forwards__allow_null_val),
		TEST(plist_it_remove_free__backwards),
		TEST(plist_it_remove_free__backwards__allow_null_val),

		TEST(plist_sort__empty),
		TEST(plist_sort__one),
		TEST(plist_sort__many),
		TEST(plist_sort__allow_null_val),
		TEST(plist_sort__no_less_than),

		TEST(plist_equal__length_different),
		TEST(plist_equal__val_pointers_ok),
		TEST(plist_equal__val_pointers_different),
		TEST(plist_equal__val_pointers__allow_null_val),

		TEST(plist_equal__equal_val_ok),
		TEST(plist_equal__equal_val_different),
		TEST(plist_equal__equal_val__allow_null_val),

		TEST(plist_str__empty),
		TEST(plist_str__pointers),
		TEST(plist_str__pointers__allow_null_val),
		TEST(plist_str__str_val),

		TEST(plist__null_inputs),
	};

	return RUN(tests);
}

