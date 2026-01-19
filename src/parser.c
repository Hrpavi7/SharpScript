/*
    Copyright (c) 2024-2026 SharpScript Programming Language
    
    Licensed under the MIT License
*/

// START OF parser.c

#include "include/parser.h"
#include "include/memory.h"
#include <stdio.h>
#include <stdlib.h> // For atof

/*
 * parser_create: Create a new parser instance
 * 
 * Allocates memory for a new parser and initializes it with the given lexer.
 * Sets up include path tracking to prevent circular includes.
 * 
 * Parameters:
 *   lexer: The lexer to use for tokenization
 * 
 * Returns: Pointer to the newly created parser
 */
Parser *parser_create(Lexer *lexer)
{
    Parser *parser = memory_allocate(sizeof(Parser));
    parser->lexer = lexer;
    parser->current_token = lexer_next_token(lexer);
    parser->include_paths = memory_allocate(sizeof(char *) * 8);
    parser->include_count = 0;
    parser->include_capacity = 8;
    return parser;
}

/*
 * parser_free: Free a parser and all its resources
 * 
 * Frees the current token, all include paths, and the parser structure itself.
 * 
 * Parameters:
 *   parser: The parser to free
 */
void parser_free(Parser *parser)
{
    token_free(parser->current_token);
    for (int i = 0; i < parser->include_count; i++)
    {
        memory_free(parser->include_paths[i]);
    }
    memory_free(parser->include_paths);
    memory_free(parser);
}

/*
 * parser_advance: Advance to the next token
 * 
 * Frees the current token and gets the next token from the lexer.
 * This is the primary way to move through the token stream during parsing.
 * 
 * Parameters:
 *   parser: The parser instance
 */
static void parser_advance(Parser *parser)
{
    token_free(parser->current_token);
    parser->current_token = lexer_next_token(parser->lexer);
}

/*
 * parser_expect: Expect a specific token type and advance if matched
 * 
 * Checks if the current token matches the expected type. If it does, advances to the next token.
 * If not, prints an error message to stderr.
 * 
 * Parameters:
 *   parser: The parser instance
 *   type: The expected token type
 * 
 * Returns: 1 if token matched, 0 otherwise
 */
static int parser_expect(Parser *parser, TokenType type)
{
    if (parser->current_token->type != type)
    {
        fprintf(stderr, "Parse error at line %d: unexpected token '%s', expected type %d\n",
                parser->current_token->line, parser->current_token->value, type);
        return 0;
    }
    parser_advance(parser);
    return 1;
}

/*
 * parser_peek_token: Peek at the next token without consuming it
 * 
 * Gets the next token from the lexer while preserving the current lexer state.
 * This allows looking ahead in the token stream without advancing the current position.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The next token (caller must free it)
 */
static Token *parser_peek_token(Parser *parser)
{
    int pos = parser->lexer->position;
    int line = parser->lexer->line;
    int col = parser->lexer->column;
    
    Token *tok = lexer_next_token(parser->lexer);
    
    parser->lexer->position = pos;
    parser->lexer->line = line;
    parser->lexer->column = col;
    
    return tok;
}

/*
 * parser_has_included: Check if a file has already been included
 * 
 * Prevents circular includes by checking if a file path is already in the include history.
 * 
 * Parameters:
 *   parser: The parser instance
 *   path: The file path to check
 * 
 * Returns: 1 if file has been included, 0 otherwise
 */
static int parser_has_included(Parser *parser, const char *path)
{
    for (int i = 0; i < parser->include_count; i++)
    {
        if (strcmp(parser->include_paths[i], path) == 0)
            return 1;
    }
    return 0;
}

/*
 * parser_add_include: Add a file path to the include history
 * 
 * Records that a file has been included to prevent circular includes.
 * Dynamically resizes the include_paths array if needed.
 * 
 * Parameters:
 *   parser: The parser instance
 *   path: The file path to add (will be copied)
 */
static void parser_add_include(Parser *parser, const char *path)
{
    if (parser->include_count >= parser->include_capacity)
    {
        parser->include_capacity *= 2;
        parser->include_paths = memory_reallocate(parser->include_paths, sizeof(char *) * parser->include_capacity);
    }
    parser->include_paths[parser->include_count++] = memory_strdup(path);
}
static ASTNode *parse_expression(Parser *parser);
static ASTNode *parse_statement(Parser *parser);
static ASTNode *parse_block(Parser *parser);

/*
 * parse_primary: Parse primary expressions (literals, identifiers, function calls, etc.)
 * 
 * Handles the lowest level of the expression hierarchy including:
 * - Numeric literals
 * - String literals  
 * - Boolean literals (true/false)
 * - Null literal
 * - Array literals
 * - Map literals
 * - Function calls
 * - Identifiers
 * - Parenthesized expressions
 * - Include statements
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node, or NULL on error
 */
