#ifndef __CLOX_OBJECT_H__
#define __CLOX_OBJECT_H__

#include "common.h"
#include "value.h"

typedef enum {
    CLOX_OBJ_STRING,
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

static inline bool clox_is_obj_type(clox_value value, clox_obj_type type)
{
    return CLOX_IS_OBJ(value) && CLOX_AS_OBJ(value)->type == type;
}

#define CLOX_OBJ_TYPE(value) (CLOX_AS_OBJ(value)->type)

#define CLOX_IS_STRING(value) clox_is_obj_type(value, CLOX_OBJ_STRING)

#define CLOX_AS_STRING(value) ((clox_obj_string*)CLOX_AS_OBJ(value))
#define CLOX_AS_CSTRING(value) (((clox_obj_string*)CLOX_AS_OBJ(value))->chars)

clox_obj_string* clox_copy_string(const char *chars, int length);
clox_obj_string* clox_take_string(char* chars, int length);
void clox_print_object(clox_value value);

#endif // __CLOX_OBJECT_H__
