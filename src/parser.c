#include "include/parser.h"
#include "include/memory.h"
#include <stdio.h>
#include <stdlib.h> // for atof

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

static void parser_advance(Parser *parser)
{
    token_free(parser->current_token);
    parser->current_token = lexer_next_token(parser->lexer);
}

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

static int parser_has_included(Parser *parser, const char *path)
{
    for (int i = 0; i < parser->include_count; i++)
    {
        if (strcmp(parser->include_paths[i], path) == 0)
            return 1;
    }
    return 0;
}

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

    if (token->type == TOKEN_IDENTIFIER)
    {
        char *name = memory_strdup(token->value);
        parser_advance(parser);

        if (parser->current_token->type == TOKEN_LPAREN)
        {
            parser_advance(parser);
            int capacity = 8;
            int count = 0;
            ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

            while (parser->current_token->type != TOKEN_RPAREN &&
                   parser->current_token->type != TOKEN_EOF)
            {
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
            ASTNode *call = ast_create_call(name, args, count);
            memory_free(name);
            return call;
        }

        ASTNode *ident = ast_create_identifier(name);
        memory_free(name);
        return ident;
    }

    if (token->type == TOKEN_LPAREN)
    {
        parser_advance(parser);
        ASTNode *expr = parse_expression(parser);
        parser_expect(parser, TOKEN_RPAREN);
        return expr;
    }

    if (token->type == TOKEN_PRINT || token->type == TOKEN_INPUT ||
        token->type == TOKEN_LEN || token->type == TOKEN_TYPE ||
        token->type == TOKEN_OUTPUT || token->type == TOKEN_ERRORFN ||
        token->type == TOKEN_WARNING || token->type == TOKEN_HELP)
    {
        TokenType func_type = token->type;
        parser_advance(parser);
        parser_expect(parser, TOKEN_LPAREN);

        int capacity = 8;
        int count = 0;
        ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

        while (parser->current_token->type != TOKEN_RPAREN &&
               parser->current_token->type != TOKEN_EOF)
        {
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

static ASTNode *parse_postfix(Parser *parser)
{
    ASTNode *left = parse_primary(parser);

    while (parser->current_token->type == TOKEN_LBRACKET)
    {
        parser_advance(parser);
        ASTNode *index = parse_expression(parser);
        parser_expect(parser, TOKEN_RBRACKET);
        left = ast_create_index(left, index);
    }

    return left;
}

static ASTNode *parse_unary(Parser *parser)
{
    if (parser->current_token->type == TOKEN_NOT ||
        parser->current_token->type == TOKEN_SUB)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        return ast_create_unary_op(op, parse_unary(parser));
    }

    return parse_postfix(parser);
}

static ASTNode *parse_multiplicative(Parser *parser)
{
    ASTNode *left = parse_unary(parser);

    while (parser->current_token->type == TOKEN_MUL ||
           parser->current_token->type == TOKEN_DIV ||
           parser->current_token->type == TOKEN_MOD)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_unary(parser));
    }

    return left;
}

static ASTNode *parse_additive(Parser *parser)
{
    ASTNode *left = parse_multiplicative(parser);

    while (parser->current_token->type == TOKEN_ADD ||
           parser->current_token->type == TOKEN_SUB)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_multiplicative(parser));
    }

    return left;
}

static ASTNode *parse_comparison(Parser *parser)
{
    ASTNode *left = parse_additive(parser);

    while (parser->current_token->type == TOKEN_LT ||
           parser->current_token->type == TOKEN_GT ||
           parser->current_token->type == TOKEN_LTE ||
           parser->current_token->type == TOKEN_GTE)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_additive(parser));
    }

    return left;
}

static ASTNode *parse_equality(Parser *parser)
{
    ASTNode *left = parse_comparison(parser);

    while (parser->current_token->type == TOKEN_EQ ||
           parser->current_token->type == TOKEN_NEQ)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_comparison(parser));
    }

    return left;
}

static ASTNode *parse_logical_and(Parser *parser)
{
    ASTNode *left = parse_equality(parser);

    while (parser->current_token->type == TOKEN_AND)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_equality(parser));
    }

    return left;
}

