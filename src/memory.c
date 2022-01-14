#include <stdlib.h>

#include "memory.h"
#include "clox/vm.h"

static void free_object(clox_obj* object);

void *reallocate(void *pointer, size_t old_size, size_t new_size)
{
    if (new_size == 0) {
        free(pointer);
        return NULL;
    }

    void *result = realloc(pointer, new_size);
    if (result == NULL) exit(1);
    return result;
}

void free_objects()
{
    clox_obj* object = clox_vm_instance.objects;
    while (object != NULL) {
        clox_obj* next = object->next;
        free_object(object);
        object = next;
    }
}

static void free_object(clox_obj* object)
{
    switch (object->type) {
        case CLOX_OBJ_STRING: {
            clox_obj_string* string = (clox_obj_string*)object;
            FREE_ARRAY(char, string->chars, string->length + 1);
            FREE(clox_obj_string, object);
            break;
        }
    }
}
