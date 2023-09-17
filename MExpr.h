#ifndef __MEXPR__
#define __MEXPR__

#include <stdint.h>
#include <stdbool.h>
#include "ParserExport.h"

typedef struct mexpt_node_ {

    int token_code;
    /* Below fields are relevant only when this node is operand nodes*/
    bool is_resolved;   /* Have we obtained the math value of this operand*/
    double math_val;   /* Actual Math Value */
    uint8_t variable_name[32];

    struct mexpt_node_ *left;
    struct mexpt_node_ *right;

} mexpt_node_t;

typedef struct res_{

    bool rc;
    double ovalue;
    
} res_t; 

lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int*size_out);

mexpt_node_t *
mexpr_convert_postfix_to_expression_tree (
                                    lex_data_t **lex_data, int size) ;

void 
mexpr_print_mexpt_node (mexpt_node_t *root);

void 
mexpr_debug_print_expression_tree (mexpt_node_t *root) ;

void 
mexpt_destroy(mexpt_node_t *root);

res_t
mexpt_evaluate (mexpt_node_t *root);

bool 
double_is_integer (double d);

mexpt_node_t *
Expression_build_expression_tree ();

int
Inequality_build_expression_trees (mexpt_node_t **tree1, mexpt_node_t **tree2);

extern parse_rc_t E () ;
extern parse_rc_t Q () ;


#endif 