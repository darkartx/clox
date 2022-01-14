#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "clox/value.h"
#include "clox/object.h"
#include "memory.h"

void clox_init_value_array(clox_value_array *array)
{
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void clox_write_value_array(clox_value_array *array, clox_value value)
{
    if (array->capacity < array->count + 1) {
        int old_capacity = array->capacity;
        array->capacity = GROW_CAPACITY(old_capacity);
        array->values = GROW_ARRAY(clox_value, array->values, old_capacity, array->capacity);
    }

    array->values[array->count] = value;
    array->count++;
}

void clox_free_value_array(clox_value_array *array)
{
    FREE_ARRAY(clox_value, array->values, array->capacity);
    clox_init_value_array(array);
}

void clox_print_value(clox_value value)
{
    switch (value.type) {
        case CLOX_VAL_BOOL:
            printf(CLOX_AS_BOOL(value) ? "true" : "false");
            break;
        case CLOX_VAL_NIL: printf("nil"); break;
        case CLOX_VAL_NUMBER: 
            printf("%g", CLOX_AS_NUMBER(value));
            break;
        case CLOX_VAL_OBJ: clox_print_object(value); break;
    }
}

bool clox_value_equal(clox_value a, clox_value b)
{
    
    if (a.type != b.type) return false;
    switch (a.type) {
        case CLOX_VAL_BOOL: return CLOX_AS_BOOL(a) == CLOX_AS_BOOL(b);
        case CLOX_VAL_NIL: return true;
        case CLOX_VAL_NUMBER: return CLOX_AS_NUMBER(a) == CLOX_AS_NUMBER(b);
        case CLOX_VAL_OBJ: {
            clox_obj_string *a_string = CLOX_AS_STRING(a);
            clox_obj_string *b_string = CLOX_AS_STRING(b);
            return a_string->length == b_string->length &&
                memcmp(a_string->chars, b_string->chars, a_string->length) == 0;
        }
        default: return false;
    }
}
