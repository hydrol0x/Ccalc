#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include "tokenizer.h"

Tokens tokens = {0};
program p;
const char keywords[] = {"fn"};

void free_tokens(Tokens *ts) {
    if (ts->items != NULL) {
        free(ts->items);
    }
    ts->items = NULL;
    ts->count = 0;
    ts->capacity = 0;
    ts->pos = 0; 
}

void reset_program(program *prog, char *input) {
    prog->string = input;
    prog->pos = 0;
}

bool eof() {
    return p.pos > strlen(p.string);
}

char peek_char() {
    if (eof()) return '\0';
    return p.string[p.pos];
}

char consume_char() {
    if (eof()) return '\0';
    return p.string[p.pos++];
}

void advance_char() {
    if (eof()) return;
    p.pos++;
}

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
    while ((current = peek_char())) {
        if (isspace(current)) {
            advance_char();
            continue;
        } 
        else if (isdigit(current)) {
            char digit_c;
            String number = {0};
            while (!isspace(digit_c=peek_char())) {
                if (isdigit(digit_c)) {
                    vec_append(number, digit_c);
                    advance_char();
                    continue;
                } else if (is_op(digit_c) || is_digit_delim(digit_c) || digit_c=='.') {
                    break;
                } else {
                    printf("[Error] Invalid decimal input, expected digit found '%c'\n", digit_c);
                    return false;
                }
            }
            if (peek_char() == '.') {
                vec_append(number,'.');
                advance_char();
                while (!isspace(digit_c=peek_char())) {
                    if (isdigit(digit_c)) {
                        vec_append(number, digit_c);
                        advance_char();
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
            while ((isalnum(id_c=peek_char())) && i<sizeof(identifier)-1) {
                vec_append(identifier, id_c);    
                advance_char();
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
                    advance_char();
                    while (!isspace(digit_c=peek_char())) {
                        if (isdigit(digit_c)) {
                            vec_append(number, digit_c);
                            advance_char();
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
            token.as.op=current;
            vec_append((*output), token);
        }
        advance_char();
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
            str[0] = token.as.op; 
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
            printf("%c", token.as.op);
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
