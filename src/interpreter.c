/*
    Copyright (c) 2024-2026 SharpScript Programming Language

    Licensed under the MIT License
*/

// START OF interpreter.c

/*
 * SharpScript Interpreter
 *
 * This is the main interpreter implementation for the SharpScript language.
 * It handles:
 * - Environment management (variable scoping and storage)
 * - AST node evaluation
 * - Built-in function implementations
 * - Error handling and type checking
 * - Memory management for runtime values
 *
 * The interpreter uses a tree-walking approach to evaluate the AST produced by the parser.
 * Each AST node type has a corresponding evaluation function that processes it.
 */

#include "include/interpreter.h"
#include "include/memory.h"
#include "builtins/io.h"
#include "builtins/docs.h"
#include "builtins/errors.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

/* Global variables for interpreter state management */
static Environment *calc_mem = NULL; /* Calculator memory environment */
static Value **history = NULL;       /* Command history storage */
static int history_count = 0;        /* Number of commands in history */
static int history_capacity = 0;     /* Current capacity of history array */

/*
 * Create a new environment with optional parent scope
 *
 * @param parent: Parent environment for scope chaining (NULL for global scope)
 * @return: Newly allocated environment
 *
 * Environments store variables, their values, const status, and type information.
 * They support lexical scoping through the parent pointer.
 */
static Environment *env_create(Environment *parent)
{
    Environment *env = memory_allocate(sizeof(Environment));
    env->names = memory_allocate(sizeof(char *) * 16);
    env->values = memory_allocate(sizeof(Value *) * 16);
    env->is_const = memory_allocate(sizeof(int) * 16);
    env->types = memory_allocate(sizeof(char *) * 16);
    env->count = 0;
    env->capacity = 16;
    env->parent = parent;
    return env;
}

/*
 * Free an environment and all its associated memory
 *
 * @param env: Environment to free
 *
 * This function cleans up all variable names, values, type information,
 * and the environment structure itself. It does NOT free parent environments.
 */
static void env_free(Environment *env)
{
    for (int i = 0; i < env->count; i++)
    {
        memory_free(env->names[i]);
        value_free(env->values[i]);
        if (env->types[i])
            memory_free(env->types[i]);
    }
    memory_free(env->names);
    memory_free(env->values);
    memory_free(env->is_const);
    memory_free(env->types);
    memory_free(env);
}

/*
 * Get the string name of a value's type
 *
 * @param val: Value to get type name for (can be NULL)
 * @return: String representation of the value's type
 *
 * This function maps the internal VAL_* constants to human-readable type names
 * used in error messages and type annotations.
 */
static const char *type_name(Value *val)
{
    switch (val ? val->type : VAL_NULL)
    {
    case VAL_NUMBER:
        return "number";
    case VAL_STRING:
        return "string";
    case VAL_BOOLEAN:
        return "boolean";
    case VAL_NULL:
        return "null";
    case VAL_FUNCTION:
        return "function";
    case VAL_ARRAY:
        return "array";
    case VAL_MAP:
        return "map";
    default:
        return "unknown";
    }
}

/*
 * Set or update a variable in the environment
 *
 * @param env: Environment to set variable in
 * @param name: Variable name
 * @param value: Value to assign
 *
 * This function handles variable assignment with type checking and const protection.
 * If the variable exists, it checks if it's const and validates type compatibility.
 * If the variable doesn't exist, it creates a new entry.
 */
static void env_set(Environment *env, const char *name, Value *value)
{
    for (int i = 0; i < env->count; i++)
    {
        if (strcmp(env->names[i], name) == 0)
        {
            if (env->is_const[i])
            {
                fprintf(stderr, "Cannot assign to const variable: %s\n", name);
                value_free(value);
                return;
            }
            if (env->types[i])
            {
                const char *tn = type_name(value);
                if (strcmp(env->types[i], tn) != 0 && strcmp(env->types[i], "unknown") != 0)
                {
                    fprintf(stderr, "Type mismatch for %s: expected %s, got %s\n", name, env->types[i], tn);
                    value_free(value);
                    return;
                }
            }
            value_free(env->values[i]);
            env->values[i] = value;
            return;
        }
    }

    if (env->count >= env->capacity)
    {
        env->capacity *= 2;
        env->names = memory_reallocate(env->names, sizeof(char *) * env->capacity);
        env->values = memory_reallocate(env->values, sizeof(Value *) * env->capacity);
        env->is_const = memory_reallocate(env->is_const, sizeof(int) * env->capacity);
        env->types = memory_reallocate(env->types, sizeof(char *) * env->capacity);
    }

    env->names[env->count] = memory_strdup(name);
    env->values[env->count] = value;
    env->is_const[env->count] = 0;
    env->types[env->count] = NULL;
    env->count++;
}

/*
 * Get a variable's value from the environment
 *
 * @param env: Environment to search in
 * @param name: Variable name to look up
 * @return: Value pointer if found, NULL otherwise
 *
 * This function searches the current environment and walks up the parent
 * chain to find variables in outer scopes (lexical scoping).
 */
static Value *env_get(Environment *env, const char *name)
{
    for (int i = 0; i < env->count; i++)
    {
        if (strcmp(env->names[i], name) == 0)
        {
            return env->values[i];
        }
    }

    if (env->parent)
    {
        return env_get(env->parent, name);
    }

    return NULL;
}

/*
 * Check if a variable exists in the environment
 *
 * @param env: Environment to search in
 * @param name: Variable name to check
 * @return: 1 if variable exists, 0 otherwise
 *
 * Similar to env_get but only checks existence without returning the value.
 * Also walks up the parent chain for lexical scoping.
 */
static int env_has(Environment *env, const char *name)
{
    for (int i = 0; i < env->count; i++)
    {
        if (strcmp(env->names[i], name) == 0)
            return 1;
    }
    if (env->parent)
        return env_has(env->parent, name);
    return 0;
}

/*
 * Declare a new variable in the environment
 *
 * @param env: Environment to declare in
 * @param name: Variable name
 * @param value: Initial value
 * @param is_const: 1 for const variables, 0 for mutable
 *
 * This function creates new variables and stores their inferred type.
 * It prevents redeclaration of existing variables.
 */
void env_declare(Environment *env, const char *name, Value *value, int is_const)
{
    for (int i = 0; i < env->count; i++)
    {
        if (strcmp(env->names[i], name) == 0)
        {
            fprintf(stderr, "Variable already declared: %s\n", name);
            value_free(value);
            return;
        }
    }
    if (env->count >= env->capacity)
    {
        env->capacity *= 2;
        env->names = memory_reallocate(env->names, sizeof(char *) * env->capacity);
        env->values = memory_reallocate(env->values, sizeof(Value *) * env->capacity);
        env->is_const = memory_reallocate(env->is_const, sizeof(int) * env->capacity);
        env->types = memory_reallocate(env->types, sizeof(char *) * env->capacity);
    }
    env->names[env->count] = memory_strdup(name);
    env->values[env->count] = value;
    env->is_const[env->count] = is_const ? 1 : 0;
    const char *tn = type_name(value);
    env->types[env->count] = memory_strdup(tn);
    env->count++;
}

/*
 * Add or update type annotation for an existing variable
 *
 * @param env: Environment containing the variable
 * @param name: Variable name
 * @param type_name: Type name to annotate (NULL for "unknown")
 *
 * This function allows runtime type annotation updates, used by the
 * system.annotate() builtin function.
 */