static ASTNode *parse_primary(Parser *parser)
{
    Token *token = parser->current_token;

    if (token->type == TOKEN_NUMBER)
    {
        double value = atof(token->value);
        parser_advance(parser);
        return ast_create_number(value);
    }

    if (token->type == TOKEN_STRING)
    {
        ASTNode *node = ast_create_string(token->value);
        parser_advance(parser);
        return node;
    }

    if (token->type == TOKEN_TRUE)
    {
        parser_advance(parser);
        return ast_create_boolean(1);
    }

    if (token->type == TOKEN_FALSE)
    {
        parser_advance(parser);
        return ast_create_boolean(0);
    }

    if (token->type == TOKEN_NULL)
    {
        parser_advance(parser);
        return ast_create_null();
    }

    // Parse array literals: [expr1, expr2, ...]
    if (token->type == TOKEN_LBRACKET)
    {
        parser_advance(parser);
        int capacity = 8;
        int count = 0;
        ASTNode **elements = memory_allocate(sizeof(ASTNode *) * capacity);

        while (parser->current_token->type != TOKEN_RBRACKET &&
               parser->current_token->type != TOKEN_EOF)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                elements = memory_reallocate(elements, sizeof(ASTNode *) * capacity);
            }
            elements[count++] = parse_expression(parser);

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
            }
        }

        parser_expect(parser, TOKEN_RBRACKET);
        return ast_create_array(elements, count);
    }

    // Parse map literals: {key1: value1, key2: value2, ...}
    if (token->type == TOKEN_LBRACE)
    {
        parser_advance(parser); // eat {
        
        int capacity = 8;
        int count = 0;
        ASTNode **keys = memory_allocate(sizeof(ASTNode *) * capacity);
        ASTNode **values = memory_allocate(sizeof(ASTNode *) * capacity);

        // Parse key-value pairs if map is not empty
        if (parser->current_token->type != TOKEN_RBRACE) {
            while (1) {
                // Resize arrays if needed
                if (count >= capacity) {
                    capacity *= 2;
                    keys = memory_reallocate(keys, sizeof(ASTNode *) * capacity);
                    values = memory_reallocate(values, sizeof(ASTNode *) * capacity);
                }

                // Parse key expression
                keys[count] = parse_expression(parser);
                
                // Expect colon separator
                parser_expect(parser, TOKEN_COLON);

                // Parse value expression
                values[count] = parse_expression(parser);
                count++;

                // Handle comma separator or end of map
                if (parser->current_token->type == TOKEN_COMMA) {
                    parser_advance(parser);
                } else {
                    break;
                }
            }
        }
        parser_expect(parser, TOKEN_RBRACE);
        return ast_create_map(keys, values, count);
    }

    // Parse identifiers and function calls
    if (token->type == TOKEN_IDENTIFIER)
    {
        char *name = memory_strdup(token->value);
        parser_advance(parser);

        // Function call: identifier(...)
        if (parser->current_token->type == TOKEN_LPAREN)
        {
            parser_advance(parser); // eat (
            int capacity = 8;
            int count = 0;
            ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

            // Parse function arguments
            while (parser->current_token->type != TOKEN_RPAREN &&
                   parser->current_token->type != TOKEN_EOF)
            {
                // Resize array if needed
                if (count >= capacity)
                {
                    capacity *= 2;
                    args = memory_reallocate(args, sizeof(ASTNode *) * capacity);
                }
                // Parse argument expression
                args[count++] = parse_expression(parser);

                // Handle comma separator
                if (parser->current_token->type == TOKEN_COMMA)
                {
                    parser_advance(parser);
                }
            }

            parser_expect(parser, TOKEN_RPAREN);
            ASTNode *call = ast_create_call(name, args, count);
            memory_free(name);
            return call;
        }

        ASTNode *ident = ast_create_identifier(name);
        memory_free(name);
        return ident;
    }

    // Parse parenthesized expressions or lambda expressions
    if (token->type == TOKEN_LPAREN)
    {
        parser_advance(parser); // eat (
        
        int capacity = 4;
        int count = 0;
        ASTNode **exprs = memory_allocate(sizeof(ASTNode*) * capacity);
        
        // Parse expressions inside parentheses
        if (parser->current_token->type != TOKEN_RPAREN) {
            while (1) {
                // Resize array if needed
                if (count >= capacity) {
                    capacity *= 2;
                    exprs = memory_reallocate(exprs, sizeof(ASTNode*) * capacity);
                }
                // Parse expression
                exprs[count++] = parse_expression(parser);
                
                // Handle comma separator
                if (parser->current_token->type == TOKEN_COMMA) {
                    parser_advance(parser);
                } else {
                    break;
                }
            }
        }
        
        parser_expect(parser, TOKEN_RPAREN);
        
        // Check if this is a lambda expression: (params) => body
        if (parser->current_token->type == TOKEN_ARROW) {
            parser_advance(parser); // eat =>
            
            // Convert exprs to parameter names
            char **params = memory_allocate(sizeof(char*) * count);
            for (int i=0; i<count; i++) {
                // Validate that all parameters are identifiers
                if (exprs[i]->type != AST_IDENTIFIER) {
                    fprintf(stderr, "Parse error: lambda parameters must be identifiers\n");
                    return ast_create_null();
                }
                params[i] = memory_strdup(exprs[i]->data.identifier.name);
                ast_free(exprs[i]);
            }
            memory_free(exprs);
            
            // Parse lambda body
            ASTNode *body;
            if (parser->current_token->type == TOKEN_LBRACE)
                body = parse_block(parser);  // Block body
            else
                body = parse_expression(parser);  // Expression body
                
            return ast_create_lambda(params, count, body);
        } else {
            // Regular grouped expression
            if (count == 1) {
                ASTNode *expr = exprs[0];
                memory_free(exprs); // just the array
                return expr;
            } else {
                fprintf(stderr, "Parse error: unexpected tuple or empty group\n");
                for(int i=0; i<count; i++) ast_free(exprs[i]);
                memory_free(exprs);
                return ast_create_null();
            }
        }
    }

    // Parse built-in function calls (print, input, len, type, etc.)
    if (token->type == TOKEN_PRINT || token->type == TOKEN_INPUT ||
        token->type == TOKEN_LEN || token->type == TOKEN_TYPE ||
        token->type == TOKEN_OUTPUT || token->type == TOKEN_ERRORFN ||
        token->type == TOKEN_WARNING || token->type == TOKEN_HELP)
    {
        TokenType func_type = token->type;
        parser_advance(parser); // eat function name
        parser_expect(parser, TOKEN_LPAREN);

        int capacity = 8;
        int count = 0;
        ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

        // Parse function arguments
        while (parser->current_token->type != TOKEN_RPAREN &&
               parser->current_token->type != TOKEN_EOF)
        {
            // Resize array if needed
            if (count >= capacity)
            {
                capacity *= 2;
                args = memory_reallocate(args, sizeof(ASTNode *) * capacity);
            }
            args[count++] = parse_expression(parser);

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
            }
        }

        parser_expect(parser, TOKEN_RPAREN);

        // Map token types to system function names
        char *func_name;
        if (func_type == TOKEN_PRINT)
            func_name = "system.print";
        else if (func_type == TOKEN_INPUT)
            func_name = "system.input";
        else if (func_type == TOKEN_LEN)
            func_name = "system.len";
        else if (func_type == TOKEN_TYPE)
            func_name = "system.type";
        else if (func_type == TOKEN_OUTPUT)
            func_name = "system.output";
        else if (func_type == TOKEN_ERRORFN)
            func_name = "system.error";
        else if (func_type == TOKEN_HELP)
            func_name = "system.help";
        else
            func_name = "system.warning";

        return ast_create_call(func_name, args, count);
    }

    fprintf(stderr, "Parse error: unexpected token at line %d: %s\n",
            token->line, token->value ? token->value : "<unknown>");
    parser_advance(parser);
    return ast_create_null();
}

