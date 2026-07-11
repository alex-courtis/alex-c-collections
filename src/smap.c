#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "sset.h"

#include "smap.h"

struct SMap {
	const struct SMapParams params;
	const struct PPmap *ppmap;
};

struct SMapItState {
	const struct PPmapIt *pit;
};

static const struct SMap *clone(const struct SMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct SMap *to = calloc(1, sizeof(struct SMap));

	to->ppmap = deep ? ppmap_clone_deep(from->ppmap) : ppmap_clone(from->ppmap) ;

	memcpy((void*)&to->params, &from->params, sizeof(struct SMapParams));

	return to;
}

static const struct SMapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapIt *it = calloc(1, sizeof(struct SMapIt));
	it->st = calloc(1, sizeof(struct SMapItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMap *smap_init(void) {
	const struct SMapParams params = { 0 };
	return smap_init_with(params);
}

const struct SMap *smap_init_with(const struct SMapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = (fn_str)str_or_null,
		.str_val = params.str_val,
		.allow_null_val = params.allow_null_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMap *map =  calloc(1, sizeof(struct SMap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapParams));

	return map;
}

const struct SMap *smap_clone(const struct SMap* const from) {
	return clone(from, false);
}

const struct SMap *smap_clone_deep(const struct SMap* const from) {
	return clone(from, true);
}

void smap_free(const struct SMap* const map) {
	if (!map)
		return;

	ppmap_free(map->ppmap);

	free((void*)map);
}

void smap_free_vals(const struct SMap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void smap_it_free(const struct SMapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *smap_get(const struct SMap* const map, const char* const key) {
	return map ? ppmap_get(map->ppmap, key) : NULL;
}

bool smap_contains_key(const struct SMap* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool smap_contains_val(const struct SMap* const map, const void* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

struct SMapPair smap_match(const struct SMap* const map, fn_3pred_str_ptr match, const void* const data) {
	struct SMapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match(map->ppmap, (fn_3pred)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SMapPair smap_match_key(const struct SMap* const map, fn_2pred_str match, const void* const data) {
	struct SMapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match_key(map->ppmap, (fn_2pred)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SMapPair smap_match_val(const struct SMap* const map, fn_2pred match, const void* const data) {
	struct SMapPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match_val(map->ppmap, match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SMapIt *smap_it(const struct SMap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SMapIt *smap_match_it(const struct SMap* const map, fn_3pred_str_ptr match, const void* const data) {
	return map ? it_init(ppmap_match_it(map->ppmap, (fn_3pred)match, data)) : NULL;
}

const struct SMapIt *smap_match_key_it(const struct SMap* const map, fn_2pred_str match, const void* const data) {
	return map ? it_init(ppmap_match_key_it(map->ppmap, (fn_2pred)match, data)) : NULL;
}

const struct SMapIt *smap_match_val_it(const struct SMap* const map, fn_2pred match, const void* const data) {
	return map ? it_init(ppmap_match_val_it(map->ppmap, match, data)) : NULL;
}

const struct SMapIt *smap_it_next(const struct SMapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapIt *it_m = (struct SMapIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		smap_it_free(it);
		return NULL;
	}
}

const void *smap_put(const struct SMap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put(map->ppmap, key, val) : NULL;
}

const void *smap_put_if_absent(const struct SMap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, val) : NULL;
}

bool smap_put_free(const struct SMap* const map, const char* const key, const void* const val) {
	return map ? ppmap_put_free(map->ppmap, key, val) : false;
}

const void *smap_remove(const struct SMap* const map, const char* const key) {
	return map ? ppmap_remove(map->ppmap, key) : NULL;
}

bool smap_remove_free(const struct SMap* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t smap_remove_all(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_remove_all(map->ppmap, from->ppmap) : 0;
}

size_t smap_remove_all_free(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : 0;
}

size_t smap_put_all(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_put_all(map->ppmap, from->ppmap) : 0;
}

size_t smap_put_all_free(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

size_t smap_put_all_clone(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_put_all_clone(map->ppmap, from->ppmap) : 0;
}

size_t smap_put_all_clone_free(const struct SMap* const map, const struct SMap* const from) {
	return map && from ? ppmap_put_all_clone_free(map->ppmap, from->ppmap) : 0;
}

bool smap_equal(const struct SMap* const a, const struct SMap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *smap_keys_pslist(const struct SMap* const map) {
	return map ? ppmap_keys_pslist(map->ppmap) : NULL;
}

const struct SSet *smap_keys_sset(const struct SMap* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapIt *it = smap_it(map); it; it = smap_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

struct Pslist *smap_vals_pslist(const struct SMap* const map) {
	return map ? ppmap_vals_pslist(map->ppmap) : NULL;
}

struct Pslist *smap_vals_pslist_clone(const struct SMap* const map) {
	return map ? ppmap_vals_pslist_clone(map->ppmap) : NULL;
}

const struct PSet *smap_vals_pset(const struct SMap* const map) {
	return map ? ppmap_vals_pset(map->ppmap) : NULL;
}

const struct PSet *smap_vals_pset_clone(const struct SMap* const map) {
	return map ? ppmap_vals_pset_clone(map->ppmap) : NULL;
}

char *smap_str(const struct SMap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t smap_size(const struct SMap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
