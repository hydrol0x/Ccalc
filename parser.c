#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "parser.h"
#include "tokenizer.h"

void free_expression(Expression *expr) { //TODO: use arena allocation instead
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
        case GRAPH_EXPR:
            break; //TODO: write 
        default:
            printf("Unhandled enum in free_expression\n");
            break;
    }
    free(expr);
}

#define CREATE_ERROR (ExpressionResult){CREATE_EXPR_ERR, NULL}
#define PARSE_ERROR (ExpressionResult){PARSE_ERR, NULL}
#define CREATE_SUCCESS(expr) (ExpressionResult){SUCCESS, expr}
ExpressionResult create_ternary_expr(Expression* condition, Expression* if_true, Expression *if_false) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=TERNARY_EXPR;
    expr->as.ternary.condition = condition;
    expr->as.ternary.if_false = if_false ;
    expr->as.ternary.if_true= if_true;

    return CREATE_SUCCESS(expr);
}

ExpressionResult create_paren_expr(Expression* middle) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=PAREN_EXPR;
    expr->as.parenexpr.expr = middle;

    return CREATE_SUCCESS(expr);
}

ExpressionResult create_binary_expr(Expression* left, Token op, Expression *right) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=BINARY_EXPR;
    expr->as.binaryexpr.left=left;
    expr->as.binaryexpr.right=right;
    expr->as.binaryexpr.operator=op;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_literal_expr(Token value) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type = LITERAL_EXPR;
    expr->as.literalexpr.value = value;

    return CREATE_SUCCESS(expr);
}

ExpressionResult create_unary_expr(Token op,Expression* right) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=UNARY_EXPR;
    expr->as.unaryexpr.right=right;
    expr->as.unaryexpr.operator=op;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_assign_expr(Token identifier, Expression *r_value) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=ASSIGN_EXPR;
    expr->as.assignment.identifier=identifier;
    expr->as.assignment.r_value=r_value;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_var_expr(Token identifier) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=VAR_EXPR;
    expr->as.variable.identifier=identifier;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_fn_expr(Token identifier, Tokens *args, Expressions *body) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=FN_EXPR;
    expr->as.function.identifier=identifier;
    expr->as.function.args=args; 
    expr->as.function.body=body;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_call_expr(Token identifier, Expression **args, size_t n_args) {
    Expression *expr = malloc(sizeof(Expression));

    if (expr == NULL) {
        return CREATE_ERROR;
    }

    expr->type=CALL_EXPR;
    expr->as.callexpr.args=args; 
    expr->as.callexpr.n_args=n_args;
    expr->as.callexpr.identifier=identifier;
    return CREATE_SUCCESS(expr);
}

ExpressionResult create_graph_expr(CallExpr function) {
    Expression *expr = malloc(sizeof(Expression));
    if (expr==NULL) return CREATE_ERROR; 

    expr->type=GRAPH_EXPR;
    expr->as.graph.fn_call = function;
    return CREATE_SUCCESS(expr);
}

static Token peek() {
    return tokens.items[tokens.pos];    
}

bool end_of_tokens() {
    return peek().type == ENDSTREAM;
}

Token advance() {
    return tokens.items[tokens.pos++];
}

bool check(TokenType type) {
    return peek().type == type;
}

bool match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

Token previous() {
    if (tokens.pos==0) return tokens.items[tokens.pos];
    return tokens.items[tokens.pos-1];
}

void error(Token token, char *message) {
    printf("[Error] at token '");    
    // TODO: use tostring fn for token
    print_token(token);
    printf("', ");    
    printf("%s\n", message);    
}

static bool consume(TokenType type, char *message) {
    if (check(type)) {advance(); return true;};
    error(previous(), message);
    return false;
}

Token copy_token(Token t) {
    Token new_t = t;
    if ((t.type == IDENTIFIER || t.type == FN) && t.as.string != NULL) {
        new_t.as.string = strdup(t.as.string);
    }
    return new_t;
}

