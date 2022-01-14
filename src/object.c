#include <stdio.h>
#include <string.h>

#include "memory.h"
#include "clox/object.h"
#include "clox/value.h"
#include "clox/vm.h"
#include "clox/table.h"

#define ALLOCATE_OBJ(type, obj_type) \
    (type*)allocate_object(sizeof(type), obj_type)

static clox_obj_string* allocate_string(char* chars, int length, uint32_t hash);
static clox_obj* allocate_object(size_t size, clox_obj_type type);
static uint32_t hash_string(const char* chars, int length);

clox_obj_string* clox_copy_string(const char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);

    clox_obj_string* interned = clox_table_find_string(&clox_vm_instance.strings, chars, length, hash);
    if (interned != NULL) return interned;

    char *heap_chars = ALLOCATE(char, length + 1);
    memcpy(heap_chars, chars, length);
    heap_chars[length] = '\0';
    return allocate_string(heap_chars, length, hash);
}

clox_obj_string* clox_take_string(char* chars, int length)
{
    uint32_t hash = hash_string(chars, length);
    clox_obj_string* interned = clox_table_find_string(&clox_vm_instance.strings, chars, length, hash);

    if (interned != NULL) {
        FREE_ARRAY(char, chars, length + 1);
        return interned;
    }

    return allocate_string(chars, length, hash);
}

void clox_print_object(clox_value value)
{
    switch (CLOX_OBJ_TYPE(value)) {
        case CLOX_OBJ_STRING:
            printf("%s", CLOX_AS_CSTRING(value));
            break;
    }
}

static clox_obj_string *allocate_string(char *chars, int length, uint32_t hash)
{
    clox_obj_string *string = ALLOCATE_OBJ(clox_obj_string, CLOX_OBJ_STRING);
    string->length = length;
    string->chars = chars;
    string->hash = hash;
    clox_table_set(&clox_vm_instance.strings, string, CLOX_NIL_VAL);
    return string;
}

static clox_obj *allocate_object(size_t size, clox_obj_type type)
{
    clox_obj *object = (clox_obj*)reallocate(NULL, 0, size);
    object->type = type;
    object->next = clox_vm_instance.objects;
    clox_vm_instance.objects = object;

    return object;
}

static uint32_t hash_string(const char* chars, int length)
{
    uint32_t hash = 2166136261l;

    for (int i = 0; i < length; i++) {
        hash ^= (uint8_t)chars[i];
        hash *= 16777619;
    }

    return hash;
}
