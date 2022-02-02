#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "chunk.h"
#include "value.h"

typedef enum {
    CLOX_OBJ_STRING,
    CLOX_OBJ_FUNCTION,
    CLOX_OBJ_NATIVE_FUNCTION
} clox_obj_type;

struct clox_obj {
    clox_obj_type type;
    struct clox_obj* next;
};

struct clox_obj_string {
    clox_obj obj;
    int length;
    char* chars;
    uint32_t hash;
};

typedef struct {
    clox_obj obj;
    int arity;
    clox_chunk chunk;
    clox_obj_string* name;
} clox_obj_function;

typedef clox_value (*clox_native_fn)(int arg_count, clox_value* args);

typedef struct {
    clox_obj obj;
    clox_native_fn function;
} clox_obj_native_function;

static inline bool clox_is_obj_type(clox_value value, clox_obj_type type)
{
    return CLOX_IS_OBJ(value) && CLOX_AS_OBJ(value)->type == type;
}

#define CLOX_OBJ_TYPE(value) (CLOX_AS_OBJ(value)->type)

#define CLOX_IS_STRING(value) clox_is_obj_type(value, CLOX_OBJ_STRING)
#define CLOX_IS_FUNCTION(value) clox_is_obj_type(value, CLOX_OBJ_FUNCTION)
#define CLOX_IS_NATIVE_FUNCTION(value) clox_is_obj_type(value, CLOX_OBJ_NATIVE_FUNCTION)

#define CLOX_AS_STRING(value) ((clox_obj_string*)CLOX_AS_OBJ(value))
#define CLOX_AS_CSTRING(value) (((clox_obj_string*)CLOX_AS_OBJ(value))->chars)
#define CLOX_AS_FUNCTION(value) ((clox_obj_function*)CLOX_AS_OBJ(value))
#define CLOX_AS_NATIVE_FUNCTION(value) (((clox_obj_native_function*)CLOX_AS_OBJ(value))->function)

clox_obj_native_function* clox_new_native_function(clox_native_fn function);
clox_obj_function* clox_new_function();
clox_obj_string* clox_copy_string(const char *chars, int length);
clox_obj_string* clox_take_string(char* chars, int length);
void clox_print_object(clox_value value);

#endif // __CLOX_OBJECT_H__
