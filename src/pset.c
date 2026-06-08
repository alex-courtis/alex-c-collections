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
};

struct PSetIter {
	const void* val;
	const struct PSet *set;
	size_t position;
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
	return pset_init_with(10, 10);
}

const struct PSet *pset_init_with(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct PSet *set = calloc(1, sizeof(struct PSet));
	set->capacity = initial;
	set->grow = grow;
	set->vals = calloc(set->capacity, sizeof(void*));

	return set;
}

const struct PSet *pset_clone(const struct PSet* const set, fn_clone clone_val) {
	if (!set)
		return NULL;

	const struct PSet *cloned = pset_init_with(set->capacity, set->grow);

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (clone_val) {
			pset_add(cloned, clone_val(*v), NULL);
		} else {
			pset_add(cloned, *v, NULL);
		}
	}

	return cloned;
}

void pset_free(const void* const cvset) {
	if (!cvset)
		return;

	struct PSet *set = (struct PSet*)cvset;

	free(set->vals);

	free(set);
}

void pset_free_vals(const struct PSet* const set, fn_free free_val) {
	if (!set)
		return;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
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

bool pset_contains(const struct PSet* const set, const void* const val, fn_equal equal_val) {
	if (!set || !val)
		return false;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (equal_val) {
			if (equal_val(*v, val)) {
				return true;
			}
		} else {
			if (*v == val) {
				return true;
			}
		}
	}

	return false;
}

const struct PSetIter *pset_iter(const struct PSet* const set) {
	if (!set || set->size == 0)
		return NULL;

	// first entry
	struct PSetIter *i = calloc(1, sizeof(struct PSetIter));
	i->set = set;
	i->val = *(set->vals);
	i->position = 0;

	return i;
}

const struct PSetIter *pset_iter_next(const struct PSetIter* const iter) {
	if (!iter)
		return NULL;

	struct PSetIter *i = (struct PSetIter*)iter;

	if (!i->set) {
		pset_iter_free(i);
		return NULL;
	}

	if (++i->position < i->set->size) {
		i->val = *(i->set->vals + i->position);
		return i;
	} else {
		pset_iter_free(i);
		return NULL;
	}
}

const void *pset_iter_val(const struct PSetIter* const iter) {
	return iter ? iter->val : NULL;
}

bool pset_add(const struct PSet* const cset, const void* const val, fn_equal equal_val) {
	if (!cset || !val)
		return false;

	struct PSet *set = (struct PSet*)cset;

	// loop over vals
	const void **v;
	for (v = set->vals; v < set->vals + set->size; v++) {

		if (equal_val) {
			if (equal_val(*v, val)) {
				return false;
			}
		} else {
			if (*v == val) {
				return false;
			}
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

const void *pset_remove(const struct PSet* const cset, const void* const val, fn_equal equal_val) {
	if (!cset || !val)
		return NULL;

	struct PSet *set = (struct PSet*)cset;

	// loop over vals
	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		bool present = false;
		if (equal_val) {
			present = equal_val(*v, val);
		} else {
			present = *v == val;
		}

		if (present) {

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

void pset_sort(const struct PSet* const set, fn_less_than less_than_val) {
	if (!set)
		return;

	static const size_t gaps[] = { 701, 301, 132, 57, 23, 10, 4, 1, 0 }; // Ciura gap sequence

	for (const size_t *gap = gaps; *gap > 0; gap++) {
		for (size_t i = *gap; i < set->size; i++) {
			const void *tmp = set->vals[i];
			size_t j;
			for (j = i; (j >= *gap) && less_than_val(tmp, set->vals[j - *gap]); j -= *gap) {
				set->vals[j] = set->vals[j - *gap];
			}
			set->vals[j] = tmp;
		}
	}
}

bool pset_equal(const struct PSet* const a, const struct PSet* const b, fn_equal equal_val) {
	if (!a || !b || a->size != b->size)
		return false;

	for (const void **av = a->vals, **bv = b->vals; av < (a->vals + a->size); av++, bv++) {
		if (equal_val) {
			if (!equal_val(*av, *bv)) {
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

char *pset_str(const struct PSet* const set, fn_str str_val) {
	if (!set)
		return NULL;

	char *out = strdup("");

	for (const void **v = set->vals; v < set->vals + set->size; v++) {
		if (str_val) {
			char *val_str = str_val(*v);
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

size_t pset_capacity(const struct PSet* const set) {
	return set ? set->capacity : 0;
}
