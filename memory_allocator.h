#ifndef MY_ALLOCATOR_H
#define MY_ALLOCATOR_H

#include <stddef.h>

void *malloc(size_t size);
void free(void *block);
void *calloc(size_t count_elements, size_t element_size);
void *realloc(void *block, size_t size);

#endif