#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <assert.h>
#include "hashmap.h"

typedef struct {
    const char* start;
    int length;
} Sv; 
//stringview

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
} TokenType;
#define N_TOKEN_TYPE 17

typedef enum {
    SUCCESS,
    PARSE_ERR,
    CREATE_EXPR_ERR,
} Error;

typedef struct {
    TokenType type;
    union { char operator;
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
Tokens tokens = {0};

void free_tokens(Tokens *ts) {
    if (ts->items != NULL) {
        free(ts->items);
    }
    ts->items = NULL;
    ts->count = 0;
    ts->capacity = 0;
    ts->pos = 0; 
}

typedef struct program {
    char *string;
    int pos;
} program;
program p;

void reset_program(program *prog, char *input) {
    prog->string = input;
    prog->pos = 0;
}

bool eof() {
    return p.pos > strlen(p.string);
}

char peek() {
    if (eof()) return '\0';
    return p.string[p.pos];
}

char consume() {
    if (eof()) return '\0';
    return p.string[p.pos++];
}

void advance() {
    if (eof()) return;
    p.pos++;
}

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

int ctod(char character) {
    // char to digit
    return character - '0';
}

bool is_op(char c) {
    switch (c) {
        case '+':
        case '-':
        case '/':
        case '*':
            return true;
        default:
            return false;
    }
}

const char keywords[] = {"fn"};
int get_keyword_i(char *string) {
    // returns keyword index
    for (int i=0; i<sizeof(keywords); i++) {
        if (strcmp(&keywords[i], string)==0) {
            return i;
        };
    }
    return -1;
}

bool is_digit_delim(char c) {
    return (c==')' || c==',' || c==';' || c=='}' || c=='?' || c==':');
}
bool tokenize(Tokens *output){
    _Static_assert(N_TOKEN_TYPE==17, "Unhandled Tokens in tokenize()");
    char current;
    while ((current = peek())) {
        if (isspace(current)) {
            advance();
            continue;
        } 
        else if (isdigit(current)) {
            char digit_c;
            String number = {0};
            while (!isspace(digit_c=peek())) {
                if (isdigit(digit_c)) {
                    vec_append(number, digit_c);
                    advance();
                    continue;
                } else if (is_op(digit_c) || is_digit_delim(digit_c) || digit_c=='.') {
                    break;
                } else {
                    printf("[Error] Invalid decimal input, expected digit found '%c'\n", digit_c);
                    return false;
                }
            }
            if (peek() == '.') {
                vec_append(number,'.');
                advance();
                while (!isspace(digit_c=peek())) {
                    if (isdigit(digit_c)) {
                        vec_append(number, digit_c);
                        advance();
                        continue;
                    } else if (is_op(digit_c) || is_digit_delim(digit_c)) { 
                        break;
                    } else {
                        printf("[Error] Invalid decimal input, expected digit found '%c'\n", digit_c);
                        return false;
                    }
                }
            }
            double out = strtod(number.items, NULL);
            Token token;
            token.type=NUMBER;
            token.as.number =out;
            vec_append((*output), token);
            continue;
        }
        else if (isalpha(current)) { 
            char id_c; 
            String identifier = {0};
            int i = 0;
            while ((isalnum(id_c=peek())) && i<sizeof(identifier)-1) {
                vec_append(identifier, id_c);	
                advance();
            }
            vec_append(identifier, '\0');

            Token token;
            if (strcmp(identifier.items, "fn") == 0) {
                token.type = FN;
                token.as.string = identifier.items;
            } else {
                token.type = IDENTIFIER;
                token.as.string = identifier.items;
            }

            vec_append((*output), token);
            continue;
        }
        else {
            Token token;
            switch (current) {
                case '+':
                    token.type=PLUS;
                    break;
                case '-':
                    token.type=MINUS;
                    break;
                case '*':
                    token.type=STAR;
                    break;
                case '/':
                    token.type=SLASH;
                    break;
                case '(':
                    token.type=LPAREN;
                    break;
                case ')':
                    token.type=RPAREN;
                    break;
                case ',':
                    token.type=COMMA;
                    break;
                case '=':
                    token.type=EQUAL;
                    break;
                case ';':
                    token.type=SEMICOLON;
                    break;
                case '{':
                    token.type=LCURLY;
                    break;
                case '}':
                    token.type=RCURLY;
                    break;
                case '?':
                    token.type=QUESTION;
                    break;
                case ':':
                    token.type=COLON;
                    break;
                case '.': ;
                    // printf("[Error] Float must have number before decimal \n");
                    String number = {0};
                    char digit_c;
                    vec_append(number,'.');
                    advance();
                    while (!isspace(digit_c=peek())) {
                        if (isdigit(digit_c)) {
                            vec_append(number, digit_c);
                            advance();
                            continue;
                        } else if (is_op(digit_c) || digit_c==')' || digit_c==',' || digit_c==';') {
                            break;
                        } else {
                            printf("[Error] Invalid decimal input, expected digit found '%c'\n", digit_c);
                            return false;
                        }
                    }
                    char *remainder;	
                    double out = strtod(number.items, &remainder);
                    if (remainder[0]=='.') {
                        printf("[Error] Invalid decimal input, expected digit after '.'\n");
                        return false;
                    }
                    Token token;
                    token.type=NUMBER;
                    token.as.number =out;
                    vec_append((*output), token);
                    continue;
                default:
                    printf("[Error] Unknown input '%c'\n", current);
                    return false;
            }
            token.as.operator=current;
            vec_append((*output), token);
        }
        advance();
    }
    Token end;
    end.type = ENDSTREAM;
    vec_append((*output), end);
    return true;
}

bool token_to_str(Token token, char* str) {
    switch (token.type) {
        case PLUS:
        case MINUS:
        case STAR:
        case SLASH:
            str[0] = token.as.operator; 
            return true;
        case NUMBER:
            printf("%f", token.as.number);
            str[0] = token.as.number+'0'; 
            return true;
        case ENDSTREAM:
            str[0]='#';
            return true;
        default:
            fprintf(stderr,"Unhandled token type in token_to_str");
            return false;
    }
}

void print_token(Token token) {
    switch (token.type) {
        case PLUS:
        case MINUS:
        case STAR:
        case SLASH:
            printf("%c", token.as.operator);
            break;
        case NUMBER:;
            double number = token.as.number;
            if ((int)number == number) {
                printf("%d", (int)token.as.number);
            } else {
                printf("%f", token.as.number);
            }
            break;
        case ENDSTREAM:
            printf("<EOF>");
            break;
        case LPAREN:
            printf("(");
            break;
        case RPAREN:
            printf(")");
            break;
        case IDENTIFIER:
            printf("%s", token.as.string);
            break;
        case COMMA:
            printf(",");
            break;
        case FN:
            printf("<FN>");
            break;
        case LCURLY:
            printf("{");
            break;
        case RCURLY:
            printf("}");
            break;
        case SEMICOLON:
            printf(";");
            break;
        case EQUAL:
            printf("=");
            break;
        default:
            fprintf(stderr,"Unhandled token type in print_token enum %d\n", token.type);
    }
}

void print_tokens(Tokens tokens) {
    for (int i=0; i<tokens.count; i++) {
        print_token(tokens.items[i]);
    }
}

typedef enum {
    BINARY_EXPR,
    UNARY_EXPR,
    LITERAL_EXPR,
    PAREN_EXPR,
    CALL_EXPR,
    VAR_EXPR,
    ASSIGN_EXPR,
    FN_EXPR,
    TERNARY_EXPR
} ExpressionType;

typedef struct Expression Expression;

typedef struct {
    size_t pos;
    size_t count;
    size_t capacity;
    Expression **items;
} Expressions; // used for dynamic amount of expressions e.g in fn args

typedef struct {
    Expression *condition;
    Expression *if_true;
    Expression *if_false;
} TernaryExpr;

typedef struct {
    Token identifier;
    Expression *r_value;
} AssignExpr;

typedef struct {
    Expression *left;
    Token operator;
    Expression *right;
} BinaryExpr;

typedef struct {
    Token operator;
    Expression *right;
} UnaryExpr;

typedef struct {
    Token value;
} LiteralExpr;

typedef struct {
    Expression *expr;
} ParenExpr;

typedef struct {
    Token identifier;
    Expression **args; 
    size_t n_args;
} CallExpr;


typedef struct {
    Token identifier;
    Tokens *args; 
    Expressions *body;  
} FnExpr;

typedef struct {
    Token identifier;
} VarExpr;


struct Expression {
    ExpressionType type;
    union {
        BinaryExpr binaryexpr;
        UnaryExpr unaryexpr;
        LiteralExpr literalexpr;
        ParenExpr parenexpr;
        CallExpr callexpr;
        FnExpr function;
        AssignExpr assignment;
        TernaryExpr ternary;
        VarExpr variable;
    } as;
};

//ExpressionResult expr(); 

void free_expression(Expression *expr) {
    if (expr == NULL) return;

    switch (expr->type) {
        case BINARY_EXPR:
            free_expression(expr->as.binaryexpr.left);
            free_expression(expr->as.binaryexpr.right);
            break;
        case UNARY_EXPR:
            free_expression(expr->as.unaryexpr.right);
            break;
        case LITERAL_EXPR:
            break;
        case PAREN_EXPR:
            free_expression(expr->as.parenexpr.expr);
            break;
        case CALL_EXPR: ;
            size_t n_args = expr->as.callexpr.n_args;
            for (size_t i=0; i<n_args; i++) {
                free_expression(expr->as.callexpr.args[i]);
            }
            free(expr->as.callexpr.identifier.as.string);
            free(expr->as.callexpr.args);
            break;
        case ASSIGN_EXPR:
            free_expression(expr->as.assignment.r_value);	
            break;
        case VAR_EXPR: 
            if (expr->as.variable.identifier.as.string != NULL) {
                free(expr->as.variable.identifier.as.string);
            }
            break; 
        case TERNARY_EXPR:
            free_expression(expr->as.ternary.condition);
            free_expression(expr->as.ternary.if_false);
            free_expression(expr->as.ternary.if_true);
            break;
        case FN_EXPR:
            break;  // TODO: write free fn
        default:
            printf("Unhandled enum in free_expression\n");
            break;
    }
    free(expr);
}

typedef struct {
    Error error;
    Expression *expr;
} ExpressionResult;

ExpressionResult create_ternary_expr(Expression* condition, Expression* if_true, Expression *if_false) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=TERNARY_EXPR;
    expr->as.ternary.condition = condition;
    expr->as.ternary.if_false = if_false ;
    expr->as.ternary.if_true= if_true;

    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_paren_expr(Expression* middle) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=PAREN_EXPR;
    expr->as.parenexpr.expr = middle;

    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_binary_expr(Expression* left, Token op, Expression *right) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=BINARY_EXPR;
    expr->as.binaryexpr.left=left;
    expr->as.binaryexpr.right=right;
    expr->as.binaryexpr.operator=op;
    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_literal_expr(Token value) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type = LITERAL_EXPR;
    expr->as.literalexpr.value = value;

    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_unary_expr(Token op,Expression* right) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=UNARY_EXPR;
    expr->as.unaryexpr.right=right;
    expr->as.unaryexpr.operator=op;
    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_assign_expr(Token identifier, Expression *r_value) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=ASSIGN_EXPR;
    expr->as.assignment.identifier=identifier;
    expr->as.assignment.r_value=r_value;
    return (ExpressionResult){SUCCESS,expr};
}

ExpressionResult create_var_expr(Token identifier) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=VAR_EXPR;
    expr->as.variable.identifier=identifier;
    return (ExpressionResult){SUCCESS,expr};
}

