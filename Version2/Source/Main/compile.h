
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
    		Returns NULL if opnd is NULL, or there was an error.

*/
expr* BuildUnary(int op, expr* opnd);

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

/**
    Type-checks and builds the desired typecast.

    @param	opnd	Operand expression.
    @param	newtype	Desired new type of the expression.

    @return	Desired expression, or NULL on an error.
*/
expr* BuildTypecast(expr* opnd, type newtype);

/**
	Start an aggregate list.
	@param	a	First expression
	@param	b	Second expression

	@return	If a or b is NULL, then return NULL;
		else return a pointer to a new list, containing a and b.
*/
void* StartAggregate(expr *a, expr *b);

/**
	Add to an aggregate list.
	@param x	A pointer to a list of expressions
	@param b	Expression to add

	@return	If b is NULL, return NULL and trash x;
		otherwise, adds b to x and returns x.
*/
void* AddAggregate(void *x, expr *b);

/**
	Make an aggregate expression.
	@param	x	A pointer to a List of expressions

	@return	If x is NULL, then return NULL;
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
  		0 if it was not (i.e., if i is NULL)
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
    If the expression is NULL, we return NULL.
*/
statement* BuildExprStatement(expr *x);

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

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Initialize compiler data                    | 
// |                                                                |
// |                                                                |
// ==================================================================

void InitCompiler();

