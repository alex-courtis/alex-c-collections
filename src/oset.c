#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "oset.h"

struct OSet {
	void **vals;
	size_t capacity;
	size_t grow;
	size_t next;
};

struct OSetIterP {
	struct OSetIter iter;
	const struct OSet *set;
	void **v;
};

// grow to capacity + grow
void grow_oset(struct OSet *set) {

	// grow new arrays
	void **new_vals = calloc(set->capacity + set->grow, sizeof(void*));

	// copy old arrays
	memcpy(new_vals, set->vals, set->capacity * sizeof(void*));

	// free old arrays
	free(set->vals);

	// lock in new
	set->vals = new_vals;
	set->capacity += set->grow;
}

const struct OSet *oset_init(const size_t initial, const size_t grow) {
	if (initial == 0 || grow == 0)
		return NULL;

	struct OSet *set = calloc(1, sizeof(struct OSet));
	set->capacity = initial;
	set->grow = grow;
	set->vals = calloc(set->capacity, sizeof(void*));

	return set;
}

void oset_free(const void *cvset) {
	if (!cvset)
		return;

	struct OSet *set = (struct OSet*)cvset;

	free(set->vals);

	free(set);
}

void oset_free_vals(const struct OSet *cset, void (*free_val)(void *val)) {
	if (!cset)
		return;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	for (void **v = set->vals; v < set->vals + set->capacity; v++) {
		if (*v) {
			if (free_val) {
				free_val(*v);
			} else {
				free(*v);
			}
		}
	}

	oset_free(set);
}

void oset_iter_free(const struct OSetIter *iter) {
	if (!iter)
		return;

	free((struct OSetIterP*)iter);
}

bool oset_contains(const struct OSet *cset, const void *val) {
	if (!cset || !val)
		return false;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	for (void **v = set->vals; v < set->vals + set->next; v++) {
		if (*v == val) {
			return true;
		}
	}

	return false;
}

bool oset_remove(const struct OSet *cset, const void *val) {
	if (!cset || !val)
		return false;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	for (void **v = set->vals; v < set->vals + set->next; v++) {
		if (*v == val) {
			*v = NULL;
			return true;
		}
	}

	return false;
}

size_t oset_size(const struct OSet *cset) {
	if (!cset)
		return 0;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	void **v;
	size_t size;
	for (v = set->vals, size = 0; v < set->vals + set->next; v++) {
		if (*v) {
			size++;
		}
	}

	return size;
}

const struct OSetIter *oset_iter(const struct OSet *cset) {
	if (!cset)
		return NULL;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	for (void **v = set->vals; v < set->vals + set->next; v++) {
		if (*v) {
			struct OSetIterP *iterp = calloc(1, sizeof(struct OSetIterP));

			iterp->set = set;
			iterp->iter.val = *v;
			iterp->v = v;

			return (struct OSetIter*)iterp;
		}
	}

	return NULL;
}

const struct OSetIter *oset_next(const struct OSetIter *iter) {
	if (!iter)
		return NULL;

	struct OSetIterP *iterp = (struct OSetIterP*)iter;

	if (!iterp || !iterp->set) {
		oset_iter_free(iter);
		return NULL;
	}

	// loop over vals
	while (++iterp->v < iterp->set->vals + iterp->set->next) {
		if (*iterp->v) {
			iterp->iter.val = *(iterp->v);
			return iter;
		}
	}

	oset_iter_free(iter);
	return NULL;
}

bool oset_add(const struct OSet *cset, const void *val) {
	if (!cset || !val)
		return false;

	struct OSet *set = (struct OSet*)cset;

	// loop over vals
	void **v;
	for (v = set->vals; v < set->vals + set->next; v++) {

		// already present
		if (*v == val) {
			return false;
		}
	}

	// maybe grow for new entry
	if (set->next >= set->capacity) {
		grow_oset(set);
		v = &set->vals[set->next];
	}

	// new value
	*v = (void*)val;
	set->next++;

	return true;
}
