#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

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

typedef struct program {
	char *string;
	int pos;
} program;
program p;

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
	
	while (match(SLASH) | match(STAR)) {
		Token op = previous();
		Expression *right = unary();
		expr = create_binary_expr(expr, op, right);
	}
	return expr;
}

Expression* term() {
	Expression* expr = factor();
	// match +/- tokens
	while (match(MINUS) | match(PLUS)) {
		Token op = previous();
		Expression* right = factor(); 
		create_binary_expr(expr, op, right);	
	}
	return expr;
}

Expression* parse() {
	return term();
}

int main() {
	char line[256];
	// int i;
	if (fgets(line, sizeof(line), stdin)) {
		p.pos = 0;
		p.string=line;
		 // Token token;
		 // token.operator='+';
	  tokenize(&tokens);
		//Expression *expr = parse();
		parse();
		printf("tokens capacity %lu\n", tokens.capacity);
		printf("tokens count %lu\n", tokens.count);
		print_tokens(tokens);
		printf("\n");
	}
}
