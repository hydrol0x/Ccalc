#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
//#include "hashmap.h"

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
	COMMA,
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

// uint64_t user_hash(const void *item, uint64_t seed0, uint64_t seed1) {
//     const struct user *user = item;
//     return hashmap_sip(user->name, strlen(user->name), seed0, seed1);
// }
// struct hashmap *map = hashmap_new(sizeof(char *), 0, 0, 0, ,)

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
				} else if (is_op(digit_c) || digit_c==')' || digit_c==',') {
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
				case ',':
						token.type=COMMA;
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
	CALL_EXPR,
} ExpressionType;

typedef struct Expression Expression;

typedef struct {
	size_t pos;
	size_t count;
	size_t capacity;
	Expression **items;
} Expressions; // used for dynamic amount of expressions e.g in fn args

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

struct Expression {
	ExpressionType type;
	union {
		BinaryExpr binaryexpr;
		UnaryExpr unaryexpr;
		LiteralExpr literalexpr;
		ParenExpr parenexpr;
		CallExpr callexpr;
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
			free(expr->as.callexpr.identifier.as.identifier);
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

  if (match(IDENTIFIER)) {
	  Token identifier = previous();
	  if (match(LPAREN)) {
		// function call
		if (match(RPAREN)) {
		  printf("fn empty\n");
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
		case UNARY_EXPR:
			snprintf(buf + strlen(buf),buf_size-strlen(buf),"Unary('%c', ",expr->as.unaryexpr.operator.as.operator);
			AST_printer(expr->as.unaryexpr.right, buf, buf_size);
			snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
			return true;
		case LITERAL_EXPR:
			snprintf(buf + strlen(buf),buf_size-strlen(buf),"%d",expr->as.literalexpr.value.as.integer);
			return true;
		case CALL_EXPR:
			snprintf(buf + strlen(buf),buf_size-strlen(buf),"%s(",expr->as.callexpr.identifier.as.identifier);
			size_t i;
			size_t n_args = expr->as.callexpr.n_args;
			for (i=0; i<n_args; i++) {
				AST_printer(expr->as.callexpr.args[i], buf, buf_size);
				if (i!=n_args-1) snprintf(buf + strlen(buf),buf_size-strlen(buf),",");
			}
			snprintf(buf + strlen(buf),buf_size-strlen(buf),")");
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

#define MAKE_ERROR() (EvalResult){.status = false, .result=0}

EvalResult eval(Expression *expr) {
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
			return (EvalResult){true,expr->as.literalexpr.value.as.integer};	
		case CALL_EXPR: ;
			Expression **args = expr->as.callexpr.args;	
			size_t n_args = expr->as.callexpr.n_args;
			char *identifier = expr->as.callexpr.identifier.as.identifier;
			if (strcmp("sin", identifier) == 0) {
				if (n_args != 1) {
				  error(expr->as.callexpr.identifier, "sin(x) takes one argument.");
				  return MAKE_ERROR();
				};
				// if (val.result->type != LITERAL_EXPR || args[0]->as.literalexpr.value.type != INTEGER) {
				//   error(expr->as.callexpr.identifier, "expected integer argument");
				//   return MAKE_ERROR();
			  	// }
				EvalResult args_res = eval(args[0]);
				if (!args_res.status) return MAKE_ERROR();
				return (EvalResult){true, sin(args_res.result)};
			}
		default:
				fprintf(stderr,"Unhandled token type in eval\n");
				return MAKE_ERROR();
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
				  printf("AST: %s\n", buf);
				  EvalResult result = eval(expr);

				  if (!result.status) {
					  continue;
				  }

				  double val = result.result;
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
    return 0;
}