static FnExpr copy_fn(FnExpr src) {
    FnExpr dst;
    dst.identifier = copy_token(src.identifier);
    
    dst.args = malloc(sizeof(Tokens));
    dst.args->count = src.args->count;
    dst.args->capacity = src.args->capacity;
    dst.args->pos = 0;
    dst.args->items = malloc(sizeof(Token) * src.args->capacity);
    for (size_t i = 0; i < src.args->count; i++) {
        dst.args->items[i] = copy_token(src.args->items[i]);
    }

    dst.body = malloc(sizeof(Expressions));
    dst.body->count = src.body->count;
    dst.body->capacity = src.body->capacity;
    dst.body->pos = 0;
    dst.body->items = malloc(sizeof(Expression*) * src.body->capacity);
    for (size_t i = 0; i < src.body->count; i++) {
        dst.body->items[i] = copy_expression(src.body->items[i]);
    }
    
    return dst;
}

static CallExpr copy_call(CallExpr src) {
    CallExpr dst;
    dst.identifier = copy_token(src.identifier);
    dst.n_args = src.n_args;
    dst.args = malloc(sizeof(Expression*) * src.n_args);
    for (size_t i = 0; i < src.n_args; i++) {
        dst.args[i] = copy_expression(src.args[i]);
    }
    return dst;
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
            new_expr->as.callexpr = copy_call(expr->as.callexpr);
            break;
        case TERNARY_EXPR:
            new_expr->as.ternary.condition = copy_expression(expr->as.ternary.condition);
            new_expr->as.ternary.if_true= copy_expression(expr->as.ternary.if_true);
            new_expr->as.ternary.if_false= copy_expression(expr->as.ternary.if_false);
            break;
        case GRAPH_EXPR:
            new_expr->as.graph.fn_call = copy_call(expr->as.graph.fn_call);
            break;
        case FN_EXPR:
            new_expr->as.function = copy_fn(expr->as.function);
            break;
    }
    return new_expr;
}

// Recursive Descenent 
ExpressionResult expr();

