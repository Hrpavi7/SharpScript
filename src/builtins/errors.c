#include "errors.h"
#include <string.h>

Value *value_create_error(const char *name, const char *message, int code)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_ERROR;
    val->data.error.name = memory_strdup(name ? name : "Error");
    val->data.error.message = memory_strdup(message ? message : "");
    val->data.error.code = code;
    return val;
}
