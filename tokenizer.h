#ifndef TOKENIZER_H
#define TOKENIZER_H

#include <stddef.h>
#include <stdbool.h>

#define N_TOKEN_TYPE 17
#define INIT_CAPACITY 8

#define vec_append(xs, x)\
do {\
    if ((xs).count >= (xs).capacity) {\
        if ((xs).capacity == 0) (xs).capacity = INIT_CAPACITY;\
        else (xs).capacity *= 2;\
        (xs).items = realloc((xs).items, (xs).capacity*sizeof(*(xs).items));\
        if ((xs).items==NULL) {\
            fprintf(stderr, "vec_append realloc failed");\
            exit(1);\
        }\
    }\
    (xs).items[(xs).count++] = (x);\
} while(0)

typedef struct {
    size_t count;
    size_t capacity;
    char   *items;    
} String;

typedef enum {
    PLUS,
    MINUS,
    STAR,
    SLASH,
    LPAREN,
    RPAREN,
    NUMBER,
    IDENTIFIER,
    COMMA,
    EQUAL,
    FN,
    LCURLY,
    RCURLY,
    SEMICOLON,
    ENDSTREAM,
    QUESTION,
    COLON,
    LOGIC_AND,
    LOGIC_OR,
    BANG,
    EQUALEQUAL,
    LTEQUAL,
    GTEQUAL,
    LT,
    NEQ,
    GT,
    GRAPH,
} TokenType;

typedef struct {
    TokenType type;
    union { 
        char op;
        double number; 
        char *string;
    } as;
} Token;

typedef struct {
    size_t pos;
    size_t count;
    size_t capacity;
    Token *items;
} Tokens;

typedef struct program {
    char *string;
    int pos;
} program;

extern Tokens tokens;
extern program p;

bool tokenize(Tokens *output);

void free_tokens(Tokens *ts);
void reset_program(program *prog, char *input);
bool str_from(Token token, char *str, size_t strlen);
void print_token(Token token);
void print_tokens(Tokens tokens);

#endif /* TOKENIZER_H */
