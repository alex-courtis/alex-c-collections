#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"

#include "stable.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/ptable/xtable/g ; s/PTable/XTable/g ' inc/ptable.h) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' inc/stable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/stable/xtable/g ; s/STable/XTable/g ' inc/stable.h) <(sed -e 's/itable/xtable/g ; s/ITable/XTable/g' inc/itable.h) | less

   diff --color=always -U 10000 <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c) | less
   */

struct STable {
	const struct PTable *ptab;
};

struct STableIter {
	const struct PTableIter *pit;
};

const struct STable *stable_init(void) {
	const struct STableParams params = { 0 };
	return stable_init_with(params);
}

const struct STable *stable_init_with(const struct STableParams params) {
	const struct PTableParams ptable_params = {
		.equal_key = params.case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_alloc)strdup,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.str_key = fn_str_or_null,
		.str_val = params.str_val,
		.clone_val = params.clone_val,
		.initial = params.initial,
		.grow = params.grow,
	};
	const struct PTable *ptab = ptable_init_with(ptable_params);

	if (!ptab)
		return NULL;

	struct STable *tab = calloc(1, sizeof(struct STable));
	tab->ptab = ptab;

	return tab;
}

const struct STable *stable_clone(const struct STable* const from) {
	if (!from)
		return NULL;

	struct STable *to = calloc(1, sizeof(struct STable));
	to->ptab = ptable_clone(from->ptab);

	return to;
}

void stable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct STable *tab = (struct STable*)cvtab;

	ptable_free(tab->ptab);

	free(tab);
}

void stable_free_vals(const struct STable* const tab) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab);

	free((void*)tab);
}

void stable_iter_free(const struct STableIter* const iter) {
	if (!iter)
		return;

	ptable_iter_free(iter->pit);

	free((void*)iter);
}

const void *stable_get(const struct STable* const tab, const char* const key) {
	return tab ? ptable_get(tab->ptab, key) : NULL;
}

const struct STableIter *stable_iter(const struct STable* const tab) {
	if (!tab)
		return NULL;

	const struct PTableIter *pit = ptable_iter(tab->ptab);

	if (!pit)
		return NULL;

	struct STableIter *it = calloc(1, sizeof(struct STableIter));
	it->pit = pit;

	return it;
}

const struct STableIter *stable_filter_iter(const struct STable* const tab, fn_test_str test_key, fn_test test_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct PTableIter *pit = ptable_filter_iter(tab->ptab, (fn_test)test_key, test_val, data);

	if (!pit)
		return NULL;

	struct STableIter *it = calloc(1, sizeof(struct STableIter));
	it->pit = pit;

	return it;
}

const struct STableIter *stable_iter_next(const struct STableIter* const iter) {
	if (!iter)
		return NULL;

	struct STableIter *it = (struct STableIter*)iter;

	it->pit = ptable_iter_next(iter->pit);

	if (!it->pit) {
		free(it);
		it = NULL;
	}

	return it;
}

const char *stable_iter_key(const struct STableIter* const iter) {
	return iter ? iter->pit->key : NULL;
}

const void *stable_iter_val(const struct STableIter* const iter) {
	return iter ? iter->pit->val : NULL;
}

const void *stable_put(const struct STable* const tab, const char* const key, const void* const val) {
	return tab ? ptable_put(tab->ptab, key, val) : NULL;
}

const void *stable_remove(const struct STable* const tab, const char* const key) {
	return tab ? ptable_remove(tab->ptab, key) : NULL;
}

bool stable_equal(const struct STable* const a, const struct STable* const b) {
	return a && b ? ptable_equal(a->ptab, b->ptab) : false;
}

struct SList *stable_keys_slist(const struct STable* const tab) {
	return tab ? ptable_keys_slist(tab->ptab) : NULL;
}

struct SList *stable_vals_slist(const struct STable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *stable_str(const struct STable* const tab) {
	return tab ? ptable_str(tab->ptab) : NULL;
}

size_t stable_size(const struct STable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
