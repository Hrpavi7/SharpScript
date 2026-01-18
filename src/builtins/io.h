#ifndef SHARPSCRIPT_IO_H
#define SHARPSCRIPT_IO_H

#include "../include/interpreter.h"

Value *io_read_file(const char *path);
Value *io_write_file(const char *path, Value *data);

#endif
