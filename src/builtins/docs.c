// docs.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../include/memory.h"

static char* read_file_simple(const char* filename)
{
    FILE* f = fopen(filename, "rb");
    if (!f) return NULL;
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char* buf = (char*)malloc(sz + 1);
    if (!buf) { fclose(f); return NULL; }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    return buf; // returns buffer;
}

char* docs_get(const char* topic)
{
    const char* path = NULL;
    if (strcmp(topic, "user") == 0 || strcmp(topic, "help") == 0)
        path = "../../docs/USER_GUIDE.md";
    else if (strcmp(topic, "dev") == 0 || strcmp(topic, "developer") == 0)
        path = "../../docs/DEVELOPER_GUIDE.md";
    else
        path = "../../docs/USER_GUIDE.md"; 
    char* content = read_file_simple(path);
    if (!content)
        return memory_strdup("Documentation not found");
    return content;
}
