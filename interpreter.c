#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "interpreter.h"
#include "tokenizer.h"

struct hashmap *env_map = NULL; // declared global variables
struct hashmap *fn_map = NULL; //  declared global fns 

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
                case LOGIC_AND:
                    out = lhs && rhs;
                    break;
                case LOGIC_OR:
                    out = lhs || rhs;
                    break;
                case EQUALEQUAL:
                    out = lhs == rhs;
                    break;
                default: ;
                    char opstr[3]="";
                    str_from(expr->as.binaryexpr.operator, opstr, 3);
                    fprintf(stderr, "[ERROR] Encountered '%s', not a valid operator\n",opstr);
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
