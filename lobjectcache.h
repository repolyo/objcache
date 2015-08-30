/* -*- mode: c; indent-tabs-mode: nil; c-basic-offset: 4; -*- */
/* vim: set expandtab shiftwidth=4 softtabstop=4 : */

#ifndef __L_OBJCACHE_H__
#define __L_OBJCACHE_H__

#include <time.h>
#include <llib/lmacros.h>
#include <llib/ltypes.h>

L_BEGIN_DECLS

/**
 * @addtogroup LCache
 * @{
 */

/* Callback Functions */
typedef lpointer (*LCacheObjectCreator) (lconstpointer key);

/* types */

/** An opaque cache object container */
typedef struct _LCache LCache;

/* hash methods */

LCache * l_get_cache_instance (void);
LCache * l_cache_new (time_t ttl, time_t cleanup);

bool l_put_cache (LCache * cache, lconstpointer key, lpointer value);
lpointer l_get_cache (LCache * cache, lconstpointer key);
lpointer l_get_or_create_cache (LCache * cache, lconstpointer key, LCacheObjectCreator creator);

/* helper funcs */

/* @} */

L_END_DECLS

#endif /* __L_OBJCACHE_H__ */
