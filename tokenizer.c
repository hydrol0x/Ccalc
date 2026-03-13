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
    if (!p.string) {
        return true; 
    }
    return p.string[p.pos] == '\0';
}

static char peek() {
    if (eof()) return '\0';
    return p.string[p.pos];
}

char look_ahead_char() {
    if (eof()) return '\0';
    return p.string[p.pos+1];
}

static char consume() {
    if (eof()) return '\0';
    return p.string[p.pos++];
}

static void advance() {
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
    return (c==')' || c==',' || c==';' || c=='}' || c=='?' || c==':' || c=='&' || c=='|' || c=='=' || c=='<' || c=='>' || c=='!');
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
                case '!':
                    token.type=BANG;
                    if (look_ahead_char()=='=') {
                        token.type=NEQ;
                        advance_char();
                        break;
                    }
                    token.type=BANG;
                    break;
                case '=':
                    if (look_ahead_char()=='=') {
                        token.type=EQUALEQUAL;
                        advance_char();
                        break;
                    }
                    token.type=EQUAL;
                    break;
                 case '>':
                    if (look_ahead_char()=='=') {
                        token.type=GTEQUAL;
                        advance_char();
                        break;
                    }
                    token.type=GT;
                    break;
                case '<':
                    if (look_ahead_char()=='=') {
                        token.type=LTEQUAL;
                        advance_char();
                        break;
                    }
                    token.type=LT;
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
                case '&':
                    if (look_ahead_char()=='&') {
                        // Logical and uses & as op type
                        advance_char();
                        token.type=LOGIC_AND;
                        break;
                    }

                    // allow for later implement bitwise &
                    printf("[ERROR] Logical and is &&, no operator '&'\n");
                    return false;
                 case '|':
                    if (look_ahead_char()=='|') {
                        // Logical or uses | as op type
                        advance_char();
                        token.type=LOGIC_OR;
                        break;
                    }

                    // allow for later implement bitwise |
                    printf("[ERROR] Logical and is ||, no operator '|'\n");
                    return false;
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


bool str_from(Token token, char *str, size_t strlen) {
    // str is some fixed size buffer
    switch (token.type) {
        case PLUS:
        case MINUS:
        case STAR:
        case SLASH: ;
            snprintf(str, strlen, "%c", token.as.op);
            return true;
        case NUMBER:;
            double number = token.as.number;
            if ((int)number == number) {
                snprintf(str, strlen, "%d", (int)token.as.number);
            } else {
                snprintf(str, strlen, "%f", token.as.number);
            }
            return true;
        case ENDSTREAM:
            snprintf(str, strlen, "<EOF>");
            return true;
        case LPAREN:
            snprintf(str, strlen,"(");
            return true;
        case RPAREN:
            snprintf(str, strlen,")");
            return true;
        case IDENTIFIER:
            snprintf(str, strlen,"%s", token.as.string);
            return true;
        case COMMA:
            snprintf(str, strlen,",");
            return true;
        case FN:
            snprintf(str, strlen,"<FN>");
            return true;
        case LCURLY:
            snprintf(str, strlen,"{");
            return true;
        case RCURLY:
            snprintf(str, strlen,"}");
            return true;
        case QUESTION:
            snprintf(str, strlen,"?");
            return true;
        case COLON:
            snprintf(str, strlen,":");
            return true;
        case SEMICOLON:
            snprintf(str, strlen,";");
            return true;
        case EQUAL:
            snprintf(str, strlen,"=");
            return true;
        case LOGIC_AND:
            snprintf(str, strlen,"&&");
            return true;
        case LOGIC_OR:
            snprintf(str, strlen,"||");
            return true;
        case EQUALEQUAL:
            snprintf(str, strlen, "==");
            return true;
        case GTEQUAL:
            snprintf(str, strlen, ">=");
            return true;
        case LTEQUAL:
            snprintf(str, strlen, "<=");
            return true;
        case LT:
            snprintf(str, strlen, "<");
            return true;
        case GT:
            snprintf(str, strlen, ">");
            return true;
        case NEQ:
            snprintf(str, strlen, "!=");
            return true;
        case BANG:
            snprintf(str, strlen, "!");
            return true;
        default:
            fprintf(stderr,"Unhandled token type in token str_from; enum %d\n", token.type);
            return false;
    }
}

#define PRINT_TOK_BUFF_SIZE 10
void print_token(Token token) {
    char buffer[PRINT_TOK_BUFF_SIZE] = "";
    if (!str_from(token, buffer, sizeof(buffer))) {
        fprintf(stderr, "Error in str_from in print_token() printing token type %d", token.type);
        exit(1);
    }
    printf("%s", buffer);
}

void print_tokens(Tokens tokens) {
    for (int i=0; i<tokens.count; i++) {
        print_token(tokens.items[i]);
    }
}
