/*
 * Copyright (c) 2024, <>
 */

#include "server.h"
#include "lru_cache.h"
#include <stdio.h>

#include "utils.h"

queue_t *
q_create(unsigned int data_size, unsigned int max_size)
{
	queue_t *q = calloc(1, sizeof(queue_t));
	DIE(q == NULL, "malloc failed");
	q->data_size = data_size;
	q->max_size = max_size;
	q->read_idx = 0;
	q->write_idx = 0;
	q->size = 0;
	q->buff = calloc(max_size, sizeof(void *));
	DIE(!q->buff, "calloc failed");

	return q;
}

/*
 * Functia intoarce numarul de elemente din coada al carei pointer este trimis
 * ca parametru.
 */
unsigned int
q_get_size(queue_t *q)
{
	return q->size;
}

unsigned int
q_is_empty(queue_t *q)
{
	if (q->size == 0)
		return 0;
	return 1;
}

/*
 * Functia intoarce primul element din coada, fara sa il elimine.
 */
void *
q_front(queue_t *q)
{
	return q->buff[q->read_idx];
}

/*
 * Functia scoate un element din coada. Se va intoarce 1 daca operatia s-a
 * efectuat cu succes (exista cel putin un element pentru a fi eliminat) si
 * 0 in caz contrar.
 */
int q_dequeue(queue_t *q)
{
	if (!q_is_empty(q)) {
		return 0;
	} else {
		// free(q->buff[q->read_idx]);
		q->read_idx = (q->read_idx + 1) % q->max_size;
		q->size--;
	}

	return 1;
}

/*
 * Functia introduce un nou element in coada. Se va intoarce 1 daca
 * operatia s-a efectuat cu succes (nu s-a atins dimensiunea maxima)
 * si 0 in caz contrar.
 */
int q_enqueue(queue_t *q, void *new_data)
{
	if (q->size == q->max_size) {
		return 0;
	} else {
		q->buff[q->write_idx] = new_data;

		q->write_idx = (q->write_idx + 1) % q->max_size;
		q->size++;
		return 1;
	}
}

/*
 * Functia elimina toate elementele din coada primita ca parametru.
 */
void q_clear(queue_t *q)
{
	while (q != NULL && q->size > 0)
		q_dequeue(q);
}

/*
 * Functia elibereaza toata memoria ocupata de coada.
 */
void q_free(queue_t *q)
{
	free(q->buff);
	free(q);
}

info *
ht_get_info(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *aux = ht->buckets[index]->head;
	while (aux != NULL) {
		info *data = (info *)aux->data;
		if (ht->compare_function(data->key, key) == 0)
			return data;

		aux = aux->next;
	}

	return NULL;
}

void ht_resize(hashtable_t *ht, int new_hmax)
{
	linked_list_t **new_buckets = calloc(new_hmax, sizeof(linked_list_t *));
	DIE(new_buckets == NULL, "calloc failed");

	// Initializare noi buckets
	for (int i = 0; i < new_hmax; i++) {
		new_buckets[i] = calloc(1, sizeof(linked_list_t));
		DIE(new_buckets[i] == NULL, "calloc failed");
	}

	// Transferul elementelor din bucket-urile vechi în cele noi
	for (unsigned int i = 0; i < ht->hmax; i++) {
		ll_node_t *aux = ht->buckets[i]->head;
		while (aux != NULL) {
			info *data = (info *)aux->data;
			int index = ht->hash_function(data->key) % new_hmax;
			ll_node_t *nod = malloc(sizeof(ll_node_t));
			nod->data = data;
			nod->next = new_buckets[index]->head;
			new_buckets[index]->head = nod;
			new_buckets[index]->size++;
			aux = aux->next;
		}
	}

	// Eliberarea bucket-urilor vechi și a nodurilor asociate
	for (unsigned int i = 0; i < ht->hmax; i++) {
		ll_node_t *aux = ht->buckets[i]->head;
		while (aux != NULL) {
			ll_node_t *temp = aux;
			aux = aux->next;
			free(temp);
		}
		free(ht->buckets[i]);
	}
	free(ht->buckets);

	// Actualizarea hashtable cu noile valori
	ht->buckets = new_buckets;
	ht->hmax = new_hmax;
}