ExpressionResult create_fn_expr(Token identifier, Tokens *args, Expressions *body) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=FN_EXPR;
    expr->as.function.identifier=identifier;
    expr->as.function.args=args; 
    expr->as.function.body=body;
    return (ExpressionResult){SUCCESS, expr};
}

ExpressionResult create_call_expr(Token identifier, Expression **args, size_t n_args) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return (ExpressionResult){CREATE_EXPR_ERR, NULL};
    }

    expr->type=CALL_EXPR;
    expr->as.callexpr.args=args; 
    expr->as.callexpr.n_args=n_args;
    expr->as.callexpr.identifier=identifier;
    return (ExpressionResult){SUCCESS, expr};
}

Token peekTokens() {
    return tokens.items[tokens.pos];	
}

bool endOfTokens() {
    return peekTokens().type == ENDSTREAM;
}

Token advanceTokens() {
    return tokens.items[tokens.pos++];
}

bool check(TokenType type) {
    return peekTokens().type == type;
}

bool match(TokenType type) {
    if (check(type)) {
        advanceTokens();
        return true;
    }
    return false;
}

Token previous() {
    if (tokens.pos==0) {fprintf(stderr,"Error: previous() should never be called when pos is 0"); exit(1);};
    return tokens.items[tokens.pos-1];
}

