/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim: set expandtab shiftwidth=4 softtabstop=4 : */

/* Copyright (C) 2004-2015 Lexmark International, Inc.  All rights reserved. */

#ifndef LOBJCACHE_H_
#define LOBJCACHE_H_

/* lobjcache.h -- Stack declaration and function prototypes:  */
#include <llib/lvaluetypes.h>
#include <llib/ltypes.h>
#include <llib/lmacros.h>

L_BEGIN_DECLS

/** A Generic object caching mechanism.
 * cache algorithms (also frequently called cache replacement algorithms or cache replacement policies) are optimizing instructions—​or
 * algorithms—​that a computer program or a hardware-maintained structure can follow in order to manage a cache of
 * information stored on the computer. When the cache is full, the algorithm must choose which items to
 * discard to make room for the new ones.
 *
 * @see https://en.wikipedia.org/wiki/Cache_algorithms
 * @addtogroup LCache
 * @{
 */
typedef enum
{
    L_CACHE_LRU,    /**< Least Recently Used (LRU). Discards the least recently used items first.
                     * This algorithm requires keeping track of what was used when, which is
                     * expensive if one wants to make sure the algorithm always discards
                     * the least recently used item. */
    L_CACHE_MRU,    /**< Most Recently Used (MRU).
                         * Discards, in contrast to LRU, the most recently used items first.
                         * MRU algorithms are most useful in situations where the older an
                         * item is, the more likely it is to be accessed. */
    L_CACHE_PLRU,   /**< Pseudo-LRU (PLRU).
                         * For CPU caches with large associativity (generally >4 ways), the implementation
                         * cost of LRU becomes prohibitive. In many CPU caches, a scheme that almost always
                         * discards one of the least recently used items is sufficient.
                         * So many CPU designers choose a PLRU algorithm which only needs
                         * one bit per cache item to work.
                         * PLRU typically has a slightly worse miss ratio, has a slightly better latency,
                         * and uses slightly less power than LRU. */
    L_CACHE_RR,     /**< Random Replacement (RR)
                         * Randomly selects a candidate item and discards it to make space when necessary.
                         * This algorithm does not require keeping any information about the access history. */
    L_CACHE_SLRU,   /**< Segmented LRU (SLRU) An SLRU cache is divided into two segments, a probationary segment
                         * and a protected segment. Lines in each segment are ordered from the most to the least recently accessed.
                         * Data from misses is added to the cache at the most recently accessed end of the probationary segment.
                         *  Hits are removed from wherever they currently reside and added to the
                         *  most recently accessed end of the protected segment.
                         *  Lines in the protected segment have thus been accessed at least twice.  */
    L_CACHE_LFU,    /**< Least-Frequently Used (LFU)
                         * Counts how often an item is needed. Those that are used least often are discarded first.*/
    L_CACHE_LIRS,   /**< Low Inter-reference Recency Set (LIRS). A page replacement algorithm with an improved performance
                         * over LRU and many other newer replacement algorithms.
                         * This is achieved by using reuse distance as a metric for dynamically ranking accessed pages to make
                         * a replacement decision. The algorithm was developed by Song Jiang and Xiaodong Zhang. */
    L_CACHE_ARC,    /**< Adaptive Replacement Cache (ARC)
                         * Constantly balances between LRU and LFU, to improve the combined result.
                         * ARC improves on SLRU by using information about recently-evicted cache items to dynamically
                         * adjust the size of the protected segment and the probationary segment to make the best use of the
                         * available cache space. */
    L_CACHE_CAR,    /**< Clock with Adaptive Replacement (CAR)
                         * Combines Adaptive Replacement Cache (ARC) and CLOCK. CAR has performance comparable to ARC, and substantially
                         * outperforms both LRU and CLOCK. Like ARC, CAR is self-tuning and requires no user-specified magic parameters.
 */
} LCacheType;

/* types */
/* Callback Functions */
typedef lpointer (*LCacheObjectCreator) (lconstpointer key);

/** An opaque cache object container */
typedef struct _LCache LCache;
typedef struct _Thread Thread;

/* cache methods */

LCache * l_cache_new (LCache ** cache, int ttl, int cleanup);
void l_cache_destroy (LCache ** cache);

bool l_cache_put (LCache ** cache, lpointer key, lpointer value);
lpointer l_cache_get (LCache ** cache, lconstpointer key);
lpointer l_cache_get_or_put (LCache ** cache, lpointer key, LCacheObjectCreator creator);

int l_cache_get_length(LCache ** cache);
void l_cache_dump(LCache ** cache);

/* helper funcs */

/* @} */

L_END_DECLS

#endif /* LOBJCACHE_H_ */
