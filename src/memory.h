#ifndef __MEMORY_H__
#define __MEMORY_H__

#include "clox/common.h"
#include "clox/object.h"

#define GROW_CAPACITY(capacity) \
    ((capacity) < 8 ? 8 : (capacity) * 2)

#define GROW_ARRAY(type, pointer, old_count, new_count) \
    ((type*)reallocate(pointer, sizeof(type) * (old_count), sizeof(type) * (new_count)))

#define FREE_ARRAY(type, pointer, count) \
    reallocate(pointer, sizeof(type) * count, 0)

#define ALLOCATE(type, count) \
    (type*)reallocate(NULL, 0, sizeof(type) * (count))

#define FREE(type, pointer) reallocate(pointer, sizeof(type), 0)

void* reallocate(void *pointer, size_t old_size, size_t new_size);
void free_objects();

#endif // __MEMORY_H__
