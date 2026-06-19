#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ptable.h"

#include "smap.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/ptable/xtable/g ; s/PTable/XTable/g ' inc/ptable.h) <(sed -e 's/smap/xtable/g ; s/SMap/XTable/g' inc/smap.h) | less

   diff --color=always -U 10000 <(sed -e ' s/smap/xtable/g ; s/SMap/XTable/g ' inc/smap.h) <(sed -e 's/imap/xtable/g ; s/IMap/XTable/g' inc/imap.h) | less

   diff --color=always -U 10000 <(sed -e ' s/imap/xtable/g ; s/IMap/XTable/g ' src/imap.c) <(sed -e 's/smap/xtable/g ; s/SMap/XTable/g' src/smap.c) | less

   diff --color=always -U 10000 <(sed -e ' s/ssmap/xtable/g ; s/SSMap/XTable/g ' src/ssmap.c) <(sed -e 's/smap/xtable/g ; s/SMap/XTable/g' src/smap.c) | less
   */

struct SMap {
	const struct SMapParams params;
	const struct PTable *ptab;
};

struct SMapIterState {
	const struct PTableIter *pit;
};

const struct SMap *smap_init(void) {
	const struct SMapParams params = { 0 };
	return smap_init_with(params);
}

static const struct SMap *clone(const struct SMap* const from, bool deep) {
	if (!from)
		return NULL;

	const struct PTable *ptab;

	if (deep) {
		ptab = ptable_clone_deep(from->ptab);
	} else {
		ptab = ptable_clone_shallow(from->ptab);
	}

	if (ptab) {
		struct SMap *to = calloc(1, sizeof(struct SMap));
		to->ptab = ptab;
		memcpy((void*)&to->params, &from->params, sizeof(struct SMapParams));

		return to;
	} else {
		return NULL;
	}
}

const struct SMap *smap_init_with(const struct SMapParams params) {
	const struct PTableParams ptable_params = {
		.equal_key = params.case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_alloc)strdup,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.str_key = fn_str_or_null,
		.str_val = params.str_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMap *tab =  calloc(1, sizeof(struct SMap));
	tab->ptab = ptable_init_with(ptable_params);;
	memcpy((void*)&tab->params, &params, sizeof(struct SMapParams));

	return tab;
}

const struct SMap *smap_clone_shallow(const struct SMap* const from) {
	return clone(from, false);
}

const struct SMap *smap_clone_deep(const struct SMap* const from) {
	return clone(from, true);
}

void smap_free(const struct SMap* const tab) {
	if (!tab)
		return;

	ptable_free(tab->ptab);

	free((void*)tab);
}

void smap_free_vals(const struct SMap* const tab) {
	if (!tab)
		return;

	ptable_free_vals(tab->ptab);

	free((void*)tab);
}

void smap_iter_free(const struct SMapIter* const iter) {
	if (!iter)
		return;

	if (iter->st) {
		ptable_iter_free(iter->st->pit);
	}

	free(iter->st);
	free((void*)iter);
}

const void *smap_get(const struct SMap* const tab, const char* const key) {
	return tab ? ptable_get(tab->ptab, key) : NULL;
}

bool smap_contains_key(const struct SMap* const tab, const char* const key) {
	return tab ? ptable_contains_key(tab->ptab, key) : false;
}

const struct SMapIter *smap_iter(const struct SMap* const tab) {
	return smap_filter_iter(tab, NULL, NULL, NULL);
}

const struct SMapIter *smap_filter_iter(const struct SMap* const tab, fn_equal equal_key, fn_equal equal_val, const void* const data) {
	if (!tab)
		return NULL;

	const struct PTableIter *pit = ptable_filter_iter(tab->ptab, equal_key, equal_val, data);

	if (!pit)
		return NULL;

	struct SMapIter *it = calloc(1, sizeof(struct SMapIter));
	it->st = calloc(1, sizeof(struct SMapIterState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMapIter *smap_iter_next(const struct SMapIter* const citer) {
	if (!citer)
		return NULL;

	struct SMapIter *iter = (struct SMapIter*)citer;

	if (!iter->st) {
		smap_iter_free(iter);
		return NULL;
	}

	iter->st->pit = ptable_iter_next(citer->st->pit);

	if (iter->st->pit) {
		iter->key = iter->st->pit->key;
		iter->val = iter->st->pit->val;
		return iter;
	} else {
		smap_iter_free(iter);
		return NULL;
	}
}

const void *smap_put(const struct SMap* const tab, const char* const key, const void* const val) {
	return tab ? ptable_put(tab->ptab, key, val) : NULL;
}

const void *smap_put_if_absent(const struct SMap* const tab, const char* const key, const void* const val) {
	return tab ? ptable_put_if_absent(tab->ptab, key, val) : NULL;
}

bool smap_put_free(const struct SMap* const tab, const  char* const key, const void* const val) {
	return tab ? ptable_put_free(tab->ptab, key, val) : false;
}

const void *smap_remove(const struct SMap* const tab, const char* const key) {
	return tab ? ptable_remove(tab->ptab, key) : NULL;
}

bool smap_remove_free(const struct SMap* const tab, const char* const key) {
	return tab ? ptable_remove_free(tab->ptab, key) : false;
}

bool smap_equal(const struct SMap* const a, const struct SMap* const b) {
	return a && b ? ptable_equal(a->ptab, b->ptab) : false;
}

struct SList *smap_keys_slist_deep(const struct SMap* const tab) {
	return tab ? ptable_keys_slist_deep(tab->ptab) : NULL;
}

struct SList *smap_vals_slist_shallow(const struct SMap* const tab) {
	return tab ? ptable_vals_slist_shallow(tab->ptab) : NULL;
}

struct SList *smap_vals_slist_deep(const struct SMap* const tab) {
	return tab ? ptable_vals_slist_deep(tab->ptab) : NULL;
}

char *smap_str(const struct SMap* const tab) {
	return tab ? ptable_str(tab->ptab) : NULL;
}

size_t smap_size(const struct SMap* const tab) {
	return tab ? ptable_size(tab->ptab) : 0;
}
