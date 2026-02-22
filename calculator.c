#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>
#include <time.h>

typedef enum {
	PLUS,
	MINUS,
	STAR,
	SLASH,
	INTEGER,
	ENDSTREAM,
} TokenType;

typedef struct {
	TokenType type;
	union { char operator;
					int  integer; 
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

#define vec_append(xs, x)\
	do {\
		if ((xs).count >= (xs).capacity) {\
			if ((xs).capacity == 0) (xs).capacity = 256;\
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

void tokenize(Tokens *output){
	char current;
	while ((current = peek())) {
		if (isspace(current)) {
			advance();
			continue;
		} 
		else if (isdigit(current)) {
			char digit_c;
			int integer=0;
			while (isdigit(digit_c=peek())) {
				integer*=10;
				integer+=ctod(digit_c);
				advance();
			}
			Token token;
			token.type=INTEGER;
			token.as.integer=integer;
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
				default:
					printf("Error: expected [+, -, *, /] found '%c'", current);
					return;
			}
			token.as.operator=current;
			vec_append((*output), token);
		}
		advance();
	}
	Token end;
	end.type = ENDSTREAM;
	vec_append((*output), end);
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
		default:
			fprintf(stderr,"Unhandled token type in print_token");
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
	LITERAL_EXPR
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

struct Expression {
	ExpressionType type;
	union {
		BinaryExpr binaryexpr;
		UnaryExpr unaryexpr;
		LiteralExpr literalexpr;
	} as;
};

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
    }
    free(expr);
}

Expression* create_binary_expr(Expression* left, Token op, Expression *right) {
	Expression *expr = malloc(sizeof(Expression));

	if (expr == NULL) {
		fprintf(stderr, "Failed to create binary expression.");
		exit(1);
	}

	expr->type=BINARY_EXPR;
	expr->as.binaryexpr.left=left;
	expr->as.binaryexpr.right=right;
	expr->as.binaryexpr.operator=op;
	return expr;
}

Expression* create_literal_expr(Token value) {
	Expression *expr = malloc(sizeof(Expression));

	expr->type = LITERAL_EXPR;
	expr->as.literalexpr.value = value;

	return expr;
}

Expression* create_unary_expr(Token op,Expression* right) {
	Expression *expr = malloc(sizeof(Expression));

	if (expr == NULL) {
		fprintf(stderr, "Failed to create binary expression.");
		exit(1);
	}

	expr->type=UNARY_EXPR;
	expr->as.unaryexpr.right=right;
	expr->as.unaryexpr.operator=op;
	return expr;
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
	printf("[Error] token: ");	
	// TODO: use tostring fn for token
	print_token(token);
	printf(" ");	
	printf("%s\n", message);	
}

Expression* primary() {
	if (match(INTEGER)) {
		return create_literal_expr(previous());
	}

	// if (match(LEFT_PAREN)) {
	// 	
	// }
	error(peekTokens(),"Expected expression.");
	return NULL;
}

Expression* unary() {
	if (match(MINUS)) {
		Token op = previous();
		Expression *right = unary();
		return create_unary_expr(op, right);
	}
	return primary();
}

Expression* factor() {
	Expression* expr = unary();
	
	while (match(SLASH) || match(STAR)) {
		Token op = previous();
		Expression *right = unary();
		expr = create_binary_expr(expr, op, right);
	}
	return expr;
}

Expression* term() {
	Expression* expr = factor();
	// match +/- tokens
	while (match(MINUS) || match(PLUS)) {
		Token op = previous();
		Expression* right = factor(); 
		expr = create_binary_expr(expr, op, right);	
	}
	return expr;
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

Expression* parse() {
	 return term();
}


int main() {
    char line[256];
    while (1) {
        printf("Calc > "); 
        if (fgets(line, sizeof(line), stdin)) {
            reset_program(&p, line);
            free_tokens(&tokens); 

            tokenize(&tokens);

            Expression *expr = parse();
            
            if (expr != NULL) {
                char buf[1024] = ""; 
                AST_printer(expr, buf, sizeof(buf));
                printf("AST: %s\n", buf);

                free_expression(expr);
            } 
        }
    }
    return 0;
}
