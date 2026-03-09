#ifndef INTERPRETER_H 
#define INTERPRETER_H 

#include <stdbool.h>
#include <stdint.h>
#include "parser.h"
#include "hashmap.h"

typedef enum {
    INT, // INT is 0 = default
    FLOAT,
} Repr;

typedef struct {
    bool status;
    double result;    
} EvalResult; 

// entry for hashmap
struct var_entry {
    char *identifier;
    double value;
};

struct fn_entry { // fn entry in hashmap
    char *identifier;
    FnExpr *value;
};

/* Note: original code contained a typedef with the same name as the struct tag */
typedef struct {
    char *identifier;
    Expression *ptr;
} fn_entry_t; /* renamed slightly to avoid conflict if strictly enforced, though C allows it */

/* Global Environments */
extern struct hashmap *env_map; 
extern struct hashmap *fn_map; 

/* Hashmap Callbacks */
uint64_t var_hash(const void *item, uint64_t seed0, uint64_t seed1);
int var_compare(const void *a, const void *b, void *udata);
void var_free(void *item);

uint64_t fn_hash(const void *item, uint64_t seed0, uint64_t seed1);
int fn_compare(const void *a, const void *b, void *udata);
void fn_free(void *item);

/* Evaluation */
EvalResult eval_with_env(Expression *expr, struct hashmap *environment);

#endif /* EVALUATOR_H */
