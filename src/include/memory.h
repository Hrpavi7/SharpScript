#ifndef MEMORY_H
#define MEMORY_H // never got memory.h

#include <stdlib.h>

void* memory_allocate(size_t size);
void* memory_reallocate(void* ptr, size_t size);
void memory_free(void* ptr);
char* memory_strdup(const char* str);

#endif // MEMORY_H