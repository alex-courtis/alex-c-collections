#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "sset.h"

#include "smaps.h"

struct SMapS {
	const struct SMapSParams params;
	const struct PPmap *ppmap;
};

struct SMapSItState {
	const struct PPmapIt *pit;
};

static const struct SMapSIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapSIt *it = calloc(1, sizeof(struct SMapSIt));
	it->st = calloc(1, sizeof(struct SMapSItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = pit->val;

	return it;
}

const struct SMapS *smaps_init(void) {
	const struct SMapSParams params = { 0 };
	return smaps_init_with(params);
}

const struct SMapS *smaps_init_with(const struct SMapSParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = params.case_insensitive_val ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = (fn_clone)clone_strdup,
		.free_key = (fn_free)free,
		.free_val = (fn_free)free,
		.str_key = (fn_str)str_or_null,
		.str_val = (fn_str)str_or_null,
		.initial = params.initial,
		.allow_null_val = params.allow_null_val,
		.grow = params.grow,
	};

	struct SMapS *map = calloc(1, sizeof(struct SMapS));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapSParams));

	return map;
}

const struct SMapS *smaps_clone(const struct SMapS* const from) {
	if (!from)
		return NULL;

	struct SMapS *to = calloc(1, sizeof(struct SMapS));
	to->ppmap = ppmap_clone(from->ppmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SMapSParams));

	return to;
}

void smaps_free(const struct SMapS* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void smaps_it_free(const struct SMapSIt* const it) {
	if (!it)
		return;

	if (it->st) {
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const char *smaps_get(const struct SMapS* const map, const char* const key) {
	return map ? ppmap_get(map->ppmap, key) : NULL;
}

bool smaps_contains_key(const struct SMapS* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool smaps_contains_val(const struct SMapS* const map, const char* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

struct SMapSPair smaps_match(const struct SMapS* const map, fn_3pred_str_str match, const void* const data) {
	struct SMapSPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match(map->ppmap, (fn_3pred)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SMapSPair smaps_match_key(const struct SMapS* const map, fn_2pred_str match, const void* const data) {
	struct SMapSPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match_key(map->ppmap, (fn_2pred)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

struct SMapSPair smaps_match_val(const struct SMapS* const map, fn_2pred_str match, const void* const data) {
	struct SMapSPair res = { 0 };

	if (!map)
		return res;

	struct PPmapPair pres = ppmap_match_val(map->ppmap, (fn_2pred)match, data);

	res.key = pres.key;
	res.val = pres.val;

	return res;
}

const struct SMapSIt *smaps_it(const struct SMapS* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SMapSIt *smaps_match_it(const struct SMapS* const map, fn_3pred_str_str match, const void* const data) {
	return map ? it_init(ppmap_match_it(map->ppmap, (fn_3pred)match, data)) : NULL;
}

const struct SMapSIt *smaps_match_key_it(const struct SMapS* const map, fn_2pred_str match, const void* const data) {
	return map ? it_init(ppmap_match_key_it(map->ppmap, (fn_2pred)match, data)) : NULL;
}

const struct SMapSIt *smaps_match_val_it(const struct SMapS* const map, fn_2pred_str match, const void* const data) {
	return map ? it_init(ppmap_match_val_it(map->ppmap, (fn_2pred)match, data)) : NULL;
}

const struct SMapSIt *smaps_it_next(const struct SMapSIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smaps_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapSIt *it_m = (struct SMapSIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		smaps_it_free(it);
		return NULL;
	}
}

bool smaps_put(const struct SMapS* const map, const char* const key, const char* const val) {
	return map ? ppmap_put_free(map->ppmap, key, val): false;
}

bool smaps_put_if_absent(const struct SMapS* const map, const char* const key, const char* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, val) : false;
}

size_t smaps_put_all(const struct SMapS* const map, const struct SMapS* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

bool smaps_remove(const struct SMapS* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t smaps_remove_all(const struct SMapS* const map, const struct SMapS* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : false;
}

bool smaps_equal(const struct SMapS* const a, const struct SMapS* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *smaps_keys_pslist(const struct SMapS* const map) {
	return map ? ppmap_keys_pslist(map->ppmap) : NULL;
}

const struct SSet *smaps_keys_sset(const struct SMapS* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapSIt *it = smaps_it(map); it; it = smaps_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

struct Pslist *smaps_vals_pslist(const struct SMapS* const map) {
	return map ? ppmap_vals_pslist(map->ppmap) : NULL;
}

const struct SSet *smaps_vals_sset(const struct SMapS* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive_val,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapSIt *it = smaps_it(map); it; it = smaps_it_next(it)) {
		sset_add(set, it->val);
	}

	return set;
}

char *smaps_str(const struct SMapS* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t smaps_size(const struct SMapS* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
