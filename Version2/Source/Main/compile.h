
// $Id$

#ifndef COMPILE_H
#define COMPILE_H

#include "../Base/api.h"
#include "../Language/api.h"

/*
   Compile-time utilities and main function are here.
*/

// Good old lex/yacc style functions...
int yyparse();  

// Defined in smart.l:
int yylex();    
int yyerror(char *errmsg);

/// For use in smart.y, for simplicity
char* Filename();

/// Also used in smart.y, for simplicity
int LineNumber();

/// Build a type.  On error, returns VOID.
type MakeType(const char* modif, const char* tp);

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Expression  construction                    | 
// |                                                                |
// |                                                                |
// ==================================================================

/// Build a boolean constant
expr* MakeBoolConst(char* s);

/// Build an integer constant
expr* MakeIntConst(char* s);

/// Build a real constant
expr* MakeRealConst(char* s);

/// Build a string constant
expr* MakeStringConst(char *s);

/// Build an interval (does typechecking, etc.)
expr* BuildInterval(expr* start, expr* stop);

/// Build an interval (does typechecking, etc.)
expr* BuildInterval(expr* start, expr* stop, expr* inc);

/// Add two sets together (with typechecking and promotions)
expr* AppendSetElem(expr* left, expr* right);

/// Build a for-loop iterator (with typechecking and promotions)
array_index* BuildIterator(type t, char* n, expr* values);

/**
    Type-checks and builds the desired unary expression.

    @param	op	Operator; use constants in smart.tab.h
    @param	opnd	Operand.

    @return	Desired expression.
    		Returns NULL if opnd is NULL, 
		ERROR if there was an error.

*/
expr* BuildUnary(int op, expr* opnd);

/**
    Type-checks, promotes if necessary, and builds the desired 
    binary expression.

    @param	left	Left expression
    @param	op	Operator; use constants in smart.tab.h
    @param	right	Right expression

    @return	Desired expression.
    		If left or right are NULL, returns NULL.
		If left or right are ERROR, or there was an error,
		returns ERROR.
*/
expr* BuildBinary(expr* left, int op, expr* right);

/**
    Type-checks and builds the desired typecast.

    @param	opnd	Operand expression.
    @param	newtype	Desired new type of the expression.

    @return	Desired expression, ERROR on an error.
*/
expr* BuildTypecast(expr* opnd, type newtype);

/**
	Start an aggregate list.
	@param	a	First expression
	@param	b	Second expression

	@return	If a or b is NULL, then return NULL;
		If a or b is ERROR, return ERROR;
		else return a pointer to a new list, containing a and b.
*/
void* StartAggregate(expr *a, expr *b);

/**
	Add to an aggregate list.
	@param x	A pointer to a list of expressions
	@param b	Expression to add

	@return	If b is NULL, return NULL and trash x;
		If b is ERROR or x is ERROR, returns ERROR;
		otherwise, adds b to x and returns x.
*/
void* AddAggregate(void *x, expr *b);

/**
	Make an aggregate expression.
	@param	x	A pointer to a List of expressions

	@return	If x is NULL, then return NULL;
		if x is ERROR, then return ERROR;
		else return an aggregate expression.
*/
expr* BuildAggregate(void* x);

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Statement   construction                    | 
// |                                                                |
// |                                                                |
// ==================================================================

/**
  Add iterator to our stack of iterators (unless it is NULL).

  @param	i	Iterator to add to stack.
  @return	1 if iterator was added,
  		0 if it was not (i.e., if i is NULL or ERROR)
*/
int AddIterator(array_index* i);

/**
    Remove iterators from our stack, and roll them into a for loop.
    @param	count	Number of iterators to remove.
    @param	stmts	Block of statements inside the loop.
    @return	For loop statement
*/
statement* BuildForLoop(int count, void* stmts);

/** Builds an expression statement.
    If the expression is NULL or ERROR, we return NULL.
*/
statement* BuildExprStatement(expr *x);

/** Builds an array assignment statement with typechecking.
    @param	a	The array
    @param	e	The expression
    @return	NULL if a is NULL or e is ERROR,
    		otherwise an array assignment statement
*/
statement* BuildArrayStmt(array *a, expr *e);

/** Builds a function "statement" with typechecking.
    @param	f	The function
    @param	r	The return expression
    @return	NULL
*/
statement* BuildFuncStmt(user_func *f, expr *r);

/** Builds a "variable" statement with typechecking.
    If we're not within a converge, we return NULL.
    Otherwise, we return an assignment (eventually).
*/
statement* BuildVarStmt(type t, char* id, expr* ret);

/** Adds a statement to our list (which may be null).
    @param list	List of statements (or NULL)
    @param s	statement to add (ignored if NULL)
    @return	If list is NULL, returns a new list containing s.
    		Else returns list with s appended.
*/
void* AppendStatement(void* list, statement* s);

// ==================================================================
// |                                                                |
// |                                                                |
// |                        Parameter  lists                        | 
// |                                                                |
// |                                                                |
// ==================================================================

/// Add a formal index (name) to our list.
void* AddFormalIndex(void* list, char* ident);

/// Add a passed parameter/index to our list.
void* AddParameter(void* list, expr* pass);

/// Add a formal parameter to our list
void* AddParameter(void* list, formal_param* fp);

/// Build an array "header"
array* BuildArray(type t, char* n, void* list);

/// Build a function "header"
user_func* BuildFunction(type t, char* n, void* list);

/// Build a formal parameter (with typechecking)
formal_param* BuildFormal(type t, char* name);

/// Build a formal parameter (with typechecking)
formal_param* BuildFormal(type t, char* name, expr* deflt);


// ==================================================================
// |                                                                |
// |                                                                |
// |                         Function calls                         | 
// |                                                                |
// |                                                                |
// ==================================================================

/** Find the best "function with no parameters".
    We check formal parameters, for loop iterators, and user-defined
    functions; if not found, we'll return NULL.
    @param	name	Identifier name
    @return	Function call or NULL.
*/
expr* FindIdent(char* name);

/** 	Build an array "call".
	@param	n	The name of the array
	@param	ind	list of indexes
	@return NULL on error (e.g., no array with that name); 
		otherwise the array call expression.
*/
expr* BuildArrayCall(const char* n, void* ind);

/**	Build a function call.
	Does type checking, promotion, overloading, and everything.
	@param	n	The function name
	@param	ind	List of passed (positional) parameters
	@return	NULL on error (e.g., bad parameters, etc);
		otherwise the function call expression.
*/
expr* BuildFunctionCall(const char* n, void* posparams);

/**	Build a function call (named parameter version).
	Does type checking, promotion, overloading, and everything.
	@param	n	The function name
	@param	ind	List of passed (named) parameters
	@return	NULL on error (e.g., bad parameters, etc);
		otherwise the function call expression.
*/
expr* BuildNamedFunctionCall(const char* n, void* namedparams);


// ==================================================================
// |                                                                |
// |                             Options                            | 
// |                                                                |
// ==================================================================

/** Find option with specified name.
    If none exists, print an error message and return NULL.
*/
option* BuildOptionHeader(char* name);

/** Build an option statement.
    Does type checking on the option and the value.
    On error, displays an error message and returns NULL.
*/
statement* BuildOptionStatement(option* o, expr* v);

/** Build an (enumerated) option statement.
    Does type checking on the option and the value.
    On error, displays an error message and returns NULL.
*/
statement* BuildOptionStatement(option* o, char* n);

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Initialize compiler data                    | 
// |                                                                |
// |                                                                |
// ==================================================================

void InitCompiler();

#endif