/*
 * parse_postfix: Parse postfix expressions (array/map indexing)
 * 
 * Handles array and map indexing operations like arr[index] or map[key].
 * Left-associative: a[b][c] is parsed as ((a[b])[c]).
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the indexed expression
 */
/*
 * parse_postfix: Parse postfix expressions (array/map indexing)
 * 
 * Handles array and map indexing operations with left-to-right associativity.
 * Syntax: array[index] or map[key]
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the postfix expression
 */
static ASTNode *parse_postfix(Parser *parser)
{
    // Start with primary expression
    ASTNode *left = parse_primary(parser);

    // Handle array/map indexing: left[expression]
    while (parser->current_token->type == TOKEN_LBRACKET)
    {
        parser_advance(parser); // eat [
        ASTNode *index = parse_expression(parser);
        parser_expect(parser, TOKEN_RBRACKET); // expect ]
        left = ast_create_index(left, index); // create index node
    }

    return left;
}

/*
 * parse_unary: Parse unary expressions (NOT, negation)
 * 
 * Handles unary operators with right-to-left associativity:
 * - Logical NOT: !expression
 * - Arithmetic negation: -expression
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the unary expression
 */
static ASTNode *parse_unary(Parser *parser)
{
    // Handle unary operators: ! or -
    if (parser->current_token->type == TOKEN_NOT ||
        parser->current_token->type == TOKEN_SUB)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        ASTNode *right = parse_unary(parser); // recursive for right associativity
        return ast_create_unary_op(op, right);
    }

    return parse_postfix(parser);
}

/*
 * parse_multiplicative: Parse multiplicative expressions (*, /, %)
 * 
 * Handles multiplication, division, and modulo operations with left associativity.
 * Higher precedence than additive operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the multiplicative expression
 */
static ASTNode *parse_multiplicative(Parser *parser)
{
    ASTNode *left = parse_unary(parser);

    // Handle left-associative multiplicative operators
    while (parser->current_token->type == TOKEN_MUL ||
           parser->current_token->type == TOKEN_DIV ||
           parser->current_token->type == TOKEN_MOD)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_unary(parser));
    }

    return left;
}

/*
 * parse_additive: Parse additive expressions (+, -)
 * 
 * Handles addition and subtraction operations with left associativity.
 * Lower precedence than multiplicative operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the additive expression
 */
static ASTNode *parse_additive(Parser *parser)
{
    ASTNode *left = parse_multiplicative(parser);

    // Handle left-associative additive operators
    while (parser->current_token->type == TOKEN_ADD ||
           parser->current_token->type == TOKEN_SUB)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_multiplicative(parser));
    }

    return left;
}

/*
 * parse_comparison: Parse comparison expressions (<, >, <=, >=)
 * 
 * Handles relational comparison operations with left associativity.
 * Lower precedence than additive operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the comparison expression
 */
static ASTNode *parse_comparison(Parser *parser)
{
    ASTNode *left = parse_additive(parser);

    // Handle left-associative comparison operators
    while (parser->current_token->type == TOKEN_LT ||
           parser->current_token->type == TOKEN_GT ||
           parser->current_token->type == TOKEN_LTE ||
           parser->current_token->type == TOKEN_GTE)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_additive(parser));
    }

    return left;
}

/*
 * parse_equality: Parse equality expressions (==, !=)
 * 
 * Handles equality and inequality operations with left associativity.
 * Lower precedence than comparison operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the equality expression
 */
