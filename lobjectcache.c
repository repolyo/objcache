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
    LHash * storage;        /**< the table of lists in which the keys/values are stored */
    time_t object_ttl;   /**< cache elemant time-to-live */
	time_t cleanup_delay; 
};

struct _LCacheItem
{
	lpointer value;
	time_t last_accessed;
};

typedef struct _LCacheItem LCacheItem;
typedef struct _LCacheItem* LCacheItemP;
typedef struct _LCache* LCacheP;

static LCache *_cache_instance = NULL;

/**
 * Creates a new LHash object with integers for keys and values.
 *
 * @see l_hash_new_full()
 *
 * @returns a new LHash object.
 */

LCache * l_cache_new (time_t ttl, time_t cleanup) {
	LCacheP cache = l_calloc (sizeof (LCache), 1);
	if (!cache)
		return NULL;

	cache->storage = l_hash_new();
	if (!cache->storage) {
		l_free (cache);
		return NULL;
	}
	cache->object_ttl = ttl;
	cache->cleanup_delay = cleanup;
	/* cleanup_delay
	if (ttl > 0 && cleanupDelay > 0) {
	    Thread t = new Thread(new Runnable() {
	        public void run() {
	            while (true) {
	                try {
	                    Thread.sleep(TimeUnit.MILLISECONDS.convert(cleanupDelay, TimeUnit.SECONDS));
	                } 
	                catch (InterruptedException ex) {
	                }
	                cleanup();
	            }
	        }
	    });
	    t.setPriority(Thread.MIN_PRIORITY);
	    t.setDaemon(true);
	    t.start();
	}*/ 
	return cache;
}

LCache *
l_get_cache_instance (void)
{
	if (NULL == _cache_instance) {
		_cache_instance = l_cache_new (2000, 5000);
	}
	return _cache_instance;
}

static bool
refresh (lpointer key, lpointer value, lpointer user_data)
{
	bool success = true;
	time_t now = time(NULL);
	LCacheP cache = (LCacheP)user_data;
	time_t ttl = cache->object_ttl;
	LCacheItemP itemP = (LCacheItemP)value;
	if (itemP != NULL && (now > (ttl + itemP->last_accessed))) {
		l_hash_remove(cache->storage, key);
		fprintf(stdout, "%s: deleteKey: %p", __func__, key);
		//Thread.yield();
	}
	return success;
}

void 
cleanup()
{
	// TODO: critical section: possibly needed to lock if LHash is not thread-safe?
	LCacheP cache = l_get_cache_instance();
	l_hash_foreach(cache->storage, refresh, cache);
}

static LCacheItemP createItem(lpointer value)
{
	LCacheItemP itemP = l_calloc (sizeof (LCacheItem), 1);
	if (!itemP)
		return NULL;

	itemP->value = value;
	itemP->last_accessed = time(NULL);
	return itemP;
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
	bool ret = false;
	LCacheItemP itemP = createItem(value);
	if (!itemP) {
		ret = l_hash_insert(cache->storage, key, itemP);
	}
	return ret;
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
	lpointer value = NULL;
	LCacheItemP pitem = (LCacheItemP)l_hash_lookup(cache->storage, key);
	if (NULL != pitem) {
		pitem->last_accessed = time (NULL);
		value = pitem->value;
	}
	return value;
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
l_get_or_create_cache (LCache * cache,
               lconstpointer key,
			   LCacheObjectCreator creator)
{
	lpointer value = l_get_cache(cache, key);
	if (NULL == value) {
		value = creator(key);
		l_put_cache(cache, key, value);
	}
    return value;
}
