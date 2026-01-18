#include "io.h"
#include <stdio.h>
#include <string.h>

Value *io_read_file(const char *path)
{
    if (!path) return value_create_null();
    FILE *f = fopen(path, "rb");
    if (!f) return value_create_null();
    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buf = malloc(sz + 1);
    if (!buf) { fclose(f); return value_create_null(); }
    fread(buf, 1, sz, f);
    buf[sz] = '\0';
    fclose(f);
    Value *s = value_create_string(buf);
    free(buf);
    return s;
}

Value *io_write_file(const char *path, Value *data)
{
    if (!path) return value_create_null();
    FILE *f = fopen(path, "wb");
    if (!f) return value_create_null();
    if (data)
    {
        if (data->type == VAL_STRING)
            fwrite(data->data.string, 1, strlen(data->data.string), f);
        else if (data->type == VAL_NUMBER)
        {
            char buf[64];
            int n = snprintf(buf, sizeof(buf), "%g", data->data.number);
            fwrite(buf, 1, n, f);
        }
    }
    fclose(f);
    return value_create_null();
}