static ASTNode *parse_equality(Parser *parser)
{
    ASTNode *left = parse_comparison(parser);

    // Handle left-associative equality operators
    while (parser->current_token->type == TOKEN_EQ ||
           parser->current_token->type == TOKEN_NEQ)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_comparison(parser));
    }

    return left;
}

/*
 * parse_logical_and: Parse logical AND expressions (&&)
 * 
 * Handles logical AND operations with left associativity.
 * Lower precedence than equality operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the logical AND expression
 */
static ASTNode *parse_logical_and(Parser *parser)
{
    ASTNode *left = parse_equality(parser);

    // Handle left-associative logical AND
    while (parser->current_token->type == TOKEN_AND)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_equality(parser));
    }

    return left;
}

/*
 * parse_logical_or: Parse logical OR expressions (||)
 * 
 * Handles logical OR operations with left associativity.
 * Lower precedence than logical AND operations.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the logical OR expression
 */
static ASTNode *parse_logical_or(Parser *parser)
{
    ASTNode *left = parse_logical_and(parser);

    // Handle left-associative logical OR
    while (parser->current_token->type == TOKEN_OR)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser); // eat operator
        left = ast_create_binary_op(op, left, parse_logical_and(parser));
    }

    return left;
}

/*
 * parse_expression: Parse any expression
 * 
 * Entry point for expression parsing. Delegates to parse_logical_or
 * which handles the full precedence hierarchy.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the expression
 */
static ASTNode *parse_expression(Parser *parser)
{
    return parse_logical_or(parser);
}

/*
 * parse_block: Parse a block of statements
 * 
 * Parses a sequence of statements enclosed in braces: { stmt1; stmt2; ... }
 * Used for function bodies, if/else blocks, loops, etc.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the block
 */
static ASTNode *parse_block(Parser *parser)
{
    parser_expect(parser, TOKEN_LBRACE);

    int capacity = 16;
    int count = 0;
    ASTNode **statements = memory_allocate(sizeof(ASTNode *) * capacity);

    // Parse statements until closing brace or EOF
    while (parser->current_token->type != TOKEN_RBRACE &&
           parser->current_token->type != TOKEN_EOF)
    {
        // Resize array if needed
        if (count >= capacity)
        {
            capacity *= 2;
            statements = memory_reallocate(statements, sizeof(ASTNode *) * capacity);
        }
        statements[count++] = parse_statement(parser);
    }

    parser_expect(parser, TOKEN_RBRACE);
    return ast_create_block(statements, count);
}

/*
 * parse_namespace: Parse namespace declarations
 * 
 * Handles namespace declarations in the form: namespace name { ... }
 * Used for organizing code into logical modules.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the namespace declaration
 */
/*
 * parse_namespace: Parse namespace declarations
 * 
 * Handles namespace declarations in the form: namespace name { ... }
 * Creates a namespace scope containing the block body.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the namespace declaration
 */
static ASTNode *parse_namespace(Parser *parser)
{
    parser_advance(parser); // eat 'namespace'
    
    // Validate and extract namespace name
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected namespace name\n");
        return ast_create_null();
    }
    
    char *name = memory_strdup(parser->current_token->value);
    parser_advance(parser); // eat namespace name
    
    // Parse the namespace body (block of statements)
    ASTNode *body = parse_block(parser);
    
    // Create namespace AST node
    ASTNode *ns = ast_create_namespace(name, body);
    memory_free(name);
    return ns;
}

/*
 * parse_enum: Parse enum declarations
 * 
 * Handles enum declarations in the form: enum name { member1, member2, ... }
 * Members can have explicit values or auto-increment from 0.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the enum declaration
 */
static ASTNode *parse_enum(Parser *parser)
{
    parser_advance(parser); // eat 'enum'
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected enum name\n");
        return ast_create_null();
    }
    char *ename = memory_strdup(parser->current_token->value);
    parser_advance(parser);
    parser_expect(parser, TOKEN_LBRACE);
    
    // Parse enum members
    int capacity = 8;
    int count = 0;
    char **members = memory_allocate(sizeof(char *) * capacity);
    double *values = memory_allocate(sizeof(double) * capacity);
    double next_val = 0;
    
    while (parser->current_token->type != TOKEN_RBRACE &&
           parser->current_token->type != TOKEN_EOF)
    {
        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected enum member\n");
            break;
        }
        
        // Resize arrays if needed
        if (count >= capacity)
        {
            capacity *= 2;
            members = memory_reallocate(members, sizeof(char *) * capacity);
            values = memory_reallocate(values, sizeof(double) * capacity);
        }
        
        // Parse member name
        members[count] = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        // Handle optional explicit value assignment
        double val = next_val;
        if (parser->current_token->type == TOKEN_ASSIGN)
        {
            parser_advance(parser); // eat =
            ASTNode *expr = parse_expression(parser);
            
            // Extract numeric value from expression
            if (expr->type == AST_NUMBER)
                val = expr->data.number.value;
            ast_free(expr);
            
            // Next auto-increment value starts after this explicit value
            next_val = val + 1;
        }
        else
        {
            // Use auto-increment value
            val = next_val;
            next_val += 1;
        }
        values[count] = val;
        count++;
        
        // Handle comma separator between members
        if (parser->current_token->type == TOKEN_COMMA)
            parser_advance(parser);
    }
    parser_expect(parser, TOKEN_RBRACE);
    ASTNode *en = ast_create_enum(ename, members, values, count);
    memory_free(ename);
    return en;
}

