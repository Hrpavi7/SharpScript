// ast.h

#ifndef AST_H
#define AST_H

#include "lexer.h" // "compiler of it"

typedef enum
{
    AST_NUMBER,
    AST_STRING,
    AST_BOOLEAN,
    AST_NULL,
    AST_IDENTIFIER,
    AST_BINARY_OP, // binary operators
    AST_UNARY_OP,
    AST_ASSIGN,
    AST_IF, // god i love if statements
    AST_WHILE,
    AST_FOR,
    AST_FUNCTION,
    AST_CALL,
    AST_RETURN,
    AST_BREAK,
    AST_CONTINUE,
    AST_BLOCK,
    AST_PROGRAM,
    AST_ARRAY,
    AST_INDEX,
    AST_NAMESPACE,
    AST_ENUM,
    AST_CLASS,
    AST_MAP,
    AST_LAMBDA,
    AST_MATCH,
    AST_TRY_CATCH,
    AST_FOR_IN
} ASTNodeType;

typedef struct ASTNode
{
    ASTNodeType type;
    union
    {
        struct
        {
            double value;
        } number;
        struct
        {
            char *value;
        } string;
        struct
        {
            int value;
        } boolean;
        struct
        {
            char *name;
        } identifier;
        struct
        {
            TokenType op;
            struct ASTNode *left;
            struct ASTNode *right;
        } binary_op;
        struct
        {
            TokenType op;
            struct ASTNode *operand;
        } unary_op;
        struct
        {
            char *name;
            struct ASTNode *value;
            TokenType op;
            char *type_name;
        } assign;
        struct
        {
            struct ASTNode *condition;
            struct ASTNode *then_block;
            struct ASTNode *else_block;
        } if_stmt;
        struct
        {
            struct ASTNode *condition;
            struct ASTNode *body;
        } while_stmt;
        struct
        {
            struct ASTNode *init;
            struct ASTNode *condition;
            struct ASTNode *increment;
            struct ASTNode *body;
        } for_stmt;
        struct
        {
            char *name;
            char **params;
            int param_count;
            struct ASTNode **defaults;
            struct ASTNode *body;
        } function;
        struct
        {
            char *name;
            struct ASTNode **args;
            int arg_count;
        } call;
        struct
        {
            struct ASTNode *value;
        } return_stmt;
        struct
        {
            struct ASTNode **statements;
            int count;
        } block;
        struct
        {
            struct ASTNode **elements;
            int count;
        } array;
        struct
        {
            struct ASTNode *object;
            struct ASTNode *index;
        } index_expr;
        struct
        {
            char *name;
            struct ASTNode *body;
        } namespace_decl;
        struct
        {
            char *name;
            char **members;
            double *values;
            int count;
        } enum_decl;
        struct
        {
            char *name;
            char *base;
            struct ASTNode *body;
        } class_decl;
        struct
        {
            struct ASTNode **keys;
            struct ASTNode **values;
            int count;
        } map_expr;
        struct
        {
            char **params;
            int param_count;
            struct ASTNode *body;
        } lambda;
        struct
        {
            struct ASTNode *expr;
            struct ASTNode **cases;  // Expressions to match
            struct ASTNode **bodies; // Blocks to execute
            int case_count;
            struct ASTNode *default_case;
        } match_stmt;
        struct
        {
            struct ASTNode *try_block;
            char *error_var;
            struct ASTNode *catch_block;
            struct ASTNode *finally_block;
        } try_stmt;
        struct
        {
            char *var;
            struct ASTNode *collection;
            struct ASTNode *body;
        } for_in;
    } data;
} ASTNode;

ASTNode *ast_create_number(double value);
ASTNode *ast_create_string(const char *value);
ASTNode *ast_create_boolean(int value);
ASTNode *ast_create_null(void);
ASTNode *ast_create_identifier(const char *name);
ASTNode *ast_create_binary_op(TokenType op, ASTNode *left, ASTNode *right);
ASTNode *ast_create_unary_op(TokenType op, ASTNode *operand);
ASTNode *ast_create_assign(const char *name, ASTNode *value, TokenType op);
ASTNode *ast_create_if(ASTNode *condition, ASTNode *then_block, ASTNode *else_block);
ASTNode *ast_create_while(ASTNode *condition, ASTNode *body);
ASTNode *ast_create_for(ASTNode *init, ASTNode *condition, ASTNode *increment, ASTNode *body);
ASTNode *ast_create_function(const char *name, char **params, int param_count, ASTNode *body);
ASTNode *ast_create_call(const char *name, ASTNode **args, int arg_count);
ASTNode *ast_create_return(ASTNode *value);
ASTNode *ast_create_break(void);
ASTNode *ast_create_continue(void);
ASTNode *ast_create_block(ASTNode **statements, int count);
ASTNode *ast_create_array(ASTNode **elements, int count);
ASTNode *ast_create_index(ASTNode *object, ASTNode *index);
ASTNode *ast_create_namespace(const char *name, ASTNode *body);
ASTNode *ast_create_enum(const char *name, char **members, double *values, int count);
ASTNode *ast_create_class(const char *name, const char *base, ASTNode *body);
ASTNode *ast_create_map(ASTNode **keys, ASTNode **values, int count);
ASTNode *ast_create_lambda(char **params, int param_count, ASTNode *body);
ASTNode *ast_create_match(ASTNode *expr, ASTNode **cases, ASTNode **bodies, int case_count, ASTNode *default_case);
ASTNode *ast_create_try_catch(ASTNode *try_block, const char *error_var, ASTNode *catch_block, ASTNode *finally_block);
ASTNode *ast_create_for_in(const char *var, ASTNode *collection, ASTNode *body);

void ast_free(ASTNode *node);

#endif // AST_H
