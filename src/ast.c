#include "include/ast.h"
#include "include/memory.h"

ASTNode *ast_create_number(double value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NUMBER;
    node->data.number.value = value;
    return node;
}

ASTNode *ast_create_string(const char *value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_STRING;
    node->data.string.value = memory_strdup(value);
    return node;
}

ASTNode *ast_create_boolean(int value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BOOLEAN;
    node->data.boolean.value = value;
    return node;
}

ASTNode *ast_create_null(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NULL;
    return node;
}

ASTNode *ast_create_identifier(const char *name)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->data.identifier.name = memory_strdup(name);
    return node;
}

ASTNode *ast_create_binary_op(TokenType op, ASTNode *left, ASTNode *right)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BINARY_OP;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

ASTNode *ast_create_unary_op(TokenType op, ASTNode *operand)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

ASTNode *ast_create_assign(const char *name, ASTNode *value, TokenType op)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_ASSIGN;
    node->data.assign.name = memory_strdup(name);
    node->data.assign.value = value;
    node->data.assign.op = op;
    node->data.assign.type_name = NULL;
    return node;
}

ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_IF;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = then_block;
    node->data.if_stmt.else_block = else_block;
    return node;
}

ASTNode *ast_create_while(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

ASTNode *ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_FOR;
    node->data.for_stmt.init = init;
    node->data.for_stmt.condition = condition;
    node->data.for_stmt.increment = increment;
    node->data.for_stmt.body = body;
    return node;
}

ASTNode *ast_create_function(const char *name, char **params, int param_count, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_FUNCTION;
    node->data.function.name = memory_strdup(name);
    node->data.function.params = params;
    node->data.function.param_count = param_count;
    node->data.function.defaults = NULL;
    node->data.function.body = body;
    return node;
}

ASTNode *ast_create_call(const char *name, ASTNode **args, int arg_count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CALL;
    node->data.call.name = memory_strdup(name);
    node->data.call.args = args;
    node->data.call.arg_count = arg_count;
    return node;
}

ASTNode *ast_create_return(ASTNode *value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->data.return_stmt.value = value;
    return node;
}

ASTNode *ast_create_break(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BREAK;
    return node;
}

ASTNode *ast_create_continue(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CONTINUE;
    return node;
}

ASTNode *ast_create_block(ASTNode **statements, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->data.block.statements = statements;
    node->data.block.count = count;
    return node;
}

ASTNode *ast_create_array(ASTNode **elements, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_ARRAY;
    node->data.array.elements = elements;
    node->data.array.count = count;
    return node;
}

ASTNode *ast_create_index(ASTNode *object, ASTNode *index)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_INDEX;
    node->data.index_expr.object = object;
    node->data.index_expr.index = index;
    return node;
}

ASTNode *ast_create_namespace(const char *name, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NAMESPACE;
    node->data.namespace_decl.name = memory_strdup(name);
    node->data.namespace_decl.body = body;
    return node;
}

ASTNode *ast_create_enum(const char *name, char **members, double *values, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_ENUM;
    node->data.enum_decl.name = memory_strdup(name);
    node->data.enum_decl.members = members;
    node->data.enum_decl.values = values;
    node->data.enum_decl.count = count;
    return node;
}

ASTNode *ast_create_class(const char *name, const char *base, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CLASS;
    node->data.class_decl.name = memory_strdup(name);
    node->data.class_decl.base = base ? memory_strdup(base) : NULL;
    node->data.class_decl.body = body;
    return node;
}

ASTNode *ast_create_map(ASTNode **keys, ASTNode **values, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_MAP;
    node->data.map_expr.keys = keys;
    node->data.map_expr.values = values;
    node->data.map_expr.count = count;
    return node;
}

ASTNode *ast_create_lambda(char **params, int param_count, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_LAMBDA;
    node->data.lambda.params = params;
    node->data.lambda.param_count = param_count;
    node->data.lambda.body = body;
    return node;
}

ASTNode *ast_create_match(ASTNode *expr, ASTNode **cases, ASTNode **bodies, int case_count, ASTNode *default_case)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_MATCH;
    node->data.match_stmt.expr = expr;
    node->data.match_stmt.cases = cases;
    node->data.match_stmt.bodies = bodies;
    node->data.match_stmt.case_count = case_count;
    node->data.match_stmt.default_case = default_case;
    return node;
}

ASTNode *ast_create_try_catch(ASTNode *try_block, const char *error_var, ASTNode *catch_block, ASTNode *finally_block)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_TRY_CATCH;
    node->data.try_stmt.try_block = try_block;
    node->data.try_stmt.error_var = error_var ? memory_strdup(error_var) : NULL;
    node->data.try_stmt.catch_block = catch_block;
    node->data.try_stmt.finally_block = finally_block;
    return node;
}

