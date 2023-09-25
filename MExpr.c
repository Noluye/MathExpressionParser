#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include "MexprEnums.h"
#include "MExpr.h"
#include "UserParserL.h"

/* ====================x================x=================== */
/* Internal Stack Implementation */

#define MAX_STACK_SIZE 256

typedef struct stack{
    int top;
    void* slot[MAX_STACK_SIZE];
    int count_of_push;
    int count_of_pop;
}Stack_t;

static Stack_t*
get_new_stack(void);
static int push(Stack_t *stack, void *node);
static void* pop(Stack_t *stack);
static int isStackEmpty(Stack_t *stack);
static void free_stack(Stack_t *stack);
static void* StackGetTopElem(Stack_t *stack);

Stack_t*
get_new_stack(void)
{
    Stack_t *stack = (Stack_t *)calloc(1, sizeof(Stack_t));
    if(!stack)
        return NULL;
    memset(stack, 0, sizeof(Stack_t));
    stack->top = -1;
    stack->count_of_push = 0;
    stack->count_of_pop = 0;
    return stack;
}

void* 
StackGetTopElem(Stack_t *stack) {

    if(!stack || stack->top == -1)  return NULL;
    return stack->slot[stack->top];
}

int 
isStackEmpty(Stack_t *stack) {

    assert(stack);
    if (stack->top == -1) return 1;
    return 0;
}

void* 
pop(Stack_t *stack) {

    void *ret = NULL;
    if(!stack) return NULL;
    if(stack->top == -1) return NULL;
    ret = stack->slot[stack->top];
    stack->slot[stack->top] = NULL;
    stack->top--;
    stack->count_of_pop++;
    return ret;
}

int 
push(Stack_t *stack, void *node) {
    if(!stack || !node) return -1;
    if(stack->top < MAX_STACK_SIZE) {
        stack->top++;
        stack->slot[stack->top] = node;
        stack->count_of_push++;
        return 0;
     }
    return -1;
}

void
free_stack(Stack_t *stack)
{
    if(!stack) return;
    free(stack);
}

/* Internal Stack Implementation FINISHED*/
/* ====================x================x=================== */

static inline bool 
Math_is_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case MATH_PLUS:
        case MATH_MINUS:
        case MATH_MUL:
        case MATH_DIV:
        case MATH_MAX:
        case MATH_MIN:
        case MATH_POW:
        case MATH_SIN:
	    case MATH_COS:
        case MATH_SQR:
        case MATH_SQRT:
        case MATH_BRACKET_START:
        case MATH_BRACKET_END:
        case MATH_OR:
        case MATH_AND:
        case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ: 
        case MATH_GREATER_THAN:
        case MATH_EQ:
        case MATH_NOT_EQ:
        return true;
    }

    return false;
}

static inline bool 
Math_is_unary_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case MATH_SIN:
	    case MATH_COS:
        case MATH_SQR:
        case MATH_SQRT:
        return true;
    }

    return false;
}

static inline bool 
Math_is_binary_operator (int token_code) {

    /* Supported Operators In MatheMatical Expression */

    switch (token_code) {

        case MATH_MAX:
        case MATH_MIN:
        case MATH_PLUS:
        case MATH_MINUS:
        case MATH_MUL:
        case MATH_DIV:
        case MATH_POW:
        case MATH_AND:
        case MATH_OR:
        case MATH_GREATER_THAN:
        case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
        case MATH_EQ:
        case MATH_NOT_EQ:
        return true;
    }

    return false;
}

static inline bool 
Math_is_ineq_operator (int token_code) {

    switch (token_code) {

        case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
        case MATH_GREATER_THAN:
        case MATH_EQ:
        case MATH_NOT_EQ:
        return true;
    }

    return false;    
}

static inline bool 
Math_is_logical_operator (int token_code) {

    switch (token_code) {

        case MATH_OR:
        case MATH_AND:
        return true;
    }

    return false;    
}

static inline bool 
Math_is_operand (int token_code) {

    switch (token_code) {

        case MATH_IDENTIFIER:
        case MATH_IDENTIFIER_IDENTIFIER:
        case MATH_INTEGER_VALUE:
        case MATH_DOUBLE_VALUE:
        case MATH_STRING_VALUE:
            return true;
        default:
            return false;
    }
}


/* Higher the returned value, higher the precedence. 
    Return Minimum value for '(*/
static int 
Math_operator_precedence (int token_code) {

    assert ( Math_is_operator (token_code));

    switch (token_code) {

        case MATH_MAX:
        case MATH_MIN:
        case MATH_POW:
            return 7;
        case MATH_MUL:
        case MATH_DIV:
            return 6;            
        case MATH_PLUS:
        case MATH_MINUS:
            return 5;
        case MATH_SIN:
	    case MATH_COS:
        case MATH_SQR:
        case MATH_SQRT:
            return 4;
        case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
        case MATH_GREATER_THAN:
        case MATH_NOT_EQ:
        case MATH_EQ:
            return 3;
        case MATH_AND:
            return 2;
        case MATH_OR:
            return 1;
        case MATH_BRACKET_START:
        case MATH_BRACKET_END:
            return 0;
    }
    assert(0);
    return 0;
} 

static bool 
mexpr_is_white_space (int token_code) {

    return (token_code == PARSER_EOL || token_code == PARSER_WHITE_SPACE);
}

lex_data_t **
mexpr_convert_infix_to_postfix (lex_data_t *infix, int sizein, int *size_out) {

    int i;
    int out_index = 0;
    lex_data_t *lex_data;

    Stack_t *stack = get_new_stack();

    lex_data_t **lex_data_arr_out = 
        (lex_data_t**)calloc(MAX_EXPR_LEN, sizeof(lex_data_t *));

    for (i = 0; i < sizein; i++) {

            lex_data = &infix[i];

            if (mexpr_is_white_space (lex_data->token_code)) continue;

            if (lex_data->token_code == MATH_BRACKET_START)
            {
                    push(stack, (void *)lex_data);
            }
            else if (lex_data->token_code == MATH_BRACKET_END)
            {
                    while (!isStackEmpty(stack) && 
                        (((lex_data_t *)stack->slot[stack->top])->token_code != MATH_BRACKET_START)) {
                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                    }
                    pop(stack);

                    while (!isStackEmpty(stack)) {

                        lex_data = (lex_data_t *)StackGetTopElem(stack);

                        if (Math_is_unary_operator (lex_data->token_code)) {

                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                            continue;
                        }
                        break;
                    }
            }

            else if (lex_data->token_code == MATH_COMMA) {

                while (!isStackEmpty(stack) && 
                    (((lex_data_t *)stack->slot[stack->top])->token_code != MATH_BRACKET_START)) {
                            lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                }
            }

            else if (!Math_is_operator(lex_data->token_code))
            {
                    lex_data_arr_out[out_index++] = lex_data;
            }
            else if (isStackEmpty (stack)) {
                push(stack, (void *)lex_data);
            }
            else
            {
                    while (!isStackEmpty(stack) &&
                        !Math_is_unary_operator(lex_data->token_code) &&
                        (Math_operator_precedence(lex_data->token_code) <= 
                          Math_operator_precedence(((lex_data_t *)stack->slot[stack->top])->token_code))) {
                        
                        lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
                    }
                    push(stack, (void *)lex_data);
            }
    }

    while (!isStackEmpty(stack)) {
        lex_data_arr_out[out_index++] = (lex_data_t *)pop(stack);
    }

    *size_out = out_index;
    free_stack(stack);
    return lex_data_arr_out;
}

 static mexpr_var_t 