int ht_has_key_s(hashtable_t *ht, void *key)
{
	int index = ht->hash_function(key) % ht->hmax;
	ll_node_t *aux = ht->buckets[index]->head;
	while (aux != NULL) {
		info *data = (info *)aux->data;
		if (data->key && ht->compare_function(data->key, key) == 0)
			return 1;
		aux = aux->next;
	}
	return 0;
}
info *ht_put(hashtable_t *ht, void *key, unsigned int key_size,
			 void *value, unsigned int value_size)
{
	int index = hash_string(key) % ht->hmax;
	info *key_val = calloc(1, sizeof(info));
	key_val->key = calloc(1, key_size);
	key_val->value = calloc(1, value_size);
	memcpy(key_val->key, key, key_size - 1);
	memcpy(key_val->value, value, value_size - 1);

	ll_node_t *nod = calloc(1, sizeof(ll_node_t));
	nod->data = key_val;
	nod->next = NULL;

	if (ht->buckets[index]->size == 0) {
		ht->buckets[index]->head = nod;
		ht->buckets[index]->size++;
		ht->size++;
		if (ht->size >= ht->hmax)
			ht_resize(ht, ht->hmax * 2);
	} else {
		if (ht_has_key_s(ht, key) == 1) {
			ll_node_t *aux = ht->buckets[index]->head;
			while (aux != NULL) {
				info *data = (info *)aux->data;
				if (ht->compare_function(data->key, key) == 0) {
					key_val_free_function(data);
					free(nod);
					aux->data = key_val;
					break;
				}
				aux = aux->next;
			}
		} else {
			ht->size++;
			nod->next = ht->buckets[index]->head;
			ht->buckets[index]->head = nod;
			ht->buckets[index]->size++;
		}
	}
	return key_val;
}

void ht_free1(hashtable_t *ht)
{
	for (unsigned int i = 0; i < ht->hmax; i++) {
		ll_node_t *aux = ht->buckets[i]->head;
		while (aux != NULL) {
			ll_node_t *aux2 = aux;
			aux = aux->next;
			info *data = (info *)aux2->data;
			key_val_free_function(data);
			free(aux2);
		}
		free(ht->buckets[i]);
	}
	free(ht->buckets);
	free(ht);
}

// functiile ajutatoare ^

response *server_edit_document(server *s, char *doc_name, char *doc_content)
{
	response *raspuns = calloc(1, sizeof(response));
	DIE(!raspuns, "calloc failed");

	raspuns->server_id = s->id;
	raspuns->server_response = calloc(MAX_RESPONSE_LENGTH, sizeof(char));
	raspuns->server_log = calloc(MAX_LOG_LENGTH, sizeof(char));
	info *ht_info = NULL;
	void *key_evicted = NULL;

	char *doc_name_copy = calloc(1, strlen(doc_name) + 1);
	DIE(!doc_name_copy, "calloc failed");
	memcpy(doc_name_copy, doc_name, strlen(doc_name));
	char *doc_content_copy = calloc(1, strlen(doc_content) + 1);
	DIE(!doc_content_copy, "calloc failed");
	memcpy(doc_content_copy, doc_content, strlen(doc_content));
	void *data = lru_cache_get(s->cache, doc_name_copy);
	// verific daca exista documentul in cache
	if (data) {
		lru_cache_put(s->cache, doc_name_copy, doc_content_copy, &key_evicted);

		sprintf(raspuns->server_response, MSG_B, doc_name);
		sprintf(raspuns->server_log, LOG_HIT, doc_name);
		return raspuns;
	}

	if (ht_has_key_s(s->database, doc_name) == 1) {
		ht_info = ht_put(s->database, doc_name, strlen(doc_name) + 1,
						 doc_content, strlen(doc_content) + 1);
		// il pun in baza de date(unde se suprascrie)
		lru_cache_put(s->cache, ht_info->key, ht_info, &key_evicted);
		// il pun in cache
		free(doc_content_copy);
		free(doc_name_copy);

		if (key_evicted == NULL) {
			sprintf(raspuns->server_response, MSG_B, doc_name);
			sprintf(raspuns->server_log, LOG_MISS, doc_name);
			return raspuns;

		} else if (key_evicted != NULL) {
			sprintf(raspuns->server_response, MSG_B, doc_name);
			sprintf(raspuns->server_log, LOG_EVICT, doc_name,
					(char *)key_evicted);
			return raspuns;
		}
	} else {
		// daca nu se afla in baza de date, il adaug
		sprintf(raspuns->server_response, MSG_C, doc_name);
		ht_info = ht_put(s->database, doc_name, strlen(doc_name) + 1,
				  doc_content, strlen(doc_content) + 1);
		/*iar in cache pun ca si key doc_name, iar ca valoare ht_info(ce
		reprezinta key si value in baza de date), astfel modific cu usurinta
		direct din cache*/
		lru_cache_put(s->cache, ht_info->key, ht_info, &key_evicted);

		if (key_evicted != NULL)

			sprintf(raspuns->server_log, LOG_EVICT, doc_name,
					(char *)key_evicted);
		else
			sprintf(raspuns->server_log, LOG_MISS, doc_name);
	}
	free(doc_name_copy);
	free(doc_content_copy);
	return raspuns;
}

