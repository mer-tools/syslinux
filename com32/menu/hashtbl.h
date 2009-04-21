/*
 * hashtbl.h
 *
 * Efficient dictionary hash table class.
 */

#ifndef MENU_HASHTBL_H
#define MENU_HASHTBL_H

#include <inttypes.h>
#include <stddef.h>

/* This is expected to be embedded in a larger structure */
struct hash_symbol {
    uint64_t hash;
    const char *key;
};

struct hash_table {
	struct hash_symbol **table;
	size_t load;
	size_t size;
	size_t max_load;
};

struct hash_insert {
	uint64_t hash;
	struct hash_table  *head;
	struct hash_symbol **where;
};

uint64_t crc64(uint64_t crc, const char *string);
uint64_t crc64i(uint64_t crc, const char *string);
#define CRC64_INIT UINT64_C(0xffffffffffffffff)

/* Some reasonable initial sizes... */
#define HASH_SMALL	4
#define HASH_MEDIUM	16
#define HASH_LARGE	256

void hash_init(struct hash_table *head, size_t size);
struct hash_symbol *hash_find(struct hash_table *head, const char *string,
			      struct hash_insert *insert);
struct hash_symbol *hash_findi(struct hash_table *head, const char *string,
			       struct hash_insert *insert);
void hash_add(struct hash_insert *insert, struct hash_symbol *sym);
void hash_free(struct hash_table *head);

#endif /* MENU_HASHTBL_H */
