#include <inttypes.h>
#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "ptable.h"
#include "str.h"

#include "itable.h"

/*
   diff -u \
   <(sed -e ' s/itable/xtable/g ; s/ITable/XTable/g ' src/itable.c) \
   <(sed -e 's/stable/xtable/g ; s/STable/XTable/g' src/stable.c)
   */

struct ITable {
	const struct PTable *ptab;
};

struct ITableIter {
	const struct PTableIter *pit;
};

static bool fn_equal_key(const void* const a, const void* const b) {
	if (a == b)
		return true;

	if (!a || !b)
		return false;

	return *(uint64_t*)a == *(uint64_t*)b;
}

static const void *fn_alloc_key(const void* const val) {
	uint64_t *out = calloc(1, sizeof(uint64_t));
	*out = *(uint64_t*)val;

	return out;
}

static char *fn_str_key(const void* const val) {
	return sprintf_alloc("%"PRIu64, *(uint64_t*)val);
}

const struct ITable *itable_init(void) {
	return itable_init_with(10, 10);
}

const struct ITable *itable_init_with(const size_t initial, const size_t grow) {
	const struct PTable *ptab = ptable_init_with(
			fn_equal_key,
			fn_alloc_key,
			(fn_free)free,
			fn_str_key,
			initial, grow);

	if (!ptab)
		return NULL;

	struct ITable *tab = calloc(1, sizeof(struct ITable));
	tab->ptab = ptab;

	return tab;
}

void itable_free(const void* const cvtab) {
	if (!cvtab)
		return;

	struct ITable *tab = (struct ITable*)cvtab;

	ptable_free(tab->ptab);

	free(tab);
}

void itable_free_vals(const struct ITable* const tab, fn_free free_val) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab, free_val);

	free((void*)tab);
}

void itable_iter_free(const struct ITableIter* const iter) {
	if (!iter)
		return;

	ptable_iter_free(iter->pit);

	free((void*)iter);
}

const void *itable_get(const struct ITable* const tab, const uint64_t key) {
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

uint64_t itable_iter_key(const struct ITableIter* const iter) {
	if (!iter)
		return 0;

	const void *key = ptable_iter_key(iter->pit);

	if (key)
		return *(uint64_t*)key;
	else
		return 0;
}

const void *itable_iter_val(const struct ITableIter* const iter) {
	return iter ? ptable_iter_val(iter->pit) : NULL;
}

const void *itable_put(const struct ITable* const tab, const uint64_t key, const void* const val) {
	return tab ? ptable_put(tab->ptab, &key, val) : NULL;
}

const void *itable_remove(const struct ITable* const tab, const uint64_t key) {
	return tab ? ptable_remove(tab->ptab, &key) : NULL;
}

bool itable_equal(const struct ITable* const a, const struct ITable* const b, fn_equal equal) {
	return a && b ? ptable_equal(a->ptab, b->ptab, equal) : false;
}

struct SList *itable_vals_slist(const struct ITable* const tab) {
	return tab ? ptable_vals_slist(tab->ptab) : NULL;
}

char *itable_str(const struct ITable* const tab, fn_str str) {
	return tab ? ptable_str(tab->ptab, str) : NULL;
}

size_t itable_size(const struct ITable* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
