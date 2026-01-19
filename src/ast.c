/*
    Copyright (c) 2024-2026 SharpScript Programming Language
    
    Licensed under the MIT License
*/

// START OF ast.c

#include "include/ast.h"
#include "include/memory.h"

/*
 * ast_create_number: Create an AST node for a numeric literal
 * 
 * Allocates memory for a new AST node and initializes it with the given numeric value.
 * The node type is set to AST_NUMBER and the value is stored directly.
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_number(double value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NUMBER;
    node->data.number.value = value;
    return node;
}

/*
 * ast_create_string: Create an AST node for a string literal
 * 
 * Allocates memory for a new AST node and initializes it with a copy of the given string value.
 * The node type is set to AST_STRING and the string is duplicated using memory_strdup.
 * 
 * Parameters:
 *   value: The string value to store (will be copied)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_string(const char *value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_STRING;
    node->data.string.value = memory_strdup(value);
    return node;
}

/*
 * ast_create_boolean: Create an AST node for a boolean literal
 * 
 * Allocates memory for a new AST node and initializes it with the given boolean value.
 * The node type is set to AST_BOOLEAN and the value is stored directly.
 * 
 * Parameters:
 *   value: The boolean value (0 for false, non-zero for true)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_boolean(int value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BOOLEAN;
    node->data.boolean.value = value;
    return node;
}

/*
 * ast_create_null: Create an AST node for a null literal
 * 
 * Allocates memory for a new AST node and initializes it as a null value.
 * The node type is set to AST_NULL with no additional data.
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_null(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NULL;
    return node;
}

/*
 * ast_create_identifier: Create an AST node for an identifier
 * 
 * Allocates memory for a new AST node and initializes it with a copy of the given identifier name.
 * The node type is set to AST_IDENTIFIER and the name is duplicated using memory_strdup.
 * 
 * Parameters:
 *   name: The identifier name to store (will be copied)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_identifier(const char *name)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_IDENTIFIER;
    node->data.identifier.name = memory_strdup(name);
    return node;
}

/*
 * ast_create_binary_op: Create an AST node for a binary operation
 * 
 * Allocates memory for a new AST node and initializes it with the given operator and operands.
 * The node type is set to AST_BINARY_OP with left and right child nodes.
 * 
 * Parameters:
 *   op: The binary operator token type (e.g., TOKEN_PLUS, TOKEN_MINUS, etc.)
 *   left: The left operand AST node
 *   right: The right operand AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_binary_op(TokenType op, ASTNode *left, ASTNode *right)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BINARY_OP;
    node->data.binary_op.op = op;
    node->data.binary_op.left = left;
    node->data.binary_op.right = right;
    return node;
}

/*
 * ast_create_unary_op: Create an AST node for a unary operation
 * 
 * Allocates memory for a new AST node and initializes it with the given operator and operand.
 * The node type is set to AST_UNARY_OP with a single child node.
 * 
 * Parameters:
 *   op: The unary operator token type (e.g., TOKEN_NOT, TOKEN_SUB for negation)
 *   operand: The operand AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_unary_op(TokenType op, ASTNode *operand)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_UNARY_OP;
    node->data.unary_op.op = op;
    node->data.unary_op.operand = operand;
    return node;
}

/*
 * ast_create_assign: Create an AST node for variable assignment or declaration
 * 
 * Allocates memory for a new AST node and initializes it with assignment information.
 * The node type is set to AST_ASSIGN with the variable name, value expression, and operator.
 * Type annotations are supported via the type_name field (initialized to NULL).
 * 
 * Parameters:
 *   name: The variable name to assign to (will be copied)
 *   value: The value expression AST node
 *   op: The assignment operator token type (e.g., TOKEN_ASSIGN, TOKEN_PLUS_ASSIGN, etc.)
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_if: Create an AST node for an if-else statement
 * 
 * Allocates memory for a new AST node and initializes it with if-else statement components.
 * The node type is set to AST_IF with condition, then block, and optional else block.
 * 
 * Parameters:
 *   condition: The condition expression AST node
 *   then_block: The AST node for the then block
 *   else_block: The AST node for the else block (can be NULL for simple if statements)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_IF;
    node->data.if_stmt.condition = condition;
    node->data.if_stmt.then_block = then_block;
    node->data.if_stmt.else_block = else_block;
    return node;
}

/*
 * ast_create_while: Create an AST node for a while loop
 * 
 * Allocates memory for a new AST node and initializes it with while loop components.
 * The node type is set to AST_WHILE with condition and body expressions.
 * 
 * Parameters:
 *   condition: The loop condition expression AST node
 *   body: The loop body AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_while(ASTNode *condition, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_WHILE;
    node->data.while_stmt.condition = condition;
    node->data.while_stmt.body = body;
    return node;
}

/*
 * ast_create_for: Create an AST node for a for loop
 * 
 * Allocates memory for a new AST node and initializes it with for loop components.
 * The node type is set to AST_FOR with initialization, condition, increment, and body expressions.
 * Any of the components can be NULL for optional parts of the for loop.
 * 
 * Parameters:
 *   init: The initialization expression AST node (can be NULL)
 *   condition: The loop condition expression AST node (can be NULL for infinite loops)
 *   increment: The increment expression AST node (can be NULL)
 *   body: The loop body AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_function: Create an AST node for a function declaration
 * 
 * Allocates memory for a new AST node and initializes it with function declaration information.
 * The node type is set to AST_FUNCTION with name, parameters, and body.
 * Default parameter values are initially set to NULL and can be added later.
 * 
 * Parameters:
 *   name: The function name (will be copied)
 *   params: Array of parameter names
 *   param_count: Number of parameters
 *   body: The function body AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_call: Create an AST node for a function call
 * 
 * Allocates memory for a new AST node and initializes it with function call information.
 * The node type is set to AST_CALL with function name and argument expressions.
 * 
 * Parameters:
 *   name: The function name to call (will be copied)
 *   args: Array of argument expression AST nodes
 *   arg_count: Number of arguments
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_call(const char *name, ASTNode **args, int arg_count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CALL;
    node->data.call.name = memory_strdup(name);
    node->data.call.args = args;
    node->data.call.arg_count = arg_count;
    return node;
}

/*
 * ast_create_return: Create an AST node for a return statement
 * 
 * Allocates memory for a new AST node and initializes it with return statement information.
 * The node type is set to AST_RETURN with the value to return.
 * 
 * Parameters:
 *   value: The value expression AST node to return (can be NULL for empty returns)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_return(ASTNode *value)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_RETURN;
    node->data.return_stmt.value = value;
    return node;
}

/*
 * ast_create_break: Create an AST node for a break statement
 * 
 * Allocates memory for a new AST node and initializes it as a break statement.
 * The node type is set to AST_BREAK with no additional data.
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_break(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BREAK;
    return node;
}

/*
 * ast_create_continue: Create an AST node for a continue statement
 * 
 * Allocates memory for a new AST node and initializes it as a continue statement.
 * The node type is set to AST_CONTINUE with no additional data.
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_continue(void)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CONTINUE;
    return node;
}

/*
 * ast_create_block: Create an AST node for a block of statements
 * 
 * Allocates memory for a new AST node and initializes it with a sequence of statements.
 * The node type is set to AST_BLOCK with an array of statement nodes.
 * 
 * Parameters:
 *   statements: Array of statement AST nodes
 *   count: Number of statements in the block
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_block(ASTNode **statements, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_BLOCK;
    node->data.block.statements = statements;
    node->data.block.count = count;
    return node;
}

/*
 * ast_create_array: Create an AST node for an array literal
 * 
 * Allocates memory for a new AST node and initializes it with array elements.
 * The node type is set to AST_ARRAY with an array of element expressions.
 * 
 * Parameters:
 *   elements: Array of element expression AST nodes
 *   count: Number of elements in the array
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_array(ASTNode **elements, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_ARRAY;
    node->data.array.elements = elements;
    node->data.array.count = count;
    return node;
}

/*
 * ast_create_index: Create an AST node for array/map indexing
 * 
 * Allocates memory for a new AST node and initializes it with index expression information.
 * The node type is set to AST_INDEX with object and index expressions.
 * 
 * Parameters:
 *   object: The object expression to index (array or map)
 *   index: The index expression (number for arrays, string for maps)
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_index(ASTNode *object, ASTNode *index)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_INDEX;
    node->data.index_expr.object = object;
    node->data.index_expr.index = index;
    return node;
}

/*
 * ast_create_namespace: Create an AST node for a namespace declaration
 * 
 * Allocates memory for a new AST node and initializes it with namespace information.
 * The node type is set to AST_NAMESPACE with name and body expressions.
 * 
 * Parameters:
 *   name: The namespace name (will be copied)
 *   body: The namespace body AST node containing declarations
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_namespace(const char *name, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_NAMESPACE;
    node->data.namespace_decl.name = memory_strdup(name);
    node->data.namespace_decl.body = body;
    return node;
}

/*
 * ast_create_enum: Create an AST node for an enum declaration
 * 
 * Allocates memory for a new AST node and initializes it with enum information.
 * The node type is set to AST_ENUM with name, member names, and associated values.
 * 
 * Parameters:
 *   name: The enum name (will be copied)
 *   members: Array of member names
 *   values: Array of numeric values for each member
 *   count: Number of enum members
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_class: Create an AST node for a class declaration
 * 
 * Allocates memory for a new AST node and initializes it with class information.
 * The node type is set to AST_CLASS with name, optional base class, and body.
 * 
 * Parameters:
 *   name: The class name (will be copied)
 *   base: The base class name (can be NULL for no inheritance)
 *   body: The class body AST node containing methods and properties
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_class(const char *name, const char *base, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_CLASS;
    node->data.class_decl.name = memory_strdup(name);
    node->data.class_decl.base = base ? memory_strdup(base) : NULL;
    node->data.class_decl.body = body;
    return node;
}

/*
 * ast_create_map: Create an AST node for a map literal
 * 
 * Allocates memory for a new AST node and initializes it with map key-value pairs.
 * The node type is set to AST_MAP with arrays of keys and values.
 * 
 * Parameters:
 *   keys: Array of key expression AST nodes
 *   values: Array of value expression AST nodes
 *   count: Number of key-value pairs
 * 
 * Returns: Pointer to the newly created AST node
 */