static ASTNode *parse_logical_or(Parser *parser)
{
    ASTNode *left = parse_logical_and(parser);

    while (parser->current_token->type == TOKEN_OR)
    {
        TokenType op = parser->current_token->type;
        parser_advance(parser);
        left = ast_create_binary_op(op, left, parse_logical_and(parser));
    }

    return left;
}

static ASTNode *parse_expression(Parser *parser)
{
    return parse_logical_or(parser);
}

static ASTNode *parse_block(Parser *parser)
{
    parser_expect(parser, TOKEN_LBRACE);

    int capacity = 16;
    int count = 0;
    ASTNode **statements = memory_allocate(sizeof(ASTNode *) * capacity);

    while (parser->current_token->type != TOKEN_RBRACE &&
           parser->current_token->type != TOKEN_EOF)
    {
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

static ASTNode *parse_namespace(Parser *parser)
{
    parser_advance(parser);
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected namespace name\n");
        return ast_create_null();
    }
    char *name = memory_strdup(parser->current_token->value);
    parser_advance(parser);
    ASTNode *body = parse_block(parser);
    ASTNode *ns = ast_create_namespace(name, body);
    memory_free(name);
    return ns;
}

static ASTNode *parse_enum(Parser *parser)
{
    parser_advance(parser);
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected enum name\n");
        return ast_create_null();
    }
    char *ename = memory_strdup(parser->current_token->value);
    parser_advance(parser);
    parser_expect(parser, TOKEN_LBRACE);
/////////////////////
    ////////////////////////////////// F,P!
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
        if (count >= capacity)
        {
            capacity *= 2;
            members = memory_reallocate(members, sizeof(char *) * capacity);
            values = memory_reallocate(values, sizeof(double) * capacity);
        }
        members[count] = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        double val = next_val;
        if (parser->current_token->type == TOKEN_ASSIGN)
        {
            parser_advance(parser);
            ASTNode *expr = parse_expression(parser);
            ASTNode *tmp = expr;
            Parser *p = parser;
            (void)p;
            if (expr->type == AST_NUMBER)
                val = expr->data.number.value;
            ast_free(tmp);
            next_val = val + 1;
        }
        else
        {
            val = next_val;
            next_val += 1;
        }
        values[count] = val;
        count++;
        if (parser->current_token->type == TOKEN_COMMA)
            parser_advance(parser);
    }
    parser_expect(parser, TOKEN_RBRACE);
    ASTNode *en = ast_create_enum(ename, members, values, count);
    memory_free(ename);
    return en;
}

