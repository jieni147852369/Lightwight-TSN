#include <stdlib.h>
#include <string.h>

#include "hash.h"

#define HASH_TABLE_SIZE 200

struct node {
	char *key;
	void *data;
	struct node *next;
};

struct hash {
	struct node *table[HASH_TABLE_SIZE];
};

static unsigned int hash_function(const char* s)
{
	unsigned int i;

	for (i = 0; *s; s++) {
		i = 131 * i + *s;
	}
	return i % HASH_TABLE_SIZE;
}

struct hash *hash_create(void)
{
	struct hash *ht = calloc(1, sizeof(*ht));
	return ht;
}

void hash_destroy(struct hash *ht, void (*func)(void *))
{
	unsigned int i;
	struct node *n, *next, **table = ht->table;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		for (n = table[i] ; n; n = next) {
			next = n->next;
			if (func) {
				func(n->data);
			}
			free(n->key);
			free(n);
		}
	}

	free(ht);
}

int hash_insert(struct hash *ht, const char* key, void *data)
{
	unsigned int h;
	struct node *n, **table = ht->table;

	h = hash_function(key);

	for (n = table[h] ; n; n = n->next) {
		if (!strcmp(n->key, key)) {
			/* reject duplicate keys */
			return -1;
		}
	}
	n = calloc(1, sizeof(*n));
	if (!n) {
		return -1;
	}
	n->key = strdup(key);
	if (!n->key) {
		free(n);
		return -1;
	}
	n->data = data;
	n->next = table[h];
	table[h] = n;
	return 0;
}

void *hash_lookup(struct hash *ht, const char* key)
{
	unsigned int h;
	struct node *n, **table = ht->table;

	h = hash_function(key);

	for (n = table[h] ; n; n = n->next) {
		if (!strcmp(n->key, key)) {
			return n->data;
		}
	}
	return NULL;
}
