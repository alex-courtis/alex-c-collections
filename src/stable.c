#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"
#include "str.h"

#include "stable.h"

/*
   diff -u \
   <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) \
   <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c)
   */

struct STable {
	const struct PTable *ptab;
};

struct STableIter {
	const struct PTableIter *pit;
};

static char *fn_str_str(const void* const val) {
	return sprintf_alloc("%s", val ? (char*)val : "(null)");
}

const struct STable *stable_init(void) {
	return stable_init_with(10, 10, false);
}

const struct STable *stable_init_with(const size_t initial, const size_t grow, const bool case_insensitive) {
	const struct PTable *ptab = ptable_init_with(
			case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
			(fn_alloc)strdup,
			(fn_free)free,
			fn_str_str,
			initial, grow);

	if (!ptab)
		return NULL;

	struct STable *tab = calloc(1, sizeof(struct STable));
	tab->ptab = ptab;

	return tab;
}

void stable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct STable *tab = (struct STable*)cvtab;

	ptable_free(tab->ptab);

	free(tab);
}

void stable_free_vals(const struct STable* const tab, fn_free free_val) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab, free_val);

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
	return iter ? ptable_iter_key(iter->pit) : NULL;
}

const void *stable_iter_val(const struct STableIter* const iter) {
	return iter ? ptable_iter_val(iter->pit) : NULL;
}

const void *stable_put(const struct STable* const tab, const char* const key, const void* const val) {
	return tab ? ptable_put(tab->ptab, key, val) : NULL;
}

const void *stable_remove(const struct STable* const tab, const char* const key) {
	return tab ? ptable_remove(tab->ptab, key) : NULL;
}

bool stable_equal(const struct STable* const a, const struct STable* const b, fn_equal equal) {
	return a && b ? ptable_equal(a->ptab, b->ptab, equal) : false;
}

struct SList *stable_keys_slist(const struct STable* const tab) {
	return tab ? ptable_keys_slist(tab->ptab) : NULL;
}

struct SList *stable_vals_slist(const struct STable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *stable_str(const struct STable* const tab, fn_str str) {
	return tab ? ptable_str(tab->ptab, str) : NULL;
}

size_t stable_size(const struct STable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}

size_t stable_capacity(const struct STable* const tab) {
	return tab ? ptable_capacity(tab->ptab) : 0;
}
