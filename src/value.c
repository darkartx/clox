#include <stdlib.h>
#include <stdio.h>

#include "clox/value.h"
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
    printf("%g", value);
}
