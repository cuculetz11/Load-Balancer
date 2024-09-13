/*
 * Copyright (c) 2024, <>
 */

#include "load_balancer.h"
#include "server.h"

void scot_din_database(server *server, char *doc_name)
{
	int index = server->database->hash_function(doc_name) %
				server->database->hmax;
	ll_node_t *aux = server->database->buckets[index]->head;
	ll_node_t *prev = NULL;
	while (aux) {
		info *data = (info *)aux->data;
		if (strcmp((char *)data->key, doc_name) == 0) {
			if (prev == NULL) {
				server->database->buckets[index]->head = aux->next;
			} else {
				prev->next = aux->next;
			}
			server->database->size--;
			free(data->key);
			free(data->value);
			free(data);
			free(aux);
			break;
		}
		prev = aux;
		aux = aux->next;
	}
}

void adugare_in_database(server *server, char *doc_name, char *doc_content)
{
	ht_put(server->database, doc_name, strlen(doc_name) + 1,
		   doc_content, strlen(doc_content) + 1);
	free(doc_content);
	free(doc_name);
}

load_balancer *init_load_balancer(bool enable_vnodes)
{
	load_balancer *main = malloc(sizeof(load_balancer));
	DIE(main == NULL, "malloc failed");

	main->servers_count = 0;
	main->enable_vnodes = enable_vnodes;
	main->hash_function_servers = hash_uint;
	main->hash_function_docs = hash_string;

	return main;
}

void loader_add_server(load_balancer *main, int server_id, int cache_size)
{
	main->servers[main->servers_count] = init_server(cache_size, server_id);
	main->servers_count++;
	hash_ring_t *hash_ring = calloc(main->servers_count, sizeof(hash_ring_t));
	DIE(hash_ring == NULL, "calloc failed");
	for (int i = 0; i < main->servers_count; i++) {
		void *copy_id = malloc(sizeof(int));
		DIE(copy_id == NULL, "malloc failed");
		memcpy(copy_id, &main->servers[i]->id, sizeof(int));
		hash_ring[i].hash = main->hash_function_servers(copy_id);
		hash_ring[i].server = main->servers[i];
		free(copy_id);
	}
	// am facut un vector de structuri care retine serverul si hash-ul

	for (int i = 0; i < main->servers_count - 1; i++) {
		for (int j = i + 1; j < main->servers_count; j++) {
			if (hash_ring[i].hash > hash_ring[j].hash) {
				hash_ring_t aux = hash_ring[i];
				hash_ring[i] = hash_ring[j];
				hash_ring[j] = aux;
			}
		}
	}
	// am sortat vectorul de structuri dupa hash

	int index_hash_ring_dest = -1;
	for (int i = 0; i < main->servers_count; i++) {
		if (hash_ring[i].server == main->servers[main->servers_count - 1]) {
			index_hash_ring_dest = i;
			break;
		}
	}
	server *server_destinatie = main->servers[main->servers_count - 1];

	int index_hash_ring_sursa = index_hash_ring_dest + 1;
	if (index_hash_ring_dest == main->servers_count - 1) {
		index_hash_ring_sursa = 0;
	}

	server *server_sursa = hash_ring[index_hash_ring_sursa].server;
	if (server_sursa->tasks_edit->size > 0) {
		while (server_sursa->tasks_edit->size > 0) {
			request *req_ed = (request *)q_front(server_sursa->tasks_edit);
			response *raspuns_edit = server_edit_document(server_sursa,
														  req_ed->doc_name, req_ed->doc_content);
			PRINT_RESPONSE(raspuns_edit);
			free(req_ed->doc_name);
			free(req_ed->doc_content);
			free(req_ed);
			q_dequeue(server_sursa->tasks_edit);
		}
	}
	// am facut toate task-urile de editare de pe serverul sursa

	char **vector_doc_name = calloc(server_sursa->database->size, sizeof(char *));
	DIE(vector_doc_name == NULL, "calloc failed");
	int index = 0;
	for (unsigned int i = 0; i < server_sursa->database->hmax; i++) {
		ll_node_t *aux = server_sursa->database->buckets[i]->head;
		while (aux) {
			info *data = (info *)aux->data;
			if (data == NULL) {
				aux = aux->next;
				continue;
			}
			vector_doc_name[index] = calloc(1, strlen((char *)data->key) + 1);
			DIE(vector_doc_name[index] == NULL, "calloc failed");
			memcpy(vector_doc_name[index], (char *)data->key,
				   strlen((char *)data->key) + 1);
			index++;
			aux = aux->next;
		}
	}
	// am facut un vector de nume de documente de pe serverul sursa
	long long hash_server_dest = hash_ring[index_hash_ring_dest].hash;

	int lungime = index;
	for (int i = 0; i < lungime; i++) {
		if (vector_doc_name[i] == NULL) {
			continue;
		}
		long long hash_doc = main->hash_function_docs(vector_doc_name[i]);
		if (hash_doc < hash_server_dest || (index_hash_ring_sursa == 0 &&
											hash_doc > hash_server_dest)) {
			info *data = ht_get_info(server_sursa->database, vector_doc_name[i]);
			char *doc_content = calloc(1, strlen((char *)data->value) + 1);
			DIE(doc_content == NULL, "calloc failed");
			memcpy(doc_content, (char *)data->value, strlen((char *)data->value) + 1);
			char *doc_name = calloc(1, strlen((char *)data->key) + 1);
			DIE(doc_name == NULL, "calloc failed");
			memcpy(doc_name, (char *)data->key, strlen((char *)data->key) + 1);
			// adugare_in_database(server_destinatie, doc_name, doc_content);
			ht_put(server_destinatie->database, doc_name, strlen(doc_name) + 1,
				   doc_content, strlen(doc_content) + 1);
			lru_cache_remove(server_sursa->cache, doc_name);

			scot_din_database(server_sursa, vector_doc_name[i]);
			free(doc_name);
			free(doc_content);
		}
	}

	for (int i = 0; i < lungime; i++) {
		free(vector_doc_name[i]);
	}
	free(vector_doc_name);

	free(hash_ring);
}