/*
 * parse_class: Parse class or struct declarations
 * 
 * Handles class/struct declarations with optional inheritance.
 * Classes support inheritance while structs are typically value types.
 * 
 * Parameters:
 *   parser: The parser instance
 *   is_struct: 1 for struct, 0 for class
 * 
 * Returns: The parsed AST node for the class/struct declaration
 */
static ASTNode *parse_class(Parser *parser, int is_struct)
{
    parser_advance(parser); // eat 'class' or 'struct'
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected class name\n");
        return ast_create_null();
    }
    char *name = memory_strdup(parser->current_token->value);
    parser_advance(parser);
    
    // Handle optional base class inheritance
    char *base = NULL;
    if (parser->current_token->type == TOKEN_COLON)
    {
        parser_advance(parser); // eat :
        if (parser->current_token->type == TOKEN_IDENTIFIER)
        {
            base = memory_strdup(parser->current_token->value);
            parser_advance(parser);
        }
    }
    ASTNode *body = parse_block(parser);
    ASTNode *cls = ast_create_class(name, base, body);
    memory_free(name);
    if (base) memory_free(base);
    return cls;
}
/*
 * parse_if_statement: Parse if statements
 * 
 * Handles if statements with optional else blocks.
 * Supports arrow syntax: if (condition) => { ... }
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the if statement
 */
static ASTNode *parse_if_statement(Parser *parser)
{
    parser_advance(parser); // eat 'if'
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    
    // Handle optional arrow syntax
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    
    ASTNode *then_block = parse_block(parser);
    ASTNode *else_block = NULL;
    
    // Handle optional else block
    if (parser->current_token->type == TOKEN_ELSE)
    {
        parser_advance(parser); // eat 'else'
        if (parser->current_token->type == TOKEN_ARROW)
            parser_advance(parser);
        else_block = parse_block(parser);
    }
    return ast_create_if(condition, then_block, else_block);
}

/*
 * parse_while_statement: Parse while loops
 * 
 * Handles while loops with condition and body.
 * Supports arrow syntax: while (condition) => { ... }
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the while statement
 */
/*
 * parse_while_statement: Parse while loops
 * 
 * Handles while loops with condition and body.
 * Supports arrow syntax: while (condition) => { ... }
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the while statement
 */
static ASTNode *parse_while_statement(Parser *parser)
{
    parser_advance(parser); // eat 'while'
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    
    // Handle optional arrow syntax
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    
    ASTNode *body = parse_block(parser);
    return ast_create_while(condition, body);
}

/*
 * parse_for_statement: Parse for loops
 * 
 * Handles two types of for loops:
 * 1. For-in loops: for (item in collection) { ... }
 * 2. Traditional C-style for loops: for (init; condition; increment) { ... }
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the for statement
 */
static ASTNode *parse_for_statement(Parser *parser)
{
    parser_advance(parser); // eat 'for'
    parser_expect(parser, TOKEN_LPAREN);

    // Check for for-in loop: for (identifier in collection)
    if (parser->current_token->type == TOKEN_IDENTIFIER)
    {
        Token *peek = parser_peek_token(parser);
        int is_in = (peek->type == TOKEN_IN);
        token_free(peek);

        if (is_in)
        {
            char *var = memory_strdup(parser->current_token->value);
            parser_advance(parser); // eat var
            parser_advance(parser); // eat in

            ASTNode *collection = parse_expression(parser);
            parser_expect(parser, TOKEN_RPAREN);

            // Handle optional arrow syntax
            if (parser->current_token->type == TOKEN_ARROW)
                parser_advance(parser);

            ASTNode *body = parse_block(parser);
            ASTNode *node = ast_create_for_in(var, collection, body);
            memory_free(var);
            return node;
        }
    }

    // Traditional C-style for loop: for (init; condition; increment)
    ASTNode *init = parse_statement(parser);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *increment = parse_expression(parser);

    parser_expect(parser, TOKEN_RPAREN);
    
    // Handle optional arrow syntax
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    
    ASTNode *body = parse_block(parser);

    return ast_create_for(init, condition, increment, body);
}

/*
 * parse_function: Parse function declarations
 * 
 * Handles function declarations with parameters and optional default values.
 * Supports void parameter lists and parameter arrays.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the function declaration
 */
static ASTNode *parse_function(Parser *parser)
{
    parser_advance(parser); // eat 'function'

    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected function name\n");
        return ast_create_null();
    }

    char *name = memory_strdup(parser->current_token->value);
    parser_advance(parser);

    parser_expect(parser, TOKEN_LPAREN);

    int capacity = 8;
    int count = 0;
    char **params = memory_allocate(sizeof(char *) * capacity);
    ASTNode **defaults = memory_allocate(sizeof(ASTNode *) * capacity);
    for (int i = 0; i < capacity; i++) defaults[i] = NULL;

    // Handle void parameter list or regular parameters
    if (parser->current_token->type == TOKEN_VOID)
    {
        // consume 'void' in parameter list
        parser_advance(parser);
    }
    else
    {
        // Parse parameter list
        while (parser->current_token->type == TOKEN_IDENTIFIER)
        {
            // Resize arrays if needed
            if (count >= capacity)
            {
                capacity *= 2;
                params = memory_reallocate(params, sizeof(char *) * capacity);
                defaults = memory_reallocate(defaults, sizeof(ASTNode *) * capacity);
                for (int i = count; i < capacity; i++) defaults[i] = NULL;
            }
            // Parse parameter name
            params[count++] = memory_strdup(parser->current_token->value);
            parser_advance(parser);
            
            // Handle optional default parameter value
            if (parser->current_token->type == TOKEN_ASSIGN)
            {
                parser_advance(parser); // eat =
                defaults[count - 1] = parse_expression(parser);
            }

            // Handle comma separator
            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
            }
        }
    }
    parser_expect(parser, TOKEN_RPAREN);
    
    // Handle optional arrow syntax
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    
    ASTNode *body = parse_block(parser);
    ASTNode *func = ast_create_function(name, params, count, body);
    func->data.function.defaults = defaults;
    memory_free(name);
    return func;
}


