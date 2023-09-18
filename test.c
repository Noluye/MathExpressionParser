#include <stdio.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <arpa/inet.h>

/* Mandatory : #include the Parser Interface */
#include "ParserExport.h"


/* #include the files which contains Constants and IDs. These constants
    must match with those in Parser.l file */
#include "SQLParserStruct.h"
#include "common.h"


/* #include Other User heler file MExpr.h */
#include "MExpr.h"

#if 1
/* Test Math Expressions (Math Expressions, Inequalities and Logical Expressions ) */
int 
main (int argc, char **argv) {

    parse_init();

    while (1) {
        
        printf ("Type Expr: ");
        
        fgets (lex_buffer, sizeof (lex_buffer), stdin);

        if (lex_buffer[0] == '\n') {
            lex_buffer[0] = 0;
            continue;
        }

        lex_set_scan_buffer (lex_buffer);

        do {

            err = PARSER_CALL(S);

            if (err == PARSE_SUCCESS) {

                printf ("Logical Expression Parsed Successfully\n");
                break;
            }

            err = PARSER_CALL(Q);

            if (err == PARSE_SUCCESS) {

                printf ("Inequality Expression Parsed Successfully\n");
                break;
            }

            err = PARSER_CALL(E);

            if (err == PARSE_SUCCESS) {

                printf ("Math Expression Parsed Successfully\n");
                break;
            }

        } while(0);

        if (err == PARSE_ERR) {

            printf ("Failed to parse Expression\n");
            assert (curr_ptr == lex_buffer);
            continue;
        }        

        /* Let us print the tokens*/
        int len; void *value;
        ITERATE_LEX_STACK_BEGIN(0 , INT32_MAX , token_code, len, value) {

            printf ("token_code = %d,  len = %d,  value = %s\n", token_code, len, (char *)value);

        } ITERATE_LEX_STACK_END;

        Parser_stack_reset();
    }
}
#endif 