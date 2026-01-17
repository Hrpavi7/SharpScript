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
    default:
        break;
    }

    memory_free(node);
}
