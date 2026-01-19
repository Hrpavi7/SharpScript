/*
    Copyright (c) 2024-2026 SharpScript Programming Language
    
    Licensed under the MIT License
*/

// START OF memory.c

#include "include/memory.h"
#include <string.h>
#include <stdio.h>

/*
 * memory_allocate: Allocate memory with error handling
 * 
 * Wrapper around malloc that provides error handling. If allocation fails,
 * prints an error message and exits the program with status 1.
 * 
 * Parameters:
 *   size: Number of bytes to allocate
 * 
 * Returns: Pointer to the allocated memory
 * 
 * Note: This function does not return on allocation failure - it terminates the program
 */
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

/*
 * memory_reallocate: Reallocate memory with error handling
 * 
 * Wrapper around realloc that provides error handling. If reallocation fails,
 * prints an error message and exits the program with status 1.
 * 
 * Parameters:
 *   ptr: Pointer to the memory block to reallocate (can be NULL)
 *   size: New size in bytes
 * 
 * Returns: Pointer to the reallocated memory
 * 
 * Note: This function does not return on reallocation failure - it terminates the program
 */
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

/*
 * memory_free: Safely free allocated memory
 * 
 * Wrapper around free that checks for NULL pointer before freeing.
 * Prevents double-free errors by only freeing non-NULL pointers.
 * 
 * Parameters:
 *   ptr: Pointer to the memory block to free (can be NULL)
 * 
 * Note: This function is safe to call with NULL pointers
 */
void memory_free(void *ptr)
{
    if (ptr)
        free(ptr);
}

/*
 * memory_strdup: Duplicate a string with memory allocation
 * 
 * Creates a new copy of the input string using memory_allocate for allocation.
 * The caller is responsible for freeing the returned string using memory_free.
 * 
 * Parameters:
 *   str: The string to duplicate (can be NULL)
 * 
 * Returns: Pointer to the newly allocated copy of the string, or NULL if input is NULL
 * 
 * Note: The returned string must be freed by the caller using memory_free()
 */
char *memory_strdup(const char *str)
{
    if (!str)
        return NULL;
    size_t len = strlen(str);
    char *copy = memory_allocate(len + 1);
    strcpy(copy, str);
    return copy;
}

// END OF memory.c
