#include <stdlib.h>
#include <stdio.h>

#include "llog.h"
#include "ltypes.h"
#include "lmemory.h"
#include "lhash.h"
#include "lobjectcache.h"


/** \internal
 * The structure comprising a cache object container.
 */
struct _LCache
{
    LCache * storage;        /**< the table of lists in which the keys/values are stored */
    long object_ttl;   /**< cache elemant time-to-live */
};

static LCache *_cache_instance = NULL;

LCache * l_cache_instance (void) {
	if (NULL == _cache_instance) {
		_cache_instance = l_calloc (sizeof (LCache), 1);
		if (!_cache_instance)
			return NULL;

		_cache_instance->storage = l_hash_new();
		if (!_cache_instance->storage) {
			l_free (_cache_instance);
			return NULL;
		}
		_cache_instance->object_ttl = 1000;
	}
	return _cache_instance;
}

/**
 * Insert a new key/value pair into \e hash. The types contained in key and
 * value should match the types \e hash is set up to deal with. Both \e key and
 * \e value will be owned by \e hash from this point on.
 *
 * Inserting a key that already exists in the hash will result in that
 * key/value pair being overwritten (with the appropriate key/value destroy
 * functions being called.
 *
 * @see l_hash_remove()
 *
 * @param hash the hash into which \e key and \e value should be inserted.
 * @param key the key to insert
 * @param value the value to insert
 * @return FALSE if out of memory. Otherwise return TRUE
 */
bool
l_put_cache (LCache * cache, lpointer key, lpointer value)
{
	return l_hash_insert(cache->storage, key, value);
}
 
/**
 * Search \e hash for \e key returning the associated value if \e key is
 * found, NULL otherwise.
 *
 * @param hash the hash in which to look for \e key
 * @param key the key to look for in \e hash.
 *
 * @returns the value associated with \e key in \e hash or NULL if no matching
 * key was found.
 */
lpointer
l_get_cache (LCache * cache,
               lconstpointer key)
{
    LHash * hash = cache->storage;
	return l_hash_lookup(hash, key);
}

/**
 * Search \e hash for \e key returning the associated value if \e key is
 * found, NULL otherwise.
 *
 * @param hash the hash in which to look for \e key
 * @param key the key to look for in \e hash.
 *
 * @returns the value associated with \e key in \e hash or NULL if no matching
 * key was found.
 */
lpointer
l_lookup_cache (LCache * cache,
               lconstpointer key,
			   LCacheObjectCreator creator)
{
	lpointer value = l_get_cache(cache, key);
	if (NULL == value) {
		value = creator((lpointer)key);
		l_put_cache(cache, key, value);
	}
    return value;
}
