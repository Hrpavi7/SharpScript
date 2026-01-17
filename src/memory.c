#include "include/memory.h"
#include <string.h>
#include <stdio.h>

void *memory_allocate(size_t size)
{
    void *ptr = malloc(size);
    if (!ptr)
    {
        fprintf(stderr, "Memory allocation failed\n");
        exit(1);
    }
    return ptr;
}

void *memory_reallocate(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);
    if (!new_ptr)
    {
        fprintf(stderr, "Memory reallocation failed\n");
        exit(1);
    }
    return new_ptr;
}

void memory_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

char *memory_strdup(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    char *copy = memory_allocate(len + 1);
    strcpy(copy, str);
    return copy;
}