/*
 * ast_create_map: Create an AST node for a map (dictionary) literal
 * 
 * Allocates memory for a new AST node and initializes it with map expression data.
 * The node type is set to AST_MAP with parallel arrays of keys and values.
 * 
 * Parameters:
 *   keys: Array of key AST nodes
 *   values: Array of value AST nodes (parallel to keys)
 *   count: Number of key-value pairs
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_map(ASTNode **keys, ASTNode **values, int count)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_MAP;
    node->data.map_expr.keys = keys;
    node->data.map_expr.values = values;
    node->data.map_expr.count = count;
    return node;
}

/*
 * ast_create_lambda: Create an AST node for a lambda (anonymous function)
 * 
 * Allocates memory for a new AST node and initializes it with lambda expression information.
 * The node type is set to AST_LAMBDA with parameters and body.
 * 
 * Parameters:
 *   params: Array of parameter names
 *   param_count: Number of parameters
 *   body: The lambda body AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_lambda(char **params, int param_count, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_LAMBDA;
    node->data.lambda.params = params;
    node->data.lambda.param_count = param_count;
    node->data.lambda.body = body;
    return node;
}

/*
 * ast_create_match: Create an AST node for a match expression
 * 
 * Allocates memory for a new AST node and initializes it with match expression information.
 * The node type is set to AST_MATCH with expression, case patterns, and corresponding bodies.
 * 
 * Parameters:
 *   expr: The expression to match against
 *   cases: Array of case pattern AST nodes
 *   bodies: Array of body AST nodes for each case
 *   case_count: Number of cases
 *   default_case: The default case body (can be NULL)
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_try_catch: Create an AST node for a try-catch-finally statement
 * 
 * Allocates memory for a new AST node and initializes it with try-catch-finally information.
 * The node type is set to AST_TRY_CATCH with try, catch, and finally blocks.
 * 
 * Parameters:
 *   try_block: The try block AST node
 *   error_var: The error variable name for catch block (can be NULL)
 *   catch_block: The catch block AST node (can be NULL)
 *   finally_block: The finally block AST node (can be NULL)
 * 
 * Returns: Pointer to the newly created AST node
 */
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