static ASTNode *parse_class(Parser *parser, int is_struct)
{
    parser_advance(parser);
    if (parser->current_token->type != TOKEN_IDENTIFIER)
    {
        fprintf(stderr, "Parse error: expected class name\n");
        return ast_create_null();
    }
    char *name = memory_strdup(parser->current_token->value);
    parser_advance(parser);
    char *base = NULL;
    if (parser->current_token->type == TOKEN_COLON)
    {
        parser_advance(parser);
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
static ASTNode *parse_if_statement(Parser *parser)
{
    parser_advance(parser);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    ASTNode *then_block = parse_block(parser);
    ASTNode *else_block = NULL;
    if (parser->current_token->type == TOKEN_ELSE)
    {
        parser_advance(parser);
        if (parser->current_token->type == TOKEN_ARROW)
            parser_advance(parser);
        else_block = parse_block(parser);
    }
    return ast_create_if(condition, then_block, else_block);
}

static ASTNode *parse_while_statement(Parser *parser)
{
    parser_advance(parser);
    parser_expect(parser, TOKEN_LPAREN);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_RPAREN);
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    ASTNode *body = parse_block(parser);
    return ast_create_while(condition, body);
}

static ASTNode *parse_for_statement(Parser *parser)
{
    parser_advance(parser);
    parser_expect(parser, TOKEN_LPAREN);

    ASTNode *init = parse_statement(parser);
    ASTNode *condition = parse_expression(parser);
    parser_expect(parser, TOKEN_SEMICOLON);
    ASTNode *increment = parse_expression(parser);

    parser_expect(parser, TOKEN_RPAREN);
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    ASTNode *body = parse_block(parser);

    return ast_create_for(init, condition, increment, body);
}

static ASTNode *parse_function(Parser *parser)
{
    parser_advance(parser);

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

    if (parser->current_token->type == TOKEN_VOID)
    {
        // consume 'void' in parameter list
        parser_advance(parser);
    }
    else
    {
        while (parser->current_token->type == TOKEN_IDENTIFIER)
        {
            if (count >= capacity)
            {
                capacity *= 2;
                params = memory_reallocate(params, sizeof(char *) * capacity);
            }
            params[count++] = memory_strdup(parser->current_token->value);
            parser_advance(parser);

            if (parser->current_token->type == TOKEN_COMMA)
            {
                parser_advance(parser);
            }
        }
    }
    parser_expect(parser, TOKEN_RPAREN);
    if (parser->current_token->type == TOKEN_ARROW)
        parser_advance(parser);
    ASTNode *body = parse_block(parser);
    ASTNode *func = ast_create_function(name, params, count, body);
    memory_free(name);
    return func;
}


static ASTNode *parse_return(Parser *parser)
{
    parser_advance(parser);
    ASTNode *value = NULL;

    if (parser->current_token->type != TOKEN_RBRACE &&
        parser->current_token->type != TOKEN_EOF &&
        parser->current_token->type != TOKEN_SEMICOLON)
    {
        value = parse_expression(parser);
    }

    return ast_create_return(value);
}

static ASTNode *parse_statement(Parser *parser)
{
    if (parser->current_token->type == TOKEN_SEMICOLON)
    {
        parser_advance(parser);
        return ast_create_null();
    }
    if (parser->current_token->type == TOKEN_ERROR)
    {
        // skip invalid token and continue
        parser_advance(parser);
        return ast_create_null();
    }
    if (parser->current_token->type == TOKEN_INCLUDE)
    {
        char *path = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        char *full = NULL;
        FILE *f = fopen(path, "r");
        if (!f)
        {
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
        if (parser_has_included(parser, guard_path))
        {
            if (full) memory_free(full);
            memory_free(path);
            return ast_create_null();
        }
        parser_add_include(parser, guard_path);
        FILE *file = fopen(full, "r");
        if (!file)
        {
            fprintf(stderr, "Include error: could not open %s\n", full);
            if (full) memory_free(full);
            memory_free(path);
            return ast_create_null();
        }
        fseek(file, 0, SEEK_END);
        long size = ftell(file);
        fseek(file, 0, SEEK_SET);
        char *content = malloc(size + 1);
        fread(content, 1, size, file);
        content[size] = '\0';
        fclose(file);
        Lexer *inc_lexer = lexer_create(content);
        Parser *inc_parser = parser_create(inc_lexer);
        ASTNode *inc_ast = parser_parse(inc_parser);
        parser_free(inc_parser);
        lexer_free(inc_lexer);
        free(content);
        if (full) memory_free(full);
        memory_free(path);
        return inc_ast;
    }
    if (parser->current_token->type == TOKEN_NAMESPACE)
    {
        return parse_namespace(parser);
    }
    if (parser->current_token->type == TOKEN_ENUM)
    {
        return parse_enum(parser);
    }
    if (parser->current_token->type == TOKEN_CLASS)
    {
        return parse_class(parser, 0);
    }
    if (parser->current_token->type == TOKEN_STRUCT)
    {
        return parse_class(parser, 1);
    }
    if (parser->current_token->type == TOKEN_CONST)
    {
        parser_advance(parser);
        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected identifier after const\n");
            return ast_create_null();
        }
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *value = parse_expression(parser);
        ASTNode *assign = ast_create_assign(name, value, TOKEN_CONST);
        memory_free(name);
        return assign;
    }

    // TODO: fix to shut compiler up
    // i forgot i have adhd 2026
    if (parser->current_token->type == TOKEN_INSERT)
    {
        parser_advance(parser);
        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected identifier after &insert\n");
            return ast_create_null();
        }
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *value = parse_expression(parser);
        ASTNode *assign = ast_create_assign(name, value, TOKEN_INSERT);
        memory_free(name);
        return assign;
    }

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
        parser_advance(parser);
        return ast_create_break();
    }

    if (parser->current_token->type == TOKEN_CONTINUE)
    {
        parser_advance(parser);
        return ast_create_continue();
    }

    // Keyword based assignments (add, sub, mul etc identifiers as keywords)
    if (parser->current_token->type == TOKEN_ADD ||
        parser->current_token->type == TOKEN_SUB ||
        parser->current_token->type == TOKEN_MUL ||
        parser->current_token->type == TOKEN_DIV ||
        parser->current_token->type == TOKEN_MOD)
    {

        TokenType op = parser->current_token->type;
        parser_advance(parser);

        if (parser->current_token->type != TOKEN_IDENTIFIER)
        {
            fprintf(stderr, "Parse error: expected identifier after keyword assignment\n");
            return ast_create_null();
        }

        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);
        parser_expect(parser, TOKEN_ASSIGN);
        ASTNode *value = parse_expression(parser);

        ASTNode *assign = ast_create_assign(name, value, op);
        memory_free(name);
        return assign;
    }

    if (parser->current_token->type == TOKEN_IDENTIFIER)
    {
        char *name = memory_strdup(parser->current_token->value);
        parser_advance(parser);

        if (parser->current_token->type == TOKEN_ASSIGN ||
            parser->current_token->type == TOKEN_PLUS_ASSIGN ||
            parser->current_token->type == TOKEN_MINUS_ASSIGN ||
            parser->current_token->type == TOKEN_MUL_ASSIGN ||
            parser->current_token->type == TOKEN_DIV_ASSIGN ||
            parser->current_token->type == TOKEN_MOD_ASSIGN)
        {
            TokenType op = parser->current_token->type;
            parser_advance(parser);
            ASTNode *value = parse_expression(parser);
            ASTNode *assign = ast_create_assign(name, value, op);
            memory_free(name);
            return assign;
        }
        else if (parser->current_token->type == TOKEN_INC ||
                 parser->current_token->type == TOKEN_DEC)
        {
            TokenType incdec = parser->current_token->type;
            parser_advance(parser);
            ASTNode *one = ast_create_number(1);
            TokenType op = (incdec == TOKEN_INC) ? TOKEN_PLUS_ASSIGN : TOKEN_MINUS_ASSIGN;
            ASTNode *assign = ast_create_assign(name, one, op);
            memory_free(name);
            return assign;
        }
        else if (parser->current_token->type == TOKEN_LPAREN)
        {
            // Reconstructed Logic for function calls as statements
            parser_advance(parser);
            int capacity = 8;
            int count = 0;
            ASTNode **args = memory_allocate(sizeof(ASTNode *) * capacity);

            while (parser->current_token->type != TOKEN_RPAREN &&
                   parser->current_token->type != TOKEN_EOF)
            {
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
            ASTNode *call = ast_create_call(name, args, count);
            memory_free(name);
            return call;
        }
        else if (parser->current_token->type == TOKEN_NEW)
        {
            // future: support 'new' prefix; currently handled as builtin
        }
        memory_free(name);
    }

    return parse_expression(parser);
}

ASTNode *parser_parse(Parser *parser)
{
    int capacity = 16;
    int count = 0;
    ASTNode **statements = memory_allocate(sizeof(ASTNode *) * capacity);

    while (parser->current_token->type != TOKEN_EOF)
    {
        if (count >= capacity)
        {
            capacity *= 2;
            statements = memory_reallocate(statements, sizeof(ASTNode *) * capacity);
        }
        statements[count++] = parse_statement(parser);

        // option ";" support (oh boy)
        if (parser->current_token->type == TOKEN_SEMICOLON)
        {
            parser_advance(parser);
        }
    }

    return ast_create_block(statements, count);
}
