#include <assert.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "pslist.h"
#include "str.h"

#include "plist.h"

#define PLIST_DEFAULT_INITIAL 10
#define PLIST_DEFAULT_GROW 10

struct Plist {
	const struct PlistParams params;
	const void **vals;
	size_t capacity;
	size_t size;
};

struct PlistItState {
	const struct Plist *list;
	const struct PlistFilter filter;
	size_t position;
	bool attached;
};

// grow to capacity + grow
static void grow(struct Plist *list) {
	size_t new_capacity = list->capacity + (list->params.grow ? list->params.grow : PLIST_DEFAULT_GROW);

	// grow new arrays
	const void **new_vals = calloc(new_capacity, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, list->vals, list->capacity * sizeof(void*));

	// free old arrays
	free(list->vals);

	// lock in new
	list->vals = new_vals;
	list->capacity = new_capacity;
}

static const struct PlistIt *it_init(const struct Plist *list) {
	if (list->size == 0)
		return NULL;

	struct PlistIt *it = calloc(1, sizeof(struct PlistIt));
	it->st = calloc(1, sizeof(struct PlistItState));
	it->st->list = list;

	return it;
}

static bool add(const struct Plist* const list, const void* const val, fn_clone alloc_val) {
	fprintf(stderr, "add %p\n", val);
	if (!val)
		return false;

	struct Plist *list_m = (struct Plist*)list;

	// create new value
	const void *new = alloc_val ? alloc_val(val) : val;
	if (!new)
		return false;

	// maybe grow for new entry
	if (list->size >= list->capacity) {
		grow(list_m);
	}

	const void **v = &list->vals[list->size];

	// assign new value
	*v = new;
	list_m->size++;

	return true;
}

// TODO better remove_ name strategy
// TODO deal with the clist/cset/cmap and map_m/set_m
static size_t remove_(const struct Plist* const clist, const void* const val, fn_free free_val) {
	if (!val)
		return false;

	size_t removed = 0;

	bool freed = false;

	struct Plist *list = (struct Plist*)clist;

	fprintf(stderr, "\nremove_\nsize=%zu\n", list->size);
	for (size_t i = list->size; i > 0; i--) {
		const void **v = list->vals + i - 1;
		fprintf(stderr, "i=%zu *v=%p\n", i, *v);
		if (list->params.equal_val ? list->params.equal_val(*v, val) : *v == val) {
			if (free_val && !freed) {
				fprintf(stderr, " freeing\n");
				free_val((void*)*v);
				freed = true;
			}

			*v = NULL;
			list->size--;

			// shift down over removed
			const void **m;
			for (m = v; m < list->vals + list->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			removed++;
			fprintf(stderr, " removed %zu\n", removed);
		}
	}

	return removed;
}

static size_t remove_all(const struct Plist* const clist, bool do_free) {
	struct Plist *list = (struct Plist*)clist;

	// values to free, no duplicates or nulls
	const void **to_free = calloc(list->size, sizeof(void*));
	size_t ntf = 0;

	fprintf(stderr, "\nremove_all\nlist->size=%zu\n", list->size);

	if (do_free) {
		for (const void **vl = list->vals; vl < list->vals + list->size; vl++) {
			if (!*vl)
				continue;

			bool dup = false;

			fprintf(stderr, " freeing %p\n", *vl);
			for (const void **vf = to_free; vf < to_free + ntf; vf++) {
			// for (size_t j = 0; j < ntf; j++) {
				// if (*vl == *(to_free + j)) {
				if (*vl == *vf) {
					fprintf(stderr, "  dup %p\n", *vl);
					dup = true;
				}
			}

			if (!dup)
				*(to_free + ntf++) = *vl;
		}
	}

	for (const void **vf = to_free; vf < to_free + ntf; vf++) {
		fprintf(stderr, " will free %p\n", *vf);
		if (list->params.free_val) {
			list->params.free_val((void*)*vf);
		} else {
			free((void*)*vf);
		}
	}

	free(to_free);

	memset(list->vals, 0, list->size * sizeof(void*));

	size_t removed = list->size;
	list->size = 0;

	return removed;
}

static void it_remove(const struct PlistIt* const it, bool do_free) {
	if (!it)
		return;

	struct PlistItState *st = it->st;
	if (!st) {
		plist_it_free(it);
		return;
	}

	remove_(st->list, it->val, do_free ? st->list->params.free_val: NULL);

	if (st->position > 0) {
		st->position--;
	} else {
		st->attached = false;
	}

	((struct PlistIt*)it)->val = NULL;
}

static bool filter_blocks(const struct PlistFilter *filter, const void* const val) {
	return
		(filter->val          && !filter->val         (val              )) ||
		(filter->val_data     && !filter->val_data    (val, filter->data));
}

static size_t add_all(const struct Plist* const list, const struct Plist* const from, fn_clone clone_val) {
	size_t added = 0;

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		add(list, *v, clone_val);
		added++;
	}

	return added;
}