/*
 * ast_create_for_in: Create an AST node for a for-in loop
 * 
 * Allocates memory for a new AST node and initializes it with for-in loop information.
 * The node type is set to AST_FOR_IN with iteration variable, collection, and body.
 * 
 * Parameters:
 *   var: The iteration variable name (will be copied)
 *   collection: The collection expression to iterate over
 *   body: The loop body AST node
 * 
 * Returns: Pointer to the newly created AST node
 */
ASTNode *ast_create_for_in(const char *var, ASTNode *collection, ASTNode *body)
{
    ASTNode *node = memory_allocate(sizeof(ASTNode));
    node->type = AST_FOR_IN;
    node->data.for_in.var = memory_strdup(var);
    node->data.for_in.collection = collection;
    node->data.for_in.body = body;
    return node;
}

/*
 * ast_free: Free an AST node and all its children
 * 
 * Recursively frees all memory associated with an AST node and its children.
 * Handles different node types by freeing their specific data structures.
 * Strings are freed, arrays/structures are recursively processed, and child nodes are freed.
 * 
 * Parameters:
 *   node: The AST node to free (can be NULL)
 */
/*
 * ast_free: Free an AST node and all its children
 * 
 * Recursively frees all memory associated with an AST node and its children.
 * Handles different node types by freeing their specific data structures.
 * Strings are freed, arrays/structures are recursively processed, and child nodes are freed.
 * 
 * Parameters:
 *   node: The AST node to free (can be NULL)
 */
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

// END OF ast.c