mexpt_compute (int opr_token_code,
                            mexpr_var_t lrc,
                            mexpr_var_t rrc);

static mexpt_node_t*
mexpr_create_mexpt_node (
                int token_id,
                int len,
                void *operand) {

    char *endptr;
    mexpt_node_t *mexpt_node;

    mexpt_node = (mexpt_node_t *)calloc (1, sizeof (mexpt_node_t));

    /* If this node is a Math Operator node*/
    if (Math_is_operator (token_id)) {
        mexpt_node->token_code = token_id;
        return mexpt_node;
    }

    /* If this node is a Math Operand Node*/
    switch (token_id) {

        case MATH_IDENTIFIER:
        case MATH_IDENTIFIER_IDENTIFIER:
            strncpy(mexpt_node->u.opd_node.opd_value.variable_name, operand, len);
            mexpt_node->u.opd_node.is_resolved = false;
            mexpt_node->u.opd_node.is_numeric = false;
            mexpt_node->token_code = token_id;
            return mexpt_node;
        case MATH_INTEGER_VALUE:
            mexpt_node->u.opd_node.opd_value.math_val= (double)atoi(operand);
            mexpt_node->u.opd_node.is_resolved = true;
            mexpt_node->u.opd_node.is_numeric = true;
            mexpt_node->token_code = token_id;
            return mexpt_node;
        case MATH_DOUBLE_VALUE:
            mexpt_node->u.opd_node.opd_value.math_val = strtod(operand, &endptr);
            mexpt_node->u.opd_node.is_resolved = true;
            mexpt_node->u.opd_node.is_numeric = true;
            mexpt_node->token_code = token_id;
            return mexpt_node;
        case MATH_STRING_VALUE:
            strncpy (mexpt_node->u.opd_node.opd_value.string_name, operand, 
                sizeof (mexpt_node->u.opd_node.opd_value.string_name));
            mexpt_node->u.opd_node.is_resolved = true;
            mexpt_node->u.opd_node.is_numeric = false;
            mexpt_node->token_code = token_id;
            break;
        default:
            break;
    }

    return mexpt_node;
}

mexpt_tree_t *
mexpr_convert_postfix_to_expression_tree (
                                    lex_data_t **lex_data, int size) {

    int i;
    mexpt_tree_t *tree;
    mexpt_node_t *mexpt_node;
    Stack_t *stack = get_new_stack();

    tree = (mexpt_tree_t *)calloc (1, sizeof (mexpt_tree_t));

    for (i = 0; i < size; i++) {

        if (!Math_is_operator(lex_data[i]->token_code)) {
        
            mexpt_node = mexpr_create_mexpt_node (
                                    lex_data[i]->token_code, lex_data[i]->token_len, lex_data[i]->token_val);
            push(stack, (void *)mexpt_node);

            if (mexpt_node->token_code == MATH_IDENTIFIER ||
                mexpt_node->token_code == MATH_IDENTIFIER_IDENTIFIER) {

                if (!tree->opd_list_head.lst_right) {
                    tree->opd_list_head.lst_right = mexpt_node;
                    mexpt_node->lst_left = &tree->opd_list_head;
                }
                else {
                    mexpt_node->lst_right = tree->opd_list_head.lst_right;
                    mexpt_node->lst_left = &tree->opd_list_head;
                    tree->opd_list_head.lst_right = mexpt_node;
                    mexpt_node->lst_right->lst_left = mexpt_node;
                }
            }

        }

        else if (Math_is_binary_operator (lex_data[i]->token_code)){

            mexpt_node_t *right = pop(stack);
            mexpt_node_t *left = pop(stack);
            mexpt_node_t * opNode = mexpr_create_mexpt_node (
                                                        lex_data[i]->token_code, 0, NULL);
            opNode->left = left;
            opNode->right = right;
            left->parent = opNode;
            right->parent = opNode;
            push(stack, opNode);
        }

        else if (Math_is_unary_operator (lex_data[i]->token_code)){

            mexpt_node_t *left = pop(stack);
            mexpt_node_t * opNode = mexpr_create_mexpt_node (
                                                        lex_data[i]->token_code, 0, NULL);
            opNode->left = left;
            opNode->right = NULL;
            left->parent = opNode;
            push(stack, opNode);
        }

    }

    tree->root = pop(stack);
    assert (isStackEmpty (stack));
    free_stack(stack);
    assert(!tree->root->parent);
    return tree;
}

void 
mexpr_print_mexpt_node (mexpt_node_t *root) {

    if (Math_is_operator (root->token_code)) {
        printf ("Mop (%d)  ", root->token_code);
    }
    else {
        printf ("Operand (%d)  ", root->token_code);
    }
}

/* Inorder traversal of expression tree print infix notation of 
    where clause */
void 
mexpr_debug_print_expression_tree (mexpt_node_t *root) {

    if (!root) return;
    mexpr_debug_print_expression_tree (root->left);
    mexpr_print_mexpt_node (root);
    mexpr_debug_print_expression_tree (root->right);
}

static void 
mexpt_node_remove_list (mexpt_node_t *node) {

    if(!node->lst_left){
        if(node->lst_right){
            node->lst_right->lst_left = NULL;
            node->lst_right = 0;
            return;
        }
        return;
    }
    if(!node->lst_right){
        node->lst_left->lst_right = NULL;
        node->lst_left = NULL;
        return;
    }

    node->lst_left->lst_right = node->lst_right;
    node->lst_right->lst_left = node->lst_left;
    node->lst_left = 0;
    node->lst_right = 0;
}

