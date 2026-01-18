#ifndef LEXER_H
#define LEXER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

typedef enum
{
    TOKEN_ADD,
    TOKEN_SUB,
    TOKEN_MUL,
    TOKEN_DIV, // <div class=""></div> 
    TOKEN_MOD,
    TOKEN_IDENTIFIER,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_IF,
    TOKEN_ELSE,
    TOKEN_WHILE,
    TOKEN_FOR,
    TOKEN_FUNCTION,
    TOKEN_RETURN,
    TOKEN_BREAK,
    TOKEN_CONTINUE,
    TOKEN_ASSIGN,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_MUL_ASSIGN,
    TOKEN_DIV_ASSIGN,
    TOKEN_MOD_ASSIGN,
    TOKEN_EQ,
    TOKEN_NEQ,
    TOKEN_LT,
    TOKEN_GT,
    TOKEN_LTE,
    TOKEN_GTE,
    TOKEN_AND,
    TOKEN_OR,
    TOKEN_NOT,
    TOKEN_INC,
    TOKEN_DEC,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACE,
    TOKEN_RBRACE,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_ARROW,
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_SEMICOLON,
    TOKEN_SYSTEM,
    TOKEN_PRINT,
    TOKEN_INPUT,
    TOKEN_LEN,
    TOKEN_TYPE,
    TOKEN_OUTPUT,
    TOKEN_ERRORFN,
    TOKEN_WARNING,
    TOKEN_INSERT,
    TOKEN_CONST,
    TOKEN_END,
    TOKEN_VOID,
    TOKEN_HELP,
    TOKEN_NAMESPACE,
    TOKEN_ENUM,
    TOKEN_CLASS,
    TOKEN_STRUCT,
    TOKEN_NEW,
    TOKEN_COLON,
    TOKEN_TRUE,
    TOKEN_FALSE,
    TOKEN_NULL,
    TOKEN_INCLUDE,
    TOKEN_INVOLVE,
    TOKEN_MATCH,
    TOKEN_CASE,
    TOKEN_DEFAULT,
    TOKEN_TRY,
    TOKEN_CATCH,
    TOKEN_FINALLY,
    TOKEN_IN,
    TOKEN_EOF,
    TOKEN_ERROR
} TokenType;

typedef struct
{
    TokenType type;
    char *value;
    int line;
    int column;
} Token;

typedef struct
{
    char *source;
    int position;
    int line;
    int column;
    int length;
} Lexer;

Lexer *lexer_create(const char *source);
void lexer_free(Lexer *lexer);
Token *lexer_next_token(Lexer *lexer);
void token_free(Token *token);

#endif // LEXER_H
