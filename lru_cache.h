/*
 * Copyright (c) 2024, <>
 */

#ifndef LRU_CACHE_H
#define LRU_CACHE_H

#include <stdbool.h>


typedef struct dll_node_t
{
	void* data;
	struct dll_node_t *prev, *next;
} dll_node_t;

typedef struct doubly_linked_list_t
{
	dll_node_t* head;
    dll_node_t* tail;
	unsigned int size;
} doubly_linked_list_t;

typedef struct ll_node_t
{
    void* data;
    struct ll_node_t* next;
} ll_node_t;

typedef struct linked_list_t
{
    ll_node_t* head;
    unsigned int size;
} linked_list_t;

typedef struct info info;
struct info {
	void *key;
	void *value;
};

typedef struct hashtable_t hashtable_t;
struct hashtable_t {
	linked_list_t **buckets;
	unsigned int size;
	unsigned int hmax;
	unsigned int (*hash_function)(void*);
	int (*compare_function)(void*, void*);
	void (*key_val_free_function)(void*);
};

// toate structurile sunt luate din laburi


typedef struct lru_cache {
    doubly_linked_list_t *ordinea_memoriei_cache;
    hashtable_t *hashmap_cache;
    int size;
    int capacitate;
} lru_cache;

lru_cache *init_lru_cache(unsigned int cache_capacity);

bool lru_cache_is_full(lru_cache *cache);

void free_lru_cache(lru_cache **cache);

/**
 * lru_cache_put() - Adds a new pair in our cache.
 * 
 * @param cache: Cache where the key-value pair will be stored.
 * @param key: Key of the pair.
 * @param value: Value of the pair.
 * @param evicted_key: The function will RETURN via this parameter the
 *      key removed from cache if the cache was full.
 * 
 * @return - true if the key was added to the cache,
 *      false if the key already existed.
 */
bool lru_cache_put(lru_cache *cache, void *key, void *value,
                   void **evicted_key);

/**
 * lru_cache_get() - Retrieves the value associated with a key.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
 * 
 * @return - The value associated with the key,
 *      or NULL if the key is not found.
 */
void *lru_cache_get(lru_cache *cache, void *key);

/**
 * lru_cache_remove() - Removes a key-value pair from the cache.
 * 
 * @param cache: Cache where the key-value pair is stored.
 * @param key: Key of the pair.
*/
void lru_cache_remove(lru_cache *cache, void *key);

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void*),
		int (*compare_function)(void*, void*),
		void (*key_val_free_function)(void*));

int compare_function_strings(void *a, void *b);

void key_val_free_function(void *data);
void pun_in_ordine_cache(lru_cache *cache, void *key);
int ht_has_key(hashtable_t *ht, void *key);
#endif /* LRU_CACHE_H */