static void env_annotate(Environment *env, const char *name, const char *type_name)
{
    for (int i = 0; i < env->count; i++)
    {
        if (strcmp(env->names[i], name) == 0)
        {
            if (env->types[i])
                memory_free(env->types[i]);
            env->types[i] = memory_strdup(type_name ? type_name : "unknown");
            return;
        }
    }
}

/*
 * Create a new number value
 *
 * @param num: Numeric value
 * @return: Newly allocated Value containing the number
 */
static Value *value_create_number(double num)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_NUMBER;
    val->data.number = num;
    return val;
}

/*
 * Create a new string value
 *
 * @param str: String value (will be copied)
 * @return: Newly allocated Value containing the string
 *
 * The input string is duplicated using memory_strdup to ensure proper memory management.
 */
static Value *value_create_string(const char *str)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_STRING;
    val->data.string = memory_strdup(str);
    return val;
}

/*
    int main()
    {
        return VAL_VAL
    }
*/

/*
 * Create a new boolean value
 *
 * @param b: Boolean value (0 or non-zero)
 * @return: Newly allocated Value containing the boolean
 */
static Value *value_create_boolean(int b)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_BOOLEAN;
    val->data.boolean = b;
    return val;
}

/*
 * Create a new null value
 *
 * @return: Newly allocated Value of type VAL_NULL
 *
 * Null represents the absence of a value in SharpScript.
 */
static Value *value_create_null(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_NULL;
    return val;
}

/*
 * Create a new function value (closure)
 *
 * @param func: AST node representing the function definition
 * @param closure: Environment where the function was created (for lexical scoping)
 * @return: Newly allocated function Value
 *
 * Functions in SharpScript are first-class values that capture their creation environment.
 */
static Value *value_create_function(ASTNode *func, Environment *closure)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_FUNCTION;
    val->data.function.function = func;
    val->data.function.closure = closure;
    return val;
}

/*
 * Create a new empty array value
 *
 * @return: Newly allocated array Value with initial capacity of 8
 *
 * Arrays in SharpScript are dynamic and can hold any type of Value.
 */
static Value *value_create_array(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_ARRAY;
    val->data.array.elements = memory_allocate(sizeof(Value *) * 8);
    val->data.array.count = 0;
    val->data.array.capacity = 8;
    return val;
}

/*
 * Create a new empty map (dictionary) value
 *
 * @return: Newly allocated map Value with initial capacity of 8
 *
 * Maps in SharpScript store key-value pairs where keys are strings and values
 * can be any type of Value.
 */
static Value *value_create_map(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_MAP;
    val->data.map.keys = memory_allocate(sizeof(char *) * 8);
    val->data.map.values = memory_allocate(sizeof(Value *) * 8);
    val->data.map.count = 0;
    val->data.map.capacity = 8;
    return val;
}

// error creation moved to builtins/errors.c

/*
 * Check if two values are equal
 *
 * @param a: First value
 * @param b: Second value
 * @return: 1 if values are equal, 0 otherwise
 *
 * For primitive types (numbers, strings, booleans), compares the actual values.
 * For complex types (arrays, maps, functions), uses reference equality.
 */
static int values_equal(Value *a, Value *b)
{
    if (a->type != b->type)
        return 0;

    switch (a->type)
    {
    case VAL_NUMBER:
        return a->data.number == b->data.number;
    case VAL_STRING:
        return strcmp(a->data.string, b->data.string) == 0;
    case VAL_BOOLEAN:
        return a->data.boolean == b->data.boolean;
    case VAL_NULL:
        return 1; // Both are null
    case VAL_ARRAY:
        // Arrays are reference equal for now
        return a == b;
    case VAL_MAP:
        // Maps are reference equal for now
        return a == b;
    case VAL_FUNCTION:
        // Functions are reference equal
        return a == b;
    default:
        return 0;
    }
}

/*
 * Create a break control flow value
 *
 * @return: Newly allocated break Value
 *
 * Used internally by the interpreter to handle break statements in loops.
 */
static Value *value_create_break(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_BREAK;
    return val;
}

/*
 * Create a continue control flow value
 *
 * @return: Newly allocated continue Value
 *
 * Used internally by the interpreter to handle continue statements in loops.
 */
static Value *value_create_continue(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_CONTINUE;
    return val;
}

/*
 * Create a return control flow value
 *
 * @param ret_val: Value to return (can be NULL)
 * @return: Newly allocated return Value
 *
 * Used internally by the interpreter to handle return statements in functions.
 * The return value is wrapped in this special control flow value.
 */
static Value *value_create_return(Value *ret_val)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_RETURN;
    val->data.return_val.value = ret_val;
    return val;
}

/*
 * Free a Value and all its associated memory
 *
 * @param val: Value to free (can be NULL)
 *
 * This function properly cleans up different types of values:
 * - Strings: frees the string memory
 * - Errors: frees name and message strings
 * - Arrays: recursively frees all elements and the array itself
 * - Maps: frees all keys and values, then the map structure
 * - Return values: recursively frees the wrapped value
 * - Other types: just frees the Value structure
 */
void value_free(Value *val)
{
    if (!val)
        return;

    switch (val->type)
    {
    case VAL_STRING:
        memory_free(val->data.string);
        break;
    case VAL_ERROR:
        if (val->data.error.name)
            memory_free(val->data.error.name);
        if (val->data.error.message)
            memory_free(val->data.error.message);
        break;
    case VAL_ARRAY:
        for (int i = 0; i < val->data.array.count; i++)
        {
            value_free(val->data.array.elements[i]);
        }
        memory_free(val->data.array.elements);
        break;
    case VAL_MAP:
        for (int i = 0; i < val->data.map.count; i++)
        {
            memory_free(val->data.map.keys[i]);
            value_free(val->data.map.values[i]);
        }
        memory_free(val->data.map.keys);
        memory_free(val->data.map.values);
        break;
    case VAL_RETURN:
        value_free(val->data.return_val.value);
        break;
    default:
        break;
    }

    memory_free(val);
}

/*
 * Check if a value is truthy (evaluates to true in boolean context)
 *
 * @param val: Value to check
 * @return: 1 if truthy, 0 if falsy
 *
 * Truthy rules:
 * - NULL and non-existent values are falsy
 * - Booleans: true is truthy, false is falsy
 * - Numbers: 0 is falsy, all others are truthy
 * - Strings: empty string is falsy, others are truthy
 * - Arrays, maps, functions: always truthy
 */
static int value_is_truthy(Value *val)
{
    if (!val || val->type == VAL_NULL)
        return 0;
    if (val->type == VAL_BOOLEAN)
        return val->data.boolean;
    if (val->type == VAL_NUMBER)
        return val->data.number != 0;
    if (val->type == VAL_STRING)
        return strlen(val->data.string) > 0;
    return 1;
}

/*
 * Print a value to stdout
 *
 * @param val: Value to print
 *
 * This function handles pretty-printing of different value types:
 * - Numbers: prints without decimal for integers, with decimals for floats
 * - Strings: prints as-is
 * - Booleans: prints "true" or "false"
 * - Errors: prints in format "<ErrorName: message>"
 * - NULL: prints "null"
 * - Other types: prints their string representation
 */
