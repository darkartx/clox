#ifndef __CLOX_TABLE_H__
#define __CLOX_TABLE_H__

#include "common.h"
#include "value.h"

typedef struct {
    clox_obj_string* key;
    clox_value value;
} clox_entry;

typedef struct {
    int count;
    int capacity;
    clox_entry* entries;
} clox_table;

void clox_init_table(clox_table* table);
void clox_free_table(clox_table* table);
bool clox_table_get(clox_table* table, clox_obj_string* key, clox_value* out_value);
bool clox_table_set(clox_table* table, clox_obj_string* key, clox_value value);
bool clox_table_delete(clox_table* table, clox_obj_string* key);
void clox_table_add_all(clox_table* from, clox_table* to);
clox_obj_string* clox_table_find_string(clox_table* table, const char* chars, int length, uint32_t hash);

#endif // __CLOX_TABLE_H__
