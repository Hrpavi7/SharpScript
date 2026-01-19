/*
    Copyright (c) 2024-2026 SharpScript Programming Language
    
    Licensed under the MIT License
*/

// START OF lexer.c

#include "include/lexer.h"
#include "include/memory.h"

/*
 * lexer_create: Create a new lexer instance
 * 
 * Allocates memory for a new Lexer and initializes it with the provided source code.
 * Sets up position tracking (line 1, column 1) and calculates source length.
 * 
 * Parameters:
 *   source: The source code string to tokenize
 * 
 * Returns: Pointer to the newly created Lexer instance
 */
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

/*
 * lexer_free: Free a lexer instance and its resources
 * 
 * Deallocates the lexer source string and the lexer structure itself.
 * Safe to call on NULL pointers (no-op).
 * 
 * Parameters:
 *   lexer: The lexer instance to free
 */
void lexer_free(Lexer *lexer)
{
    if (!lexer) return;
    free(lexer->source);
    free(lexer);
}

/*
 * token_free: Free a token and its associated value
 * 
 * Deallocates the token's value string (if present) and the token structure itself.
 * Safe to call on NULL pointers (no-op).
 * 
 * Parameters:
 *   token: The token instance to free
 */
void token_free(Token *token)
{
    if (!token) return;
    if (token->value)
        free(token->value);
    free(token);
}

/*
 * lexer_peek: Peek at the current character without advancing
 * 
 * Returns the character at the current lexer position without consuming it.
 * Returns null terminator ('\0') if at end of input.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: The current character or '\0' if at end
 */
static char lexer_peek(Lexer *lexer)
{
    if (lexer->position >= lexer->length)
        return '\0';
    return lexer->source[lexer->position];
}

/*
 * lexer_advance: Advance the lexer position and return the character
 * 
 * Consumes the current character, updates line/column tracking, and returns it.
 * Handles newline detection for accurate error reporting.
 * Returns null terminator ('\0') if at end of input.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: The consumed character or '\0' if at end
 */
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

/*
 * lexer_skip_whitespace: Skip whitespace characters
 * 
 * Advances the lexer past any whitespace characters (spaces, tabs, newlines).
 * Used to ignore formatting between tokens.
 * 
 * Parameters:
 *   lexer: The lexer instance
 */
static void lexer_skip_whitespace(Lexer *lexer)
{
    while (isspace(lexer_peek(lexer)))
    {
        lexer_advance(lexer);
    }
}

/*
 * lexer_skip_comment: Skip comment lines (but preserve #include/#involve directives)
 * 
 * Handles comment lines starting with '#'. Special cases for #include and #involve
 * directives which are preserved as tokens rather than skipped as comments.
 * Regular comments are consumed until end of line.
 * 
 * Parameters:
 *   lexer: The lexer instance
 */
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
            return; // Don't skip #include directives
        }
        
        // check for #involve
        kw = "#involve";
        i = 0;
        while (kw[i] && (pos + i) < lexer->length && lexer->source[pos + i] == kw[i]) i++;
        if (i == 8)
        {
            return; // Don't skip #involve directives
        }
        while (lexer_peek(lexer) != '\n' && lexer_peek(lexer) != '\0')
        {
            lexer_advance(lexer);
        }
    }
}

/*
 * token_create: Create a new token with the given properties
 * 
 * Allocates memory for a new Token and initializes it with type, value, and position info.
 * Creates a copy of the value string using memory_strdup if provided.
 * 
 * Parameters:
 *   type: The token type (TOKEN_NUMBER, TOKEN_STRING, etc.)
 *   value: The token value string (may be NULL for operators/punctuation)
 *   line: The source line number where the token starts
 *   col: The source column number where the token starts
 * 
 * Returns: Pointer to the newly created Token
 */
static Token *token_create(TokenType type, const char *value, int line, int col)
{
    Token *token = malloc(sizeof(Token));
    token->type = type;
    token->value = value ? memory_strdup(value) : NULL;
    token->line = line;
    token->column = col;
    return token;
}

/*
 * lexer_read_number: Read a numeric literal token
 * 
 * Consumes digits and optional decimal points to form a number token.
 * Handles both integer and floating-point number formats.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: A new TOKEN_NUMBER token with the numeric value
 */
