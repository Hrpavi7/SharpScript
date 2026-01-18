/*
    copyright sharpscript language.
    licensed under the mit license
*/

#include "include/interpreter.h"
#include "include/memory.h"
#include "builtins/io.h"
#include "builtins/docs.h"
#include "builtins/errors.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

static Environment *calc_mem = NULL;
static Value **history = NULL;
static int history_count = 0;
static int history_capacity = 0;

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

static Value *value_create_number(double num)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_NUMBER;
    val->data.number = num;
    return val;
}

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

static Value *value_create_boolean(int b)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_BOOLEAN;
    val->data.boolean = b;
    return val;
}

static Value *value_create_null(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_NULL;
    return val;
}

static Value *value_create_function(ASTNode *func, Environment *closure)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_FUNCTION;
    val->data.function.function = func;
    val->data.function.closure = closure;
    return val;
}

static Value *value_create_array(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_ARRAY;
    val->data.array.elements = memory_allocate(sizeof(Value *) * 8);
    val->data.array.count = 0;
    val->data.array.capacity = 8;
    return val;
}

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

static Value *value_create_break(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_BREAK;
    return val;
}

static Value *value_create_continue(void)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_CONTINUE;
    return val;
}

static Value *value_create_return(Value *ret_val)
{
    Value *val = memory_allocate(sizeof(Value));
    val->type = VAL_RETURN;
    val->data.return_val.value = ret_val;
    return val;
}

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
        printf("]");
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
        printf("}");
        break;
    case VAL_FUNCTION:
        printf("<function>");
        break;
    default:
        printf("null");
        break;
    }
}

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

static Value *eval_binary_op(Interpreter *interp, ASTNode *node)
{
    Value *left = eval_node(interp, node->data.binary_op.left);
    Value *right = eval_node(interp, node->data.binary_op.right);

    if (node->data.binary_op.op == TOKEN_ADD)
    {
        if (left->type == VAL_STRING || right->type == VAL_STRING)
        {
            char buffer[1024];
            char left_str[512], right_str[512];

            if (left->type == VAL_STRING)
                strcpy(left_str, left->data.string);
            else if (left->type == VAL_NUMBER)
                sprintf(left_str, "%g", left->data.number);
            else if (left->type == VAL_BOOLEAN)
                strcpy(left_str, left->data.boolean ? "true" : "false");
            else
                strcpy(left_str, "null");

            if (right->type == VAL_STRING)
                strcpy(right_str, right->data.string);
            else if (right->type == VAL_NUMBER)
                sprintf(right_str, "%g", right->data.number);
            else if (right->type == VAL_BOOLEAN)
                strcpy(right_str, right->data.boolean ? "true" : "false");
            else
                strcpy(right_str, "null");

            sprintf(buffer, "%s%s", left_str, right_str);
            Value *result = value_create_string(buffer);
            value_free(left);
            value_free(right);
            return result;
        }

        double result = left->data.number + right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_SUB)
    {
        double result = left->data.number - right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_MUL)
    {
        double result = left->data.number * right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_DIV)
    {
        double result = left->data.number / right->data.number;
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_MOD)
    {
        double result = fmod(left->data.number, right->data.number);
        value_free(left);
        value_free(right);
        return value_create_number(result);
    }

    if (node->data.binary_op.op == TOKEN_EQ)
    {
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
        int result = 1;
        if (left->type == VAL_NUMBER && right->type == VAL_NUMBER)
        {
            result = left->data.number != right->data.number;
        }
        else if (left->type == VAL_STRING && right->type == VAL_STRING)
        {
            result = strcmp(left->data.string, right->data.string) != 0;
        }
        // line 444
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
        int result = left->data.number < right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_GT)
    {
        int result = left->data.number > right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_LTE)
    {
        int result = left->data.number <= right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_GTE)
    {
        int result = left->data.number >= right->data.number;
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_AND)
    {
        int result = value_is_truthy(left) && value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    if (node->data.binary_op.op == TOKEN_OR)
    {
        int result = value_is_truthy(left) || value_is_truthy(right);
        value_free(left);
        value_free(right);
        return value_create_boolean(result);
    }

    value_free(left);
    value_free(right);
    return value_create_null();
}

static Value *eval_builtin(Interpreter *interp, const char *name, ASTNode **args, int arg_count)
{
    if (strcmp(name, "system.print") == 0)
    {
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
                // print non-strings via value_print to stdout, capture via buffer? Simpler: just use value_print to stdout.
                // Fallback to number/boolean printing to stderr
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
            buffer[strcspn(buffer, "\n")] = 0;
            return value_create_string(buffer);
        }
        return value_create_string("");
    }

    // oh boy, if statmenets
    if (strcmp(name, "system.sin") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(sin(v));
    }
    if (strcmp(name, "system.cos") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(cos(v));
    }
    if (strcmp(name, "system.tan") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(tan(v));
    }
    if (strcmp(name, "system.asin") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(asin(v));
    }
    if (strcmp(name, "system.acos") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(acos(v));
    }
    if (strcmp(name, "system.atan") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(atan(v));
    }
    if (strcmp(name, "system.log") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(log10(v));
    }
    if (strcmp(name, "system.ln") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        // line 666 hehehe
        value_free(x);
        return value_create_number(log(v));
    }
    if (strcmp(name, "system.exp") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(exp(v));
    }
    if (strcmp(name, "system.sqrt") == 0 && arg_count >= 1)
    {
        Value *x = eval_node(interp, args[0]);
        double v = x->type == VAL_NUMBER ? x->data.number : 0.0;
        value_free(x);
        return value_create_number(sqrt(v));
    }
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
    if (strcmp(name, "system.history.clear") == 0)
    {
        for (int i = 0; i < history_count; i++)
        {
            value_free(history[i]);
        }
        history_count = 0;
        return value_create_null();
    }

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

    if (strcmp(name, "file.read") == 0 && arg_count >= 1)
    {
        Value *p = eval_node(interp, args[0]);
        Value *out = value_create_null();
        if (p->type == VAL_STRING)
            out = io_read_file(p->data.string);
        value_free(p);
        return out;
    }

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

static Value *eval_node(Interpreter *interp, ASTNode *node)
{
    if (!node)
        return value_create_null();

    switch (node->type)
    {
    case AST_NUMBER:
        return value_create_number(node->data.number.value);

    case AST_STRING:
        return value_create_string(node->data.string.value);

    case AST_BOOLEAN:
        return value_create_boolean(node->data.boolean.value);

    case AST_NULL:
        return value_create_null();

    case AST_IDENTIFIER:
    {
        Value *val = env_get(interp->current, node->data.identifier.name);
        if (!val)
        {
            fprintf(stderr, "Undefined variable: %s\n", node->data.identifier.name);
            return value_create_null();
        }

        Value *copy = memory_allocate(sizeof(Value));
        memcpy(copy, val, sizeof(Value));
        if (val->type == VAL_STRING)
        {
            copy->data.string = memory_strdup(val->data.string);
        }
        return copy;
    }

    case AST_BINARY_OP:
        return eval_binary_op(interp, node);

    case AST_UNARY_OP:
    {
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
        Value *func = value_create_function(node, interp->current);
        env_set(interp->current, node->data.function.name, func);
        return value_create_null();
    }

    case AST_CALL:
    {
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

// Throw an error (used by try-catch)
void throw_error(Interpreter *interp, Value *error)
{
    interp->current_error = error;
    longjmp(interp->jmp_buf, 1);
}

Value *interpreter_eval(Interpreter *interp, ASTNode *node)
{
    return eval_node(interp, node);
}
