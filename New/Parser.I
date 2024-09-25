/* 
 * Usage: (1) $ flex Parser.lex
 *        (2) $ gcc lex.yy.c -lfl
 *        (3) $ ./a.out
 *            stdin> username
 *	      stdin> Ctrl-D
 * Question: What is the purpose of '%{' and '%}'?
 *           What else could be included in this section?
 */

%{
// c code declarations defitions

// define all supportive data structures BEGIN


// define all supportive data structures END

// all global variables which parser will use
#ifdef __cplusplus
extern "C" {
#endif

int yylex();

#ifdef __cplusplus
}
#endif

extern char lex_buffer[512];

char lex_buffer[512];

#include "MexprEnums.h"
%}

%%
\n {
	return PARSER_EOL;
}

0|-?[1-9][0-9]* {  // regular expressions to generate tokens
	return MATH_CPP_INT;
}

-?[0-9]*\.[0-9]+ {
	MATH_CPP_DOUBLE;
}

[a-zA-z0-9_]+ {
	return MATH_CPP_VARIABLE;
}


(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9])\.(25[0-5]|2[0-4][0-9]|1[0-9][0-9]|[1-9][0-9]|[0-9]) {

    return MATH_CPP_IPV4;
}

[ ] {
}

. {

	printf("ignore any other non-parsable character - %s\n", yytext);
}
%%

int main (int argc) {
	while (1) {
		printf("Input->");
		fgets(lex_buffer, sizeof(lex_buffer), stdin);
		if (lex_buffer[0]=='\n') {
			lex_buffer[0] = 0;
			continue;
		}
		
		yy_scan_string(lex_buffer);
		int token_code;
		
		token_code = yylex();
		
		while(token_code != PARSER_EOL) {
			printf("token_code=%d, token=%s, token_len=%d\n", token_code, yytext, yyleng);
			token_code = yylex();
		}
	}

	yy_scan_string(lex_buffer);
}