void 
mexpt_destroy(mexpt_node_t *root, bool free_data_src) {

    if (root != NULL) {

        mexpt_destroy(root->left, free_data_src);
        mexpt_destroy(root->right, free_data_src);

        if (root->token_code == MATH_IDENTIFIER ||
            root->token_code == MATH_IDENTIFIER_IDENTIFIER) {

            if (free_data_src) free(root->u.opd_node.data_src);
            mexpt_node_remove_list (root);
        }
        free(root);
    }
}

bool 
mexpr_double_is_integer (double d) {

    double int_part = floor (d);
    return int_part == d;
}

mexpr_tree_res_t
mexpt_evaluate (mexpt_node_t *root) {

    mexpr_tree_res_t res;
    res.retc = failure_type_t;

    if (!root) return res;

    mexpr_tree_res_t lrc = mexpt_evaluate (root->left);
    mexpr_tree_res_t rrc = mexpt_evaluate (root->right);

    /* If I am leaf */
    if (!root->left && !root->right) {

        switch (root->token_code) {

            case MATH_IDENTIFIER:
            case MATH_IDENTIFIER_IDENTIFIER:
                if ( root->u.opd_node.is_resolved == false) {
                    res.retc = failure_type_t;
                    return res;
                }
                return root->u.opd_node.compute_fn_ptr(
                        root->u.opd_node.opd_value.variable_name,
                        root->u.opd_node.data_src);
            case MATH_INTEGER_VALUE:
            case MATH_DOUBLE_VALUE:
                res.retc = numeric_type_t;
                res.u.ovalue = root->u.opd_node.opd_value.math_val;
                return res;
            case MATH_STRING_VALUE:
                res.retc = alphanum_type_t;
                res.u.o_str_value = root->u.opd_node.opd_value.string_name;
                return res;
            /* Due to optimization leaf may contain : Ineq Op Or Logical Op also*/
            case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
            case MATH_GREATER_THAN:
            case MATH_EQ:
            case MATH_NOT_EQ:
                assert (root->u.ineq_node.is_optimized);
                res.retc = boolean_type_t;
                res.u.rc = root->u.ineq_node.result;
                return res;
            case MATH_OR:
            case MATH_AND:
                assert (root->u.log_op_node.is_optimized);
                res.retc = boolean_type_t;
                res.u.rc = root->u.log_op_node.result; 
                return res;
        }
    }

    /* If I am half node*/
    if (root->left && !root->right)
    {
        assert (Math_is_unary_operator (root->token_code));

        if (lrc.retc == failure_type_t) return res;

        switch (root->token_code)
        {
            case MATH_SIN:
            {
                res.retc = numeric_type_t;
                res.u.ovalue = sin(lrc.u.ovalue);
            }
            break;

	    case MATH_COS:
	    {
		res.retc = numeric_type_t;
		 res.u.ovalue = cos (lrc.u.ovalue);
	    }
	    break;

            case MATH_SQRT:
            {
                res.retc = numeric_type_t;
                res.u.ovalue = sqrt(lrc.u.ovalue);                
            }
            break;

            case MATH_SQR:
            {
                res.retc = numeric_type_t;
                res.u.ovalue = lrc.u.ovalue * lrc.u.ovalue;
            }
            break;
        }
        return res;
    }

    /* If I am Full node */
    if (lrc.retc == failure_type_t || rrc.retc == failure_type_t) return res;

    assert (Math_is_binary_operator (root->token_code));

    switch (root->token_code) {

        case MATH_MAX:
        {
            res.retc = numeric_type_t;
            res.u.ovalue = lrc.u.ovalue < rrc.u.ovalue ? rrc.u.ovalue : lrc.u.ovalue;
        }
        break;
        case MATH_MIN:
        {
            res.retc = numeric_type_t;
            res.u.ovalue = lrc.u.ovalue < rrc.u.ovalue ? lrc.u.ovalue : rrc.u.ovalue;
        }
        break;

        case MATH_PLUS:
        {
             res.retc = numeric_type_t;
             res.u.ovalue = lrc.u.ovalue + rrc.u.ovalue;         
        }
        break;

        case MATH_MINUS:
        {
            res.retc = numeric_type_t;
            res.u.ovalue = lrc.u.ovalue - rrc.u.ovalue;                    
        }
        break;

        case MATH_MUL:
        {
            res.retc = numeric_type_t;
            res.u.ovalue = rrc.u.ovalue * lrc.u.ovalue;                            
        }
        break;

        case MATH_DIV:
        {
            if (rrc.u.ovalue == 0) {
                res.retc = failure_type_t;
                return res;
            }
            res.retc = numeric_type_t;
            res.u.ovalue = lrc.u.ovalue / rrc.u.ovalue;           
        }
        break;

        case MATH_POW:
        {
            res.retc = numeric_type_t;
            res.u.ovalue = pow(lrc.u.ovalue , rrc.u.ovalue);           
        }
        break;

        case MATH_LESS_THAN:
            res.retc = boolean_type_t;
            res.u.rc = lrc.u.ovalue < rrc.u.ovalue;
            return res;

	case MATH_LESS_THAN_EQ:
	    res.retc = boolean_type_t;
	    res.u.rc = lrc.u.ovalue <= rrc.u.ovalue;
	    return res;

        case MATH_GREATER_THAN:
            res.retc = boolean_type_t;
            res.u.rc = lrc.u.ovalue > rrc.u.ovalue;
            return res;

        case MATH_EQ:
            if (lrc.retc == alphanum_type_t) {
                res.retc = boolean_type_t;
                res.u.rc = strcmp (lrc.u.o_str_value, rrc.u.o_str_value) == 0;
                return res;
            }
            if (lrc.retc == numeric_type_t) {
                res.retc = boolean_type_t;
                res.u.rc = lrc.u.ovalue == rrc.u.ovalue;
                return res;
            }
        break;

        case MATH_NOT_EQ:
            if (lrc.retc == alphanum_type_t) {
                res.retc = boolean_type_t;
                res.u.rc = strcmp (lrc.u.o_str_value, rrc.u.o_str_value) != 0;
                return res;
            }
            if (lrc.retc == numeric_type_t) {
                res.retc = boolean_type_t;
                res.u.rc = lrc.u.ovalue != rrc.u.ovalue;
                return res;
            }        
        break;

        case MATH_AND:
        {
            if (lrc.retc == rrc.retc &&
                    lrc.retc == boolean_type_t) {
                
                res.retc = boolean_type_t;
                res.u.rc = lrc.u.rc && rrc.u.rc;
                return res;
            }
        }
        break;
        case MATH_OR:
        {
            if (lrc.retc == rrc.retc &&
                    lrc.retc == boolean_type_t) {
                
                res.retc = boolean_type_t;
                res.u.rc = lrc.u.rc || rrc.u.rc;
                return res;
            }
        }        
        break;

        default:
            break;

    }
    return res;
}

