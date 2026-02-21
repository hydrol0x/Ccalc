#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>


typedef struct program {
	char *string;
	int pos;
} program;
program p;

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

// void expand_vec(Strings vector) {
// 	if (vector.capacity == 0) {
// 		vector.capacity=256;
// 	}
// 	vector.capacity*=2;
// 	vector.items = realloc(vector.items, vector.capacity*sizeof(*vector.items));
// }
// 
// void append_vec(Strings vector, char* string) {
// 	if (vector.count>=vector.capacity) {
// 		expand_vec(vector);
// 	}
// 	vector.items[vector.count++] = string;
// }

int ctod(char character) {
	// char to digit
	return character - '0';
}

typedef enum {
	OPERATOR,
	INTEGER,
} TokenType;

typedef struct {
	TokenType type;
	union { char operator;
					int  integer; 
	} as;
} Token;

typedef struct {
	size_t count;
	size_t capacity;
	Token *items;
} Tokens;

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
			printf("Parsed integer is %d\n", integer);
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
				case '-':
				case '*':
				case '/':
						token.type=OPERATOR;
						token.as.operator=current;
						vec_append((*output), token);
					break;
				default:
					printf("Error: expected [+, -, *, /] found '%c'", current);
					return;
			}
		}
		advance();
	}
}

void print_token(Token token) {
	if (token.type==OPERATOR) {
		switch (token.as.operator) {
			case '+':
			case '-':
			case '*':
			case '/':
					printf("%c", token.as.operator);
		}
	} else if (token.type==INTEGER){
			printf("%d", token.as.integer);
	}
}

void print_tokens(Tokens tokens) {
	for (int i=0; i<tokens.count; i++) {
		print_token(tokens.items[i]);
	}
}

int main() {
	char line[256];
	// int i;
	if (fgets(line, sizeof(line), stdin)) {
		p.pos = 0;
		p.string=line;
		 // Token token;
		 // token.operator='+';
		Tokens output = {0};
	  tokenize(&output);
		printf("output capacity %lu\n", output.capacity);
		printf("output count %lu\n", output.count);
		print_tokens(output);
		printf("\n");
	}
}