ASTNode *ast_create_for_in(const char *var, ASTNode *collection, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_FOR_IN;
    node->data.for_in.var = memory_strdup(var);
    node->data.for_in.collection = collection;
    node->data.for_in.body = body;
    return node;
}

void ast_free(ASTNode *node)
{
    if (!node)
        return;

    switch (node->type)
    {
    case AST_STRING:
        memory_free(node->data.string.value);
        break;
    case AST_IDENTIFIER:
        memory_free(node->data.identifier.name);
        break;
    case AST_BINARY_OP:
        ast_free(node->data.binary_op.left);
        ast_free(node->data.binary_op.right);
        break;
    case AST_UNARY_OP:
        ast_free(node->data.unary_op.operand);
        break;
    case AST_ASSIGN:
        memory_free(node->data.assign.name);
        ast_free(node->data.assign.value);
        break;
    case AST_IF:
        ast_free(node->data.if_stmt.condition);
        ast_free(node->data.if_stmt.then_block);
        ast_free(node->data.if_stmt.else_block);
        break;
    case AST_WHILE:
        ast_free(node->data.while_stmt.condition);
        ast_free(node->data.while_stmt.body);
        break;
    case AST_FOR:
        ast_free(node->data.for_stmt.init);
        ast_free(node->data.for_stmt.condition);
        ast_free(node->data.for_stmt.increment);
        ast_free(node->data.for_stmt.body);
        break;
    case AST_FUNCTION:
        memory_free(node->data.function.name);
        for (int i = 0; i < node->data.function.param_count; i++)
        {
            memory_free(node->data.function.params[i]);
        }
        memory_free(node->data.function.params);
        ast_free(node->data.function.body);
        break;
    case AST_CALL:
        memory_free(node->data.call.name);
        for (int i = 0; i < node->data.call.arg_count; i++)
        {
            ast_free(node->data.call.args[i]);
        }
        memory_free(node->data.call.args);
        break;
    case AST_RETURN:
        ast_free(node->data.return_stmt.value);
        break;
    case AST_BLOCK:
        for (int i = 0; i < node->data.block.count; i++)
        {
            ast_free(node->data.block.statements[i]);
        }
        memory_free(node->data.block.statements);
        break;
    case AST_ARRAY:
        for (int i = 0; i < node->data.array.count; i++)
        {
            ast_free(node->data.array.elements[i]);
        }
        memory_free(node->data.array.elements);
        break;
    case AST_INDEX:
        ast_free(node->data.index_expr.object);
        ast_free(node->data.index_expr.index);
        break;
    case AST_NAMESPACE:
        memory_free(node->data.namespace_decl.name);
        ast_free(node->data.namespace_decl.body);
        break;
    case AST_ENUM:
        memory_free(node->data.enum_decl.name);
        for (int i = 0; i < node->data.enum_decl.count; i++)
        {
            memory_free(node->data.enum_decl.members[i]);
        }
        memory_free(node->data.enum_decl.members);
        memory_free(node->data.enum_decl.values);
        break;
    case AST_CLASS:
        memory_free(node->data.class_decl.name);
        if (node->data.class_decl.base)
            memory_free(node->data.class_decl.base);
        ast_free(node->data.class_decl.body);
        break;
    case AST_MAP:
        for (int i = 0; i < node->data.map_expr.count; i++) {
            ast_free(node->data.map_expr.keys[i]);
            ast_free(node->data.map_expr.values[i]);
        }
        memory_free(node->data.map_expr.keys);
        memory_free(node->data.map_expr.values);
        break;
    case AST_LAMBDA:
        for (int i = 0; i < node->data.lambda.param_count; i++) {
            memory_free(node->data.lambda.params[i]);
        }
        memory_free(node->data.lambda.params);
        ast_free(node->data.lambda.body);
        break;
    case AST_MATCH:
        ast_free(node->data.match_stmt.expr);
        for (int i = 0; i < node->data.match_stmt.case_count; i++) {
            ast_free(node->data.match_stmt.cases[i]);
            ast_free(node->data.match_stmt.bodies[i]);
        }
        memory_free(node->data.match_stmt.cases);
        memory_free(node->data.match_stmt.bodies);
        if (node->data.match_stmt.default_case)
            ast_free(node->data.match_stmt.default_case);
        break;
    case AST_TRY_CATCH:
        ast_free(node->data.try_stmt.try_block);
        if (node->data.try_stmt.error_var) memory_free(node->data.try_stmt.error_var);
        if (node->data.try_stmt.catch_block) ast_free(node->data.try_stmt.catch_block);
        if (node->data.try_stmt.finally_block) ast_free(node->data.try_stmt.finally_block);
        break;
    case AST_FOR_IN:
        memory_free(node->data.for_in.var);
        ast_free(node->data.for_in.collection);
        ast_free(node->data.for_in.body);
        break;
    default:
        break;
    }

    memory_free(node);
}