static response *server_get_document(server *s, char *doc_name)
{
	response *raspuns = calloc(1, sizeof(response));
	DIE(!raspuns, "calloc failed");

	raspuns->server_id = s->id;
	raspuns->server_response = calloc(MAX_RESPONSE_LENGTH, sizeof(char));
	raspuns->server_log = calloc(MAX_LOG_LENGTH, sizeof(char));
	info *ht_info;
	void *key_evicted = NULL;

	void *value = lru_cache_get(s->cache, doc_name);
	// verific daca exista in cache
	if (value) {
		memcpy(raspuns->server_response, value, strlen((char *)value));
		pun_in_ordine_cache(s->cache, doc_name);
		// documentul fiind accesat, il pun in ordinea de accesare
		sprintf(raspuns->server_log, LOG_HIT, doc_name);
		return raspuns;
	}
	if (ht_has_key_s(s->database, doc_name) == 1) {
		ht_info = ht_get_info(s->database, doc_name);
		char *string = ht_info->value;
		memcpy(raspuns->server_response, string,
			   strlen((char *)ht_info->value));
		lru_cache_put(s->cache, ht_info->key, ht_info, &key_evicted);
		// il pun in cache
		if (key_evicted != NULL)
			sprintf(raspuns->server_log, LOG_EVICT, doc_name,
					(char *)key_evicted);
		else
			sprintf(raspuns->server_log, LOG_MISS, doc_name);
		return raspuns;

	} else {
		char *MSG_D = "(null)";
		sprintf(raspuns->server_response, MSG_D, doc_name);
		sprintf(raspuns->server_log, LOG_FAULT, doc_name);
		return raspuns;
	}
}

server *init_server(unsigned int cache_size, int server_id)
{
	server *s = calloc(1, sizeof(server));
	DIE(!s, "server's malloc failed");
	s->cache = init_lru_cache(cache_size);
	s->tasks_edit = q_create(sizeof(request), TASK_QUEUE_SIZE);
	s->database = ht_create(SERVER_HMAX_INITAL, hash_string,
				  compare_function_strings, key_val_free_function);
	/* baza de date am facut o un hashmap ce este redimensionabil deoarece
	 e eficenta aceasta implementare*/
	s->id = server_id;
	return s;
}

response *server_handle_request(server *s, request *req)
{
	if (req->type == EDIT_DOCUMENT) {
		response *raspuns = calloc(1, sizeof(response));
		DIE(!raspuns, "calloc failed");
		raspuns->server_id = s->id;
		raspuns->server_response = calloc(MAX_RESPONSE_LENGTH, sizeof(char));
		raspuns->server_log = calloc(MAX_LOG_LENGTH, sizeof(char));

		sprintf(raspuns->server_response, MSG_A, EDIT_REQUEST, req->doc_name);

		request *req_copy = calloc(1, sizeof(request));
		// fac o copie pe care o bag in coada
		req_copy->type = req->type;
		req_copy->doc_name = calloc(1, strlen(req->doc_name) + 1);
		DIE(!req_copy->doc_name, "calloc failed");
		req_copy->doc_content = calloc(1, strlen(req->doc_content) + 1);
		DIE(!req_copy->doc_content, "calloc failed");
		memcpy(req_copy->doc_name, req->doc_name, strlen(req->doc_name));
		memcpy(req_copy->doc_content, req->doc_content,
			   strlen(req->doc_content));

		q_enqueue(s->tasks_edit, req_copy);

		sprintf(raspuns->server_log, LOG_LAZY_EXEC, s->tasks_edit->size);

		return raspuns;
	} else if (req->type == GET_DOCUMENT) {
		while (s->tasks_edit->size > 0) {
			request *req_edit = (request *)q_front(s->tasks_edit);
			response *raspuns_edit = server_edit_document(s,
									 req_edit->doc_name,
									 req_edit->doc_content);

			PRINT_RESPONSE(raspuns_edit);
			free(req_edit->doc_name);
			free(req_edit->doc_content);
			free(req_edit);
			q_dequeue(s->tasks_edit);
		}

		response *raspuns_get = server_get_document(s, req->doc_name);
		return raspuns_get;
	}
	return NULL;
}

void free_server(server **s)
{
	while ((*s)->tasks_edit->size > 0) {
		request *req_edit = (request *)q_front((*s)->tasks_edit);

		free(req_edit->doc_name);
		free(req_edit->doc_content);
		free(req_edit);
		q_dequeue((*s)->tasks_edit);
	}

	free_lru_cache(&(*s)->cache);
	q_free((*s)->tasks_edit);
	ht_free1((*s)->database);
	free(*s);
	*s = NULL;
}
