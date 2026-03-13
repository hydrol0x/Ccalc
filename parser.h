#ifndef PARSER_H
#define PARSER_H

#include <stddef.h>
#include <stdbool.h>
#include "tokenizer.h"

typedef enum {
    SUCCESS,
    PARSE_ERR,
    CREATE_EXPR_ERR,
} Error;

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
} Expressions;

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

typedef struct {
    Error error;
    Expression *expr;
} ExpressionResult;

ExpressionResult parse();

void free_expression(Expression *expr);
Token copy_token(Token t);
Expression* copy_expression(Expression *expr);

ExpressionResult create_ternary_expr(Expression* condition, Expression* if_true, Expression *if_false);
ExpressionResult create_paren_expr(Expression* middle);
ExpressionResult create_binary_expr(Expression* left, Token op, Expression *right);
ExpressionResult create_literal_expr(Token value);
ExpressionResult create_unary_expr(Token op, Expression* right);
ExpressionResult create_assign_expr(Token identifier, Expression *r_value);
ExpressionResult create_var_expr(Token identifier);
ExpressionResult create_fn_expr(Token identifier, Tokens *args, Expressions *body);
ExpressionResult create_call_expr(Token identifier, Expression **args, size_t n_args);

bool AST_printer(Expression *expr, char *buf, size_t buf_size);

#endif /* PARSER_H */
