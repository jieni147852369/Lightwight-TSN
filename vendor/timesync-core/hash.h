#ifndef HAVE_HASH_H
#define HAVE_HASH_H

struct hash;

/**
 * Create a new hash table.
 * @return  A pointer to a new hash table on success, NULL otherwise.
 */
struct hash *hash_create(void);

/**
 * Destroy an instance of a hash table.
 * @param ht   Pointer to a hash table obtained via @ref hash_create().
 * @param func Callback function, possibly NULL, to apply to the
 *             data of each element in the table.
 */
void hash_destroy(struct hash *ht, void (*func)(void *));

/**
 * Inserts an element into a hash table.
 * @param ht   Hash table into which the element is to be stored.
 * @param key  Key that identifies the element.
 * @param data Pointer to the user data to be stored.
 * @return Zero on success and non-zero on error.  Attempting to
 *         insert a duplicate key will fail with an error.
 */
int hash_insert(struct hash *ht, const char* key, void *data);

/**
 * Looks up an element from the hash table.
 * @param ht   Hash table to consult.
 * @param key  Key identifying the element of interest.
 * @return  Pointer to the element's data, or NULL if the key is not found.
 */
void *hash_lookup(struct hash *ht, const char* key);

#endif