void error(Token token, char *message) {
    printf("[Error] at token '");	
    // TODO: use tostring fn for token
    print_token(token);
    printf("', ");	
    printf("%s\n", message);	
}

bool consume_token(TokenType type, char *message) {
    if (check(type)) {advanceTokens(); return true;};
    error(peekTokens(), message);
    return false;
}

 
Token copy_token(Token t) {
    Token new_t = t;
    if ((t.type == IDENTIFIER || t.type == FN) && t.as.string != NULL) {
        new_t.as.string = strdup(t.as.string);
    }
    return new_t;
}

Expression* copy_expression(Expression *expr) {
    if (expr == NULL) return NULL;

    Expression *new_expr = malloc(sizeof(Expression));
    if (new_expr == NULL) {
        fprintf(stderr, "Allocation failed in copy_expression\n");
        exit(1);
    }
    
    new_expr->type = expr->type;

    switch (expr->type) {
        case LITERAL_EXPR:
            new_expr->as.literalexpr.value = expr->as.literalexpr.value;
            break;
        case VAR_EXPR:
            new_expr->as.variable.identifier = copy_token(expr->as.variable.identifier);
            break;
        case BINARY_EXPR:
            new_expr->as.binaryexpr.left = copy_expression(expr->as.binaryexpr.left);
            new_expr->as.binaryexpr.right = copy_expression(expr->as.binaryexpr.right);
            new_expr->as.binaryexpr.operator = copy_token(expr->as.binaryexpr.operator);
            break;
        case UNARY_EXPR:
            new_expr->as.unaryexpr.right = copy_expression(expr->as.unaryexpr.right);
            new_expr->as.unaryexpr.operator = copy_token(expr->as.unaryexpr.operator);
            break;
        case PAREN_EXPR:
            new_expr->as.parenexpr.expr = copy_expression(expr->as.parenexpr.expr);
            break;
        case ASSIGN_EXPR:
            new_expr->as.assignment.identifier = copy_token(expr->as.assignment.identifier);
            new_expr->as.assignment.r_value = copy_expression(expr->as.assignment.r_value);
            break;
        case CALL_EXPR:
            new_expr->as.callexpr.identifier = copy_token(expr->as.callexpr.identifier);
            new_expr->as.callexpr.n_args = expr->as.callexpr.n_args;
            new_expr->as.callexpr.args = malloc(sizeof(Expression*) * expr->as.callexpr.n_args);
            for (size_t i = 0; i < expr->as.callexpr.n_args; i++) {
                new_expr->as.callexpr.args[i] = copy_expression(expr->as.callexpr.args[i]);
            }
            break;
        case TERNARY_EXPR:
            new_expr->as.ternary.condition = copy_expression(expr->as.ternary.condition);
            new_expr->as.ternary.if_true= copy_expression(expr->as.ternary.if_true);
            new_expr->as.ternary.if_false= copy_expression(expr->as.ternary.if_false);
            break;
        case FN_EXPR:
            new_expr->as.function.identifier = copy_token(expr->as.function.identifier);
            
            new_expr->as.function.args = malloc(sizeof(Tokens));
            new_expr->as.function.args->count = expr->as.function.args->count;
            new_expr->as.function.args->capacity = expr->as.function.args->capacity;
            new_expr->as.function.args->pos = 0;
            new_expr->as.function.args->items = malloc(sizeof(Token) * expr->as.function.args->capacity);
            for (size_t i = 0; i < expr->as.function.args->count; i++) {
                new_expr->as.function.args->items[i] = copy_token(expr->as.function.args->items[i]);
            }

            new_expr->as.function.body = malloc(sizeof(Expressions));
            new_expr->as.function.body->count = expr->as.function.body->count;
            new_expr->as.function.body->capacity = expr->as.function.body->capacity;
            new_expr->as.function.body->pos = 0;
            new_expr->as.function.body->items = malloc(sizeof(Expression*) * expr->as.function.body->capacity);
            for (size_t i = 0; i < expr->as.function.body->count; i++) {
                new_expr->as.function.body->items[i] = copy_expression(expr->as.function.body->items[i]);
            }
            break;
    }
    return new_expr;
}