static size_t remove_in(const struct Plist* const list, const struct Plist* const in, fn_free free_val) {
	size_t removed = 0;

	for (const void **v = in->vals; v < in->vals + in->size; v++) {
		removed += remove_(list, *v, free_val);
	}

	return removed;
}

static const struct Plist *clone(const struct Plist* const from, fn_clone clone_val) {
	const struct Plist *to = plist_init_with(from->params);

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		add(to, *v, clone_val);
	}

	return to;
}

static struct Pslist *pslist(const struct Plist* const list, fn_clone clone_val) {
	struct Pslist *slist = NULL;

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (clone_val) {
			pslist_append(&slist, (void*)clone_val(*v));
		} else {
			pslist_append(&slist, (void*)*v);
		}
	}

	return slist;
}

const struct Plist *plist_init(void) {
	const struct PlistParams params = { 0 };
	return plist_init_with(params);
}

const struct Plist *plist_init_with(const struct PlistParams params) {
	struct Plist *list = calloc(1, sizeof(struct Plist));

	list->capacity = params.initial ? params.initial : PLIST_DEFAULT_INITIAL;
	list->vals = calloc(list->capacity, sizeof(void*));

	memcpy((void*)&list->params, &params, sizeof(struct PlistParams));

	return list;
}

const struct Plist *plist_clone(const struct Plist* const from) {
	return from ? clone(from, from->params.alloc_val) : NULL;
}

const struct Plist *plist_clone_deep(const struct Plist* const from) {
	return from && from->params.clone_val ? clone(from, from->params.clone_val) : NULL;
}

void plist_free(const struct Plist * const list) {
	if (!list)
		return;

	free(list->vals);

	free((void*)list);
}

void plist_free_vals(const struct Plist* const list) {
	if (!list)
		return;

	remove_all(list, true);

	plist_free(list);
}

void plist_it_free(const struct PlistIt* const it) {
	if (!it)
		return;

	free((void*)it->st);
	free((void*)it);
}

bool plist_contains(const struct Plist* const list, const void* const val) {
	if (!list || !val)
		return false;

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (list->params.equal_val ? list->params.equal_val(*v, val) : *v == val) {
			return true;
		}
	}

	return false;
}

const void *plist_at(const struct Plist* const list, const size_t i) {
	return list && i < list->size ? *(list->vals + i) : NULL;
}

const void *plist_find(const struct Plist* const list, const struct PlistFilter filter) {
	if (!list)
		return NULL;

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (!filter_blocks(&filter, *v)) {
			return *v;
		}
	}

	return NULL;
}

const struct PlistIt *plist_it(const struct Plist* const list) {
	if (!list || list->size == 0)
		return NULL;

	const struct PlistIt *it = it_init(list);

	return plist_it_next(it);
}

// cppcheck-suppress unusedFunction
const struct PlistIt *plist_it_end(const struct Plist* const list) {
	// TODO
	assert(false);
}

const struct PlistIt *plist_filter_it(const struct Plist* const list, const struct PlistFilter filter) {
	if (!list)
		return NULL;

	const struct PlistIt *it = it_init(list);
	if (!it)
		return NULL;

	memcpy((void*)&it->st->filter, &filter, sizeof(struct PlistFilter));

	return plist_it_next(it);
}

