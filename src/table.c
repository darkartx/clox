#include <stdlib.h>
#include <string.h>

#include "memory.h"
#include "clox/object.h"
#include "clox/table.h"
#include "clox/value.h"

#define TABLE_MAX_LOAD 0.75

static clox_entry* find_entry(clox_entry* entries, int capacity, clox_obj_string* key);
static void adjust_capacity(clox_table* table, int capacity);

void clox_init_table(clox_table* table)
{
    table->count = 0;
    table->capacity = 0;
    table->entries = NULL;
}

void clox_free_table(clox_table* table) {
    FREE_ARRAY(clox_entry, table->entries, table->capacity);
    clox_init_table(table);
}

bool clox_table_get(clox_table* table, clox_obj_string* key, clox_value* out_value)
{
    if (table->count == 0) return false;

    clox_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    *out_value = entry->value;
    return true; 
}

bool clox_table_set(clox_table* table, clox_obj_string* key, clox_value value)
{
    if (table->count + 1 > table->capacity * TABLE_MAX_LOAD) {
        int capacity = GROW_CAPACITY(table->capacity);
        adjust_capacity(table, capacity);
    }

    clox_entry* entry = find_entry(table->entries, table->capacity, key);
    bool is_new_key = entry->key == NULL;
    
    if (is_new_key && CLOX_IS_NIL(entry->value)) table->count++;

    entry->key = key;
    entry->value = value;

    return is_new_key;
}

bool clox_table_delete(clox_table* table, clox_obj_string* key)
{
    if (table->count == 0) return false;

    clox_entry* entry = find_entry(table->entries, table->capacity, key);
    if (entry->key == NULL) return false;

    entry->key = NULL;
    entry->value = CLOX_BOOL_VAL(true);

    return true;
}

void clox_table_add_all(clox_table* from, clox_table* to)
{
    for (int i = 0; i < from->capacity; i++) {
        clox_entry* entry = &from->entries[i];
        if (entry->key != NULL) {
            clox_table_set(to, entry->key, entry->value);
        }
    }
}

clox_obj_string* clox_table_find_string(clox_table* table, const char* chars, int length, uint32_t hash)
{
    if (table->count == 0) return NULL;

    uint32_t index = hash % table->capacity;
    for (;;) {
        clox_entry* entry = &table->entries[index];

        if (entry->key == NULL) {
            if (CLOX_IS_NIL(entry->value)) return NULL;
        } else if (
            entry->key->length == length 
            && entry->key->hash == hash 
            && memcmp(entry->key->chars, chars, length) == 0
        ) {
            return entry->key;
        }
    }
}

static clox_entry* find_entry(clox_entry* entries, int capacity, clox_obj_string* key)
{
    uint32_t index = key->hash % capacity;
    clox_entry* tombstone = NULL;

    for (;;) {
        clox_entry* entry = &entries[index];
        if (entry->key == NULL) {
            if (CLOX_IS_NIL(entry->value)) {
                return tombstone != NULL ? tombstone : entry;
            } else {
                if (tombstone == NULL) tombstone = entry;
            }
        } else if (entry->key == key) {
            return entry;
        }

        index = (index + 1) % capacity;
    }
}

static void adjust_capacity(clox_table* table, int capacity)
{
    clox_entry* entries = ALLOCATE(clox_entry, capacity);
    for (int i = 0; i < capacity; i++) {
        entries[i].key = NULL;
        entries[i].value = CLOX_NIL_VAL;
    }

    table->count = 0;
    for (int i = 0; i < table->capacity; i++) {
        clox_entry* entry = &table->entries[i];
        if (entry->key == NULL) continue;

        clox_entry* dest = find_entry(entries, capacity, entry->key);
        dest->key = entry->key;
        dest->value = entry->value;
        table->count++;
    }

    FREE_ARRAY(clox_entry, table->entries, table->capacity);

    table->entries = entries;
    table->capacity = capacity;
}

