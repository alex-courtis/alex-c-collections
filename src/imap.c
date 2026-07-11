#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "ppmap.h"

#include "imap.h"

struct IMap {
	const struct IMapParams params;
	const struct PPmap *ppmap;
};

struct IMapMatchData {
	fn_3pred_szt_ptr match_key_val;
	fn_2pred_szt match_key;
	fn_2pred match_val;
	const void *data;
};

struct IMapItState {
	const struct PPmapIt *pit;
	const struct IMapMatchData *match_data;
};

static bool match_key_val_wrapper(const void* const key, const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_key_val(*(size_t*)key, val, matcher->data);
}

static bool match_key_wrapper(const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_key(*(size_t*)val, matcher->data);
}

static bool match_val_wrapper(const void* const val, const void* const data) {
	const struct IMapMatchData* const matcher = data;
	return matcher->match_val(val, matcher->data);
}

static const struct IMap *clone(const struct IMap* const from, bool deep) {
	if (!from)
		return NULL;

	struct IMap *to = calloc(1, sizeof(struct IMap));

	to->ppmap = deep ? ppmap_clone_deep(from->ppmap) : ppmap_clone(from->ppmap);

	memcpy((void*)&to->params, &from->params, sizeof(struct IMapParams));

	return to;
}

static struct IMapIt *it_init(const struct PPmapIt *pit) {
	if (!pit)
		return NULL;

	struct IMapIt *it = calloc(1, sizeof(struct IMapIt));
	it->st = calloc(1, sizeof(struct IMapItState));

	it->st->pit = pit;
	it->key = *(size_t*)pit->key;
	it->val = pit->val;

	return it;
}

const struct IMap *imap_init(void) {
	const struct IMapParams params = { 0 };
	return imap_init_with(params);
}

const struct IMap *imap_init_with(const struct IMapParams params) {
	const struct PPmapParams ppmap_params = {
		.equal_key = (fn_equal)equal_stp,
		.equal_val = params.equal_val,
		.alloc_key = (fn_clone)clone_size_t_ptr,
		.alloc_val = params.alloc_val,
		.free_key = (fn_free)free,
		.free_val = params.free_val,
		.clone_val = params.clone_val,
		.str_key = (fn_str)str_size_t_ptr,
		.str_val = params.str_val,
		.allow_null_val = params.allow_null_val,
		.initial = params.initial,
		.grow = params.grow,
	};

	struct IMap *map =  calloc(1, sizeof(struct IMap));
	map->ppmap = ppmap_init_with(ppmap_params);;
	memcpy((void*)&map->params, &params, sizeof(struct IMapParams));

	return map;
}

const struct IMap *imap_clone(const struct IMap* const from) {
	return clone(from, false);
}

const struct IMap *imap_clone_deep(const struct IMap* const from) {
	return clone(from, true);
}

void imap_free(const struct IMap* const map) {
	if (!map)
		return;

	ppmap_free(map->ppmap);

	free((void*)map);
}

void imap_free_vals(const struct IMap* const map) {
	if (!map)
		return;

	ppmap_free_vals(map->ppmap);

	free((void*)map);
}

void imap_it_free(const struct IMapIt* const it) {
	if (!it)
		return;

	if (it->st) {
		free((void*)it->st->match_data);
		ppmap_it_free(it->st->pit);
	}

	free(it->st);
	free((void*)it);
}

const void *imap_get(const struct IMap* const map, const size_t key) {
	return map ? ppmap_get(map->ppmap, &key) : NULL;
}

bool imap_contains_key(const struct IMap* const map, const size_t key) {
	return map ? ppmap_contains_key(map->ppmap, &key) : false;
}

bool imap_contains_val(const struct IMap* const map, const void* const val) {
	return map ? ppmap_contains_val(map->ppmap, val) : false;
}