static void value_print(Value *val)
{
    if (!val || val->type == VAL_NULL)
    {
        printf("null");
        return;
    }

    switch (val->type)
    {
    case VAL_NUMBER:
        if (floor(val->data.number) == val->data.number)
        {
            printf("%.0f", val->data.number);
        }
        else
        {
            printf("%g", val->data.number);
        }
        break;
    case VAL_STRING:
        printf("%s", val->data.string);
        break;
    case VAL_ERROR:
        printf("<%s: %s>", val->data.error.name ? val->data.error.name : "Error",
               val->data.error.message ? val->data.error.message : "");
        break;
    case VAL_BOOLEAN:
        printf("%s", val->data.boolean ? "true" : "false");
        break;
    case VAL_ARRAY:
        printf("[");
        for (int i = 0; i < val->data.array.count; i++)
        {
            value_print(val->data.array.elements[i]);
            if (i < val->data.array.count - 1)
                printf(", ");
        }
        printf("]"); // Print arrays as [elem1, elem2, ...]
        break;
    case VAL_MAP:
        printf("{");
        for (int i = 0; i < val->data.map.count; i++)
        {
            printf("\"%s\": ", val->data.map.keys[i]);
            value_print(val->data.map.values[i]);
            if (i < val->data.map.count - 1)
                printf(", ");
        }
        printf("}"); // Print maps as {"key": value, ...}
        break;
    case VAL_FUNCTION:
        printf("<function>"); // Functions print as <function>
        break;
    default:
        printf("null");
        break;
    }
}

/*
 * Create a deep copy of a value
 *
 * @param val: Value to clone (can be NULL)
 * @return: Newly allocated copy of the value
 *
 * This function creates deep copies of values:
 * - Strings: duplicates the string content
 * - Errors: duplicates name and message
 * - Arrays: recursively clones all elements
 * - Maps: recursively clones all values and duplicates keys
 * - Other types: shallow copy of the value structure
 */
static Value *value_clone(Value *val)
{
    if (!val)
        return NULL;
    Value *copy = memory_allocate(sizeof(Value));
    memcpy(copy, val, sizeof(Value));
    switch (val->type)
    {
    case VAL_STRING:
        copy->data.string = memory_strdup(val->data.string);
        break;
    case VAL_ERROR:
        copy->data.error.name = val->data.error.name ? memory_strdup(val->data.error.name) : NULL;
        copy->data.error.message = val->data.error.message ? memory_strdup(val->data.error.message) : NULL;
        copy->data.error.code = val->data.error.code;
        break;
    case VAL_ARRAY:
    {
        copy->data.array.elements = memory_allocate(sizeof(Value *) * val->data.array.capacity);
        copy->data.array.count = 0;
        copy->data.array.capacity = val->data.array.capacity;
        for (int i = 0; i < val->data.array.count; i++)
        {
            Value *elem_copy = value_clone(val->data.array.elements[i]);
            copy->data.array.elements[copy->data.array.count++] = elem_copy;
        }
        break;
    }
    case VAL_MAP:
    {
        copy->data.map.keys = memory_allocate(sizeof(char *) * val->data.map.capacity);
        copy->data.map.values = memory_allocate(sizeof(Value *) * val->data.map.capacity);
        copy->data.map.count = val->data.map.count;
        copy->data.map.capacity = val->data.map.capacity;
        for (int i = 0; i < val->data.map.count; i++)
        {
            copy->data.map.keys[i] = memory_strdup(val->data.map.keys[i]);
            copy->data.map.values[i] = value_clone(val->data.map.values[i]);
        }
        break;
    }
    case VAL_RETURN:
        copy->data.return_val.value = value_clone(val->data.return_val.value);
        break;
    default:
        break;
    }
    return copy;
}
/*
    "get rect microsoft" -- my friend
*/
/*
 * Create a new interpreter instance
 *
 * @return: Newly allocated Interpreter with initialized global environment
 *
 * Sets up the interpreter with:
 * - Global environment for built-in functions and variables
 * - Calculator memory environment for persistent calculations
 * - History storage for command replay
 */
Interpreter *interpreter_create(void)
{
    Interpreter *interp = memory_allocate(sizeof(Interpreter));
    interp->global = env_create(NULL);
    interp->current = interp->global;
    calc_mem = env_create(NULL);
    history_capacity = 16;
    history_count = 0;
    history = memory_allocate(sizeof(Value *) * history_capacity);
    return interp;
}

/*
 * Free an interpreter instance and all associated memory
 *
 * @param interp: Interpreter to free
 *
 * Cleans up all environments, history, and the interpreter structure itself.
 */
void interpreter_free(Interpreter *interp)
{
    for (int i = 0; i < history_count; i++)
    {
        value_free(history[i]);
    }
    memory_free(history);
    env_free(calc_mem);
    env_free(interp->global);
    memory_free(interp);
}

static Value *eval_node(Interpreter *interp, ASTNode *node);

/*
 * Evaluate a binary operation node
 *
 * @param interp: Interpreter instance
 * @param node: Binary operation AST node
 * @return: Result of the binary operation
 *
 * Handles all binary operations including:
 * - Addition (with string concatenation support)
 * - Subtraction, multiplication, division, modulo
 * - Comparison operations (==, !=, <, <=, >, >=)
 * - Logical operations (&&, ||)
 *
 * For addition, supports automatic string conversion for mixed types.
 */
