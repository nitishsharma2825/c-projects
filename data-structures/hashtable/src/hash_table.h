#ifndef HASH_TABLE_H
#define HASH_TABLE_H
extern const int HT_PRIME_1;
extern const int HT_PRIME_2;
extern const int HT_INITIAL_BASE_SIZE;
typedef struct
{
    char* key;
    char* value;
} ht_item;

typedef struct
{
    int size;
    int count;
    int base_size;
    ht_item** items;
} ht_hash_table;

ht_hash_table* ht_new();
void ht_del_hash_table(ht_hash_table*);
void ht_insert(ht_hash_table* ht, const char* key, const char* value);
char* ht_search(ht_hash_table* ht, const char* key);
void ht_delete(ht_hash_table* ht, const char* key);
#endif