void loader_remove_server(load_balancer *main, int server_id)
{
	int index = -1;
	for (int i = 0; i < main->servers_count; i++) {
		if (main->servers[i]->id == server_id) {
			index = i;
			break;
		}
	}

	while (main->servers[index]->tasks_edit->size > 0) {
		request *req_ed = (request *)q_front(main->servers[index]->tasks_edit);
		// printf("req_ed->doc_name:%s\n", req_ed->doc_name);
		response *raspuns_edit = server_edit_document(main->servers[index],
													  req_ed->doc_name, req_ed->doc_content);

		PRINT_RESPONSE(raspuns_edit);
		free(req_ed->doc_name);
		free(req_ed->doc_content);
		free(req_ed);
		q_dequeue(main->servers[index]->tasks_edit);
	}
	int index_dest = -1;
	int index_mic = -1;
	long long minn = 9999999999, minn_mic = 99999999999;
	long long hash_server_sursa;
	void *copy_id1 = malloc(sizeof(int));
	DIE(copy_id1 == NULL, "malloc failed");
	memcpy(copy_id1, &main->servers[index]->id, sizeof(int));
	hash_server_sursa = main->hash_function_servers(copy_id1);
	free(copy_id1);

	for (int i = 0; i < main->servers_count; i++) {
		void *copy_id = malloc(sizeof(int));
		DIE(copy_id == NULL, "malloc failed");
		memcpy(copy_id, &main->servers[i]->id, sizeof(int));
		long long hash_server_dest = main->hash_function_servers(copy_id);

		if (hash_server_sursa < hash_server_dest) {
			if (hash_server_dest < minn) {
				minn = hash_server_dest;
				index_dest = i;
			}
		}
		if (hash_server_dest < minn_mic) {
			minn_mic = hash_server_dest;
			index_mic = i;
		}

		free(copy_id);
	}

	if (index_dest == -1) {
		index_dest = index_mic;
	}

	for (unsigned int i = 0; i < main->servers[index]->database->hmax; i++) {
		ll_node_t *aux = main->servers[index]->database->buckets[i]->head;
		while (aux != NULL) {
			info *data = (info *)aux->data;
			char *doc_name = calloc(1, strlen((char *)data->key) + 1);
			DIE(doc_name == NULL, "calloc failed");
			memcpy(doc_name, (char *)data->key, strlen((char *)data->key) + 1);
			char *doc_content = calloc(1, strlen((char *)data->value) + 1);
			DIE(doc_content == NULL, "calloc failed");
			memcpy(doc_content, (char *)data->value,
				   strlen((char *)data->value) + 1);

			adugare_in_database(main->servers[index_dest],
								doc_name, doc_content);
			aux = aux->next;
		}
	}
	free_server(&main->servers[index]);
	for (int i = index; i < main->servers_count - 1; i++) {
		main->servers[i] = main->servers[i + 1];
	}
	main->servers_count--;
}

response *loader_forward_request(load_balancer *main, request *req)
{
	long long hash_doc = main->hash_function_docs(req->doc_name);
	long long hash_server;
	long long minn = 999999999999999, minn_mic = 999999999999999;
	int index_hash_mic;
	int index = -1;
	for (int i = 0; i < main->servers_count; i++) {
		void *copy_id = malloc(sizeof(int));
		DIE(copy_id == NULL, "malloc failed");
		memcpy(copy_id, &main->servers[i]->id, sizeof(int));
		hash_server = main->hash_function_servers(copy_id);
		// printf("hash_server:%lld\n", hash_server);
		free(copy_id);
		if (hash_doc < hash_server && hash_server < minn) {
			minn = hash_server;
			index = i;
		}
		if (hash_server < minn_mic) {
			minn_mic = hash_server;
			index_hash_mic = i;
		}
	}
	if (index == -1) {
		index = index_hash_mic;
	}

	return server_handle_request(main->servers[index], req);
}

void free_load_balancer(load_balancer **main)
{
	for (int i = 0; i < (*main)->servers_count; i++) {
		free_server(&(*main)->servers[i]);
	}
	free(*main);

	*main = NULL;
}