// cppcheck-suppress unusedFunction
const struct PlistIt *plist_filter_it_end(const struct Plist* const list, const struct PlistFilter filter) {
	// TODO
	assert(false);
}

const struct PlistIt *plist_it_next(const struct PlistIt* const it) {
	if (!it)
		return NULL;

	struct PlistItState *st = it->st;
	if (!st) {
		plist_it_free(it);
		return NULL;
	}

	if (st->attached) {
		st->position++;
	} else {
		st->position = 0;
	}
	st->attached = true;

	for ( ; st->position < st->list->size; st->position++) {

		struct PlistIt *it_m = (struct PlistIt*)it;
		it_m->val = *(st->list->vals + st->position);

		if (filter_blocks(&st->filter, it->val)) {
			continue;
		}

		return it;
	}

	plist_it_free(it);
	return NULL;
}

// cppcheck-suppress unusedFunction
const struct PlistIt *plist_it_prev(const struct PlistIt* const it) {
	// TODO
	assert(false);
}

// TODO this should be void
bool plist_add(const struct Plist* const list, const void* const val) {
	return list ? add(list, val, list->params.alloc_val) : false;
}

size_t plist_add_all(const struct Plist* const list, const struct Plist* const from) {
	return list && from ? add_all(list, from, list->params.alloc_val) : 0;
}

size_t plist_add_all_clone(const struct Plist* const list, const struct Plist* const from) {
	return list && from && list->params.clone_val ? add_all(list, from, list->params.clone_val) : 0;
}

size_t plist_remove(const struct Plist* const list, const void* const val) {
	return list ? remove_(list, val, NULL) : 0;
}

size_t plist_remove_free(const struct Plist* const list, const void* const val) {
	return list ? remove_(list, val, list->params.free_val ? list->params.free_val : free) : false;
}

size_t plist_remove_all(const struct Plist* const list) {
	return list ? remove_all(list, false) : 0;
}

size_t plist_remove_all_free(const struct Plist* const list) {
	return list ? remove_all(list, true) : 0;
}

size_t plist_remove_in(const struct Plist* const list, const struct Plist* const in) {
	return list && in ? remove_in(list, in, NULL) : 0;
}

size_t plist_remove_in_free(const struct Plist* const list, const struct Plist* const in) {
	return list && in ? remove_in(list, in, list->params.free_val ? list->params.free_val : free) : 0;
}

void plist_it_remove(const struct PlistIt* const it) {
	it_remove(it, false);
}

void plist_it_remove_free(const struct PlistIt* const it) {
	it_remove(it, true);
}

void plist_sort(const struct Plist* const list, fn_less_than less_than_val) {
	if (!list || !less_than_val)
		return;

	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 }; // Ciura gap sequence

	for (const size_t *gap = gaps; *gap > 0; gap++) {
		for (size_t i = *gap; i < list->size; i++) {
			const void *tmp = list->vals[i];
			size_t j;
			for (j = i; (j >= *gap) && less_than_val(tmp, list->vals[j - *gap]); j -= *gap) {
				list->vals[j] = list->vals[j - *gap];
			}
			list->vals[j] = tmp;
		}
	}
}

bool plist_equal(const struct Plist* const a, const struct Plist* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {
		if (a->params.equal_val) {
			if (!a->params.equal_val(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct Pslist *plist_pslist(const struct Plist* const list) {
	return list ? pslist(list, list->params.alloc_val) : NULL;
}

struct Pslist *plist_pslist_clone(const struct Plist* const list) {
	if (!list || !list->params.clone_val)
		return NULL;

	return pslist(list, list->params.clone_val);
}

char *plist_str(const struct Plist* const list) {
	if (!list)
		return NULL;

	char *out = strdup("");

	for (const void **v = list->vals; v < list->vals + list->size; v++) {
		if (list->params.str_val) {
			char *val_str = list->params.str_val(*v);
			out = sprintf_append(out, "%s\n", val_str);
			free(val_str);
		} else {
			out = sprintf_append(out, "%p\n", *v);
		}
	}

	return out;
}
size_t plist_size(const struct Plist* const list) {
	return list ? list->size : 0;
}
