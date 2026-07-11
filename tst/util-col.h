#ifndef UTIL_COL_H
#define UTIL_COL_H

#include <stdarg.h>
#include <stddef.h>

#include "ipmap.h"
#include "ppmap.h"
#include "pset.h"
#include "smap.h"
#include "smapi.h"
#include "smaps.h"
#include "sset.h"

/*
 * variadic mutation, not for production use, will flag code scanners
 */

// add if the set does not contain each val, return number added, variadic args must be NULL terminated [equal_val, alloc_val]
size_t pset_add_many(const struct PSet* const set, ... /* , NULL */ );
size_t pset_add_many_v(const struct PSet* const set, va_list __args);

// add if the set does not contain each val, return number added, variadic args must be NULL terminated
size_t sset_add_many(const struct SSet* const set, ... /* , NULL */ );

// set key/vals, free old vals, return number overwritten, variadic key/val pairs must be terminated with a NULL key [equal_key, alloc_key, alloc_val, free_key, free_val]
size_t ppmap_put_many(const struct PPmap* const map, ... /* key, val, NULL */ );
size_t ppmap_put_many_v(const struct PPmap* const map, va_list __args);

// set key/vals, free old vals, return number overwritten, variadic key/val pairs must be terminated with a 0 key [alloc_val, free_val]
size_t ipmap_put_many(const struct IPmap* const map, ... /* key, val, NULL */ );

// set key/vals, free old vals, return number overwritten, variadic key/val pairs must be terminated with a NULL key [alloc_val, free_val]
size_t smap_put_many(const struct SMap* const map, ... /* key, val, NULL */ );

// set key/vals, return number overwritten, variadic key/val pairs must be terminated with a NULL key
size_t smapi_put_many(const struct SMapI* const map, ... /* key, val, NULL */ );

// set key/vals, return number overwritten, variadic key/val pairs must be terminated with a NULL key
size_t smaps_put_many(const struct SMapS* const map, ... /* key, val, NULL */ );

#endif // UTIL_COL_H
