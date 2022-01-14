#ifndef __CLOX_VALUE_H__
#define __CLOX_VALUE_H__

#include "common.h"

typedef enum {
    CLOX_VAL_BOOL,
    CLOX_VAL_NIL,
    CLOX_VAL_NUMBER,
    CLOX_VAL_OBJ
} clox_value_type;

typedef struct clox_obj clox_obj;
typedef struct clox_obj_string clox_obj_string;

typedef struct {
    clox_value_type type;
    union {
        bool boolean;
        double number;
        clox_obj *obj;
    } as;
} clox_value;

typedef struct {
    int capacity;
    int count;
    clox_value* values;
} clox_value_array;

#define CLOX_BOOL_VAL(value) ((clox_value){ CLOX_VAL_BOOL, {.boolean = value} })
#define CLOX_NIL_VAL ((clox_value){ CLOX_VAL_NIL, {.number = 0} })
#define CLOX_NUMBER_VAL(value) ((clox_value){ CLOX_VAL_NUMBER, {.number = value} })
#define CLOX_OBJ_VAL(object) ((clox_value){ CLOX_VAL_OBJ, {.obj = (clox_obj*)object} })

#define CLOX_AS_BOOL(value) ((value).as.boolean)
#define CLOX_AS_NUMBER(value) ((value).as.number)
#define CLOX_AS_OBJ(value) ((value).as.obj)

#define CLOX_IS_BOOL(value) ((value).type == CLOX_VAL_BOOL)
#define CLOX_IS_NIL(value) ((value).type == CLOX_VAL_NIL)
#define CLOX_IS_NUMBER(value) ((value).type == CLOX_VAL_NUMBER)
#define CLOX_IS_OBJ(value) ((value).type == CLOX_VAL_OBJ)
 
void clox_init_value_array(clox_value_array *array);
void clox_write_value_array(clox_value_array *array, clox_value value);
void clox_free_value_array(clox_value_array *array);
void clox_print_value(clox_value value);
bool clox_value_equal(clox_value a, clox_value b);

#endif // __CLOX_VALUE_H__
