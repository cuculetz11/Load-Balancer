/*
 * Copyright (c) 2024, <>
 */

#include "lru_cache.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

int compare_function_strings(void *a, void *b)
{
	char *str_a = (char *)a;
	char *str_b = (char *)b;

	return strcmp(str_a, str_b);
}

void key_val_free_function(void *data)
{
	info *key_val = (info *)data;
	free(key_val->key);
	free(key_val->value);
	free(key_val);
	key_val = NULL;
}

ll_node_t *ll_remove_nth_node(linked_list_t *list, unsigned int n)
{
	ll_node_t *prev, *curr;

	if (!list || !list->head) {
		return NULL;
	}

	/* n >= list->size - 1 inseamna eliminarea nodului de la finalul listei. */
	if (n > list->size - 1) {
		n = list->size - 1;
	}

	curr = list->head;
	prev = NULL;
	while (n > 0) {
		prev = curr;
		curr = curr->next;
		--n;
	}

	if (prev == NULL) {
		/* Adica n == 0. */
		list->head = curr->next;
	} else {
		prev->next = curr->next;
	}

	list->size--;

	return curr;
}

dll_node_t *add_in_dll_order_list(doubly_linked_list_t *list, void *data)
{
	dll_node_t *nod = calloc(1, sizeof(dll_node_t));
	nod->prev = NULL;
	DIE(nod == NULL, "malloc failed");
	nod->data = data;
	if (list->size == 0) {
		nod->next = NULL;
		list->tail = nod;
		list->head = nod;
	} else {
		nod->next = list->head;
		list->head->prev = nod;
		list->head = nod;
	}
	list->size++;

	return nod;
}

void remove_from_dll_order_list(doubly_linked_list_t *list)
{
	if (list->size != 0) {
		if (list->size == 1) {
			free(list->head);
			list->head = NULL;
			list->tail = NULL;
		} else {
			dll_node_t *nod_to_remove = list->tail;

			list->tail = nod_to_remove->prev;
			list->tail->next = NULL;


			free(nod_to_remove);
		}
		list->size--;
	}
}

int ht_has_key(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *aux = ht->buckets[index]->head;
	while (aux != NULL) {
		dll_node_t *nod_data = (dll_node_t *)aux->data;
		info *data = (info *)nod_data->data;
		if (ht->compare_function(data->key, key) == 0)
			return 1;
		aux = aux->next;
	}

	return 0;
}

dll_node_t *ht_get_nod(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *aux = ht->buckets[index]->head;
	while (aux != NULL) {
		dll_node_t *nod_data = (dll_node_t *)aux->data;
		info *data = (info *)nod_data->data;
		if (ht->compare_function(data->key, key) == 0)
			return nod_data;
		aux = aux->next;
	}

	return NULL;
}
void add_in_hashtable(hashtable_t *ht, dll_node_t *nod_dll_list)
{
	info *info_nod = (info *)nod_dll_list->data;
	void *key = info_nod->key;
	int index = ht->hash_function(key) % ht->hmax;

	ll_node_t *nod = calloc(1, sizeof(ll_node_t));
	DIE(!nod, "CALLOC FAILED");

	nod->data = nod_dll_list;

	if (ht_has_key(ht, key) == 1) {
		dll_node_t *nod_vechi = ht_get_nod(ht, key);
		info *info_vechi = (info *)nod_vechi->data;
		ht->key_val_free_function(info_vechi);
		free(nod_vechi);
		nod_vechi = nod_dll_list;
	} else {
		nod->next = ht->buckets[index]->head;
		ht->buckets[index]->head = nod;
		ht->buckets[index]->size++;
		ht->size++;
	}
}

void ht_remove_entry(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *aux = ht->buckets[index]->head;
	int n = 0;
	while (aux != NULL) {
		dll_node_t *nod_dll_list = (dll_node_t *)aux->data;
		info *data = (info *)nod_dll_list->data;
		if (ht->compare_function(data->key, key) == 0) {
			break;
		}

		n++;
		aux = aux->next;
	}
	ht->size--;
	ll_node_t *nod = ll_remove_nth_node(ht->buckets[index], n);
	free(nod);
}

