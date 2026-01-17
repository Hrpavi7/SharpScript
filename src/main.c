#include "include/lexer.h"
#include "include/parser.h"
#include "include/interpreter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*
    the only reason i cant put comments in the top of the includes is
    cuz the goddamn compiler keeps yelling at me
    and it takes up 2 gb on my pCAcah a dha 
*/

char *read_file(const char *filename)
{
    FILE *file = fopen(filename, "r");
    if (!file)
    {
        fprintf(stderr, "Error: Could not open file %s\n", filename);
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    fseek(file, 0, SEEK_SET);

    char *content = malloc(size + 1);
    fread(content, 1, size, file);
    content[size] = '\0';

    fclose(file);
    return content;
}

void show_help(void)
{
    // printf("die")
    printf("SharpScript Language Environment\n");
    printf("Usage:\n");
    printf("  sharpscript            - Starts the interactive REPL\n");
    printf("  sharpscript <file>     - Executes a .sharp script\n");
    printf("  sharpscript --help     - Displays this help message\n\n");
    
    printf("Language Syntax Overview:\n");
    printf("  - Declaration:  &insert x = 10;\n");
    printf("  - Functions:    function name(void) { ... }\n");
    printf("  - Control:      if (cond) { ... } else { ... }\n");
    printf("  - Output:       system.output(expr);\n");
    printf("  - Error/Warn:   system.error(msg); system.warning(msg);\n");
    printf("  - Comments:     # This is a comment\n");
}

void run_repl(void)
{
    Interpreter *interp = interpreter_create();
    char line[1024];

    printf("SharpScript REPL v1.0\n");
    printf("Type 'exit' to quit\n\n");

    while (1)
    {
        printf(">> ");
        if (!fgets(line, sizeof(line), stdin))
            break;

        if (strcmp(line, "exit\n") == 0)
            break;

        Lexer *lexer = lexer_create(line);
        Parser *parser = parser_create(lexer);
        ASTNode *ast = parser_parse(parser);

        Value *result = interpreter_eval(interp, ast);
        value_free(result);

        ast_free(ast);
        parser_free(parser);
        lexer_free(lexer);
    }

    interpreter_free(interp);
}

void run_file(const char *filename)
{
    char *source = read_file(filename);
    if (!source)
        return;

    Lexer *lexer = lexer_create(source);
    Parser *parser = parser_create(lexer);
    ASTNode *ast = parser_parse(parser);

    Interpreter *interp = interpreter_create();
    Value *result = interpreter_eval(interp, ast);
    value_free(result);

    // auto-invoke main(void) if defined
    ASTNode *main_call = ast_create_call("main", NULL, 0);
    Value *main_result = interpreter_eval(interp, main_call);
    value_free(main_result);
    ast_free(main_call);

    interpreter_free(interp);
    ast_free(ast);
    parser_free(parser);
    lexer_free(lexer);
    free(source);
}

int main(int argc, char* argv[]) {
    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0)) {
        show_help();
        return 0;
    }
    if (argc == 1) {
        run_repl();
        return 0;
    }
    if (argc == 2) {
        run_file(argv[1]);
        return 0;
    }
    // never knew why compilers do this error but i kinda like it
    fprintf(stderr, "Error: Too many arguments.\n");
    show_help();
    return 1; // returns one "i use arch linux btw"
}