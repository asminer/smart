
// $Id$

#include "../Language/api.h"

/*
   Compile-time utilities and main function are here.
*/

// Good old lex/yacc style functions...
int yyparse();  

// Defined in smart.l:
int yylex();    
int yyerror(char *errmsg);


/// Build a boolean constant
expr* MakeBoolConst(char* s);

/// Build an integer constant
expr* MakeIntConst(char* s);

/// Build a real constant
expr* MakeRealConst(char* s);



/**
    Type-checks, promotes if necessary, and builds the desired 
    binary expression.

    @param	left	Left expression
    @param	op	Operator; use constants in smart.tab.h
    @param	right	Right expression

    @return	Desired expression.
    		If left or right are NULL, or there was an error, returns NULL.
*/

expr* BuildBinary(expr* left, int op, expr* right);


/** Builds statement list for a single expression statement.
    If the expression is NULL, we return NULL.
*/
void* BuildExprStatement(expr *x);

