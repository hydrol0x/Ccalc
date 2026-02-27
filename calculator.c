#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

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
	INTEGER,
	IDENTIFIER,
	ENDSTREAM,
} TokenType;

typedef enum {
	SUCCESS,
	PARSE_ERR,
	CREATE_EXPR_ERR,
} Error;

typedef struct {
	TokenType type;
	union { char operator;
					int  integer; 
					char *identifier;
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

// typedef struct {
// 	Tokens tokens;
// 	int pos;
// } TokenStream;
// TokenStream token_stream;
// 
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

bool tokenize(Tokens *output){
	char current;
	while ((current = peek())) {
		if (isspace(current)) {
			advance();
			continue;
		} 
		else if (isdigit(current)) {
			char digit_c;
			int integer=0;
			while (!isspace(digit_c=peek())) {
				if (isdigit(digit_c)) {
						integer*=10;
						integer+=ctod(digit_c);
						advance();
						continue;
				} else if (is_op(digit_c)) {
						break;
				} else {
					printf("[Error] Invalid decimal input, expected digit found '%c'\n", digit_c);
					return false;
				}
			}
			Token token;
			token.type=INTEGER;
			token.as.integer=integer;
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

			Token token = {.type = IDENTIFIER, .as.identifier = identifier.items};
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
		case INTEGER:
			printf("%d", token.as.integer);
			str[0] = token.as.integer+'0'; 
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
		case INTEGER:
			printf("%d", token.as.integer);
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
			printf("%s", token.as.identifier);
			break;
		default:
			fprintf(stderr,"Unhandled token type in print_token\n");
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
} ExpressionType;

typedef struct Expression Expression;


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



struct Expression {
	ExpressionType type;
	union {
		BinaryExpr binaryexpr;
		UnaryExpr unaryexpr;
		LiteralExpr literalexpr;
		ParenExpr parenexpr;
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
    }
    free(expr);
}

typedef struct {
	Error error;
	Expression *expr;
} ExpressionResult;

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

ExpressionResult expr(); 

ExpressionResult primary() {
 if (match(INTEGER)) {
	 ExpressionResult res = create_literal_expr(previous());
	 return res;
 }

 if (match(LPAREN)) {
	 ExpressionResult res = expr();
	 if (!consume_token(RPAREN, "Expect closing ')'.")) return (ExpressionResult){PARSE_ERR, NULL};

	 return res;
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

ExpressionResult expr() {
 return term();
}
ExpressionResult parse() {
return expr();
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
			break;
		case UNARY_EXPR:
			snprintf(buf + strlen(buf),buf_size-strlen(buf),"Unary('%c', ",expr->as.unaryexpr.operator.as.operator);
			AST_printer(expr->as.unaryexpr.right, buf, buf_size);
			snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
			return true;
			break;
		case LITERAL_EXPR:
			snprintf(buf + strlen(buf),buf_size-strlen(buf),"%d",expr->as.literalexpr.value.as.integer);
			return true;
		default:
				fprintf(stderr,"Unhandled token type in AST_printer");
				return false;
	}
}

typedef struct {
	bool status;
	int result;
} EvalResult; 

EvalResult eval(Expression *expr) {
	if (expr == NULL) return (EvalResult) {false, 0};
	switch (expr->type) {
		case BINARY_EXPR: ;
			EvalResult lhs_r = eval(expr->as.binaryexpr.left);
			EvalResult rhs_r = eval(expr->as.binaryexpr.right);
			if (!lhs_r.status || !rhs_r.status) return (EvalResult){false, 0};
			int lhs = lhs_r.result;
			int rhs = rhs_r.result;
			int out;
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
			int right = right_r.result;
			switch (expr->as.unaryexpr.operator.type) {
				case MINUS:
					out = -right;
					break;
				default:
					fprintf(stderr, "Not a valid unary operator.");
					return (EvalResult){false,0};
			}
			return (EvalResult){true,out};
			break;
		case LITERAL_EXPR:
			return (EvalResult){true,expr->as.literalexpr.value.as.integer};	
		default:
				fprintf(stderr,"Unhandled token type in eval");
				return (EvalResult){false,0};
	}
}

int main() {
    char line[256];
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
										EvalResult result = eval(expr);

										if (!result.status) {
											continue;
										}

										printf("AST: %s\n%d\n", buf,result.result);
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
    return 0;
}