static ret_codes_t
mexpr_validate_expression_tree_internal (mexpt_node_t *node) {

    if (!node) return failure_type_t ;
    
    ret_codes_t lrc = mexpr_validate_expression_tree_internal (node->left);
    ret_codes_t rrc = mexpr_validate_expression_tree_internal (node->right);

    /* Operand Nodes*/
    if (!node->left && !node->right) {

        switch (node->token_code) {

            case MATH_IDENTIFIER:
            case MATH_IDENTIFIER_IDENTIFIER:
                if (!node->u.opd_node.is_resolved) return unresolved_type_t;
                if (node->u.opd_node.is_numeric) return numeric_type_t;
                return alphanum_type_t;
            case MATH_INTEGER_VALUE:
            case MATH_DOUBLE_VALUE:
                if (!node->u.opd_node.is_resolved) return failure_type_t;
                return numeric_type_t;
            case MATH_STRING_VALUE:
                if (!node->u.opd_node.is_resolved) return failure_type_t;
                return alphanum_type_t;
            case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
            case MATH_GREATER_THAN:
            case MATH_EQ:
            case MATH_NOT_EQ:
                assert (node->u.ineq_node.is_optimized);
                return boolean_type_t;
            case MATH_AND:
            case MATH_OR:
                assert (node->u.log_op_node.is_optimized);
                return boolean_type_t;
        }
    }

    switch (node->token_code ) {

        case MATH_MAX:
        case MATH_MIN:
        case MATH_POW:
        case MATH_MUL:
        case MATH_DIV:
        case MATH_PLUS:
        case MATH_MINUS:
            if (lrc == numeric_type_t && rrc == numeric_type_t)
                return numeric_type_t;
            if (lrc == numeric_type_t && rrc == unresolved_type_t)
                return numeric_type_t;
            if (lrc == unresolved_type_t && rrc == numeric_type_t)
                return numeric_type_t;
            if (lrc == unresolved_type_t && rrc == unresolved_type_t)
                return unresolved_type_t;
            return failure_type_t;
        case MATH_SIN:
        case MATH_COS:
        case MATH_SQR:
        case MATH_SQRT:
            if (lrc == numeric_type_t || lrc == unresolved_type_t) return lrc;
            return failure_type_t;
        case MATH_LESS_THAN:
	    case MATH_LESS_THAN_EQ:
        case MATH_GREATER_THAN:
            if (lrc == numeric_type_t &&
                rrc == numeric_type_t) return boolean_type_t;
            if (lrc == numeric_type_t &&
                rrc == unresolved_type_t) return boolean_type_t;
            if (lrc == unresolved_type_t &&
                rrc == numeric_type_t) return boolean_type_t;
            if (lrc == unresolved_type_t &&
                rrc == unresolved_type_t) return unresolved_type_t;
            return failure_type_t;
        case MATH_NOT_EQ:
        case MATH_EQ:
            if (lrc == numeric_type_t && (rrc == lrc )) return boolean_type_t;
            if (lrc == alphanum_type_t && (rrc == lrc )) return boolean_type_t;
            if (lrc == failure_type_t && (rrc == lrc )) return failure_type_t;
            if (lrc == unresolved_type_t && (rrc == lrc )) return unresolved_type_t;
            if (lrc == numeric_type_t && rrc == unresolved_type_t) return boolean_type_t;
            if (lrc == alphanum_type_t && rrc == unresolved_type_t) return boolean_type_t;
            if (rrc == numeric_type_t && lrc == unresolved_type_t) return boolean_type_t;
            if (rrc == alphanum_type_t && lrc == unresolved_type_t) return boolean_type_t;
            return failure_type_t;

        case MATH_AND:
        case MATH_OR:
            if (lrc == boolean_type_t && (lrc == rrc)) return boolean_type_t;
            if (lrc == unresolved_type_t && (rrc == lrc )) return unresolved_type_t;
            return failure_type_t;
    }

    return failure_type_t;
}

