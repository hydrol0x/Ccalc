#include <stdio.h>
#include <stdlib.h>
#include "tokenizer.h"
#include "parser.h"
#include "interpreter.h"
#include "hashmap.h"

int main() {
    char line[256];
    env_map=hashmap_new(sizeof(struct var_entry), 0, 0, 0,var_hash, var_compare, var_free, NULL);
    fn_map=hashmap_new(sizeof(struct fn_entry), 0, 0, 0,fn_hash, fn_compare, fn_free, NULL);
    while (1) {
        printf("Calc > "); 
        if (fgets(line, sizeof(line), stdin)) {
            reset_program(&p, line);
            free_tokens(&tokens); 
            if (!tokenize(&tokens)) {
                continue;
            }

            printf("Token stream: ");
            for (int i=0; i<tokens.count; i++) {
                printf("'"); print_token(tokens.items[i]); printf("'");
                printf(", ");
            }
            printf("\n");

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

                    EvalResult result = eval_with_env(expr, env_map);

                    if (!result.status) {
                        continue;
                    }

                    double val = result.result;
                    struct var_entry ans = {
                        .identifier="Ans",
                        .value=val,
                    };
                    hashmap_set(env_map, &ans); // special 'ans' variable that contains previous calculation's answer

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
    hashmap_free(env_map);
    return 0;
}
