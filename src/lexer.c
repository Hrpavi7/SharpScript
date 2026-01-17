#include "include/lexer.h"
#include "include/memory.h"

// JDADH AWU DHAKU DHAW DH aaaaaAAAA
// noice

Lexer *lexer_create(const char *source)
{
    Lexer *lexer = malloc(sizeof(Lexer));
    lexer->source = memory_strdup(source);
    lexer->position = 0;
    lexer->line = 1;
    lexer->column = 1;
    lexer->length = strlen(source);
    return lexer;
}

void lexer_free(Lexer *lexer)
{
    free(lexer->source);
    free(lexer);
}

void token_free(Token *token)
{
    if (token->value)
        free(token->value);
    free(token);
}

static char lexer_peek(Lexer *lexer)
{
    if (lexer->position >= lexer->length)
        return '\0';
    return lexer->source[lexer->position];
}

static char lexer_advance(Lexer *lexer)
{
    if (lexer->position >= lexer->length)
        return '\0';
    char c = lexer->source[lexer->position++];
    if (c == '\n')
    {
        lexer->line++;
        lexer->column = 1;
    }
    else
    {
        lexer->column++;
    }
    return c;
}

static void lexer_skip_whitespace(Lexer *lexer)
{
    while (isspace(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }
}

static void lexer_skip_comment(Lexer *lexer)
{
    if (lexer_peek(lexer) == '#')
    {
        int pos = lexer->position;
        const char *kw = "#include";
        int i = 0;
        while (kw[i] && (pos + i) < lexer->length && lexer->source[pos + i] == kw[i]) i++;
        if (i == 8)
        {
            return;
        }
        while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0')
        {
            lexer_advance(lexer);
        }
    }
}

static Token *token_create(TokenType type, const char *value, int line, int col)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? memory_strdup(value) : NULL;
    token->line = line;
    token->column = col;
    return token;
}

static Token *lexer_read_number(Lexer *lexer)
{
    int start = lexer->position;
    int line = lexer->line;
    int col = lexer->column;

    while (isdigit(lexer_peek(lexer)) || lexer_peek(lexer) == '.')
    {
        lexer_advance(lexer);
    }

    int length = lexer->position - start;
    char *value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';

    Token *token = token_create(TOKEN_NUMBER, value, line, col);
    free(value);
    return token;
}

static Token *lexer_read_string(Lexer *lexer)
{
    int line = lexer->line;
    int col = lexer->column;
    lexer_advance(lexer);

    int start = lexer->position;
    while (lexer_peek(lexer) != '"' && lexer_peek(lexer) != '\0')
    {
        lexer_advance(lexer);
    }

    int length = lexer->position - start;
    char *value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';

    if (lexer_peek(lexer) == '"')
        lexer_advance(lexer);

    Token *token = token_create(TOKEN_STRING, value, line, col);
    free(value);
    return token;
}

static Token *lexer_read_identifier(Lexer *lexer)
{
    int start = lexer->position;
    int line = lexer->line;
    int col = lexer->column;

    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_' || lexer_peek(lexer) == '.')
    {
        lexer_advance(lexer);
    }

    int length = lexer->position - start;
    char *value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';

    // ctrl+c ctrl+v
    // elseif statements

    TokenType type = TOKEN_IDENTIFIER;
    if (strcmp(value, "add") == 0)
        type = TOKEN_ADD;
    else if (strcmp(value, "sub") == 0)
        type = TOKEN_SUB;
    else if (strcmp(value, "mul") == 0)
        type = TOKEN_MUL;
    else if (strcmp(value, "div") == 0)
        type = TOKEN_DIV;
    else if (strcmp(value, "mod") == 0)
        type = TOKEN_MOD;
    else if (strcmp(value, "if") == 0)
        type = TOKEN_IF;
    else if (strcmp(value, "else") == 0)
        type = TOKEN_ELSE;
    else if (strcmp(value, "while") == 0)
        type = TOKEN_WHILE;
    else if (strcmp(value, "for") == 0)
        type = TOKEN_FOR;
    else if (strcmp(value, "function") == 0)
        type = TOKEN_FUNCTION;
    else if (strcmp(value, "void") == 0)
        type = TOKEN_VOID;
    else if (strcmp(value, "const") == 0)
        type = TOKEN_CONST;
    else if (strcmp(value, "help") == 0)
        type = TOKEN_HELP;
    else if (strcmp(value, "end") == 0)
        type = TOKEN_END;
    else if (strcmp(value, "return") == 0)
        type = TOKEN_RETURN;
    else if (strcmp(value, "break") == 0)
        type = TOKEN_BREAK;
    else if (strcmp(value, "continue") == 0)
        type = TOKEN_CONTINUE;
    else if (strcmp(value, "true") == 0)
        type = TOKEN_TRUE;
    else if (strcmp(value, "false") == 0)
        type = TOKEN_FALSE;
    else if (strcmp(value, "null") == 0)
        type = TOKEN_NULL;
    else if (strcmp(value, "system.print") == 0)
        type = TOKEN_PRINT;
    else if (strcmp(value, "system.input") == 0)
        type = TOKEN_INPUT;
    else if (strcmp(value, "system.len") == 0)
        type = TOKEN_LEN;
    else if (strcmp(value, "system.type") == 0)
        type = TOKEN_TYPE;
    else if (strcmp(value, "system.output") == 0)
        type = TOKEN_OUTPUT;
    else if (strcmp(value, "system.error") == 0)
        type = TOKEN_ERRORFN;
    else if (strcmp(value, "system.warning") == 0)
        type = TOKEN_WARNING;
    else if (strcmp(value, "namespace") == 0)
        type = TOKEN_NAMESPACE;
    else if (strcmp(value, "enum") == 0)
        type = TOKEN_ENUM;
    else if (strcmp(value, "class") == 0)
        type = TOKEN_CLASS;
    else if (strcmp(value, "struct") == 0)
        type = TOKEN_STRUCT;
    else if (strcmp(value, "new") == 0)
        type = TOKEN_NEW;

    Token *token = token_create(type, value, line, col);
    free(value);
    return token;
}