bool
mexpt_optimize_new (mexpt_node_t *root) {

    bool rc = false;
    mexpr_var_t res;
    mexpt_node_t *lchild, *rchild;

    if (!root) return false;

    bool lrc = mexpt_optimize_new  (root->left);
    bool rrc = mexpt_optimize_new  (root->right);

    /* Leaf node*/
    if (!root->left && !root->right) {
        
        /* If leaf node is resolved and is constant value, then return true*/
        if (root->u.opd_node.is_resolved &&
                root->u.opd_node.compute_fn_ptr == NULL) {

            return true;
        }

        return false;
    }

    /* Half node*/
    if (root->left && !root->right) {

        if (lrc == false) return false;

        lchild = root->left;

        /* Half node cannot have child with string value*/
        assert(lchild->u.opd_node.is_numeric);

        res.dtype = MEXPR_DTYPE_DOUBLE;
        res.u.d_val = lchild->u.opd_node.opd_value.math_val;
        res = mexpt_compute(root->token_code, res, res);

        /* Passing gouble arg, so expecting double output only*/
        assert(res.dtype == MEXPR_DTYPE_DOUBLE);

        /* Rejuvenate the self*/
        root->token_code = MATH_DOUBLE_VALUE;
        root->left = NULL;
        root->u.opd_node.is_resolved = true;
        root->u.opd_node.is_numeric = true;
        root->u.opd_node.opd_value.math_val = res.u.d_val;
        mexpt_destroy(lchild, true);
        return true;
    }

    /* Full nodes*/

    mexpr_var_t lval, rval;

    switch (root->token_code) {

      case MATH_LESS_THAN:
      case MATH_LESS_THAN_EQ:
      case MATH_GREATER_THAN:

          if (!lrc || !rrc) return false;

          lchild = root->left;
          rchild = root->right;

          /* In Mexpr Tree We store numerc values as double only*/
          assert(lchild->u.opd_node.is_numeric);
          assert(rchild->u.opd_node.is_numeric);

          lval.dtype = MEXPR_DTYPE_DOUBLE;
          lval.u.d_val = lchild->u.opd_node.opd_value.math_val;
          rval.dtype = MEXPR_DTYPE_DOUBLE;
          rval.u.d_val = rchild->u.opd_node.opd_value.math_val;

          res = mexpt_compute(root->token_code, lval, rval);
          assert(res.dtype == MEXPR_DTYPE_BOOL);

          root->u.ineq_node.is_optimized = true;
          root->u.ineq_node.result = res.u.b_val;
          mexpt_destroy(root->left, true);
          mexpt_destroy(root->right, true);
          root->left = NULL;
          root->right = NULL;
          return true;

    /* Support Strings also*/
    case MATH_EQ:
    case MATH_NOT_EQ:
         if (!lrc || !rrc) return false;
         lchild = root->left;
         rchild = root->right;

         if (lchild->u.opd_node.is_numeric == false) {
             // this API is used only after  mexpr_validate_expression_tree( )
             assert(rchild->u.opd_node.is_numeric == false);
             assert(lchild->u.opd_node.is_resolved &&
                    rchild->u.opd_node.is_resolved);

            lval.dtype = MEXPR_DTYPE_STRING;
            lval.u.str_val = lchild->u.opd_node.opd_value.string_name;
            rval.dtype = MEXPR_DTYPE_STRING;
            rval.u.str_val = rchild->u.opd_node.opd_value.string_name;

            res = mexpt_compute (root->token_code, lval, rval);
            assert(res.dtype == MEXPR_DTYPE_BOOL);

             root->u.ineq_node.is_optimized = true;
             root->u.ineq_node.result = res.u.b_val;

             mexpt_destroy(root->left, true);
             mexpt_destroy(root->right, true);
             root->left = NULL;
             root->right = NULL;
             return true;
         }
         else
         {
             assert(lchild->u.opd_node.is_numeric);
             assert(rchild->u.opd_node.is_numeric);
            lval.dtype = MEXPR_DTYPE_DOUBLE;
            lval.u.d_val = lchild->u.opd_node.opd_value.math_val;
            rval.dtype = MEXPR_DTYPE_DOUBLE;
            rval.u.d_val = rchild->u.opd_node.opd_value.math_val;

            res = mexpt_compute (root->token_code, lval, rval);
            assert(res.dtype == MEXPR_DTYPE_BOOL);
             root->u.ineq_node.is_optimized = true;
             root->u.ineq_node.result =res.u.b_val;

             mexpt_destroy(root->left, true);
             mexpt_destroy(root->right, true);
             root->left = NULL;
             root->right = NULL;
             return true;
         }
        break;


          case MATH_OR:
        {
            if (!lrc && !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            bool l_bool = false, r_bool = false;
            bool l_bool_set = false, r_bool_set = false;
            bool optimize_me = false;

            assert (Math_is_ineq_operator (lchild->token_code) ||
                Math_is_logical_operator (lchild->token_code));

            do {

                if (Math_is_ineq_operator (lchild->token_code) &&
                        lchild->u.ineq_node.is_optimized) {
                    
                    l_bool =  lchild->u.ineq_node.result;
                    l_bool_set = true;
                    if (l_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (lchild->token_code) &&
                        lchild->u.log_op_node.is_optimized) {
                    
                    l_bool =  lchild->u.log_op_node.result;
                    l_bool_set = true;
                    if (l_bool) optimize_me = true;
                }

                if (optimize_me) break;

                if (Math_is_ineq_operator (rchild->token_code) &&
                        rchild->u.ineq_node.is_optimized) {
                    
                    r_bool =  rchild->u.ineq_node.result;
                    r_bool_set = true;
                    if (r_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (rchild->token_code) &&
                        rchild->u.log_op_node.is_optimized) {
                    
                    r_bool =  rchild->u.log_op_node.result;
                    r_bool_set = true;
                    if (r_bool) optimize_me = true;
                }

            } while (0);

            if (optimize_me) {
                rc = true; 
            }
            else if (l_bool_set && r_bool_set ) {
                 optimize_me = true;
                 rc = false;
            }

            if (optimize_me) {

                root->u.log_op_node.is_optimized = true;
                root->u.log_op_node.result = rc;
                mexpt_destroy (lchild, true);
                mexpt_destroy (rchild, true);
                root->left = NULL;
                root->right = NULL;
                return true;
            }
        }
        break;



        case MATH_AND:
        {
            if (!lrc && !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            bool l_bool = false, r_bool = false;
            bool l_bool_set = false, r_bool_set = false;
            bool optimize_me = false;

            assert (Math_is_ineq_operator (lchild->token_code) ||
                Math_is_logical_operator (lchild->token_code));

            do {

                if (Math_is_ineq_operator (lchild->token_code) &&
                        lchild->u.ineq_node.is_optimized) {
                    
                    l_bool =  lchild->u.ineq_node.result;
                    l_bool_set = true;
                    if (!l_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (lchild->token_code) &&
                        lchild->u.log_op_node.is_optimized) {
                    
                    l_bool =  lchild->u.log_op_node.result;
                    l_bool_set = true;
                    if (!l_bool) optimize_me = true;
                }

                if (optimize_me) break;

                if (Math_is_ineq_operator (rchild->token_code) &&
                        rchild->u.ineq_node.is_optimized) {
                    
                    r_bool =  rchild->u.ineq_node.result;
                    r_bool_set = true;
                    if (!r_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (rchild->token_code) &&
                        rchild->u.log_op_node.is_optimized) {
                    
                    r_bool =  rchild->u.log_op_node.result;
                    r_bool_set = true;
                    if (!r_bool) optimize_me = true;
                }

            } while (0);

            if (optimize_me) {
                rc = false;
            }
            else if (l_bool_set && r_bool_set ) {
                 optimize_me = true;
                 rc = true;
            }

            if (optimize_me) {

                root->u.log_op_node.is_optimized = true;
                root->u.log_op_node.result = rc;
                mexpt_destroy (lchild, true);
                mexpt_destroy (rchild, true);
                root->left = NULL;
                root->right = NULL;
                return true;
            }
        }
        break;

    /* Supported on Strings also */
    case MATH_PLUS:
    case MATH_MINUS:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;

            if (lchild->u.opd_node.is_numeric) {
                lval.dtype = MEXPR_DTYPE_DOUBLE;
                lval.u.d_val = lchild->u.opd_node.opd_value.math_val ;
            }
            else {
                lval.dtype = MEXPR_DTYPE_STRING;
                lval.u.str_val = lchild->u.opd_node.opd_value.string_name;
            }
            if (rchild->u.opd_node.is_numeric) {
                rval.dtype = MEXPR_DTYPE_DOUBLE;
                rval.u.d_val = rchild->u.opd_node.opd_value.math_val ;
            }
            else {
                rval.dtype = MEXPR_DTYPE_STRING;
                rval.u.str_val = rchild->u.opd_node.opd_value.string_name;
            }            
    
            res = mexpt_compute (root->token_code, lval, rval);
            
            if (res.dtype == MEXPR_DTYPE_DOUBLE) {
                root->token_code = MATH_DOUBLE_VALUE;
                root->u.opd_node.is_numeric = true;
                root->u.opd_node.opd_value.math_val = res.u.d_val;
            }
            else if (res.dtype == MEXPR_DTYPE_STRING) {
                root->token_code = MATH_STRING_VALUE;
                root->u.opd_node.is_numeric = false;
                memset (root->u.opd_node.opd_value.string_name, 0,
                    sizeof (root->u.opd_node.opd_value.string_name));
                strncpy(root->u.opd_node.opd_value.string_name,
                    res.u.str_val, sizeof (root->u.opd_node.opd_value.string_name));
            }
            root->u.opd_node.is_resolved = true;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;        

    case MATH_MUL:
    case MATH_DIV:
    case MATH_MAX:
    case MATH_MIN:
    case MATH_POW:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);
            lval.dtype = MEXPR_DTYPE_DOUBLE;
            lval.u.d_val = lchild->u.opd_node.opd_value.math_val;
            rval.dtype = MEXPR_DTYPE_DOUBLE;
            rval.u.d_val = rchild->u.opd_node.opd_value.math_val;
            res = mexpt_compute (root->token_code, lval, rval);
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.opd_value.math_val = res.u.d_val;
            root->u.opd_node.is_resolved = true;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;              


        default:
            assert(0);
    }
}

bool
mexpt_optimize (mexpt_node_t *root) {

    bool rc = false;
    mexpt_node_t *lchild, *rchild;

    if (!root) return false;

    bool lrc = mexpt_optimize  (root->left);
    bool rrc = mexpt_optimize  (root->right);

    /* Leaf node*/
    if (!root->left && !root->right) {
        
        /* If leaf node is resolved and is constant value, then return true*/
        if (root->u.opd_node.is_resolved &&
                root->u.opd_node.compute_fn_ptr == NULL) {

            return true;
        }

        return false;
    }

    /* Half node*/
    if (root->left && !root->right) {

        if (lrc == false) return false;

        lchild = root->left;

        double val;

        if (lchild->u.opd_node.is_numeric) {

            val = lchild->u.opd_node.opd_value.math_val;

            switch (root->token_code) {

                case MATH_SQR:
                    val = val *val;
                    break;
                case MATH_SQRT:
                    val = sqrt (val);
                    break;
                case MATH_SIN:
                    val = sin (val);
                    break;
                case MATH_COS:  
                    val = cos (val);
                    break;
                default:
                    assert(0);
            }
        }
        else {
            /* Half node cannot have child with string value*/
            assert (0);
        }

        /* Rejuvenate the self*/
        root->token_code = MATH_DOUBLE_VALUE;
        root->left = NULL;
        root->u.opd_node.is_resolved = true;
        root->u.opd_node.is_numeric = true;
        root->u.opd_node.opd_value.math_val = val;
        mexpt_destroy(lchild, true);
        return true;
    }

    /* Full nodes*/

    switch (root->token_code) {

        case MATH_LESS_THAN:
            
            if (!lrc || !rrc) return false;

            if ((root->left->u.opd_node.opd_value.math_val <
                    root->right->u.opd_node.opd_value.math_val )) {
                rc = true;
            }

            root->u.ineq_node.is_optimized = true;
            root->u.ineq_node.result = rc;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;


        case MATH_LESS_THAN_EQ:
            
            if (!lrc || !rrc) return false;

            if ((root->left->u.opd_node.opd_value.math_val <=
                    root->right->u.opd_node.opd_value.math_val )) {
                rc = true;
            }

            root->u.ineq_node.is_optimized = true;
            root->u.ineq_node.result = rc;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;



        case MATH_GREATER_THAN:

            if (!lrc || !rrc) return false;

            if ((root->left->u.opd_node.opd_value.math_val >
                    root->right->u.opd_node.opd_value.math_val )) {
                rc = true;
            }

            root->u.ineq_node.is_optimized = true;
            root->u.ineq_node.result = rc;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;            



        case MATH_OR:
        {
            if (!lrc && !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            bool l_bool = false, r_bool = false;
            bool l_bool_set = false, r_bool_set = false;
            bool optimize_me = false;

            assert (Math_is_ineq_operator (lchild->token_code) ||
                Math_is_logical_operator (lchild->token_code));

            do {

                if (Math_is_ineq_operator (lchild->token_code) &&
                        lchild->u.ineq_node.is_optimized) {
                    
                    l_bool =  lchild->u.ineq_node.result;
                    l_bool_set = true;
                    if (l_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (lchild->token_code) &&
                        lchild->u.log_op_node.is_optimized) {
                    
                    l_bool =  lchild->u.log_op_node.result;
                    l_bool_set = true;
                    if (l_bool) optimize_me = true;
                }

                if (optimize_me) break;

                if (Math_is_ineq_operator (rchild->token_code) &&
                        rchild->u.ineq_node.is_optimized) {
                    
                    r_bool =  rchild->u.ineq_node.result;
                    r_bool_set = true;
                    if (r_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (rchild->token_code) &&
                        rchild->u.log_op_node.is_optimized) {
                    
                    r_bool =  rchild->u.log_op_node.result;
                    r_bool_set = true;
                    if (r_bool) optimize_me = true;
                }

            } while (0);

            if (optimize_me) {
                rc = true; 
            }
            else if (l_bool_set && r_bool_set ) {
                 optimize_me = true;
                 rc = false;
            }

            if (optimize_me) {

                root->u.log_op_node.is_optimized = true;
                root->u.log_op_node.result = rc;
                mexpt_destroy (lchild, true);
                mexpt_destroy (rchild, true);
                root->left = NULL;
                root->right = NULL;
                return true;
            }
        }
        break;



        case MATH_AND:
        {
            if (!lrc && !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            bool l_bool = false, r_bool = false;
            bool l_bool_set = false, r_bool_set = false;
            bool optimize_me = false;

            assert (Math_is_ineq_operator (lchild->token_code) ||
                Math_is_logical_operator (lchild->token_code));

            do {

                if (Math_is_ineq_operator (lchild->token_code) &&
                        lchild->u.ineq_node.is_optimized) {
                    
                    l_bool =  lchild->u.ineq_node.result;
                    l_bool_set = true;
                    if (!l_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (lchild->token_code) &&
                        lchild->u.log_op_node.is_optimized) {
                    
                    l_bool =  lchild->u.log_op_node.result;
                    l_bool_set = true;
                    if (!l_bool) optimize_me = true;
                }

                if (optimize_me) break;

                if (Math_is_ineq_operator (rchild->token_code) &&
                        rchild->u.ineq_node.is_optimized) {
                    
                    r_bool =  rchild->u.ineq_node.result;
                    r_bool_set = true;
                    if (!r_bool) optimize_me = true;
                }
                else if (Math_is_logical_operator (rchild->token_code) &&
                        rchild->u.log_op_node.is_optimized) {
                    
                    r_bool =  rchild->u.log_op_node.result;
                    r_bool_set = true;
                    if (!r_bool) optimize_me = true;
                }

            } while (0);

            if (optimize_me) {
                rc = false;
            }
            else if (l_bool_set && r_bool_set ) {
                 optimize_me = true;
                 rc = true;
            }

            if (optimize_me) {

                root->u.log_op_node.is_optimized = true;
                root->u.log_op_node.result = rc;
                mexpt_destroy (lchild, true);
                mexpt_destroy (rchild, true);
                root->left = NULL;
                root->right = NULL;
                return true;
            }
        }
        break;


        case MATH_EQ:
        case MATH_NOT_EQ:
        {
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;

            if (lchild->u.opd_node.is_numeric == false ) {
  
                // this API is used only after  mexpr_validate_expression_tree( )
                assert (rchild->u.opd_node.is_numeric == false);
                assert (lchild->u.opd_node.is_resolved &&
                                rchild->u.opd_node.is_resolved);

                rc = strcmp (lchild->u.opd_node.opd_value.string_name, 
                                    rchild->u.opd_node.opd_value.string_name) == 0;

                root->u.ineq_node.is_optimized = true;
                root->u.ineq_node.result = (root->token_code == MATH_EQ) ? rc : !rc;
                mexpt_destroy(root->left, true);
                mexpt_destroy(root->right, true);
                root->left = NULL;
                root->right = NULL;
                return true;        
            }
            else {

                assert (lchild->u.opd_node.is_numeric); 
                assert (rchild->u.opd_node.is_numeric);                     
                rc = lchild->u.opd_node.opd_value.math_val == rchild->u.opd_node.opd_value.math_val;
                root->u.ineq_node.is_optimized = true;
                root->u.ineq_node.result = (root->token_code == MATH_EQ) ? rc : !rc;
                mexpt_destroy(root->left, true);
                mexpt_destroy(root->right, true);
                root->left = NULL;
                root->right = NULL;
                return true;     
            }
        }
        break;



        case MATH_PLUS:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val + rchild->u.opd_node.opd_value.math_val ;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;



        case MATH_MINUS:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val - rchild->u.opd_node.opd_value.math_val ;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;



        case MATH_MUL:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val * rchild->u.opd_node.opd_value.math_val ;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;


        case MATH_DIV:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val / rchild->u.opd_node.opd_value.math_val ;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;


        case MATH_MAX:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val > rchild->u.opd_node.opd_value.math_val ? \
                lchild->u.opd_node.opd_value.math_val : rchild->u.opd_node.opd_value.math_val;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;            


        case MATH_MIN:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                lchild->u.opd_node.opd_value.math_val < rchild->u.opd_node.opd_value.math_val ? \
                lchild->u.opd_node.opd_value.math_val : rchild->u.opd_node.opd_value.math_val;
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;      



        case MATH_POW:
            if (!lrc || !rrc) return false;
            lchild = root->left;
            rchild = root->right;
            assert (lchild->u.opd_node.is_numeric); 
            assert (rchild->u.opd_node.is_numeric);        
            root->token_code = MATH_DOUBLE_VALUE;
            root->u.opd_node.is_numeric = true;
            root->u.opd_node.is_resolved = true;
            root->u.opd_node.opd_value.math_val = 
                pow (lchild->u.opd_node.opd_value.math_val ,
                rchild->u.opd_node.opd_value.math_val);
            mexpt_destroy(root->left, true);
            mexpt_destroy(root->right, true);
            root->left = NULL;
            root->right = NULL;
            return true;     
        break;      

        default:
            break;
    }

    return false;
}


bool
mexpr_validate_expression_tree (mexpt_tree_t *tree) {

    return   (mexpr_validate_expression_tree_internal (tree->root) != failure_type_t);
}

void 
mexpt_tree_install_operand_properties (
                mexpt_node_t *node,
                bool is_numeric,
                void *data_src,
                mexpr_tree_res_t (*compute_fn_ptr)(unsigned char *, void *)) {

    assert (node->token_code == MATH_IDENTIFIER ||
                node->token_code == MATH_IDENTIFIER_IDENTIFIER);

    node->u.opd_node.is_numeric = is_numeric;
    node->u.opd_node.is_resolved = true;
    node->u.opd_node.data_src =  data_src;
    node->u.opd_node.compute_fn_ptr = compute_fn_ptr;
}

static mexpt_node_t *
mexpt_get_unresolved_operand_node (mexpt_tree_t *tree) {

    mexpt_node_t *opd_node;

    mexpt_iterate_operands_begin (tree, opd_node) {

        if (opd_node->u.opd_node.is_resolved) continue;
        return opd_node; 

    } mexpt_iterate_operands_end (tree, opd_node);

    return NULL;
}

uint8_t 
mexpt_remove_unresolved_operands (mexpt_tree_t *tree, bool free_data_src) {

    uint8_t count = 0;
    mexpt_node_t *opd_node;
    
    while ((opd_node = mexpt_get_unresolved_operand_node (tree))) {

        while (opd_node && !Math_is_ineq_operator (opd_node->token_code)) {
            opd_node = opd_node->parent;
        }

        if (!opd_node) return count;

        mexpt_destroy (opd_node->left, free_data_src);
        mexpt_destroy (opd_node->right, free_data_src);
        opd_node->left = NULL;
        opd_node->right = NULL;
        opd_node->u.ineq_node.is_optimized = true;
        opd_node->u.ineq_node.result = true;
        count++;
    }

    /* should perform optimization after removal 
        of unresolved operand nodes */
    //mexpt_optimize (tree);
    return count;
}

static void 
mexpt_node_clone_data (mexpt_node_t  *src_node, mexpt_node_t  *dst_node) {

    memcpy (dst_node, src_node, sizeof (*dst_node));
    dst_node->left = NULL;
    dst_node->right = NULL;
    dst_node->lst_left = NULL;
    dst_node->lst_right = NULL;
    dst_node->parent = NULL;
}

static void
mexpt_clone_node_recursively (mexpt_node_t  *src_node, 
                                                     mexpt_node_t  *dst_node, 
                                                     int child,  
                                                     mexpt_node_t  **new_root) {

    mexpt_node_t *child_node = NULL;

    if (!src_node) return;
    
    if (child == 0) {
        dst_node = (mexpt_node_t *) calloc (1, sizeof (mexpt_node_t));
        mexpt_node_clone_data (src_node, dst_node);
        *new_root = dst_node;
        child_node = dst_node;
    }
    else if (child == -1) {
        child_node = (mexpt_node_t *) calloc (1, sizeof (mexpt_node_t));
        mexpt_node_clone_data (src_node,  child_node);
        dst_node->left = child_node;
        child_node->parent = dst_node;
    }
    else if (child == 1) {
        child_node = (mexpt_node_t *) calloc (1, sizeof (mexpt_node_t));
        mexpt_node_clone_data (src_node,  child_node);
        dst_node->right = child_node;
        child_node->parent = dst_node;
    }  
    mexpt_clone_node_recursively (src_node->left, child_node, -1, new_root);
    mexpt_clone_node_recursively (src_node->right, child_node, 1, new_root);
}

static void 
_mexpt_tree_create_operand_list (mexpt_tree_t *tree, mexpt_node_t *node) {

    if (!node) return;

    if (node->token_code == MATH_IDENTIFIER ||
        node->token_code == MATH_IDENTIFIER_IDENTIFIER) {

        if (!tree->opd_list_head.lst_right) {

            tree->opd_list_head.lst_right = node;
            node->lst_left = &tree->opd_list_head;
        }
        else {

            node->lst_right = tree->opd_list_head.lst_right;
            node->lst_left = &tree->opd_list_head;
            tree->opd_list_head.lst_right = node;
            node->lst_right->lst_left = node;
        }
    }

    _mexpt_tree_create_operand_list  (tree, node->left);
    _mexpt_tree_create_operand_list  (tree, node->right);
}

static void 
mexpt_tree_create_operand_list (mexpt_tree_t *tree ) {

    _mexpt_tree_create_operand_list (tree, tree->root);
}

mexpt_tree_t *
mexpt_clone (mexpt_tree_t *tree) {

    mexpt_node_t *new_root = NULL;
    mexpt_tree_t *clone_tree = (mexpt_tree_t *) calloc (1, sizeof (mexpt_tree_t));
    if (!tree->root) return clone_tree;
    mexpt_clone_node_recursively (tree->root, NULL, 0, &new_root);
    clone_tree->root = new_root;
    mexpt_tree_create_operand_list (clone_tree );
    return clone_tree;
}

/* NEW IMPLEMENTATION*/

#include "MexprDb.c"

static void 
mexpr_convert_res_to_var (mexpr_tree_res_t *res, mexpr_var_t *var) {

    var->dtype = (res->retc == numeric_type_t ) ? MEXPR_DTYPE_DOUBLE : MEXPR_DTYPE_STRING;

    switch (var->dtype) {

        case MEXPR_DTYPE_DOUBLE:
            var->u.d_val = res->u.ovalue;
            break;
        case MEXPR_DTYPE_STRING:
            var->u.str_val = res->u.o_str_value;
            break;
        default:
            assert(0);
    }
}

 mexpr_var_t 
mexpt_compute (int opr_token_code,
                            mexpr_var_t lrc,
                            mexpr_var_t rrc) {

    operator_fn_ptr_t opr_fn_ptr = MexprDb[opr_token_code][lrc.dtype][rrc.dtype];
    return opr_fn_ptr (lrc, rrc);
}

mexpr_var_t
mexpt_evaluate_new (mexpt_node_t *root)  {

    mexpr_var_t res;
    res.dtype = MEXPR_DTYPE_INVALID;

    if (!root) return res;

    mexpr_var_t lrc = mexpt_evaluate_new (root->left);
    mexpr_var_t rrc = mexpt_evaluate_new (root->right);

        /* If I am leaf */
    if (!root->left && !root->right) {

        switch (root->token_code) {

            case MATH_IDENTIFIER:
            case MATH_IDENTIFIER_IDENTIFIER:
                if ( root->u.opd_node.is_resolved == false) {
                    res.dtype = MEXPR_DTYPE_INVALID;
                    return res;
                }
               mexpr_tree_res_t temp_rc =  root->u.opd_node.compute_fn_ptr(
                        root->u.opd_node.opd_value.variable_name,
                        root->u.opd_node.data_src);
                mexpr_convert_res_to_var (&temp_rc, &res);
                return res;
            case MATH_INTEGER_VALUE:
                res.dtype = MEXPR_DTYPE_INT;
                res.u.int_val = (int)  root->u.opd_node.opd_value.math_val;
                return res;
            case MATH_DOUBLE_VALUE:
                res.dtype = MEXPR_DTYPE_DOUBLE;
                res.u.d_val = root->u.opd_node.opd_value.math_val;
                return res;
            case MATH_STRING_VALUE:
                res.dtype = MEXPR_DTYPE_STRING;
                res.u.str_val =  root->u.opd_node.opd_value.string_name;
                return res;
            default:
            /* Due to optimization leaf may contain : Ineq Op Or Logical Op also*/
            if (Math_is_ineq_operator (root->token_code)) {
                assert (root->u.ineq_node.is_optimized);
                res.dtype= MEXPR_DTYPE_BOOL;
                res.u.b_val = root->u.ineq_node.result;
                return res;
            }
            if (Math_is_logical_operator (root->token_code)) {
                assert (root->u.log_op_node.is_optimized);
                res.dtype= MEXPR_DTYPE_BOOL;
                res.u.b_val = root->u.log_op_node.result;
                return res;
            }
            assert(0);
        }
    }

    if (root->left && !root->right) {

        assert (Math_is_unary_operator (root->token_code));
        if (lrc.dtype == MEXPR_DTYPE_INVALID) return res;
        return mexpt_compute (root->token_code, lrc, lrc);
    }

    /* If I am Full node */
    if (lrc.dtype== MEXPR_DTYPE_INVALID || rrc.dtype == MEXPR_DTYPE_INVALID) return res;

     assert (Math_is_binary_operator (root->token_code));
     return mexpt_compute (root->token_code, lrc, rrc);
}
