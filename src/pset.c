#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "fn.h"
#include "slist.h"
#include "str.h"

#include "pset.h"

/*
   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' inc/pset.h) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' inc/sset.h) | less

   diff --color=always -U 10000 <(sed -e ' s/pset/xset/g ; s/PSet/XSet/g ' src/pset.c) <(sed -e 's/sset/xset/g ; s/SSet/XSet/g' src/sset.c) | less
   */

struct PSet {
	const void **vals;
	size_t capacity;
	size_t grow;
	size_t size;
	fn_equal equal_val;
	fn_less_than less_than_val;
	fn_free free_val;
	fn_str str_val;
	fn_clone clone_val;
};

struct PSetIter {
	const void* val;
	const struct PSet *set;
	size_t position;
	fn_test test_val;
	const void *data;
};

// grow to capacity + grow
static void grow_pset(struct PSet *set) {

	// grow new arrays
	const void **new_vals = calloc(set->capacity + set->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	set->vals = new_vals;
	set->capacity += set->grow;
}

const struct PSet *pset_init(void) {
	const struct PSetParams params = { 0 };
	return pset_init_with(params);
}

const struct PSet *pset_init_with(const struct PSetParams params) {
	struct PSet *set = calloc(1, sizeof(struct PSet));

	set->capacity = params.initial ? params.initial : 10;
	set->grow = params.grow ? params.grow : 10;;
	set->vals = calloc(set->capacity, sizeof(void*));
	set->equal_val = params.equal_val;
	set->less_than_val = params.less_than_val;
	set->free_val = params.free_val;
	set->str_val = params.str_val;
	set->clone_val = params.clone_val;

	return set;
}

const struct PSet *pset_clone(const struct PSet* const from) {
	if (!from)
		return NULL;

	const struct PSetParams params = {
		.initial = from->capacity,
		.grow = from->grow,
		.equal_val = from->equal_val,
		.less_than_val = from->less_than_val,
		.free_val = from->free_val,
		.str_val = from->str_val,
		.clone_val = from->clone_val,
	};
	const struct PSet *to = pset_init_with(params);

	for (const void **v = from->vals; v < from->vals + from->size; v++) {
		pset_add(to, from->clone_val ? from->clone_val(*v) : *v);
	}

	return to;
}

void pset_free(const void* const cvset) {
	if (!cvset)
		return;

	struct PSet *set = (struct PSet*)cvset;

	free(set->vals);

	free(set);
}

void pset_free_vals(const struct PSet* const set) {
	if (!set)
		return;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v) {
			if (set->free_val) {
				set->free_val(*v);
			} else {
				free((void*)*v);
			}
		}
	}

	pset_free(set);
}

void pset_iter_free(const struct PSetIter* const iter) {
	if (!iter)
		return;

	free((void*)iter);
}

bool pset_contains(const struct PSet* const set, const void* const val) {
	if (!set || !val)
		return false;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v == val) {
			return true;
		}
	}

	return false;
}

const struct PSetIter *pset_iter(const struct PSet* const set) {
	return pset_filter_iter(set, NULL, NULL);
}

const struct PSetIter *pset_filter_iter(const struct PSet* const set, fn_test test_val, const void* const data) {
	if (!set || set->size == 0)
		return NULL;

	// first entry
	struct PSetIter *it = calloc(1, sizeof(struct PSetIter));
	it->set = set;
	it->test_val = test_val;
	it->data = data;

	return pset_iter_next(it);

	return it;
}

const struct PSetIter *pset_iter_next(const struct PSetIter* const iter) {
	if (!iter)
		return NULL;

	struct PSetIter *it = (struct PSetIter*)iter;

	if (!it->set) {
		pset_iter_free(it);
		return NULL;
	}

	// null val indicates first use, start at the beginning
	if (it->val) {
		it->position++;
	}

	for ( ; it->position < it->set->size; it->position++) {

		it->val = *(it->set->vals + it->position);

		if ((it->test_val && !it->test_val(it->val, it->data))) {
			continue;
		}

		return it;
	}

	pset_iter_free(it);
	return NULL;
}

const void *pset_iter_val(const struct PSetIter* const iter) {
	return iter ? iter->val : NULL;
}

bool pset_add(const struct PSet* const cset, const void* const val) {
	if (!cset || !val)
		return false;

	struct PSet *set = (struct PSet*)cset;

	const void **v;
	for (v = set->vals; v < set->vals + set->size; v++) {
		if (*v == val) {
			return false;
		}
	}

	// maybe grow for new entry
	if (set->size >= set->capacity) {
		grow_pset(set);
		v = &set->vals[set->size];
	}

	// new value
	*v = (void*)val;
	set->size++;

	return true;
}

const void *pset_remove(const struct PSet* const cset, const void* const val) {
	if (!cset || !val)
		return NULL;

	struct PSet *set = (struct PSet*)cset;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v == val) {
			const void *removed = *v;

			*v = NULL;
			set->size--;

			// shift down over removed
			const void **m;
			for (m = v; m < v + set->size; m++) {
				*m = *(m + 1);
			}
			*m = NULL;

			return removed;
		}
	}

	return NULL;
}

void pset_sort(const struct PSet* const set) {
	if (!set || !set->less_than_val)
		return;

	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 }; // Ciura gap sequence

	for (const size_t *gap = gaps; *gap > 0; gap++) {
		for (size_t i = *gap; i < set->size; i++) {
			const void *tmp = set->vals[i];
			size_t j;
			for (j = i; (j >= *gap) && set->less_than_val(tmp, set->vals[j - *gap]); j -= *gap) {
				set->vals[j] = set->vals[j - *gap];
			}
			set->vals[j] = tmp;
		}
	}
}

bool pset_equal(const struct PSet* const a, const struct PSet* const b) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {
		if (a->equal_val) {
			if (!a->equal_val(*av, *bv)) {
				return false;
			}
		} else if (*av != *bv) {
			return false;
		}
	}

	return true;
}

struct SList *pset_slist(const struct PSet* const set) {
	if (!set)
		return NULL;

	struct SList *list = NULL;

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		slist_append(&list, (void*)*v);
	}

	return list;
}

char *pset_str(const struct PSet* const set) {
	if (!set)
		return NULL;

	char *out = strdup("");

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (set->str_val) {
			char *val_str = set->str_val(*v);
			out = sprintf_append(out, "%s\n", val_str);
			free(val_str);
		} else {
			out = sprintf_append(out, "%p\n", *v);
		}
	}

	return out;
}
size_t pset_size(const struct PSet* const set) {
	return set ? set->size : 0;
}
