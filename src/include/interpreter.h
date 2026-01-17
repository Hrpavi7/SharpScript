#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "ast.h"
#include "memory.h"

typedef enum
{
    VAL_NUMBER,
    VAL_STRING,
    VAL_BOOLEAN,
    VAL_NULL, // nil == null
    VAL_FUNCTION,
    VAL_ARRAY,
    VAL_OBJECT,
    VAL_NAMESPACE,
    VAL_CLASS,
    VAL_ENUM,
    VAL_BREAK,
    VAL_CONTINUE,
    VAL_RETURN
} ValueType;

typedef struct Value
{
    ValueType type;
    union
    {
        double number;
        char *string;
        int boolean;
        struct
        {
            ASTNode *function;
            struct Environment *closure;
        } function;
        struct
        {
            struct Value **elements;
            int count;
            int capacity;
        } array;
        struct
        {
            char **keys;
            struct Value **values;
            int count;
            int capacity;
            struct Environment *class_env;
        } object;
        struct
        {
            struct Environment *env;
        } ns;
        struct
        {
            struct Environment *env;
        } classv;
        struct
        {
            struct Environment *env;
        } enumv;
        struct
        {
            struct Value *value;
        } return_val;
    } data;
} Value;

typedef struct Environment
{
    char **names;
    Value **values;
    int *is_const;
    int count;
    int capacity;
    struct Environment *parent;
} Environment;

typedef struct
{
    Environment *global;
    Environment *current;
} Interpreter;

Interpreter *interpreter_create(void);
void interpreter_free(Interpreter *interp);
Value *interpreter_eval(Interpreter *interp, ASTNode *node);
void value_free(Value *val);
void env_declare(Environment *env, const char *name, Value *value, int is_const);

#endif // INTERPRETER_H