struct IMapPair imap_match(const struct IMap* const map, fn_3pred_szt_ptr match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_key_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match(map->ppmap, match_key_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IMapPair imap_match_key(const struct IMap* const map, fn_2pred_szt match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_key = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_key(map->ppmap, match_key_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

struct IMapPair imap_match_val(const struct IMap* const map, fn_2pred match, const void* const data) {
	struct IMapPair res = { 0 };

	if (!map || !match)
		return res;

	struct IMapMatchData match_data = {
		.match_val = match,
		.data = data,
	};

	struct PPmapPair pres = ppmap_match_val(map->ppmap, match_val_wrapper, &match_data);

	res.key = pres.key ? *(size_t*)pres.key : 0;
	res.val = pres.val;

	return res;
}

const struct IMapIt *imap_it(const struct IMap* const map) {
	return map ? it_init(ppmap_it(map->ppmap)) : NULL;
}

const struct IMapIt *imap_match_it(const struct IMap* const map, fn_3pred_szt_ptr match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapMatchData *match_data = calloc(1, sizeof(struct IMapMatchData));
	match_data->match_key_val = match;
	match_data->data = data;

	struct IMapIt *it = it_init(ppmap_match_it(map->ppmap, match_key_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_match_key_it(const struct IMap* const map, fn_2pred_szt match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapMatchData *match_data = calloc(1, sizeof(struct IMapMatchData));
	match_data->match_key = match;
	match_data->data = data;

	struct IMapIt *it = it_init(ppmap_match_key_it(map->ppmap, match_key_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_match_val_it(const struct IMap* const map, fn_2pred match, const void* const data) {
	if (!map || !match)
		return NULL;

	struct IMapMatchData *match_data = calloc(1, sizeof(struct IMapMatchData));
	match_data->match_val = match;
	match_data->data = data;

	struct IMapIt *it = it_init(ppmap_match_val_it(map->ppmap, match_val_wrapper, match_data));

	if (it) {
		it->st->match_data = match_data;
		return it;
	} else {
		free(match_data);
		return NULL;
	}
}

const struct IMapIt *imap_it_next(const struct IMapIt* const it) {
	if (!it)
		return NULL;


	if (!it->st) {
		imap_it_free(it);
		return NULL;
	}

	it->st->pit = ppmap_it_next(it->st->pit);

	if (it->st->pit) {
		struct IMapIt *it_m = (struct IMapIt*)it;
		it_m->key = *(size_t*)it->st->pit->key;
		it_m->val = it->st->pit->val;
		return it;
	} else {
		imap_it_free(it);
		return NULL;
	}
}

const void *imap_put(const struct IMap* const map, const size_t key, const void* const val) {
	return map ? ppmap_put(map->ppmap, &key, val) : NULL;
}

const void *imap_put_if_absent(const struct IMap* const map, const size_t key, const void* const val) {
	return map ? ppmap_put_if_absent(map->ppmap, &key, val) : NULL;
}

bool imap_put_free(const struct IMap* const map, const size_t key, const char* const val) {
	return map ? ppmap_put_free(map->ppmap, &key, val) : false;
}

const void *imap_remove(const struct IMap* const map, const size_t key) {
	return map ? ppmap_remove(map->ppmap, &key) : NULL;
}

bool imap_remove_free(const struct IMap* const map, const size_t key) {
	return map ? ppmap_remove_free(map->ppmap, &key) : false;
}

size_t imap_remove_all(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_remove_all(map->ppmap, from->ppmap) : 0;
}

size_t imap_remove_all_free(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_remove_all_free(map->ppmap, from->ppmap) : 0;
}

size_t imap_put_all(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_put_all(map->ppmap, from->ppmap) : 0;
}

size_t imap_put_all_free(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_put_all_free(map->ppmap, from->ppmap) : 0;
}

size_t imap_put_all_clone(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_put_all_clone(map->ppmap, from->ppmap) : 0;
}

size_t imap_put_all_clone_free(const struct IMap* const map, const struct IMap* const from) {
	return map && from ? ppmap_put_all_clone_free(map->ppmap, from->ppmap) : 0;
}

bool imap_equal(const struct IMap* const a, const struct IMap* const b) {
	return a && b ? ppmap_equal(a->ppmap, b->ppmap) : false;
}

struct Pslist *imap_vals_pslist(const struct IMap* const map) {
	return map ? ppmap_vals_pslist(map->ppmap) : NULL;
}

struct Pslist *imap_vals_pslist_clone(const struct IMap* const map) {
	return map ? ppmap_vals_pslist_clone(map->ppmap) : NULL;
}

const struct PSet *imap_vals_pset(const struct IMap* const map) {
	return map ? ppmap_vals_pset(map->ppmap) : NULL;
}

const struct PSet *imap_vals_pset_clone(const struct IMap* const map) {
	return map ? ppmap_vals_pset_clone(map->ppmap) : NULL;
}

char *imap_str(const struct IMap* const map) {
	return map ? ppmap_str(map->ppmap) : NULL;
}

size_t imap_size(const struct IMap* const map) {
	return map ? ppmap_size(map->ppmap) : 0;
}
