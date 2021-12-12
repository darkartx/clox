#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

typedef double clox_value;

typedef struct {
    int capacity;
    int count;
    clox_value* values;
} clox_value_array;

void clox_init_value_array(clox_value_array *array);
void clox_write_value_array(clox_value_array *array, clox_value value);
void clox_free_value_array(clox_value_array *array);
void clox_print_value(clox_value value);

#endif // __CLOX_VALUE_H__
