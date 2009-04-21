/*
 * hashtbl.c
 *
 * Efficient symbol hash table class.
 */

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "hashtbl.h"

#define HASH_MAX_LOAD		2 /* Higher = more memory-efficient, slower */

static struct hash_symbol **alloc_table(size_t newsize)
{
	return calloc(sizeof(struct hash_symbol *), newsize);
}

void hash_init(struct hash_table *head, size_t size)
{
	head->table    = alloc_table(size);
	head->load     = 0;
	head->size     = size;
	head->max_load = size*(HASH_MAX_LOAD-1)/HASH_MAX_LOAD;
}

/*
 * Find an entry in a hash table.
 *
 * On failure, if "insert" is non-NULL, store data in that structure
 * which can be used to insert that node using hash_add().
 *
 * WARNING: this data is only valid until the very next call of
 * hash_add(); it cannot be "saved" to a later date.
 *
 * On success, return the hash_symbol pointer.
 */
struct hash_symbol *hash_find(struct hash_table *head, const char *key,
			      struct hash_insert *insert)
{
	struct hash_symbol *np;
	uint64_t hash = crc64(CRC64_INIT, key);
	struct hash_symbol **tbl = head->table;
	size_t mask = head->size-1;
	size_t pos  = hash & mask;
	size_t inc  = ((hash >> 32) & mask) | 1;	/* Always odd */

	while ((np = tbl[pos])) {
		if (hash == np->hash && !strcmp(key, np->key))
			return np;
		pos = (pos+inc) & mask;
	}

	/* Not found.  Store info for insert if requested. */
	if (insert) {
		insert->head  = head;
		insert->hash  = hash;
		insert->where = &tbl[pos];
	}
	return NULL;
}

/*
 * Same as hash_find, but for case-insensitive hashing.
 */
struct hash_symbol *hash_findi(struct hash_table *head, const char *key,
			       struct hash_insert *insert)
{
	struct hash_symbol *np;
	uint64_t hash = crc64i(CRC64_INIT, key);
	struct hash_symbol **tbl = head->table;
	size_t mask = head->size-1;
	size_t pos  = hash & mask;
	size_t inc  = ((hash >> 32) & mask) | 1;	/* Always odd */

	while ((np = tbl[pos])) {
		if (hash == np->hash && !strcasecmp(key, np->key))
			return np;
		pos = (pos+inc) & mask;
	}

	/* Not found.  Store info for insert if requested. */
	if (insert) {
		insert->head  = head;
		insert->hash  = hash;
		insert->where = &tbl[pos];
	}
	return NULL;
}

/*
 * Insert node.
 */
void hash_add(struct hash_insert *insert, struct hash_symbol *sym)
{
	struct hash_table *head = insert->head;
	struct hash_symbol **np = insert->where;

	/* Insert node.  We can always do this, even if we need to
	   rebalance immediately after. */
	sym->hash = insert->hash;
	*np = sym;

	if (++head->load > head->max_load) {
		/* Need to expand the table */
		size_t newsize = head->size << 1;
		struct hash_symbol **newtbl = alloc_table(newsize);
		size_t mask = newsize-1;
		
		if (head->table) {
			struct hash_symbol **op, **xp;
			size_t i, pos, inc;
			uint64_t hash;
			
			/* Rebalance all the entries */
			for (i = 0, op = head->table; i < head->size;
			     i++, op++) {
				if (*op) {
					hash = (*op)->hash;
					pos = hash & mask;
					inc = ((hash >> 32) & mask) | 1;

					while (*(xp = &newtbl[pos]))
						pos = (pos+inc) & mask;
					*xp = *op;
				}
			}
			free(head->table);
		}
		head->table    = newtbl;
		head->size     = newsize;
		head->max_load = newsize*(HASH_MAX_LOAD-1)/HASH_MAX_LOAD;
	}
}

/*
 * Free the hash itself.  Doesn't free the data elements; use
 * hash_iterate() to do that first, if needed.
 */
void hash_free(struct hash_table *head)
{
    void *p = head->table;
    memset(head, 0, sizeof *head);
    free(p);
}
