#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>

#include "fn.h"
#include "ppmap.h"
#include "sset.h"

#include "smapi.h"

struct SMapI {
	const struct SMapIParams params;
	const struct PPmap *ppmap;
};

struct SMapIMatchData {
	fn_3pred_str_szt match_key_val;
	fn_2pred_str match_key;
	fn_2pred_szt match_val;
	const void *data;
};

struct SMapIItState {
	const struct PPmapIt *pit;
	const struct SMapIMatchData *match_data;
};

static bool match_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct SMapIMatchData* const matcher = data;
	return matcher->match_key_val(key, *(size_t*)val, matcher->data);
}

static bool match_key_wrapper(const void* const key, const void* const data) {
	const struct SMapIMatchData* const matcher = data;
	return matcher->match_key(key, matcher->data);
}

static bool match_val_wrapper(const void* const val, const void* const data) {
	const struct SMapIMatchData* const matcher = data;
	return matcher->match_val(*(size_t*)val, matcher->data);
}

static struct SMapIIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct SMapIIt *it = calloc(1, sizeof(struct SMapIIt));
	it->st = calloc(1, sizeof(struct SMapIItState));

	it->st->pit = pit;
	it->key = pit->key;
	it->val = *(size_t*)pit->val;

	return it;
}

const struct SMapI *smapi_init(void) {
	const struct SMapIParams params = { 0 };
	return smapi_init_with(params);
}

const struct SMapI *smapi_init_with(const struct SMapIParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = params.case_insensitive_key ? (fn_equal)equal_strcasecmp : (fn_equal)equal_strcmp,
		.equal_val = (fn_equal)equal_stp,
		.alloc_key = (fn_clone)clone_strdup,
		.alloc_val = (fn_clone)clone_size_t_ptr,
		.free_key = (fn_free)free,
		.free_val = (fn_free)free,
		.str_key = (fn_str)str_or_null,
		.str_val = (fn_str)str_size_t_ptr,
		.allow_null_val = false,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct SMapI *map =  calloc(1, sizeof(struct SMapI));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct SMapIParams));

	return map;
}

const struct SMapI *smapi_clone(const struct SMapI* const from) {
	if (!from)
		return NULL;

	struct SMapI *to = calloc(1, sizeof(struct SMapI));
	to->ppmap = ppmap_clone(from->ppmap);
	memcpy((void*)&to->params, &from->params, sizeof(struct SMapIParams));

	return to;
}

void smapi_free(const struct SMapI* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void smapi_it_free(const struct SMapIIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->match_data);
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

size_t smapi_get(const struct SMapI* const map, const char* const key) {
	if (!map)
		return 0;

	const size_t *val = ppmap_get(map->ppmap, key);

	if (val) {
		return *val;
	} else {
		return 0;
	}
}

bool smapi_get_ptr(size_t* np, const struct SMapI* const map, const char* const key) {
	if (!map || !np)
		return false;

	const size_t *vp = ppmap_get(map->ppmap, key);

	if (vp) {
		*np = *vp;
		return true;
	} else {
		*np = 0;
		return false;
	}
}

bool smapi_contains_key(const struct SMapI* const map, const char* const key) {
	return map ? ppmap_contains_key(map->ppmap, key) : false;
}

bool smapi_contains_val(const struct SMapI* const map, const size_t val) {
	return map ? ppmap_contains_val(map->ppmap, &val) : false;
}

struct SMapIPair smapi_match(const struct SMapI* const map, fn_3pred_str_szt match, const void* const data) {
	struct SMapIPair res = { 0 };

	if (!map || !match)
		return res;

	struct SMapIMatchData match_data = {
		.match_key_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match(map->ppmap, match_key_val_wrapper, &match_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

struct SMapIPair smapi_match_key(const struct SMapI* const map, fn_2pred_str match, const void* const data) {
	struct SMapIPair res = { 0 };

	if (!map || !match)
		return res;

	struct SMapIMatchData match_data = {
		.match_key = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_key(map->ppmap, match_key_wrapper, &match_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

struct SMapIPair smapi_match_val(const struct SMapI* const map, fn_2pred_szt match, const void* const data) {
	struct SMapIPair res = { 0 };

	if (!map || !match)
		return res;

	struct SMapIMatchData match_data = {
		.match_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_val(map->ppmap, match_val_wrapper, &match_data);

	res.key = pres.key;
	res.val = pres.val ? *(size_t*)pres.val : 0;

	return res;
}

const struct SMapIIt *smapi_it(const struct SMapI* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct SMapIIt *smapi_match_it(const struct SMapI* const map, fn_3pred_str_szt match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct SMapIMatchData *match_data = calloc(1, sizeof(struct SMapIMatchData));
	match_data->match_key_val = match;
	match_data->data = data;

	struct SMapIIt *it = it_init(ppmap_match_it(map->ppmap, match_key_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct SMapIIt *smapi_match_key_it(const struct SMapI* const map, fn_2pred_str match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct SMapIMatchData *match_data = calloc(1, sizeof(struct SMapIMatchData));
	match_data->match_key = match;
	match_data->data = data;

	struct SMapIIt *it = it_init(ppmap_match_key_it(map->ppmap, match_key_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct SMapIIt *smapi_match_val_it(const struct SMapI* const map, fn_2pred_szt match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct SMapIMatchData *match_data = calloc(1, sizeof(struct SMapIMatchData));
	match_data->match_val = match;
	match_data->data = data;

	struct SMapIIt *it = it_init(ppmap_match_val_it(map->ppmap, match_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct SMapIIt *smapi_it_next(const struct SMapIIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		smapi_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct SMapIIt *it_m = (struct SMapIIt*)it;
		it_m->key = it->st->pit->key;
		it_m->val = *(size_t*)it->st->pit->val;
		return it;
	} else {
		smapi_it_free(it);
		return NULL;
	}
}

bool smapi_put(const struct SMapI* const map, const char* const key, const size_t val) {
	return map ? ppmap_put_free(map->ppmap, key, &val): false;
}

bool smapi_put_if_absent(const struct SMapI* const map, const char* const key, const size_t val) {
	return map ? ppmap_put_if_absent(map->ppmap, key, &val) : NULL;
}

size_t smapi_put_all(const struct SMapI* const map, const struct SMapI* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

bool smapi_remove(const struct SMapI* const map, const char* const key) {
	return map ? ppmap_remove_free(map->ppmap, key) : false;
}

size_t smapi_remove_all(const struct SMapI* const map, const struct SMapI* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : false;
}

bool smapi_equal(const struct SMapI* const a, const struct SMapI* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *smapi_keys_pslist(const struct SMapI* const map) {
	return map ? ppmap_keys_pslist(map->ppmap) : NULL;
}

const struct SSet *smapi_keys_sset(const struct SMapI* const map) {
	if (!map)
		return NULL;

	const struct SSetParams params = {
		.case_insensitive = map->params.case_insensitive_key,
		.initial = MAX(ppmap_size(map->ppmap), map->params.initial),
		.grow = map->params.grow,
	};
	const struct SSet *set = sset_init_with(params);

	for (const struct SMapIIt *it = smapi_it(map); it; it = smapi_it_next(it)) {
		sset_add(set, it->key);
	}

	return set;
}

char *smapi_str(const struct SMapI* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t smapi_size(const struct SMapI* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
