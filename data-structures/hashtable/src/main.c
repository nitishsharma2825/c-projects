#include <stdio.h>
#include "hash_table.h"

int main()
{
    ht_hash_table* ht = ht_new();
    ht_insert(ht, "a", "b");
    ht_insert(ht, "b", "c");
    char* ans = ht_search(ht, "a");
    printf("%s\n", ans);
    ht_del_hash_table(ht);
}