ExpressionResult parse_function() {
    if (!consume(IDENTIFIER, "Expected function name after 'fn' keyword")) return (ExpressionResult){PARSE_ERR, NULL};
    Token identifier = previous();
    if (!consume(LPAREN, "Expected '(' after function identifier in fn declaration")) return (ExpressionResult){PARSE_ERR, NULL};

    Tokens *arg_ids = malloc(sizeof(Tokens));
    *arg_ids = (Tokens){0};
    Expressions *body = malloc(sizeof(Expressions));
    *body = (Expressions){0};

    if (match(IDENTIFIER)) vec_append((*arg_ids), previous()); 
    while (match(COMMA)){
        if (!consume(IDENTIFIER, "Expected argument in function declaration")) return (ExpressionResult){PARSE_ERR, NULL};
        vec_append((*arg_ids), previous()); 
    };


    if (!consume(RPAREN, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};

    if (!consume(LCURLY, "Expect '{' to open function definition body'")) return (ExpressionResult){PARSE_ERR, NULL};
    do {
        ExpressionResult res = expr();
        if (res.error!=SUCCESS) return (ExpressionResult){PARSE_ERR, NULL};
        vec_append((*body), res.expr);
    } while (match(SEMICOLON));

    if (!consume(RCURLY, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};
    return create_fn_expr(identifier, arg_ids, body);
}

ExpressionResult parse_call(Token identifier) {
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

    if (!consume(RPAREN, "Expect closing ')' to close function call.")) return (ExpressionResult){PARSE_ERR,NULL};

    return create_call_expr(identifier, args.items, args.count);
}

ExpressionResult primary() {
    if (match(NUMBER)) {
        ExpressionResult res = create_literal_expr(previous());
        return res;
    }

    if (match(LPAREN)) {
        ExpressionResult res = expr();
        if (!consume(RPAREN, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};

        return res;
    }

    if (match(GRAPH)) {
        consume(IDENTIFIER, "Expected function call to follow `graph` keyword.");
        Token ident = previous();
        if (!consume(LPAREN, "Expected `(` after function identifier.")) return PARSE_ERROR;
        ExpressionResult res = parse_call(ident); 
        if (res.error!=SUCCESS) return res;

        return create_graph_expr(res.expr->as.callexpr);
    }

    if (match(FN)) {
        return parse_function();
    }

    if (match(IDENTIFIER)) {
        Token identifier = previous();
        if (match(LPAREN)) {
            //      function call
            return parse_call(identifier);
        } else if (match(EQUAL)) {
            // assignment expr L_VAL = R_VAL, returns R_VAL
            ExpressionResult res = expr();
            if (res.error!=SUCCESS) return res;
            return create_assign_expr(identifier, res.expr);
        } else {
            return create_var_expr(identifier);
        }
    }

    error(previous(),"Expected expression.");
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


ExpressionResult comparison() {
    ExpressionResult res = term();
    if (res.error != SUCCESS) return res;

    while (match(LT) || match(GT) || match(GTEQUAL) || match(LTEQUAL)) {
        Token op = previous();
        ExpressionResult right = term();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult equality() {
    ExpressionResult res = comparison();
    if (res.error != SUCCESS) return res;

    while (match(EQUALEQUAL) || match(NEQ)) {
        Token op = previous();
        ExpressionResult right = comparison();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult and() {
    ExpressionResult res = equality();
    if (res.error != SUCCESS) return res;

    while (match(LOGIC_AND)) {
        Token op = previous();
        ExpressionResult right = equality();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult or() {
    ExpressionResult res = and();
    if (res.error != SUCCESS) return res;

    while (match(LOGIC_OR)) {
        Token op = previous();
        ExpressionResult right = and();
        if (right.error != SUCCESS) {
            free_expression(res.expr); 
            return right;
        }

        res = create_binary_expr(res.expr, op, right.expr);
    }

    return res;
}

ExpressionResult ternary() {
    ExpressionResult res = or();
    if (res.error != SUCCESS) return res;

    //if (match(COLON)) return (ExpressionResult){PARSE_ERR,NULL};

    if (match(QUESTION)) {
        ExpressionResult if_true_res = term();
        if (if_true_res.error != SUCCESS) {
            free_expression(if_true_res.expr);
            return if_true_res;
        }

        consume(COLON, "Ternary '?' must have corresponding ':'");

        ExpressionResult if_false_res = term();
        if (if_false_res.error != SUCCESS) {
            free_expression(if_false_res.expr);
            return if_false_res;
        }

        return create_ternary_expr(res.expr, if_true_res.expr, if_false_res.expr);
    } else if (match(COLON)) {
        error(previous(), "Stray ':' found without a preceding '?'.");
        free_expression(res.expr);
        return (ExpressionResult){PARSE_ERR, NULL};
    }
    return res;
}

ExpressionResult expr() {
    return ternary();
}

ExpressionResult parse() {
    // _Static_assert(N_TOKEN_TYPE==17, "Unhandled Tokens in parse()");
    ExpressionResult res = expr();
    if (res.error!=SUCCESS) return (ExpressionResult){PARSE_ERR, NULL};

    if (match(RPAREN)) { 
        error(previous(), "Unmatched ')'");
        return (ExpressionResult){PARSE_ERR, NULL};
    } else if (match(EQUAL)) {
        error(previous(), "Assignment to invalid identifier");
        return (ExpressionResult){PARSE_ERR, NULL};
    }  else if (!match(ENDSTREAM)) {
        error(previous(), "Unhandled tokens in parser");
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
        case BINARY_EXPR: ;
            char opstr[3] = "";
            str_from(expr->as.binaryexpr.operator, opstr, 3);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Binary(");
            AST_printer(expr->as.binaryexpr.left, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf)," ,'%s', ", opstr);
            AST_printer(expr->as.binaryexpr.right, buf, buf_size);
            snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
            return true;
        case UNARY_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Unary('%c', ",expr->as.unaryexpr.operator.as.op);
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
        case GRAPH_EXPR:
            snprintf(buf + strlen(buf),buf_size-strlen(buf),"Graph()");
            return true;
        default:
            fprintf(stderr,"Unhandled token type in AST_printer; Enum: %d\n", expr->type);
            return false;
    }
}
