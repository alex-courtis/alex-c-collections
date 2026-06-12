#include <stdbool.h>
#include <stdlib.h>

#include "fn.h"
#include "pset.h"

#include "sset.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' inc/pset.h) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' inc/sset.h) | less

   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' src/pset.c) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' src/sset.c) | less
   */

struct SSet {
	const struct PSet *pset;
};

struct SSetIterState {
	const struct PSetIter *pit;
};

const struct SSet *sset_init(void) {
	const struct SSetParams params = { 0 };
	return sset_init_with(params);
}

const struct SSet *sset_init_with(const struct SSetParams params) {

	const struct PSetParams pset_params = {
		.equal_val = params.case_insensitive ? fn_equal_strcasecmp : fn_equal_strcmp,
		.less_than_val = params.case_insensitive ? fn_less_than_strcasecmp : fn_less_than_strcmp,
		.free_val = (fn_free)free,
		.initial = params.initial,
		.grow = params.grow,
	};
	const struct PSet *pset = pset_init_with(pset_params);

	if (!pset)
		return NULL;

	struct SSet *set = calloc(1, sizeof(struct SSet));
	set->pset = pset;

	return set;
}

const struct SSet *sset_clone(const struct SSet* const from) {
	if (!from)
		return NULL;

	struct SSet *to = calloc(1, sizeof(struct SSet));
	to->pset = pset_clone(from->pset);

	return to;
}

void sset_free(const struct SSet* const set) {
	if (!set)
		return;

	pset_free(set->pset);

	free((void*)set);
}

void sset_iter_free(const struct SSetIter* const iter) {
	if (!iter)
		return;

	if (iter->st)
		pset_iter_free(iter->st->pit);

	free((void*)iter->st);
	free((void*)iter);
}

bool sset_contains(const struct SSet* const set, const char* const val) {
	return pset_contains(set->pset, val);
}

const struct SSetIter *sset_iter(const struct SSet* const set) {
	return sset_filter_iter(set, NULL, NULL);
}

const struct SSetIter *sset_filter_iter(const struct SSet* const set, fn_test test_val, const void* const data) {
	if (!set)
		return NULL;

	const struct PSetIter *pit = pset_filter_iter(set->pset, test_val, data);

	if (!pit)
		return NULL;

	struct SSetIter *it = calloc(1, sizeof(struct SSetIter));
	it->st = calloc(1, sizeof(struct SSetIterState));

	it->st->pit = pit;
	it->val = pit->val;

	return it;
}

const struct SSetIter *sset_iter_next(const struct SSetIter* const iter) {
	if (!iter)
		return NULL;

	struct SSetIter *it = (struct SSetIter*)iter;

	if (!it->st || !it->st->pit) {
		free(it);
		return NULL;
	}

	it->st->pit = pset_iter_next(iter->st->pit);

	if (it->st->pit) {
		it->val = it->st->pit->val;
	} else {
		sset_iter_free(it);
		it = NULL;
	}

	return it;
}

bool sset_add(const struct SSet* const set, const char* const val) {
	return set ? pset_add(set->pset, val) : false;
}

bool sset_remove(const struct SSet* const set, const char* const val) {
	return set ? pset_remove(set->pset, val) : false;
}

void sset_sort(const struct SSet* const set) {
	if (set)
		pset_sort(set->pset);
}

bool sset_equal(const struct SSet* const a, const struct SSet* const b) {
	return a && b ? pset_equal(a->pset, b->pset) : false;
}

struct SList *sset_slist(const struct SSet* const set) {
	return set ? pset_slist(set->pset) : NULL;
}

char *sset_str(const struct SSet* const set) {
	return set ? pset_str(set->pset, fn_str_or_null) : NULL;
}

size_t sset_size(const struct SSet* const set) {
	return set ? pset_size(set->pset) : 0;
}
