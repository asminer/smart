
// $Id$

#ifndef COMPILE_H
#define COMPILE_H

#include "../Base/api.h"
#include "../Language/api.h"

/*
   Compile-time utilities and main function are here.
*/

/// Info for a function call
struct func_call {
  function* find;
  expr** pass;
  int np;
};

// Good old lex/yacc style functions...
int yyparse();  

// Defined in smart.l:
int yylex();    
int yyerror(char *errmsg);

/// For use in smart.y, for simplicity
char* Filename();

/** Is the input file from standard input?
    If so we flush output more frequently
*/
bool IsInteractive();

/// Also used in smart.y, for simplicity
int LineNumber();

/// Build a type.  On error, returns NO_SUCH_TYPE.
type MakeType(bool proc, const char* modif, const char* tp);


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

/// Build a set element (ensures legal types)
expr* BuildElementSet(expr* elem);

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

/**
	Start a sequence list.
	@param	a	First expression, must have type VOID
	@param	b	Second expression, must have type VOID

	@return	If a or b is NULL, then return NULL;
		If a or b is ERROR, return ERROR;
		else return a pointer to a new list, containing a and b.
*/
void* StartSequence(expr *a, expr *b);

/**
	Add to a sequence list.
	@param x	A pointer to a list of expressions
	@param b	Expression to add, must have type VOID

	@return	If b is NULL, return NULL and trash x;
		If b is ERROR or x is ERROR, returns ERROR;
		otherwise, adds b to x and returns x.
*/
void* AddToSequence(void *x, expr *b);

/**
	Make a sequence expression.
	@param	x	A pointer to a List of expressions (of type VOID)

	@return	If x is NULL, then return NULL;
		if x is ERROR, then return ERROR;
		else return a sequence expression.
*/
expr* BuildSequence(void* x);


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

/** Used when f is "forward defined".
    This allows us to clear the formal parameters when we're done with them.
*/
void DoneWithFunctionHeader(user_func *f);

/** Builds a "variable" statement with typechecking.
    If we're not within a converge, we return NULL.
    Otherwise, we return an assignment (eventually).
*/
statement* BuildVarStmt(type t, char* id, expr* ret);

/** Builds a "guess" statement with typechecking.
*/
statement* BuildGuessStmt(type t, char* id, expr* ret);

/** Builds an array "guess" statement with typechecking.
*/
statement* BuildArrayGuess(array* id, expr* ret);

/** Adds a statement to our list (which may be null).
    @param list	List of statements (or NULL)
    @param s	statement to add (ignored if NULL)
    @return	If list is NULL, returns a new list containing s.
    		Else returns list with s appended.
*/
void* AppendStatement(void* list, statement* s);

/** Everything from now on is within a converge statement.
*/
void StartConverge();

/** Complete the converge statement (started with StartConverge).
    @param list		List of statements (or NULL)
*/
statement* FinishConverge(void* list);

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

/// Add a named parameter to our list (kept sorted by name)
void* AddParameter(void* list, named_param* np);

/// Build an array "header"
array* BuildArray(type t, char* n, void* list);

/// Build a function "header"
user_func* BuildFunction(type t, char* n, void* list);

/// Build a formal parameter (with typechecking)
formal_param* BuildFormal(type t, char* name);

/// Build a formal parameter (with typechecking)
formal_param* BuildFormal(type t, char* name, expr* deflt);

/// Build a named parameter (passed)
named_param* BuildNamedParam(char* name, expr* pass);

/// Build a default named paramater (passed)
named_param* BuildNamedDefault(char* name);

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
// |                       Model construction                       | 
// |                                                                |
// |                                                                |
// ==================================================================

/// Build a model "header"
model* BuildModel(type t, char* n, void* list);


/** Builds a function "statement" with typechecking.
    @param	f	The function
    @param	block	List of statements within the model
    @return	NULL
*/
statement* BuildModelStmt(model *m, void* block);

/** Add a model variable (name) to our list.
    If this is impossible (i.e., illegal), we do nothing.
    The name is also added to the model's internal symbol table.
*/
void* AddModelVariable(void* list, char* ident);

/** Add a model array (name) to our list.
    The name is also added to the model's internal symbol table.
    The index list is checked against the enclosing for statements.
*/
void* AddModelArray(void* list, char* ident, void* indexlist);

/** Build a statement to declare model variables.
    If we're within a for-loop, these will be arrays
    (we already verified the indices in "AddModelArray").
*/
statement* BuildModelVarStmt(type t, void* list);

/** Handle the first half of a model call.
    No parameters.
*/
func_call* MakeModelCall(char* n);

/** Handle the first half of a model call.
    Positional parameters.
*/
func_call* MakeModelCallPos(char* n, void* list);

/** Handle the first half of a model call.
    Named parameters.
*/
func_call* MakeModelCallNamed(char* n, void* list);

/** Build a measure call.
    Form (funccall).measure
    Makes sure that the measure (m) exists in the model (within struct fc).
*/
expr* MakeMCall(func_call *fc, char* m);

/** Build a measure array call.
    Form (funccall).marray[indices]
    Makes sure that the measure (m) exists in the model (within struct fc),
    and matches array indices.
*/
expr* MakeMACall(func_call *, char* n, void *);

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Initialize compiler data                    | 
// |                                                                |
// |                                                                |
// ==================================================================

void InitCompiler();

#endif
