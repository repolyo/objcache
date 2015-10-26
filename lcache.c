/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim: set expandtab shiftwidth=4 softtabstop=4 : */
/*
###############################################################################
  Copyright (C) 1998-2015 Lexmark International, Inc.  All rights reserved.
  Proprietary and Confidential.
###############################################################################
###########################  Lexmark Confidential  ############################
###############################################################################
*/

/**
 * A Generic object caching mechanism.
 * cache algorithms (also frequently called cache replacement algorithms or cache replacement policies) are
 * optimizing instructions—​or algorithms—​that a computer program or a hardware-maintained structure
 * can follow in order to manage a cache of information stored on the computer.
 * When the cache is full, the algorithm must choose which items to discard to make room for the new ones.
 *
 * @see https://en.wikipedia.org/wiki/Cache_algorithms
 * @defgroup LCache
 */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <execinfo.h>

#include "lmemory.h"
#include <llib/ldict.h>
#include "lcache.h"
#include "lhash.h"
#include "lthread.h"

/** \internal
 * The structure comprising a cache object container.
 */
struct _LCache
{
    LHash * storage;  /**< the table of lists in which the keys/values are stored */
    int length;
    int object_ttl;   /**< cache elemant time-to-live */
    int cleanup_delay;
    pthread_t lru_tid;
};

struct _LCacheItem
{
    lpointer value;
    time_t last_accessed;
};

typedef struct _LCacheItem LCacheItem;
typedef struct _LCacheItem* LCacheItemP;
typedef struct _LCache* LCacheP;
typedef void * (*ThreadProc) (void * arg);
//extern void _l_hash_dump(LHash * hash);

sem_t _gCleanupSem;
char *_gEvent = NULL;
void *_gData = NULL;
static pthread_t _gEventTID = 0;
static bool keep_going = true;

/**
 * Discards the least recently used items first.
 * - caching objects in memory
 * - object cached expiration (idle time and max life)
 * - cache timeouts
 */
static bool
_least_recently_used (lpointer key, lpointer value, lpointer user_data)
{
    bool success = true;
    time_t now;

    time(&now);
    LCacheP cache = user_data;
    int ttl = cache->object_ttl;
    LCacheItemP itemP = value;
    time_t elapsed = difftime(now, itemP->last_accessed);
    if (itemP != NULL && (ttl <= elapsed)) {
        char * k = (char*)key;
        if ( l_hash_remove(cache->storage, key) ) {
            cache->length--;
            fprintf(stdout, "%s: deleteKey: %s\n", __func__, k);
        }
        //Thread.yield();
    }
    return success;
}

static void
_cache_checker_thread(void *data)
{
    LCacheP cache = data;
    sem_init(&_gCleanupSem, 0,0);

    // setPriority(Thread.MIN_PRIORITY);
    while (keep_going) {
        // cleanup_delay
        sleep(cache->cleanup_delay);

        // TODO: critical section: possibly needed to lock if LCache is not thread-safe?
        l_hash_foreach(cache->storage, _least_recently_used, cache);
    }

    sem_destroy(&_gCleanupSem);
    return;
}

static void
del_value (lpointer data)
{
    l_free(data);
    data = NULL;
}

/**
 * Creates a new LCache object with integers for keys and values.
 *
 * @see l_hash_new_full()
 *
 * @returns a new LCache object.
 */

LCache * l_cache_new (LCache ** cache, int ttl, int cleanup) {
    int rc = 0;
    pthread_attr_t attr;
    LCacheP cacheP = l_calloc (sizeof (LCache), 1);
    if (!cacheP)
        return NULL;

    *cache = cacheP;
    cacheP->storage = l_hash_new_full (l_hash_int_hash_func,
            l_hash_int_equal_func, NULL, del_value);

    if (!cacheP->storage) {
        l_free (cacheP);
        return NULL;
    }
    cacheP->object_ttl = ttl;
    cacheP->cleanup_delay = cleanup;

    /* start thread */
    rc = pthread_attr_init(&attr);
    if (rc) {
        fprintf(stderr, "[cache] event dispatch thread init failed!\n");
        return cacheP;
    }

    if (NULL == getenv("LCACHE_NO_THREAD")) {
        do {
            rc = pthread_create(&_gEventTID, &attr,
                                (void *)_cache_checker_thread,
                                (void *)cacheP);

        } while (rc != 0 && errno == EINTR);
    }

    if (rc) {
        fprintf(stderr, "[cache] event dispatch thread create failed!\n");
    }
    pthread_attr_destroy(&attr);

//    keep_going = false;
    fprintf(stderr, "tanch@%s: cache->storage: %p\n", __func__, cacheP->storage);
    sleep(1);
    sem_post(&_gCleanupSem);

//    pthread_join(_gEventTID, NULL);
    return cacheP;
}

/**
 * Insert a new key/value pair into \e cache.
 *
 * Inserting a key that already exists in the cache will result in that
 * key/value pair being overwritten.
 *
 * @param hash the hash into which \e key and \e value should be inserted.
 * @param key the key to insert
 * @param value the value to insert
 * @return FALSE if out of memory. Otherwise return TRUE
 */
bool
l_cache_put (LCache ** cache, lpointer key, lpointer value)
{
    bool ret = false;
    LCacheItemP itemP = l_calloc (sizeof (LCacheItem), 1);
    if (NULL != itemP) {
        itemP->value = value;
        time (&itemP->last_accessed);
        ret = l_hash_insert((*cache)->storage, key, itemP);
        if (ret) (*cache)->length++;
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
l_cache_get (LCache ** cache,
               lconstpointer key)
{
    lpointer value = NULL;
    LCacheItemP pitem = (LCacheItemP)l_hash_lookup((*cache)->storage, key);
    if (NULL != pitem) {
        time (&pitem->last_accessed);
        value = pitem->value;
    }
    return value;
}

/**
 * Return total quantity of objects currently in \e cache.
 * Note, that stale (see options) items are returned as part of this item count.
 *
 * @param cache The LCache
 *
 * @returns cached items count.
 */
int
l_cache_get_length(LCache ** cache)
{
    return (*cache)->length;
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
l_cache_get_or_put (LCache ** cache,
        lpointer key,
        LCacheObjectCreator creator)
{
    lpointer value = l_cache_get(cache, key);
    if (NULL == value) {
        value = creator(key);
        l_cache_put(cache, key, value);
    }
    return value;
}


static bool
dumpCacheItem (lpointer key, lpointer value, lpointer user_data)
{
    L_UNUSED_VAR(user_data);
    char * k = (char*)key;
    int i = *(int*)value;

    fprintf(stdout, "tanch@%s: key: %s, value: %d \n", __func__, k, i);
    return false;
}

void
l_cache_dump (LCache ** cache)
{
    fprintf(stderr, "tanch@%s: cache: %p \n", __func__, *cache );
    if ( *cache ) {
        fprintf(stderr, "tanch@%s: cache->storage: %p\n",
                __func__, (*cache)->storage);
        l_hash_foreach((*cache)->storage, dumpCacheItem, cache);
    }
}

void
l_cache_destroy (LCache ** cache)
{
    /* Stop the main loop in the cleanup thread. */
    keep_going = false;

    if (_gEventTID) {
        sem_post(&_gCleanupSem);
        sleep(2);
        pthread_cancel(_gEventTID);
    }
    if (*cache) {
        l_hash_destroy((*cache)->storage);
        l_free (*cache);
    }
    *cache = NULL;
}