static Value *eval_binary_op(Interpreter *interp, ASTNode *node)
{
    Value *left = eval_node(interp, node->data.binary_op.left);
    Value *right = eval_node(interp, node->data.binary_op.right);

    if (node->data.binary_op.op == TOKEN_ADD)
    {
        /* String concatenation: if either operand is a string, convert both to strings */
        if (left->type == VAL_STRING || right->type == VAL_STRING)
        {
            char buffer[1024];
            char left_str[512], right_str[512];

            /* Convert left operand to string */
            if (left->type == VAL_STRING)
                strcpy(left_str, left->data.string);
            else if (left->type == VAL_NUMBER)
                sprintf(left_str, "%g", left->data.number);
            else if (left->type == VAL_BOOLEAN)
                strcpy(left_str, left->data.boolean ? "true" : "false");
            else
                strcpy(left_str, "null");

            /* Convert right operand to string */
            if (right->type == VAL_STRING)
                strcpy(right_str, right->data.string);
            else if (right->type == VAL_NUMBER)
                sprintf(right_str, "%g", right->data.number);
            else if (right->type == VAL_BOOLEAN)
                strcpy(right_str, right->data.boolean ? "true" : "false");
            else
                strcpy(right_str, "null");

            /* Concatenate and return result */
            sprintf(buffer, "%s%s", left_str, right_str);
            Value *result = value_create_string(buffer);
            value_free(left);
            value_free(right);
            return result;
        }

        /* Numeric addition */
        double result = left->data.number + right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_SUB)
    {
        /* Numeric subtraction */
        double result = left->data.number - right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_MUL)
    {
        /* Numeric multiplication */
        double result = left->data.number * right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_DIV)
    {
        /* Numeric division */
        double result = left->data.number / right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_MOD)
    {
        /* Numeric modulo (remainder) operation */
        double result = fmod(left->data.number, right->data.number);
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_EQ)
    {
        /* Equality comparison: supports numbers, strings, and booleans */
        int result = 0;
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
        {
            result = left->data.number == right->data.number;
        }
        else if (left->type == VAL_STRING && right->type == VAL_STRING)
        {
            result = strcmp(left->data.string, right->data.string) == 0;
        }
        else if (left->type == VAL_BOOLEAN && right->type == VAL_BOOLEAN)
        {
            result = left->data.boolean == right->data.boolean;
        }
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_NEQ)
    {
        /* Not-equal comparison: supports numbers, strings, and booleans */
        int result = 1;
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
        {
            result = left->data.number != right->data.number;
        }
        else if (left->type == VAL_STRING && right->type == VAL_STRING)
        {
            result = strcmp(left->data.string, right->data.string) != 0;
        }
        else if (left->type == VAL_BOOLEAN && right->type == VAL_BOOLEAN)
        {
            result = left->data.boolean != right->data.boolean;
        }
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_LT)
    {
        /* Less-than comparison: numeric only */
        int result = left->data.number < right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_GT)
    {
        /* Greater-than comparison: numeric only */
        int result = left->data.number > right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_LTE)
    {
        /* Less-than-or-equal comparison: numeric only */
        int result = left->data.number <= right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_GTE)
    {
        /* Greater-than-or-equal comparison: numeric only */
        int result = left->data.number >= right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_AND)
    {
        /* Logical AND: both operands must be truthy */
        int result = value_is_truthy(left) && value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_OR)
    {
        /* Logical OR: at least one operand must be truthy */
        int result = value_is_truthy(left) || value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    value_free(left);
    value_free(right);
    return value_create_null();
}

/*
 * Evaluate a built-in function call
 *
 * @param interp: Interpreter instance
 * @param name: Built-in function name
 * @param args: Array of argument AST nodes
 * @param arg_count: Number of arguments
 * @return: Result of the built-in function
 *
 * Handles system built-in functions including print, output, help, and error.
 */
static Value *eval_builtin(Interpreter *interp, const char *name, ASTNode **args, int arg_count)
{
    if (strcmp(name, "system.print") == 0)
    {
        /* Print arguments to stdout with spaces between them */
        for (int i = 0; i < arg_count; i++)
        {
            Value *val = eval_node(interp, args[i]);
            value_print(val);
            value_free(val);
            if (i < arg_count - 1)
                printf(" ");
        }
        printf("\n");
        return value_create_null();
    }

    if (strcmp(name, "system.output") == 0)
    {
        /* Same as print - output arguments to stdout */
        for (int i = 0; i < arg_count; i++)
        {
            Value *val = eval_node(interp, args[i]);
            value_print(val);
            value_free(val);
            if (i < arg_count - 1)
                printf(" ");
        }
        printf("\n");
        return value_create_null();
    }

    if (strcmp(name, "system.help") == 0)
    {
        /* Display help documentation for a topic */
        const char *topic = "help";
        if (arg_count >= 1)
        {
            Value *t = eval_node(interp, args[0]);
            if (t->type == VAL_STRING)
                topic = t->data.string;
            value_free(t);
        }
        char *doc = docs_get(topic);
        printf("%s\n", doc);
        free(doc);
        return value_create_null();
    }

    /*
     * system.error: Print error message to stderr
     *
     * Takes multiple arguments and prints them as an error message.
     * Each argument is evaluated and printed to stderr with spaces between them.
     * Non-string values are converted to their string representation.
     */
    if (strcmp(name, "system.error") == 0)
    {
        fprintf(stderr, "Error: ");
        for (int i = 0; i < arg_count; i++)
        {
            Value *val = eval_node(interp, args[i]);
            if (val->type == VAL_STRING)
                fprintf(stderr, "%s", val->data.string);
            else
            {
                // Handle non-string types by converting to their string representation
                if (val->type == VAL_NUMBER)
                    fprintf(stderr, "%g", val->data.number);
                else if (val->type == VAL_BOOLEAN)
                    fprintf(stderr, "%s", val->data.boolean ? "true" : "false");
                else
                    fprintf(stderr, "null");
            }
            value_free(val);
            if (i < arg_count - 1)
                fprintf(stderr, " ");
        }
        fprintf(stderr, "\n");
        return value_create_null();
    }

    /*
     * system.warning: Print warning message to stdout
     *
     * Takes multiple arguments and prints them as a warning message.
     * Each argument is evaluated and printed to stdout with spaces between them.
     * Uses value_print() for consistent formatting across all value types.
     */
    if (strcmp(name, "system.warning") == 0)
    {
        printf("Warning: ");
        for (int i = 0; i < arg_count; i++)
        {
            Value *val = eval_node(interp, args[i]);
            value_print(val);
            value_free(val);
            if (i < arg_count - 1)
                printf(" ");
        }
        printf("\n");
        return value_create_null();
    }

    /*
     * system.input: Read user input from stdin
     *
     * Optional argument: prompt message to display
     * Returns: String containing the user's input (without newline)
     * If input fails, returns an empty string
     */
    if (strcmp(name, "system.input") == 0)
    {
        if (arg_count > 0)
        {
            Value *prompt = eval_node(interp, args[0]);
            value_print(prompt);
            value_free(prompt);
        }

        char buffer[1024];
        if (fgets(buffer, sizeof(buffer), stdin))
        {
            buffer[strcspn(buffer, "\n")] = 0; // Remove trailing newline
            return value_create_string(buffer);
        }
        return value_create_string("");
    }

    /*
     * Trigonometric functions
     * Each takes one numeric argument and returns the trigonometric result
     * Non-numeric arguments are treated as 0.0
     */

    // system.sin: Sine function (radians)
    if (strcmp(name, "system.sin") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(sin(v));
    }

    // system.cos: Cosine function (radians)
    if (strcmp(name, "system.cos") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(cos(v));
    }

    // system.tan: Tangent function (radians)
    if (strcmp(name, "system.tan") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(tan(v));
    }

    // system.asin: Arcsine function (returns radians)
    if (strcmp(name, "system.asin") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(asin(v));
    }

    // system.acos: Arccosine function (returns radians)
    if (strcmp(name, "system.acos") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(acos(v));
    }
    // system.atan: Arctangent function (returns radians)
    if (strcmp(name, "system.atan") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(atan(v));
    }

    // system.log: Base-10 logarithm function
    if (strcmp(name, "system.log") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(log10(v));
    }

    // system.ln: Natural logarithm function
    if (strcmp(name, "system.ln") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(log(v));
    }

    // system.exp: Exponential function (e^x)
    if (strcmp(name, "system.exp") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(exp(v));
    }

    // system.sqrt: Square root function
    if (strcmp(name, "system.sqrt") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(sqrt(v));
    }
    // system.pow: Power function (a^b)
    if (strcmp(name, "system.pow") == 0 && arg_count >= 2)
    {
        Value *a = eval_node(interp, args[0]);
        Value *b = eval_node(interp, args[1]);
        double av = a->type == VAL_NUMBER ? a->data.number : 0.0;
        double bv = b->type == VAL_NUMBER ? b->data.number : 0.0;
        value_free(a);
        value_free(b);
        return value_create_number(pow(av, bv));
    }

    /*
     * system.store: Store a value in calculator memory
     *
     * Takes two arguments: name (string) and value to store
     * Creates a copy of the value and stores it in the calculator memory environment
     */
    if (strcmp(name, "system.store") == 0 && arg_count >= 2)
    {
        Value *n = eval_node(interp, args[0]);
        Value *v = eval_node(interp, args[1]);
        if (n->type == VAL_STRING)
        {
            Value *copy = memory_allocate(sizeof(Value));
            memcpy(copy, v, sizeof(Value));
            if (v->type == VAL_STRING)
                copy->data.string = memory_strdup(v->data.string);
            env_set(calc_mem, n->data.string, copy);
        }
        value_free(n);
        value_free(v);
        return value_create_null();
    }
    /*
     * system.recall: Recall a value from calculator memory
     *
     * Takes one argument: name (string) of the stored value
     * Returns: Copy of the stored value, or null if not found
     */
    if (strcmp(name, "system.recall") == 0 && arg_count >= 1)
    {
        Value *n = eval_node(interp, args[0]);
        if (n->type == VAL_STRING)
        {
            Value *val = env_get(calc_mem, n->data.string);
            if (val)
            {
                Value *copy = memory_allocate(sizeof(Value));
                memcpy(copy, val, sizeof(Value));
                if (val->type == VAL_STRING)
                    copy->data.string = memory_strdup(val->data.string);
                value_free(n);
                return copy;
            }
        }
        value_free(n);
        return value_create_null();
    }
    if (strcmp(name, "system.memclear") == 0)
    {
        env_free(calc_mem);
        calc_mem = env_create(NULL);
        return value_create_null();
    }

    /*
     * system.convert: Convert between different units
     *
     * Takes three arguments: value (number), from_unit (string), to_unit (string)
     * Supported conversions:
     * - Length: meters (m), kilometers (km), miles (mi)
     * - Weight: kilograms (kg), pounds (lb)
     * - Temperature: Celsius (C), Fahrenheit (F), Kelvin (K)
     * Returns: Converted value as number, or null if conversion not supported
     */
    if (strcmp(name, "system.convert") == 0 && arg_count >= 3)
    {
        Value *val = eval_node(interp, args[0]);
        Value *from = eval_node(interp, args[1]);
        Value *to = eval_node(interp, args[2]);
        double num = val->type == VAL_NUMBER ? val->data.number : 0.0;
        const char *fu = (from->type == VAL_STRING) ? from->data.string : "";
        const char *tu = (to->type == VAL_STRING) ? to->data.string : "";
        double out = 0.0;
        int ok = 1; // okay
        if (strcmp(fu, "m") == 0 && strcmp(tu, "km") == 0)
            out = num / 1000.0;
        else if (strcmp(fu, "km") == 0 && strcmp(tu, "m") == 0)
            out = num * 1000.0;
        else if (strcmp(fu, "m") == 0 && strcmp(tu, "mi") == 0)
            out = num / 1609.344;
        else if (strcmp(fu, "mi") == 0 && strcmp(tu, "m") == 0)
            out = num * 1609.344;
        else if (strcmp(fu, "kg") == 0 && strcmp(tu, "lb") == 0)
            out = num * 2.20462;
        else if (strcmp(fu, "lb") == 0 && strcmp(tu, "kg") == 0)
            out = num / 2.20462;
        else if (strcmp(fu, "C") == 0 && strcmp(tu, "F") == 0)
            out = num * 9.0 / 5.0 + 32.0;
        else if (strcmp(fu, "F") == 0 && strcmp(tu, "C") == 0)
            out = (num - 32.0) * 5.0 / 9.0;
        else if (strcmp(fu, "C") == 0 && strcmp(tu, "K") == 0)
            out = num + 273.15;
        else if (strcmp(fu, "K") == 0 && strcmp(tu, "C") == 0)
            out = num - 273.15;
        else
            ok = 0; // okay
        value_free(val);
        value_free(from);
        value_free(to);
        if (!ok)
            return value_create_null();
        return value_create_number(out);
    }

    /*
     * system.history.add: Add a value to the command history
     *
     * Takes one argument: value to add to history
     * Automatically grows the history array when needed
     * Returns: null
     */
    if (strcmp(name, "system.history.add") == 0 && arg_count >= 1)
    {
        Value *v = eval_node(interp, args[0]);
        if (history_count >= history_capacity)
        {
            history_capacity *= 2;
            history = memory_reallocate(history, sizeof(Value *) * history_capacity);
        }
        history[history_count++] = v;
        return value_create_null();
    }
    /*
     * system.history.get: Get all values from command history
     *
     * Returns: Array containing copies of all history values
     * The returned array is a copy, so modifications don't affect the original history
     */
    if (strcmp(name, "system.history.get") == 0)
    {
        Value *arr = value_create_array();
        for (int i = 0; i < history_count; i++)
        {
            if (arr->data.array.count >= arr->data.array.capacity)
            {
                arr->data.array.capacity *= 2;
                arr->data.array.elements = memory_reallocate(
                    arr->data.array.elements,
                    sizeof(Value *) * arr->data.array.capacity);
            }
            Value *copy = memory_allocate(sizeof(Value));
            memcpy(copy, history[i], sizeof(Value));
            if (history[i]->type == VAL_STRING)
                copy->data.string = memory_strdup(history[i]->data.string);
            arr->data.array.elements[arr->data.array.count++] = copy;
        }
        return arr;
    }
    /*
     * system.history.clear: Clear all values from command history
     *
     * Frees all history values and resets the history count to zero
     * Returns: null
     */
    if (strcmp(name, "system.history.clear") == 0)
    {
        for (int i = 0; i < history_count; i++)
        {
            value_free(history[i]);
        }
        history_count = 0;
        return value_create_null();
    }

    /*
     * system.len: Get the length of a value
     *
     * Takes one argument: value to measure
     * Returns: Length as number
     * - For strings: character count
     * - For arrays: element count
     * - For other types: 0
     */
    if (strcmp(name, "system.len") == 0 && arg_count > 0)
    {
        Value *val = eval_node(interp, args[0]);
        int len = 0;

        if (val->type == VAL_STRING)
        {
            len = strlen(val->data.string);
        }
        else if (val->type == VAL_ARRAY)
        {
            len = val->data.array.count;
        }

        value_free(val);
        return value_create_number(len);
    }

    /*
     * system.type: Get the type of a value as a string
     *
     * Takes one argument: value to check
     * Returns: String representation of the type
     * Possible return values: "number", "string", "boolean", "array", "function", "null"
     */
    if (strcmp(name, "system.type") == 0 && arg_count > 0)
    {
        Value *val = eval_node(interp, args[0]);
        const char *type_name;

        switch (val->type)
        {
        case VAL_NUMBER:
            type_name = "number";
            break;
        case VAL_STRING:
            type_name = "string";
            break;
        case VAL_BOOLEAN:
            type_name = "boolean";
            break;
        case VAL_ARRAY:
            type_name = "array";
            break;
        case VAL_FUNCTION:
            type_name = "function";
            break;
        default:
            type_name = "null";
            break;
        }

        value_free(val);
        return value_create_string(type_name);
    }

    /*
     * system.annotate: Add a type annotation to a variable
     *
     * Takes two arguments: variable_name (string), type_name (string)
     * Updates the type annotation for an existing variable in the current environment
     * Returns: null
     */
    if (strcmp(name, "system.annotate") == 0 && arg_count >= 2)
    {
        Value *n = eval_node(interp, args[0]);
        Value *t = eval_node(interp, args[1]);
        if (n->type == VAL_STRING && t->type == VAL_STRING)
        {
            Environment *env = interp->current;
            for (int i = 0; i < env->count; i++)
            {
                if (strcmp(env->names[i], n->data.string) == 0)
                {
                    if (env->types[i])
                        memory_free(env->types[i]);
                    env->types[i] = memory_strdup(t->data.string);
                    break;
                }
            }
        }
        value_free(n);
        value_free(t);
        return value_create_null();
    }

    /*
     * system.throw: Throw a custom error
     *
     * Takes up to three arguments: error_name (string), error_message (string), error_code (number)
     * Default values: name="Error", message="", code=0
     * Throws the error immediately and returns null
     */
    if (strcmp(name, "system.throw") == 0 && arg_count >= 1)
    {
        const char *namev = "Error";
        const char *msg = "";
        int code = 0;
        if (arg_count >= 1)
        {
            Value *n = eval_node(interp, args[0]);
            if (n->type == VAL_STRING)
                namev = n->data.string;
            value_free(n);
        }
        if (arg_count >= 2)
        {
            Value *m = eval_node(interp, args[1]);
            if (m->type == VAL_STRING)
                msg = m->data.string;
            value_free(m);
        }
        if (arg_count >= 3)
        {
            Value *c = eval_node(interp, args[2]);
            code = (c->type == VAL_NUMBER) ? (int)c->data.number : 0;
            value_free(c);
        }
        Value *err = value_create_error(namev, msg, code);
        throw_error(interp, err);
        return value_create_null();
    }

    /*
     * file.read: Read contents of a file
     *
     * Takes one argument: file_path (string)
     * Returns: String containing file contents, or null if file cannot be read
     */
    if (strcmp(name, "file.read") == 0 && arg_count >= 1)
    {
        Value *p = eval_node(interp, args[0]);
        Value *out = value_create_null();
        if (p->type == VAL_STRING)
            out = io_read_file(p->data.string);
        value_free(p);
        return out;
    }

    /*
     * file.write: Write data to a file
     *
     * Takes two arguments: file_path (string), data (any type)
     * Writes the string representation of data to the specified file
     * Returns: null
     */
    if (strcmp(name, "file.write") == 0 && arg_count >= 2)
    {
        Value *p = eval_node(interp, args[0]);
        Value *d = eval_node(interp, args[1]);
        if (p->type == VAL_STRING)
            io_write_file(p->data.string, d);
        value_free(p);
        value_free(d);
        return value_create_null();
    }

    return value_create_null(); // return the null function for *node
}

/*
 * eval_node: Evaluate an AST node and return its computed value
 *
 * This is the core evaluation function that processes different types of AST nodes:
 * - Literals (numbers, strings, booleans, null) are converted to Value objects
 * - Identifiers are looked up in the current environment
 * - Binary and unary operations are evaluated recursively
 * - Function calls are handled by calling eval_builtin or user-defined functions
 * - Control flow (if/else, while loops) are executed with proper scoping
 * - Variable assignments and declarations update the environment
 *
 * Returns: A new Value object representing the result of evaluation
 * Note: The caller is responsible for freeing the returned Value
 */
static Value *eval_node(Interpreter *interp, ASTNode *node)
{
    if (!node)
        return value_create_null();

    switch (node->type)
    {
    case AST_NUMBER:
        // Convert numeric literal to Value
        return value_create_number(node->data.number.value);

    case AST_STRING:
        // Convert string literal to Value
        return value_create_string(node->data.string.value);

    case AST_BOOLEAN:
        // Convert boolean literal to Value
        return value_create_boolean(node->data.boolean.value);

    case AST_NULL:
        // Convert null literal to Value
        return value_create_null();

    case AST_IDENTIFIER:
    {
        // Look up variable in current environment and return a copy
        Value *val = env_get(interp->current, node->data.identifier.name);
        if (!val)
        {
            fprintf(stderr, "Undefined variable: %s\n", node->data.identifier.name);
            return value_create_null();
        }

        // Create a copy of the value to avoid reference issues
        Value *copy = memory_allocate(sizeof(Value));
        memcpy(copy, val, sizeof(Value));
        if (val->type == VAL_STRING)
        {
            // Deep copy string data
            copy->data.string = memory_strdup(val->data.string);
        }
        return copy;
    }

    case AST_BINARY_OP:
        // Delegate binary operations to specialized function
        return eval_binary_op(interp, node);

    case AST_UNARY_OP:
    {
        // Evaluate unary operations (NOT, negation)
        Value *operand = eval_node(interp, node->data.unary_op.operand);

        if (node->data.unary_op.op == TOKEN_NOT)
        {
            int result = !value_is_truthy(operand);
            value_free(operand);
            return value_create_boolean(result);
        }

        if (node->data.unary_op.op == TOKEN_SUB)
        {
            double result = -operand->data.number;
            value_free(operand);
            return value_create_number(result);
        }

        value_free(operand);
        return value_create_null();
    }

    case AST_ASSIGN:
    {
        // Handle variable assignment and declaration with various operators
        Value *value = eval_node(interp, node->data.assign.value);

        if (node->data.assign.op == TOKEN_PLUS_ASSIGN)
        {
            Value *old = env_get(interp->current, node->data.assign.name);
            if (old && old->type == VAL_NUMBER && value->type == VAL_NUMBER)
            {
                Value *new_val = value_create_number(old->data.number + value->data.number);
                value_free(value);
                value = new_val;
            }
        }
        else if (node->data.assign.op == TOKEN_MINUS_ASSIGN)
        {
            Value *old = env_get(interp->current, node->data.assign.name);
            if (old && old->type == VAL_NUMBER && value->type == VAL_NUMBER)
            {
                Value *new_val = value_create_number(old->data.number - value->data.number);
                value_free(value);
                value = new_val;
            }
        }
        else if (node->data.assign.op == TOKEN_MUL_ASSIGN)
        {
            Value *old = env_get(interp->current, node->data.assign.name);
            if (old && old->type == VAL_NUMBER && value->type == VAL_NUMBER)
            {
                Value *new_val = value_create_number(old->data.number * value->data.number);
                value_free(value);
                value = new_val;
            }
        }
        else if (node->data.assign.op == TOKEN_DIV_ASSIGN)
        {
            Value *old = env_get(interp->current, node->data.assign.name);
            if (old && old->type == VAL_NUMBER && value->type == VAL_NUMBER)
            {
                Value *new_val = value_create_number(old->data.number / value->data.number);
                value_free(value);
                value = new_val;
            }
        }
        else if (node->data.assign.op == TOKEN_MOD_ASSIGN)
        {
            Value *old = env_get(interp->current, node->data.assign.name);
            if (old && old->type == VAL_NUMBER && value->type == VAL_NUMBER)
            {
                Value *new_val = value_create_number(fmod(old->data.number, value->data.number));
                value_free(value);
                value = new_val;
            }
        }
        else if (node->data.assign.op == TOKEN_ASSIGN)
        {
            if (!env_has(interp->current, node->data.assign.name))
            {
                fprintf(stderr, "Assignment to undeclared variable: %s\n", node->data.assign.name);
                value_free(value);
                return value_create_null();
            }
        }
        else if (node->data.assign.op == TOKEN_INSERT)
        {
            if (node->data.assign.type_name)
            {
                const char *tn = type_name(value);
                if (strcmp(tn, node->data.assign.type_name) != 0 && strcmp(node->data.assign.type_name, "unknown") != 0)
                {
                    fprintf(stderr, "Type mismatch for %s: expected %s, got %s\n",
                            node->data.assign.name, node->data.assign.type_name, tn);
                    value_free(value);
                    return value_create_null();
                }
            }
            env_declare(interp->current, node->data.assign.name, value, 0);
            if (node->data.assign.type_name)
                env_annotate(interp->current, node->data.assign.name, node->data.assign.type_name);
            return value_create_null();
        }
        else if (node->data.assign.op == TOKEN_CONST)
        {
            if (node->data.assign.type_name)
            {
                const char *tn = type_name(value);
                if (strcmp(tn, node->data.assign.type_name) != 0 && strcmp(node->data.assign.type_name, "unknown") != 0)
                {
                    fprintf(stderr, "Type mismatch for %s: expected %s, got %s\n",
                            node->data.assign.name, node->data.assign.type_name, tn);
                    value_free(value);
                    return value_create_null();
                }
            }
            env_declare(interp->current, node->data.assign.name, value, 1);
            if (node->data.assign.type_name)
                env_annotate(interp->current, node->data.assign.name, node->data.assign.type_name);
            return value_create_null();
        }

        env_set(interp->current, node->data.assign.name, value);
        return value_create_null();
    }

    case AST_IF:
    {
        // Evaluate if-else statement
        Value *condition = eval_node(interp, node->data.if_stmt.condition);
        int is_true = value_is_truthy(condition);
        value_free(condition);

        if (is_true)
        {
            return eval_node(interp, node->data.if_stmt.then_block);
        }
        else if (node->data.if_stmt.else_block)
        {
            return eval_node(interp, node->data.if_stmt.else_block);
        }
        return value_create_null();
    }

    case AST_WHILE:
    {
        // Evaluate while loop with break/continue/return support
        Value *result = value_create_null();

        while (1)
        {
            Value *condition = eval_node(interp, node->data.while_stmt.condition);
            int is_true = value_is_truthy(condition);
            value_free(condition);

            if (!is_true)
                break;

            value_free(result);
            result = eval_node(interp, node->data.while_stmt.body);

            if (result->type == VAL_BREAK)
            {
                value_free(result);
                return value_create_null();
            }
            if (result->type == VAL_CONTINUE)
            {
                value_free(result);
                result = value_create_null();
                continue;
            }
            if (result->type == VAL_RETURN)
            {
                return result;
            }
        }

        return result;
    }

    case AST_FOR:
    {
        if (node->data.for_stmt.init)
        {
            Value *init_val = eval_node(interp, node->data.for_stmt.init);
            value_free(init_val);
        }

        Value *result = value_create_null();

        while (1)
        {
            if (node->data.for_stmt.condition)
            {
                Value *condition = eval_node(interp, node->data.for_stmt.condition);
                int is_true = value_is_truthy(condition);
                value_free(condition);
                if (!is_true)
                    break;
            }

            value_free(result);
            result = eval_node(interp, node->data.for_stmt.body);

            if (result->type == VAL_BREAK)
            {
                value_free(result);
                return value_create_null();
            }
            if (result->type == VAL_CONTINUE)
            {
                value_free(result);
                result = value_create_null();
            }
            if (result->type == VAL_RETURN)
            {
                return result;
            }

            if (node->data.for_stmt.increment)
            {
                Value *inc_val = eval_node(interp, node->data.for_stmt.increment);
                value_free(inc_val);
            }
        }

        return result;
    }

    case AST_FUNCTION:
    {
        // Define a named function in the current environment
        Value *func = value_create_function(node, interp->current);
        env_set(interp->current, node->data.function.name, func);
        return value_create_null();
    }

    case AST_CALL:
    {
        // Handle function calls (both built-in and user-defined)
        if (strcmp(node->data.call.name, "system.print") == 0 ||
            strcmp(node->data.call.name, "system.input") == 0 ||
            strcmp(node->data.call.name, "system.len") == 0 ||
            strcmp(node->data.call.name, "system.type") == 0 ||
            strcmp(node->data.call.name, "system.output") == 0 ||
            strcmp(node->data.call.name, "system.error") == 0 ||
            strcmp(node->data.call.name, "system.warning") == 0 ||
            strcmp(node->data.call.name, "system.annotate") == 0 ||
            strcmp(node->data.call.name, "system.throw") == 0 ||
            strcmp(node->data.call.name, "system.sin") == 0 ||
            strcmp(node->data.call.name, "system.cos") == 0 ||
            strcmp(node->data.call.name, "system.tan") == 0 ||
            strcmp(node->data.call.name, "system.asin") == 0 ||
            strcmp(node->data.call.name, "system.acos") == 0 ||
            strcmp(node->data.call.name, "system.atan") == 0 ||
            strcmp(node->data.call.name, "system.log") == 0 ||
            strcmp(node->data.call.name, "system.ln") == 0 ||
            strcmp(node->data.call.name, "system.exp") == 0 ||
            strcmp(node->data.call.name, "system.sqrt") == 0 ||
            strcmp(node->data.call.name, "system.pow") == 0 ||
            strcmp(node->data.call.name, "system.store") == 0 ||
            strcmp(node->data.call.name, "system.recall") == 0 ||
            strcmp(node->data.call.name, "system.memclear") == 0 ||
            strcmp(node->data.call.name, "system.convert") == 0 ||
            strcmp(node->data.call.name, "system.history.add") == 0 ||
            strcmp(node->data.call.name, "system.history.get") == 0 ||
            strcmp(node->data.call.name, "system.history.clear") == 0 ||
            strcmp(node->data.call.name, "file.read") == 0 ||
            strcmp(node->data.call.name, "file.write") == 0)
        {
            return eval_builtin(interp, node->data.call.name,
                                node->data.call.args, node->data.call.arg_count);
        }

        Value *func = env_get(interp->current, node->data.call.name);
        if (!func || func->type != VAL_FUNCTION)
        {
            fprintf(stderr, "Undefined function: %s\n", node->data.call.name);
            return value_create_null();
        }

        ASTNode *func_node = func->data.function.function;
        Environment *func_env = env_create(func->data.function.closure);

        for (int i = 0; i < func_node->data.function.param_count; i++)
        {
            if (i < node->data.call.arg_count)
            {
                Value *arg = eval_node(interp, node->data.call.args[i]);
                env_set(func_env, func_node->data.function.params[i], arg);
            }
            else if (func_node->data.function.defaults && func_node->data.function.defaults[i])
            {
                Value *defv = eval_node(interp, func_node->data.function.defaults[i]);
                env_set(func_env, func_node->data.function.params[i], defv);
            }
            else
            {
                env_set(func_env, func_node->data.function.params[i], value_create_null());
            }
        }

        Environment *saved_env = interp->current;
        interp->current = func_env;
        Value *result = eval_node(interp, func_node->data.function.body);
        interp->current = saved_env;

        if (result->type == VAL_RETURN)
        {
            Value *ret_val = result->data.return_val.value;
            result->data.return_val.value = NULL;
            value_free(result);
            env_free(func_env);
            return ret_val ? ret_val : value_create_null();
        }

        env_free(func_env);
        value_free(result);
        return value_create_null();
    }

    case AST_RETURN:
        return value_create_return(eval_node(interp, node->data.return_stmt.value));

    case AST_BREAK:
        return value_create_break();

    case AST_CONTINUE:
        return value_create_continue();

    case AST_BLOCK:
    {
        Value *result = value_create_null();
        for (int i = 0; i < node->data.block.count; i++)
        {
            value_free(result);
            result = eval_node(interp, node->data.block.statements[i]);

            if (result->type == VAL_BREAK || result->type == VAL_CONTINUE ||
                result->type == VAL_RETURN)
            {
                return result;
            }
        }
        return result;
    }

    case AST_NAMESPACE:
    {
        Environment *saved = interp->current;
        Environment *ns_env = env_create(saved);
        interp->current = ns_env;
        Value *ns_result = eval_node(interp, node->data.namespace_decl.body);
        value_free(ns_result);
        for (int i = 0; i < ns_env->count; i++)
        {
            int name_len = (int)strlen(node->data.namespace_decl.name) + 1 + (int)strlen(ns_env->names[i]) + 1;
            char *full_name = memory_allocate(name_len);
            sprintf(full_name, "%s.%s", node->data.namespace_decl.name, ns_env->names[i]);
            Value *vclone = value_clone(ns_env->values[i]);
            env_declare(saved, full_name, vclone, ns_env->is_const[i]);
            memory_free(full_name);
        }
        interp->current = saved;
        return value_create_null();
    }

    case AST_ENUM:
    {
        for (int i = 0; i < node->data.enum_decl.count; i++)
        {
            int name_len = (int)strlen(node->data.enum_decl.name) + 1 + (int)strlen(node->data.enum_decl.members[i]) + 1;
            char *full_name = memory_allocate(name_len);
            sprintf(full_name, "%s.%s", node->data.enum_decl.name, node->data.enum_decl.members[i]);
            Value *val = value_create_number(node->data.enum_decl.values[i]);
            env_declare(interp->current, full_name, val, 1);
            memory_free(full_name);
        }
        return value_create_null();
    }

    case AST_ARRAY:
    {
        Value *arr = value_create_array();
        for (int i = 0; i < node->data.array.count; i++)
        {
            Value *elem = eval_node(interp, node->data.array.elements[i]);

            if (arr->data.array.count >= arr->data.array.capacity)
            {
                arr->data.array.capacity *= 2;
                arr->data.array.elements = memory_reallocate(
                    arr->data.array.elements,
                    sizeof(Value *) * arr->data.array.capacity);
            }

            arr->data.array.elements[arr->data.array.count++] = elem;
        }
        return arr;
    }

    case AST_INDEX:
    {
        Value *obj = eval_node(interp, node->data.index_expr.object);
        Value *idx = eval_node(interp, node->data.index_expr.index);

        if (obj->type == VAL_ARRAY && idx->type == VAL_NUMBER)
        {
            int index = (int)idx->data.number;
            if (index >= 0 && index < obj->data.array.count)
            {
                Value *elem = obj->data.array.elements[index];
                Value *copy = memory_allocate(sizeof(Value));
                memcpy(copy, elem, sizeof(Value));
                if (elem->type == VAL_STRING)
                {
                    copy->data.string = memory_strdup(elem->data.string);
                }
                value_free(obj);
                value_free(idx);
                return copy;
            }
        }

        value_free(obj);
        value_free(idx);
        return value_create_null();
    }

    case AST_TRY_CATCH: // try-catch statement
    {
        Value *result = value_create_null();
        Value *error_value = NULL;

        // Execute try block
        if (setjmp(interp->jmp_buf) == 0)
        {
            // Normal execution path
            value_free(result);
            result = eval_node(interp, node->data.try_stmt.try_block);
        }
        else
        {
            // Exception was thrown
            error_value = interp->current_error;
            interp->current_error = NULL;

            // Execute catch block if available
            if (node->data.try_stmt.catch_block && error_value)
            {
                // Store error in variable if specified
                if (node->data.try_stmt.error_var)
                {
                    Value *error_copy = value_clone(error_value);
                    env_set(interp->current, node->data.try_stmt.error_var, error_copy);
                }

                value_free(result);
                result = eval_node(interp, node->data.try_stmt.catch_block);
            }
        }

        // Execute finally block if available (always executes)
        if (node->data.try_stmt.finally_block)
        {
            Value *finally_result = eval_node(interp, node->data.try_stmt.finally_block);
            value_free(finally_result); // Finally result doesn't affect return value
        }

        return result;
    }

    case AST_MATCH:
    {
        // Evaluate the expression to match against
        Value *match_value = eval_node(interp, node->data.match_stmt.expr);
        Value *result = value_create_null();

        // Try each case
        int matched = 0;
        for (int i = 0; i < node->data.match_stmt.case_count; i++)
        {
            Value *case_value = eval_node(interp, node->data.match_stmt.cases[i]);

            // Check if values match (using loose equality for now)
            if (values_equal(match_value, case_value))
            {
                matched = 1;
                value_free(result);
                result = eval_node(interp, node->data.match_stmt.bodies[i]);
                value_free(case_value);
                break;
            }
            value_free(case_value);
        }

        // If no case matched and there's a default case, execute it
        if (!matched && node->data.match_stmt.default_case)
        {
            value_free(result);
            result = eval_node(interp, node->data.match_stmt.default_case);
        }

        value_free(match_value);
        return result;
    }

    case AST_FOR_IN:
    {
        // Evaluate the collection
        Value *collection = eval_node(interp, node->data.for_in.collection);
        Value *result = value_create_null();

        if (collection->type == VAL_ARRAY)
        {
            // Iterate over array elements
            for (int i = 0; i < collection->data.array.count; i++)
            {
                // Set the loop variable
                Value *element = collection->data.array.elements[i];
                Value *element_copy = value_clone(element);
                env_set(interp->current, node->data.for_in.var, element_copy);

                // Execute the loop body
                value_free(result);
                result = eval_node(interp, node->data.for_in.body);

                // Handle control flow statements
                if (result->type == VAL_BREAK)
                {
                    value_free(result);
                    result = value_create_null();
                    break;
                }
                else if (result->type == VAL_CONTINUE)
                {
                    value_free(result);
                    result = value_create_null();
                    continue;
                }
                else if (result->type == VAL_RETURN)
                {
                    // Return from function
                    value_free(collection);
                    return result;
                }
            }
        }
        else if (collection->type == VAL_MAP)
        {
            // Iterate over map entries (key-value pairs)
            for (int i = 0; i < collection->data.map.count; i++)
            {
                // Create a tuple-like object for key-value pair
                Value *pair = value_create_map();

                // Add key and value to the pair
                Value *key_val = value_create_string(collection->data.map.keys[i]);
                Value *value_val = value_clone(collection->data.map.values[i]);

                // Simple map for the pair (could be enhanced with proper tuple support)
                if (pair->data.map.count >= pair->data.map.capacity)
                {
                    pair->data.map.capacity *= 2;
                    pair->data.map.keys = memory_reallocate(pair->data.map.keys, sizeof(char *) * pair->data.map.capacity);
                    pair->data.map.values = memory_reallocate(pair->data.map.values, sizeof(Value *) * pair->data.map.capacity);
                }

                pair->data.map.keys[pair->data.map.count] = memory_strdup("key");
                pair->data.map.values[pair->data.map.count] = key_val;
                pair->data.map.count++;

                pair->data.map.keys[pair->data.map.count] = memory_strdup("value");
                pair->data.map.values[pair->data.map.count] = value_val;
                pair->data.map.count++;

                // Set the loop variable
                env_set(interp->current, node->data.for_in.var, pair);

                // Execute the loop body
                value_free(result);
                result = eval_node(interp, node->data.for_in.body);

                // Handle control flow statements
                if (result->type == VAL_BREAK)
                {
                    value_free(result);
                    result = value_create_null();
                    break;
                }
                else if (result->type == VAL_CONTINUE)
                {
                    value_free(result);
                    result = value_create_null();
                    continue;
                }
                else if (result->type == VAL_RETURN)
                {
                    // Return from function
                    value_free(collection);
                    value_free(pair);
                    return result;
                }

                value_free(pair);
            }
        }
        else
        {
            fprintf(stderr, "Error: for-in loop requires an array or map, got type %d\n", collection->type);
        }

        value_free(collection);
        return result;
    }

    case AST_LAMBDA:
    {
        // Create a function value from the lambda
        Value *func = value_create_function(node, interp->current);
        return func;
    }

    default:
        return value_create_null();
    }
}

/*
 * throw_error: Throw an error for try-catch handling
 *
 * Stores the error in the interpreter and performs a non-local jump
 * to the nearest catch block. This is used by the system.throw function
 * and can be used by other built-in functions to signal errors.
 *
 * The error value is stored in interp->current_error for retrieval
 * by the catch block, and longjmp transfers control to the setjmp
 * point established by the try-catch statement.
 */
void throw_error(Interpreter *interp, Value *error)
{
    interp->current_error = error;
    longjmp(interp->jmp_buf, 1);
}

/*
 * interpreter_eval: Public interface for evaluating an AST node
 *
 * This is the main entry point for evaluating AST nodes from outside
 * the interpreter module. It delegates to the internal eval_node function.
 *
 * Returns: A new Value object representing the result of evaluation
 * Note: The caller is responsible for freeing the returned Value
 */
Value *interpreter_eval(Interpreter *interp, ASTNode *node)
{
    return eval_node(interp, node);
}

// END OF interpreter.c