/*
 * parse_return: Parse return statements
 * 
 * Handles return statements with optional return value.
 * Supports both 'return;' and 'return expression;' syntax.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the return statement
 */
static ASTNode *parse_return(Parser *parser)
{
    parser_advance(parser); // eat 'return'
    ASTNode *value = NULL;

    // Check if there's a return value (not followed by semicolon, RBRACE, or EOF)
    if (parser->current_token->type != TOKEN_RBRACE &&
        parser->current_token->type != TOKEN_EOF &&
        parser->current_token->type != TOKEN_SEMICOLON)
    {
        value = parse_expression(parser);
    }

    return ast_create_return(value);
}

/*
 * parse_match: Parse match expressions (pattern matching)
 * 
 * Handles match expressions with case/default clauses.
 * Syntax: match (expression) { case value: body; default: body }
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the match expression
 */
static ASTNode *parse_match(Parser *parser) {
    parser_advance(parser); // eat 'match'
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *expr = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    parser_expect(parser, TOKEN_LBRACE);
    
    int capacity = 4;
    int count = 0;
    ASTNode **cases = memory_allocate(sizeof(ASTNode*) * capacity);
    ASTNode **bodies = memory_allocate(sizeof(ASTNode*) * capacity);
    ASTNode *default_case = NULL;
    
    // Parse case and default clauses
    while (parser->current_token->type != TOKEN_RBRACE && parser->current_token->type != TOKEN_EOF) {
        if (parser->current_token->type == TOKEN_CASE) {
            parser_advance(parser); // eat 'case'
            
            // Resize arrays if needed
            if (count >= capacity) {
                capacity *= 2;
                cases = memory_reallocate(cases, sizeof(ASTNode*) * capacity);
                bodies = memory_reallocate(bodies, sizeof(ASTNode*) * capacity);
            }
            
            // Parse case value and body
            cases[count] = parse_expression(parser);
            parser_expect(parser, TOKEN_COLON);
            
            // Parse case body (block or single statement)
            if (parser->current_token->type == TOKEN_LBRACE) {
                bodies[count] = parse_block(parser);
            } else {
                bodies[count] = parse_statement(parser);
            }
            count++;
        } else if (parser->current_token->type == TOKEN_DEFAULT) {
            parser_advance(parser);
            parser_expect(parser, TOKEN_COLON);
            if (parser->current_token->type == TOKEN_LBRACE) {
                default_case = parse_block(parser);
            } else {
                default_case = parse_statement(parser);
            }
        } else {
            parser_advance(parser);
        }
    }
    parser_expect(parser, TOKEN_RBRACE);
    return ast_create_match(expr, cases, bodies, count, default_case);
}

/*
 * parse_try: Parse try-catch-finally statements
 * 
 * Handles exception handling with try, optional catch, and optional finally blocks.
 * Syntax: try { ... } catch(error) { ... } finally { ... }
 * Both catch and finally are optional, but at least one must be present.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the try-catch-finally statement
 */
static ASTNode *parse_try(Parser *parser) {
    parser_advance(parser); // eat 'try'
    ASTNode *try_block = parse_block(parser);
    
    char *error_var = NULL;
    ASTNode *catch_block = NULL;
    ASTNode *finally_block = NULL;
    
    // Parse optional catch block with optional error variable
    if (parser->current_token->type == TOKEN_CATCH) {
        parser_advance(parser); // eat 'catch'
        
        // Parse optional error variable in parentheses: catch(error)
        if (parser->current_token->type == TOKEN_LPAREN) {
            parser_advance(parser); // eat (
            if (parser->current_token->type == TOKEN_IDENTIFIER) {
                error_var = memory_strdup(parser->current_token->value);
                parser_advance(parser); // eat variable name
            }
            parser_expect(parser, TOKEN_RPAREN);
        }
        catch_block = parse_block(parser);
    }
    
    // Parse optional finally block
    if (parser->current_token->type == TOKEN_FINALLY) {
        parser_advance(parser); // eat 'finally'
        finally_block = parse_block(parser);
    }
    
    ASTNode *node = ast_create_try_catch(try_block, error_var, catch_block, finally_block);
    if (error_var) memory_free(error_var);
    return node;
}

/*
 * parse_statement: Parse any statement
 * 
 * Main entry point for statement parsing. Handles all statement types:
 * - Variable declarations (var, const)
 * - Control flow (if, while, for, break, continue, return)
 * - Exception handling (try, throw)
 * - Function/class/struct/enum declarations
 * - Namespaces
 * - Match expressions
 * - Switch statements
 * - Expressions ending with semicolon
 * - Empty statements (semicolon alone)
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The parsed AST node for the statement
 */
