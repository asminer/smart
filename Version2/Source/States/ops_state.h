
// $Id$

#ifndef OPS_STATE_H
#define OPS_STATE_H

#include "../Language/models.h"
#include "stateheap.h"


/** @name ops_state.h
    @type File
    @args \ 

   Special, internally-used operator classes for dealing with states.

   There is a minimalist front end.  I.e.,
   the classes themselves are hidden.

   E.g., a specialized operator to compare a state variable to a 
   constant integer, used to determine if a Petri net
   transition is enabled or not.

   Right now, this is used only by model formalisms.

 */

//@{

/**
    Build a specialized state variable comparison.
    Technically, it is a constant, because it takes no expressions
    as "parameters".

    @param	var 	The state variable
    @param	op	The operator (as defined in smart.tab.h)
    @param	right	The right-hand constant.

    @param	f	Source file, or NULL for internal
    @param	ln	Line number, or -1 for internal

    @return	Appropriate expression of type PROC_BOOL
*/
expr* MakeConstCompare(model_var* var, int op, int right, const char* f, int ln);

/**
    Build a specialized state variable comparison.

    @param	var     The state variable
    @param	op	The operator (as defined in smart.tab.h)
    @param	right	The right-hand expression (type PROC_INT)

    @param	f	Source file, or NULL for internal
    @param	ln	Line number, or -1 for internal

    @return	Appropriate expression of type PROC_BOOL
*/
expr* MakeExprCompare(model_var* var, int op, expr* right, const char* f, int ln);

/**
    Build a specialized state variable bounding comparison.
    I.e., determine if the state var is between upper and lower bounds.

    @param	lower	The constant lower bound
    @param	var     State variable
    @param	upper	The constant upper bound

    @param	file	Source file, or NULL for internal
    @param	line	Line number, or -1 for internal

    @return	Appropriate expression of type PROC_BOOL that is true if
		lower <= opnd <= upper	
*/
expr* MakeConstBounds(int lower, model_var* var, 
			int upper, const char* f, int ln);

/**
    Build an expression to change a state variable.
*/
expr* ChangeStateVar(const char* mdl, model_var* var, int delta, const char* f, int ln);

/**
    Build an expression to change a state variable.
*/
expr* ChangeStateVar(const char* mdl, model_var* var, int OP, expr* rhs, const char* f, int ln);

#endif

//@}