ExpressionResult expr(); 

ExpressionResult primary() {
    if (match(NUMBER)) {
        ExpressionResult res = create_literal_expr(previous());
        return res;
    }

    if (match(LPAREN)) {
        ExpressionResult res = expr();
        if (!consume_token(RPAREN, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};

        return res;
    }

    if (match(FN)) {
        if (!consume_token(IDENTIFIER, "Expected function name after 'fn' keyword")) return (ExpressionResult){PARSE_ERR, NULL};
        Token identifier = previous();
        if (!consume_token(LPAREN, "Expected '(' after function identifier in fn declaration")) return (ExpressionResult){PARSE_ERR, NULL};

        Tokens *arg_ids = malloc(sizeof(Tokens));
        *arg_ids = (Tokens){0};
        Expressions *body = malloc(sizeof(Expressions));
        *body = (Expressions){0};

        if (match(IDENTIFIER)) vec_append((*arg_ids), previous()); 
        while (match(COMMA)){
            if (!consume_token(IDENTIFIER, "Expected argument in function declaration")) return (ExpressionResult){PARSE_ERR, NULL};
            vec_append((*arg_ids), previous()); 
        };


        if (!consume_token(RPAREN, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};

        if (!consume_token(LCURLY, "Expect '{' to open function definition body'")) return (ExpressionResult){PARSE_ERR, NULL};
        do {
            ExpressionResult res = expr();
            if (res.error!=SUCCESS) return (ExpressionResult){PARSE_ERR, NULL};
            vec_append((*body), res.expr);
        } while (match(SEMICOLON));

        if (!consume_token(RCURLY, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};
        return create_fn_expr(identifier, arg_ids, body);
    }

    if (match(IDENTIFIER)) {
        Token identifier = previous();
        if (match(LPAREN)) {
            //	 function call
            if (match(RPAREN)) {
                // empty function args
                return create_call_expr(identifier, NULL, 0);
            }

            Expressions args = {0};
            do {
                ExpressionResult res = expr();
                if (res.error!=SUCCESS) return res; // if one of the args fails then we stop parsing
                vec_append(args, res.expr); 
            } while (match(COMMA));

            if (!consume_token(RPAREN, "Expect closing ')' to close function call.")) return (ExpressionResult){PARSE_ERR,NULL};

            return create_call_expr(identifier, args.items, args.count);
        } else if (match(EQUAL)) {
            // assignment expr L_VAL = R_VAL, returns R_VAL
            ExpressionResult res = expr();
            if (res.error!=SUCCESS) return res;
            return create_assign_expr(identifier, res.expr);
        } else {
            return create_var_expr(identifier);
        }
    }

    error(peekTokens(),"Expected expression.");
    return (ExpressionResult){PARSE_ERR, NULL};
}

ExpressionResult unary() {
    if (match(MINUS)) {
        Token op = previous();
        ExpressionResult res = unary();
        if (res.error!=SUCCESS) {
            return res;
        }
        return create_unary_expr(op, res.expr);
    }
    return primary();
}

ExpressionResult factor() {
    ExpressionResult res = unary();
    if (res.error != SUCCESS) return res;

    while (match(SLASH) || match(STAR)) {
        Token op = previous();
        ExpressionResult right = unary();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult term() {
    ExpressionResult res = factor();
    if (res.error != SUCCESS) return res;

    while (match(MINUS) || match(PLUS)) {
        Token op = previous();
        ExpressionResult right = factor();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult ternary() {
    ExpressionResult res = term();
    if (res.error != SUCCESS) return res;

    //if (match(COLON)) return (ExpressionResult){PARSE_ERR,NULL};

    if (match(QUESTION)) {
        ExpressionResult if_true_res = term();
        if (if_true_res.error != SUCCESS) {
            free_expression(if_true_res.expr);
            return if_true_res;
        }

        consume_token(COLON, "Ternary '?' must have corresponding ':'");

        ExpressionResult if_false_res = term();
        if (if_false_res.error != SUCCESS) {
            free_expression(if_false_res.expr);
            return if_false_res;
        }

        return create_ternary_expr(res.expr, if_true_res.expr, if_false_res.expr);
    } else if (match(COLON)) {
        error(peekTokens(), "Stray ':' found without a preceding '?'.");
        free_expression(res.expr);
        return (ExpressionResult){PARSE_ERR, NULL};
    }
    return res;
}

ExpressionResult expr() {
    return ternary();
}

ExpressionResult parse() {
    _Static_assert(N_TOKEN_TYPE==17, "Unhandled Tokens in parse()");
    ExpressionResult res = expr();
    if (res.error!=SUCCESS) return (ExpressionResult){PARSE_ERR, NULL};

    if (match(RPAREN)) { 
        error(peekTokens(), "Unmatched ')'");
        return (ExpressionResult){PARSE_ERR, NULL};
    }
    return res;
}


bool AST_printer(Expression *expr, char *buf, size_t buf_size) {
    if (expr==NULL)  return false ;
    if (strlen(buf) >= buf_size - 1) {
        fprintf(stderr, "Buffer overflow AST_printer()");
        return false;
    }
    switch (expr->type) {
        case BINARY_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Binary(");
            AST_printer(expr->as.binaryexpr.left, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf)," ,'%c', ",expr->as.binaryexpr.operator.as.operator);
            AST_printer(expr->as.binaryexpr.right, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case UNARY_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Unary('%c', ",expr->as.unaryexpr.operator.as.operator);
            AST_printer(expr->as.unaryexpr.right, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case LITERAL_EXPR:;
            double number = expr->as.literalexpr.value.as.number;
            if ((int)number==number) {
                snprintf(buf + strlen(buf),buf_size-strlen(buf),"%d",(int)number);
                return true;
            }
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"%f",number);
            return true;
        case CALL_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"%s(",expr->as.callexpr.identifier.as.string);
            size_t i;
            size_t n_args = expr->as.callexpr.n_args;
            for (i=0; i<n_args; i++) {
                AST_printer(expr->as.callexpr.args[i], buf, buf_size);
                if (i!=n_args-1) snprintf(buf + strlen(buf),buf_size-strlen(buf),",");
            }
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case VAR_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"%s",expr->as.variable.identifier.as.string);
            return true;
        case ASSIGN_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Assign(%s,",expr->as.assignment.identifier.as.string);
            AST_printer(expr->as.assignment.r_value, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case TERNARY_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Ternary(");
            AST_printer(expr->as.ternary.condition, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf)," ? ");
            AST_printer(expr->as.ternary.if_true, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf)," : ");
            AST_printer(expr->as.ternary.if_false, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case FN_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Fn(%s)",expr->as.function.identifier.as.string);
            return true;
        default:
            fprintf(stderr,"Unhandled token type in AST_printer; Enum: %d\n", expr->type);
            return false;
    }
}

typedef enum {
    INT, // INT is 0 = default
    FLOAT,
} Repr;

typedef struct {
    bool status;
    double result;	
} EvalResult; 

// entry for hashmap
struct var_entry {
    char *identifier;
    double value;
};

struct fn_entry { // fn entry in hashmap
    char *identifier;
    // Expressions *value;
    FnExpr *value;
};

typedef struct {
    char *identifier;
    Expression *ptr;
} fn_entry;

uint64_t var_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct var_entry *entry = item;
    return hashmap_sip(entry->identifier, strlen(entry->identifier), seed0, seed1);
}

int var_compare(const void *a, const void *b, void *udata) {
    const struct var_entry *ea = a;
    const struct var_entry *eb = b;
    return strcmp(ea->identifier, eb->identifier);
}

void var_free(void *item) {
    struct var_entry *entry = item;
    free(entry->identifier); 
}

uint64_t fn_hash(const void *item, uint64_t seed0, uint64_t seed1) {
    const struct fn_entry *entry = item;
    return hashmap_sip(entry->identifier, strlen(entry->identifier), seed0, seed1);
}

int fn_compare(const void *a, const void *b, void *udata) {
    const struct fn_entry *ea = a;
    const struct fn_entry *eb = b;
    return strcmp(ea->identifier, eb->identifier);
}

void fn_free(void *item) {
    struct fn_entry *entry = item;
    free(entry->identifier); 
    free(entry->value->body->items); 
    free(entry->value); 
}

struct hashmap *env_map = NULL; // declared global variables
struct hashmap *fn_map = NULL; //  declared global fns 

#define MAKE_ERROR() (EvalResult){.status = false, .result=0}

#define Literal_Expr(n) (Expression){ \
    .type = LITERAL_EXPR, \
    .as = { \
        .literalexpr = { \
            .value = { \
                .type = NUMBER, \
                .as = { \
                    .number = (double)(n) \
                } \
            } \
        } \
    } \
}

#define eval(expr) eval_with_env((expr), environment);

EvalResult eval_with_env(Expression *expr, struct hashmap *environment) { 
// TODO: eval should take a fn_env too, for functions defined in a function theoretically
    if (expr == NULL) return (EvalResult) {false, 0};
    switch (expr->type) {
        case BINARY_EXPR: ;
            EvalResult lhs_r = eval(expr->as.binaryexpr.left);
            EvalResult rhs_r = eval(expr->as.binaryexpr.right);
            if (!lhs_r.status || !rhs_r.status) return MAKE_ERROR();
            double lhs = lhs_r.result;
            double rhs = rhs_r.result;
            double out;
            switch (expr->as.binaryexpr.operator.type) {
                case PLUS:
                    out = lhs+rhs;
                    break;
                case MINUS:
                    out = lhs-rhs;
                    break;
                case SLASH:
                    if (rhs==0) {
                        error(expr->as.binaryexpr.operator, "Division by 0");
                        return (EvalResult){false,0};
                    }
                    out = lhs/rhs;
                    break;
                case STAR:
                    out = lhs*rhs;
                    break;
                default:
                    fprintf(stderr, "Not an operator.");
                    return (EvalResult){false,0};
            }
            return (EvalResult){true, out};
        case UNARY_EXPR: ;
            EvalResult right_r = eval(expr->as.unaryexpr.right);
            double right = right_r.result;
            switch (expr->as.unaryexpr.operator.type) {
                case MINUS:
                    out = -right;
                    break;
                default:
                    fprintf(stderr, "Not a valid unary operator.");
                    return MAKE_ERROR();
            }
            return (EvalResult){true,out};
            break;
        case LITERAL_EXPR:
            return (EvalResult){true,expr->as.literalexpr.value.as.number};	
        case CALL_EXPR: ;
            Expression **args = expr->as.callexpr.args;	
            size_t n_args = expr->as.callexpr.n_args;
            char *identifier = expr->as.callexpr.identifier.as.string;
            if (strcmp("sin", identifier) == 0) {
                if (n_args != 1) {
                    error(expr->as.callexpr.identifier, "sin(x) takes one argument.");
                    return MAKE_ERROR();
                };

                EvalResult args_res = eval(args[0]);
                if (!args_res.status) return MAKE_ERROR();
                return (EvalResult){true, sin(args_res.result)};
            }
            
            // user defn function
            struct fn_entry fnkey = { .identifier = identifier }; 
            const struct fn_entry *found_fn = hashmap_get(fn_map, &fnkey);
            if (!found_fn) {
                error(expr->as.function.identifier, "undefined function");
                return MAKE_ERROR();
            }
            Expression **fn_body = found_fn->value->body->items;
            size_t num_exprs = found_fn->value->body->count;
            Tokens *fn_args = found_fn->value->args;
            size_t expected_n_args = fn_args->count;

            if (n_args != expected_n_args) {
                error(expr->as.callexpr.identifier, "Wrong number of args to function call.");
                return MAKE_ERROR();
            };

            struct hashmap *local_var_env = hashmap_new(sizeof(struct var_entry), 0, 0, 0,var_hash, var_compare, var_free, NULL);
            for (int i=0; i<n_args; i++) {
                EvalResult arg_res = eval(args[i]);
                if (!arg_res.status) return arg_res;

                struct var_entry entry = {
                    .identifier=strdup(fn_args->items[i].as.string),
                    .value=arg_res.result,
                };
                hashmap_set(local_var_env, &entry);
            }

            for (int i=0; i<num_exprs-1; i++) {
                // loop over all but last expr
                EvalResult res = eval_with_env(fn_body[i], local_var_env);
                if (!res.status) {
                    error(expr->as.callexpr.identifier, "Error in function body.");
                    return res;
                }
            }

            Expression *return_expr = num_exprs>0 ? fn_body[num_exprs-1] : &Literal_Expr(0);
            EvalResult return_val = eval_with_env(return_expr, local_var_env);
            return return_val;
        case TERNARY_EXPR: ;
            EvalResult condition_res = eval(expr->as.ternary.condition);
            if (!condition_res.status) return MAKE_ERROR();
            if (condition_res.result == 0) {
                // false
                return eval(expr->as.ternary.if_false); 
            }
            return eval(expr->as.ternary.if_true); 
        case ASSIGN_EXPR: ;
            EvalResult right_res = eval(expr->as.assignment.r_value);	
            if (!right_res.status) return MAKE_ERROR();
            struct var_entry entry = {
                .identifier=strdup(expr->as.assignment.identifier.as.string),
                .value=right_res.result			
            };
            hashmap_set(environment, &entry); // hashmap copies entry
            return right_res;
        case VAR_EXPR: ;
            struct var_entry key = { .identifier = expr->as.variable.identifier.as.string};
            const struct var_entry *found = hashmap_get(environment, &key);
            if (found) return (EvalResult){true, found->value};
            else {
                error(expr->as.variable.identifier, "Undefined variable.");
                return (EvalResult){false, 0};
            }
        case FN_EXPR: ;
            Expression *fn_expr = copy_expression(expr);

            struct fn_entry fn_entry = {
                .value=&fn_expr->as.function,
                .identifier = strdup(fn_expr->as.function.identifier.as.string),
            };
            hashmap_set(fn_map, &fn_entry);
            return (EvalResult){true, 1}; // TODO: figure out what fn returns, should be return pointer to fn maybe, or the fn identifier
        default:
            fprintf(stderr,"Unhandled token type in eval\n");
            return MAKE_ERROR();
    }
}

int main() {
    char line[256];
    env_map=hashmap_new(sizeof(struct var_entry), 0, 0, 0,var_hash, var_compare, var_free, NULL);
    fn_map=hashmap_new(sizeof(struct fn_entry), 0, 0, 0,fn_hash, fn_compare, fn_free, NULL);
    while (1) {
        printf("Calc > "); 
        if (fgets(line, sizeof(line), stdin)) {
            reset_program(&p, line);
            free_tokens(&tokens); 
            if (!tokenize(&tokens)) {
                continue;
            }

            ExpressionResult res = parse();
            switch (res.error) {
                case SUCCESS: ;
                    if (res.expr==NULL) {
                        fprintf(stderr, "Unreachable: res.expr==NULL but res.error=SUCCESS.");
                        continue; // should not be possible
                    }
                    Expression *expr = res.expr;
                    
                    char buf[1024] = ""; 
                    AST_printer(expr, buf, sizeof(buf));
                    printf("AST: %s\n", buf);

                    EvalResult result = eval_with_env(expr, env_map);

                    if (!result.status) {
                        continue;
                    }

                    double val = result.result;
                    struct var_entry ans = {
                        .identifier="Ans",
                        .value=val,
                    };
                    hashmap_set(env_map, &ans); // special 'ans' variable that contains previous calculation's answer

                    if ((int)val==val) {
                        printf("%d\n", (int)val); 
                    }else {
                        printf("%f\n", val); 
                    }
                    free_expression(expr);
                    break;	
                case CREATE_EXPR_ERR:
                    fprintf(stderr,"An unexpected error occured (CREATE_EXPR_ERR)");
                    break;
                case PARSE_ERR:
                    continue;
            }
        }
    }
    hashmap_free(env_map);
    return 0;
}
