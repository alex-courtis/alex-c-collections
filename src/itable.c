#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "ptable.h"
#include "str.h"

#include "itable.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/ptable/xtable/g ; s/PTable/XTable/g ' inc/ptable.h) <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' inc/itable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' inc/itable.h) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' inc/stable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less
   */

struct ITable {
	const struct PTable *ptab;
};

struct ITableIter {
	const struct PTableIter *pit;
	fn_test_size_t test_key;
	fn_test test_val;
	const void *data;
};

static bool fn_equal_key(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return *(size_t*)a == *(size_t*)b;
}

static const void *fn_alloc_key(const void* const val) {
	size_t *out = calloc(1, sizeof(size_t));
	*out = *(size_t*)val;

	return out;
}

static char *fn_str_key(const void* const val) {
	return sprintf_alloc("%zu", *(size_t*)val);
}

const struct ITable *itable_init(void) {
	const struct ITableParams params = { 0 };
	return itable_init_with(params);
}

const struct ITable *itable_init_with(const struct ITableParams params) {
	const struct PTableParams ptable_params = {
		.equal_key = fn_equal_key,
		.equal_val = params.equal_val,
		.alloc_key = fn_alloc_key,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.str_key = fn_str_key,
		.str_val = params.str_val,
		.clone_val = params.clone_val,
		.initial = params.initial,
		.grow = params.grow,
	};
	const struct PTable *ptab = ptable_init_with(ptable_params);

	if (!ptab)
		return NULL;

	struct ITable *tab = calloc(1, sizeof(struct ITable));
	tab->ptab = ptab;

	return tab;
}

const struct ITable *itable_clone(const struct ITable* const from) {
	if (!from)
		return NULL;

	struct ITable *to = calloc(1, sizeof(struct ITable));
	to->ptab = ptable_clone(from->ptab);

	return to;
}

void itable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct ITable *tab = (struct ITable*)cvtab;

	ptable_free(tab->ptab);

	free(tab);
}

void itable_free_vals(const struct ITable* const tab) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab);

	free((void*)tab);
}

void itable_iter_free(const struct ITableIter* const iter) {
	if (!iter)
		return;

	ptable_iter_free(iter->pit);

	free((void*)iter);
}

const void *itable_get(const struct ITable* const tab, const size_t key) {
	return tab ? ptable_get(tab->ptab, &key) : NULL;
}

const struct ITableIter *itable_iter(const struct ITable* const tab) {
	if (!tab)
		return NULL;

	const struct PTableIter *pit = ptable_iter(tab->ptab);

	if (!pit)
		return NULL;

	struct ITableIter *it = calloc(1, sizeof(struct ITableIter));
	it->pit = pit;

	return it;
}

static bool fn_test_key_wrapper(const void* const val, const void* const data) {
	const struct ITableIter * const it = data;
	return it->test_key(*(size_t*)val, it->data);
}

static bool fn_test_val_wrapper(const void* const val, const void* const data) {
	const struct ITableIter * const it = data;
	return it->test_val(val, it->data);
}

const struct ITableIter *itable_filter_iter(const struct ITable* const tab, fn_test_size_t test_key, fn_test test_val, const void* const data) {
	if (!tab)
		return NULL;

	struct ITableIter *it = calloc(1, sizeof(struct ITableIter));
	it->test_key = test_key;
	it->test_val = test_val;
	it->data = data;
	it->pit = ptable_filter_iter(tab->ptab, fn_test_key_wrapper, fn_test_val_wrapper, it);

	if (!it->pit) {
		itable_iter_free(it);
		it = NULL;
	}

	return it;
}

const struct ITableIter *itable_iter_next(const struct ITableIter* const iter) {
	if (!iter)
		return NULL;

	struct ITableIter *it = (struct ITableIter*)iter;

	it->pit = ptable_iter_next(iter->pit);

	if (!it->pit) {
		free(it);
		it = NULL;
	}

	return it;
}

size_t itable_iter_key(const struct ITableIter* const iter) {
	if (!iter)
		return 0;

	const void *key = ptable_iter_key(iter->pit);

	if (key)
		return *(size_t*)key;
	else
		return 0;
}

const void *itable_iter_val(const struct ITableIter* const iter) {
	return iter ? ptable_iter_val(iter->pit) : NULL;
}

const void *itable_put(const struct ITable* const tab, const size_t key, const void* const val) {
	return tab ? ptable_put(tab->ptab, &key, val) : NULL;
}

const void *itable_remove(const struct ITable* const tab, const size_t key) {
	return tab ? ptable_remove(tab->ptab, &key) : NULL;
}

bool itable_equal(const struct ITable* const a, const struct ITable* const b) {
	return a && b ? ptable_equal(a->ptab, b->ptab) : false;
}

struct SList *itable_vals_slist(const struct ITable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *itable_str(const struct ITable* const tab) {
	return tab ? ptable_str(tab->ptab) : NULL;
}

size_t itable_size(const struct ITable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