static Token *lexer_read_number(Lexer *lexer)
{
    int start = lexer->position;
    int line = lexer->line;
    int col = lexer->column;

    // Consume digits and decimal points
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

/*
 * lexer_read_string: Read a string literal token
 * 
 * Consumes characters between double quotes to form a string token.
 * Handles proper opening/closing quote detection and string extraction.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: A new TOKEN_STRING token with the string value (without quotes)
 */
static Token *lexer_read_string(Lexer *lexer)
{
    int line = lexer->line;
    int col = lexer->column;
    lexer_advance(lexer); // consume opening quote

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
        lexer_advance(lexer); // consume closing quote

    Token *token = token_create(TOKEN_STRING, value, line, col);
    free(value);
    return token;
}

/*
 * lexer_read_identifier: Read an identifier or keyword token
 * 
 * Consumes alphanumeric characters, underscores, and dots to form an identifier.
 * Checks against keyword list to determine if it's a reserved word or user identifier.
 * Handles extensive keyword recognition including operators, control flow, and system functions.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: A token with appropriate type (keyword or TOKEN_IDENTIFIER)
 */
static Token *lexer_read_identifier(Lexer *lexer)
{
    int start = lexer->position;
    int line = lexer->line;
    int col = lexer->column;

    // Consume identifier characters (alphanumeric, underscore, dot)
    while (isalnum(lexer_peek(lexer)) || lexer_peek(lexer) == '_' || lexer_peek(lexer) == '.')
    {
        lexer_advance(lexer);
    }

    int length = lexer->position - start;
    char *value = malloc(length + 1);
    strncpy(value, lexer->source + start, length);
    value[length] = '\0';

    // Check if identifier is a keyword or reserved word
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
    else if (strcmp(value, "match") == 0)
        type = TOKEN_MATCH;
    else if (strcmp(value, "case") == 0)
        type = TOKEN_CASE;
    else if (strcmp(value, "default") == 0)
        type = TOKEN_DEFAULT;
    else if (strcmp(value, "try") == 0)
        type = TOKEN_TRY;
    else if (strcmp(value, "catch") == 0)
        type = TOKEN_CATCH;
    else if (strcmp(value, "finally") == 0)
        type = TOKEN_FINALLY;
    else if (strcmp(value, "in") == 0)
        type = TOKEN_IN;

    Token *token = token_create(type, value, line, col);
    free(value);
    return token;
}

/*
 * lexer_next_token: Get the next token from the lexer
 * 
 * Main tokenization function that processes the lexer state and returns the next token.
 * Handles whitespace/comment skipping, EOF detection, and delegates to specialized readers
 * for numbers, strings, and identifiers. Processes single and multi-character operators.
 * 
 * Parameters:
 *   lexer: The lexer instance
 * 
 * Returns: The next token in the input stream, or TOKEN_ERROR for unrecognized input
 */
Token *lexer_next_token(Lexer *lexer)
{
    // Skip whitespace and comments before tokenizing
    lexer_skip_whitespace(lexer);
    lexer_skip_comment(lexer);
    lexer_skip_whitespace(lexer);

    // Handle end of file
    if (lexer->position >= lexer->length)
    {
        return token_create(TOKEN_EOF, NULL, lexer->line, lexer->column);
    }

    char c = lexer_peek(lexer);
    int line = lexer->line;
    int col = lexer->column;

    // Delegate to specialized readers for different token types
    if (isdigit(c))
        return lexer_read_number(lexer);
    if (c == '"')
        return lexer_read_string(lexer);
    if (isalpha(c) || c == '_')
        return lexer_read_identifier(lexer);

    // Consume the current character for operator processing
    lexer_advance(lexer);

    // Process operators and punctuation
    switch (c)
    {
    case '#':
    {
        // Handle preprocessor directives: #include and #involve
        // Check for #include directive
        const char *kw = "include";
        int i = 0;
        int match = 1;
        int start = lexer->position;
        
        // Compare against "include" keyword
        while (kw[i]) {
            if (lexer->position + i >= lexer->length || lexer->source[lexer->position + i] != kw[i]) {
                match = 0;
                break;
            }
            i++;
        }
        
        if (match) {
            // Consume "include" keyword
            for(int k=0; k<i; k++) lexer_advance(lexer);
            
            // Skip whitespace after directive
            while (isspace(lexer_peek(lexer))) lexer_advance(lexer);
            
            // Expect string literal for file path
            if (lexer_peek(lexer) == '"')
            {
                Token *path = lexer_read_string(lexer);
                Token *tok = token_create(TOKEN_INCLUDE, path->value, line, col);
                token_free(path);
                return tok;
            }
            
            return token_create(TOKEN_ERROR, "Expected string after #include", line, col);
        }

        // Check for #involve directive
        kw = "involve";
        i = 0;
        match = 1;
        
        // Compare against "involve" keyword
        while (kw[i]) {
            if (lexer->position + i >= lexer->length || lexer->source[lexer->position + i] != kw[i]) {
                match = 0;
                break;
            }
            i++;
        }
        
        if (match) {
            // Consume "involve" keyword
            for(int k=0; k<i; k++) lexer_advance(lexer);
            
            // Skip whitespace after directive
            while (isspace(lexer_peek(lexer))) lexer_advance(lexer);
            
            // Expect string literal for file path
            if (lexer_peek(lexer) == '"')
            {
                Token *path = lexer_read_string(lexer);
                Token *tok = token_create(TOKEN_INVOLVE, path->value, line, col);
                token_free(path);
                return tok;
            }
            
            return token_create(TOKEN_ERROR, "Expected string after #involve", line, col);
        }
        
        // Handle unknown directives or comments that weren't caught by lexer_skip_comment
        // This can happen when lexer_skip_comment sees #include/#involve but they don't match exactly
        return token_create(TOKEN_ERROR, "Unknown directive or invalid comment", line, col);
    }
    case '=':
        // Handle assignment and comparison operators
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer); // consume second '='
            return token_create(TOKEN_EQ, "==", line, col);
        }
        if (lexer_peek(lexer) == '>')
        {
            lexer_advance(lexer); // consume '>'
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
        // Handle subtraction, decrement, and compound assignment operators
        if (lexer_peek(lexer) == '=')
        {
            lexer_advance(lexer); // consume '='
            return token_create(TOKEN_MINUS_ASSIGN, "-=", line, col);
        }
        if (lexer_peek(lexer) == '-')
        {
            lexer_advance(lexer); // consume second '-'
            return token_create(TOKEN_DEC, "--", line, col);
        }
        return token_create(TOKEN_SUB, "-", line, col);
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

// END OF lexer.c