static ASTNode *parse_statement(Parser *parser)
{
    // Handle empty statement (semicolon alone)
    if (parser->current_token->type == TOKEN_SEMICOLON)
    {
        parser_advance(parser);
        return ast_create_null();
    }
    
    // Skip error tokens and continue parsing
    if (parser->current_token->type == TOKEN_ERROR)
    {
        // skip invalid token and continue
        parser_advance(parser);
        return ast_create_null();
    }
    
    // Handle match expressions
    if (parser->current_token->type == TOKEN_MATCH)
        return parse_match(parser);
        
    // Handle try-catch-finally statements
    if (parser->current_token->type == TOKEN_TRY)
        return parse_try(parser);
        
    // Handle include/involve statements (file inclusion)
    if (parser->current_token->type == TOKEN_INCLUDE || parser->current_token->type == TOKEN_INVOLVE)
    {
        char *path = memory_strdup(parser->current_token->value);
        parser_advance(parser); // eat include/involve
        char *full = NULL;
        
        // Try to open file directly first
        FILE *f = fopen(path, "r");
        if (!f)
        {
            // Try with src/ prefix
            int len = strlen("src/") + strlen(path) + 1;
            full = memory_allocate(len);
            sprintf(full, "src/%s", path);
        }
        else
        {
            full = memory_strdup(path);
            fclose(f);
        }
        
        const char *guard_path = full ? full : path;
        
        // Check if file has already been included (prevent circular includes)
        if (parser_has_included(parser, guard_path))
        {
            if (full) memory_free(full);
            memory_free(path);
            return ast_create_null();
        }
        
        parser_add_include(parser, guard_path);
        
        // Open and read the file
        FILE *file = fopen(full, "r");
        if (!file)
        {
            fprintf(stderr, "Include error: could not open %s\n", full);
            if (full) memory_free(full);
            memory_free(path);
            return ast_create_null();
        }
        
        // Read entire file content
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *content = malloc(size + 1);
        fread(content, 1, size, file);
        content[size] = '\0';
        fclose(file);
        
        // Create lexer and parser for included file
        Lexer *inc_lexer = lexer_create(content);
        Parser *inc_parser = parser_create(inc_lexer);
        
        // Parse the included file
        ASTNode *inc_ast = parser_parse(inc_parser);
        
        // Clean up resources
        parser_free(inc_parser);
        lexer_free(inc_lexer);
        free(content);
        if (full) memory_free(full);
        memory_free(path);
        return inc_ast;
    }
    // Handle namespace declarations
    if (parser->current_token->type == TOKEN_NAMESPACE)
    {
        return parse_namespace(parser);
    }
    
    // Handle enum declarations
    if (parser->current_token->type == TOKEN_ENUM)
    {
        return parse_enum(parser);
    }
    
    // Handle class declarations
    if (parser->current_token->type == TOKEN_CLASS)
    {
        return parse_class(parser, 0); // 0 = class
    }
    
    // Handle struct declarations
    if (parser->current_token->type == TOKEN_STRUCT)
    {
        return parse_class(parser, 1); // 1 = struct
    }
    // Handle const declarations: const name[: type] = value
    if (parser->current_token->type == TOKEN_CONST)
    {
        parser_advance(parser); // eat 'const'
        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected identifier after const\n");
            return ast_create_null();
        }
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        
        // Handle optional type annotation
        char *type_name = NULL;
        if (parser->current_token->type == TOKEN_COLON)
        {
            parser_advance(parser); // eat :
            if (parser->current_token->type == TOKEN_IDENTIFIER)
            {
                type_name = memory_strdup(parser->current_token->value);
                parser_advance(parser);
            }
            else
            {
                fprintf(stderr, "Parse error: expected type name after ':'\n");
            }
        }
        
        // Expect assignment
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *value = parse_expression(parser);
        
        // Create assignment node with const flag
        ASTNode *assign = ast_create_assign(name, value, TOKEN_CONST);
        assign->data.assign.type_name = type_name;
        memory_free(name);
        return assign;
    }

    // Handle insert declarations: &insert name[: type] = value
    if (parser->current_token->type == TOKEN_INSERT)
    {
        parser_advance(parser); // eat '&insert'
        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected identifier after &insert\n");
            return ast_create_null();
        }
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        
        // Handle optional type annotation
        char *type_name = NULL;
        if (parser->current_token->type == TOKEN_COLON)
        {
            parser_advance(parser); // eat :
            if (parser->current_token->type == TOKEN_IDENTIFIER)
            {
                type_name = memory_strdup(parser->current_token->value);
                parser_advance(parser);
            }
            else
            {
                fprintf(stderr, "Parse error: expected type name after ':'\n");
            }
        }
        
        // Expect assignment
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *value = parse_expression(parser);
        
        // Create assignment node with insert flag
        ASTNode *assign = ast_create_assign(name, value, TOKEN_INSERT);
        assign->data.assign.type_name = type_name;
        memory_free(name);
        return assign;
    }

    // Handle control flow statements
    if (parser->current_token->type == TOKEN_IF)
    {
        return parse_if_statement(parser);
    }

    if (parser->current_token->type == TOKEN_WHILE)
    {
        return parse_while_statement(parser);
    }

    if (parser->current_token->type == TOKEN_FOR)
    {
        return parse_for_statement(parser);
    }

    if (parser->current_token->type == TOKEN_FUNCTION)
    {
        return parse_function(parser);
    }

    if (parser->current_token->type == TOKEN_RETURN)
    {
        return parse_return(parser);
    }

    if (parser->current_token->type == TOKEN_BREAK)
    {
        parser_advance(parser); // eat 'break'
        return ast_create_break();
    }

    if (parser->current_token->type == TOKEN_CONTINUE)
    {
        parser_advance(parser); // eat 'continue'
        return ast_create_continue();
    }

    // Keyword based assignments: add x = 5, sub x = 3, mul x = 2, div x = 4, mod x = 2
        // These are shorthand for x = x + 5, x = x - 3, x = x * 2, x = x / 4, x = x % 2
        if (parser->current_token->type == TOKEN_ADD ||
            parser->current_token->type == TOKEN_SUB ||
            parser->current_token->type == TOKEN_MUL ||
            parser->current_token->type == TOKEN_DIV ||
            parser->current_token->type == TOKEN_MOD)
        {
            TokenType op = parser->current_token->type;
            parser_advance(parser); // eat operator keyword

            // Expect identifier after operator keyword
            if (parser->current_token->type != TOKEN_IDENTIFIER)
            {
                fprintf(stderr, "Parse error: expected identifier after keyword assignment\n");
                return ast_create_null();
            }

            char *name = memory_strdup(parser->current_token->value);
            parser_advance(parser); // eat identifier
            parser_expect(parser, TOKEN_ASSIGN); // expect =
            ASTNode *value = parse_expression(parser);

            // Create assignment node with operator type
            ASTNode *assign = ast_create_assign(name, value, op);
            memory_free(name);
            return assign;
        }

    // Regular variable assignment or increment/decrement
    if (parser->current_token->type == TOKEN_IDENTIFIER)
    {
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser); // eat identifier

        // Compound assignment operators: +=, -=, *=, /=, %=
        if (parser->current_token->type == TOKEN_ASSIGN ||
            parser->current_token->type == TOKEN_PLUS_ASSIGN ||
            parser->current_token->type == TOKEN_MINUS_ASSIGN ||
            parser->current_token->type == TOKEN_MUL_ASSIGN ||
            parser->current_token->type == TOKEN_DIV_ASSIGN ||
            parser->current_token->type == TOKEN_MOD_ASSIGN)
        {
            TokenType op = parser->current_token->type;
            parser_advance(parser); // eat assignment operator
            ASTNode *value = parse_expression(parser);
            ASTNode *assign = ast_create_assign(name, value, op);
            memory_free(name);
            return assign;
        }
        // Increment/decrement operators: ++, --
        else if (parser->current_token->type == TOKEN_INC ||
                 parser->current_token->type == TOKEN_DEC)
        {
            TokenType incdec = parser->current_token->type;
            parser_advance(parser); // eat ++ or --
            ASTNode *one = ast_create_number(1);
            TokenType op = (incdec == TOKEN_INC) ? TOKEN_PLUS_ASSIGN : TOKEN_MINUS_ASSIGN;
            ASTNode *assign = ast_create_assign(name, one, op);
            memory_free(name);
            return assign;
        }
        else if (parser->current_token->type == TOKEN_LPAREN)
        {
            // Function call parsing: identifier followed by parentheses
            // Example: myFunction(arg1, arg2, arg3)
            parser_advance(parser); // eat '('
            
            // Initialize dynamic array for function arguments
            int capacity = 8;  // Initial capacity for arguments array
            int count = 0;     // Current number of arguments
            ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

            // Parse function arguments until closing parenthesis
            while (parser->current_token->type != TOKEN_RPAREN &&
                   parser->current_token->type != TOKEN_EOF)
            {
                // Expand array if needed (doubling strategy)
                if (count >= capacity)
                {
                    capacity *= 2;
                    args = memory_reallocate(args, sizeof(ASTNode *) * capacity);
                }
                
                // Parse the next argument expression
                args[count++] = parse_expression(parser);

                // Handle comma separator between arguments
                if (parser->current_token->type == TOKEN_COMMA)
                {
                    parser_advance(parser); // eat ','
                }
            }

            parser_expect(parser, TOKEN_RPAREN); // expect ')'
            
            // Create function call AST node with function name and arguments
            ASTNode *call = ast_create_call(name, args, count);
            memory_free(name);
            return call;
        }
        else if (parser->current_token->type == TOKEN_NEW)
        {
            // Future enhancement: support 'new' prefix for object instantiation
            // Currently handled as builtin function calls
        }
        memory_free(name);
    }

    // If no statement pattern matched, parse as a general expression
    return parse_expression(parser);
}

/*
 * parser_parse: Main parser entry point
 * 
 * Parses the entire input stream into an AST block node containing all statements.
 * Handles optional semicolon separators between statements.
 * 
 * Parameters:
 *   parser: The parser instance
 * 
 * Returns: The root AST node containing all parsed statements
 */
ASTNode *parser_parse(Parser *parser)
{
    // Initialize dynamic array for top-level statements
    int capacity = 16;  // Initial capacity for statements array
    int count = 0;      // Current number of statements
    ASTNode **statements = memory_allocate(sizeof(ASTNode *) * capacity);

    // Parse statements until end of file
    while (parser->current_token->type != TOKEN_EOF)
    {
        // Expand array if needed (doubling strategy)
        if (count >= capacity)
        {
            capacity *= 2;
            statements = memory_reallocate(statements, sizeof(ASTNode *) * capacity);
        }
        
        // Parse the next statement
        statements[count++] = parse_statement(parser);

        // Handle optional semicolon separator
        if (parser->current_token->type == TOKEN_SEMICOLON)
        {
            parser_advance(parser); // eat ';'
        }
    }

    // Create block node containing all parsed statements
    return ast_create_block(statements, count);
}