Token *lexer_next_token(Lexer *lexer)
{
    lexer_skip_whitespace(lexer);
    lexer_skip_comment(lexer);
    lexer_skip_whitespace(lexer);

    if (lexer->position >= lexer->length)
    {
        return token_create(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }

    char c = lexer_peek(lexer);
    int line = lexer->line;
    int col = lexer->column;

    if (isdigit(c))
        return lexer_read_number(lexer);
    if (c == '"')
        return lexer_read_string(lexer);
    if (isalpha(c) || c == '_')
        return lexer_read_identifier(lexer);

    lexer_advance(lexer);

    switch (c)
    {
    case '//':
    {
        const char *kw = "include";
        int i = 0;
        while (kw[i] && lexer->position < lexer->length &&
               lexer->source[lexer->position] == kw[i])
        {
            lexer_advance(lexer);
            i++;
        }
        if (!kw[i])
        {
            while (isspace(lexer_peek(lexer))) lexer_advance(lexer);
            if (lexer_peek(lexer) == '"')
            {
                Token *path = lexer_read_string(lexer);
                Token *tok = token_create(TOKEN_INCLUDE, path->value, line, col);
                token_free(path);
                return tok;
            }
            return token_create(TOKEN_ERROR, NULL, line, col);
        }
        while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0') lexer_advance(lexer);
        return token_create(TOKEN_ERROR, NULL, line, col);
    }
    case '=':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_EQ, "==", line, col);
        }
        if (lexer_peek(lexer) == '>')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_ARROW, "=>", line, col);
        }
        return token_create(TOKEN_ASSIGN, "=", line, col);
    case '+':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_PLUS_ASSIGN, "+=", line, col);
        }
        if (lexer_peek(lexer) == '+')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_INC, "++", line, col);
        }
        return token_create(TOKEN_ADD, "+", line, col);
    case '-':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_MINUS_ASSIGN, "-=", line, col);
        }
        if (lexer_peek(lexer) == '-')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_DEC, "--", line, col);
        }
        return token_create(TOKEN_SUB, "-", line, col); // never knew programming a language would be this hard
    case '*':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_MUL_ASSIGN, "*=", line, col);
        }
        return token_create(TOKEN_MUL, "*", line, col);
    case '/':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_DIV_ASSIGN, "/=", line, col);
        }
        return token_create(TOKEN_DIV, "/", line, col);
    case '%':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_MOD_ASSIGN, "%=", line, col);
        }
        return token_create(TOKEN_MOD, "%", line, col);
    case '!':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_NEQ, "!=", line, col);
        }
        return token_create(TOKEN_NOT, "!", line, col);
    case '<':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_LTE, "<=", line, col);
        }
        return token_create(TOKEN_LT, "<", line, col);
    case '>':
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_GTE, ">=", line, col);
        }
        return token_create(TOKEN_GT, ">", line, col);
    case '&':
        if (lexer_peek(lexer) == '&')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_AND, "&&", line, col);
        }
        {
            int i = 0;
            const char *kw = "insert";
            while (kw[i] && lexer->position < lexer->length &&
                   lexer->source[lexer->position] == kw[i])
            {
                lexer_advance(lexer);
                i++;
            }
            if (!kw[i])
            {
                return token_create(TOKEN_INSERT, "&insert", line, col);
            }
        }
        break;
    case '|':
        if (lexer_peek(lexer) == '|')
        {
            lexer_advance(lexer);
            return token_create(TOKEN_OR, "||", line, col);
        }
        break;
        // I LOVE THIS
    case '(':
        return token_create(TOKEN_LPAREN, "(", line, col);
    case ')':
        return token_create(TOKEN_RPAREN, ")", line, col);
    case '{':
        return token_create(TOKEN_LBRACE, "{", line, col);
    case '}':
        return token_create(TOKEN_RBRACE, "}", line, col);
    case '[':
        return token_create(TOKEN_LBRACKET, "[", line, col);
    case ']':
        return token_create(TOKEN_RBRACKET, "]", line, col);
    case ',':
        return token_create(TOKEN_COMMA, ",", line, col);
    case '.':
        return token_create(TOKEN_DOT, ".", line, col);
    case ';':
        return token_create(TOKEN_SEMICOLON, ";", line, col);
    case ':':
        return token_create(TOKEN_COLON, ":", line, col);
    }

    return token_create(TOKEN_ERROR, NULL, line, col);
}