void dll_free(doubly_linked_list_t **pp_list)
{
	dll_node_t *current = (*pp_list)->head;
	dll_node_t *next = NULL;
	while (current) {
		next = current->next;
		free(current->data);
		free(current);
		current = next;
	}
	free((*pp_list));
}

void ll_free(linked_list_t **list)
{
	ll_node_t *current = (*list)->head;
	ll_node_t *next = NULL;
	while (current) {
		next = current->next;
		free(current);
		current = next;
	}
	free((*list));
}

void ht_free(hashtable_t *ht)
{
	for (unsigned int i = 0; i < ht->hmax; i++) {
		ll_free(&ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
}

void printf_hashtable(hashtable_t *ht)
{
	for (unsigned int i = 0; i < ht->hmax; i++) {
		ll_node_t *aux = ht->buckets[i]->head;
		while (aux != NULL) {
			dll_node_t *nod = (dll_node_t *)aux->data;
			info *data = (info *)nod->data;
			printf("%s %s\n", (char *)data->key, (char *)data->value);
			aux = aux->next;
		}
	}
}

hashtable_t *ht_create(unsigned int hmax, unsigned int (*hash_function)(void *),
					   int (*compare_function)(void *, void *),
					   void (*key_val_free_function)(void *))
{
	hashtable_t *ht = calloc(1, sizeof(hashtable_t));
	DIE(!ht, "CALLOC FAILED");
	ht->size = 0;
	ht->hmax = hmax;
	ht->buckets = calloc(hmax, sizeof(linked_list_t *));
	DIE(!ht->buckets, "CALLOC FAILED");
	for (unsigned int i = 0; i < hmax; i++) {
		ht->buckets[i] = calloc(1, sizeof(linked_list_t));
		DIE(!ht->buckets[i], "CALLOC FAILED");
		ht->buckets[i]->head = NULL;
		ht->buckets[i]->size = 0;
	}
	ht->hash_function = hash_function;
	ht->compare_function = compare_function;
	ht->key_val_free_function = key_val_free_function;
	return ht;
}

void add_in_dll_list_fara_alocare(doubly_linked_list_t *list, dll_node_t *nod)
{
	nod->prev = NULL;
	if (list->size == 0) {
		nod->next = NULL;
		list->tail = nod;
		list->head = nod;
	} else {
		nod->next = list->head;
		list->head->prev = nod;
		list->head = nod;
	}
	list->size++;
}

void pun_in_ordine_cache(lru_cache *cache, void *key)
{
	dll_node_t *nod = ht_get_nod(cache->hashmap_cache, key);
	if (nod->prev) {
		if (cache->ordinea_memoriei_cache->tail == nod)
			cache->ordinea_memoriei_cache->tail = nod->prev;
		nod->prev->next = nod->next;
		if (nod->next)
			nod->next->prev = nod->prev;
		cache->ordinea_memoriei_cache->size--;
		add_in_dll_list_fara_alocare(cache->ordinea_memoriei_cache, nod);
	}
}
// functiile de care ma folosesc ^

lru_cache *init_lru_cache(unsigned int cache_capacity)
{
	lru_cache *cache = calloc(1, sizeof(lru_cache));
	cache->hashmap_cache = ht_create(cache_capacity, hash_string,
									 compare_function_strings, key_val_free_function);
	cache->capacitate = cache_capacity;
	cache->size = 0;
	cache->ordinea_memoriei_cache = calloc(1, sizeof(doubly_linked_list_t));
	DIE(!cache->ordinea_memoriei_cache, "CALLOC FAILED");
	cache->ordinea_memoriei_cache->size = 0;
	cache->ordinea_memoriei_cache->head = NULL;
	cache->ordinea_memoriei_cache->tail = NULL;
	return cache;
}

bool lru_cache_is_full(lru_cache *cache)
{
	if (cache->size == cache->capacitate)
		return true;
	return false;
}

void free_lru_cache(lru_cache **cache)
{
	ht_free((*cache)->hashmap_cache);
	dll_free(&(*cache)->ordinea_memoriei_cache);
	free(*cache);
	*cache = NULL;
}

bool lru_cache_put(lru_cache *cache, void *key, void *value,
				   void **evicted_key)
{
	info *info_adaugat_dll_list = calloc(1, sizeof(info));
	info_adaugat_dll_list->key = key;
	info_adaugat_dll_list->value = value;  // info-ul din baza de date

	if (lru_cache_is_full(cache) == true &&
		ht_has_key(cache->hashmap_cache, key) == 0) {
		// verific daca memoria cache este plina si daca key-ul nu exista in cache
		info *eliminat = cache->ordinea_memoriei_cache->tail->data;

		*evicted_key = eliminat->key;

		ht_remove_entry(cache->hashmap_cache, eliminat->key);
		remove_from_dll_order_list(cache->ordinea_memoriei_cache);
		dll_node_t *nod = add_in_dll_order_list(cache->ordinea_memoriei_cache,
												info_adaugat_dll_list);
		add_in_hashtable(cache->hashmap_cache, nod);
		// adaug in hashtable si in ordinea memoriei noul document
		free(eliminat);

		return true;
	}
	if (ht_has_key(cache->hashmap_cache, key) == 1) {
		dll_node_t *nod_vechi = ht_get_nod(cache->hashmap_cache, key);
		// daca cheia exista in cache, suprascriu valoarea si il pun in ordine

		info *info_vechi = (info *)nod_vechi->data;
		info *info_vechi_data = info_vechi->value;
		free(info_vechi_data->value);
		info_vechi_data->value = value;
		free(info_adaugat_dll_list);
		free(key);

		if (nod_vechi->prev) {
			if (cache->ordinea_memoriei_cache->tail == nod_vechi)
				cache->ordinea_memoriei_cache->tail = nod_vechi->prev;

			nod_vechi->prev->next = nod_vechi->next;

			if (nod_vechi->next)
				nod_vechi->next->prev = nod_vechi->prev;

			cache->ordinea_memoriei_cache->size--;
			add_in_dll_list_fara_alocare(cache->ordinea_memoriei_cache,
										 nod_vechi);
		}
		return false;
	}

	dll_node_t *nod = add_in_dll_order_list(cache->ordinea_memoriei_cache,
											info_adaugat_dll_list);
	add_in_hashtable(cache->hashmap_cache, nod);
	cache->size++;
	return true;
}

void *lru_cache_get(lru_cache *cache, void *key)
{
	int index = hash_string(key) %
				cache->hashmap_cache->hmax;

	ll_node_t *aux = cache->hashmap_cache->buckets[index]->head;
	while (aux != NULL) {
		dll_node_t *nod_dll_list = (dll_node_t *)aux->data;
		info *data = (info *)nod_dll_list->data;
		if (compare_function_strings(data->key, key) == 0) {
			info *info_data = (info *)data->value;
			return info_data->value;
		}
		aux = aux->next;
	}
	return NULL;
}
void lru_cache_remove(lru_cache *cache, void *key)
{
	int index = cache->hashmap_cache->hash_function(key) %
				cache->hashmap_cache->hmax;
	ll_node_t *aux = cache->hashmap_cache->buckets[index]->head;
	int n = 0;
	while (aux != NULL) {
		dll_node_t *nod_dll_list = (dll_node_t *)aux->data;
		info *data = (info *)nod_dll_list->data;
		if (cache->hashmap_cache->compare_function(data->key, key) == 0) {
			break;
		}

		n++;
		aux = aux->next;
	}
	cache->size--;
	ll_node_t *nod = ll_remove_nth_node(cache->hashmap_cache->buckets[index], n);
	free(nod);
}
