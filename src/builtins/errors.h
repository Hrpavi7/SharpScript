#ifndef SHARPSCRIPT_ERRORS_H
#define SHARPSCRIPT_ERRORS_H

#include "../include/interpreter.h"

Value *value_create_error(const char *name, const char *message, int code);

#endif
