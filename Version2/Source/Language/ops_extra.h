
// $Id$

#ifndef OPS_EXTRA_H
#define OPS_EXTRA_H

#include "exprs.h"


/** @name ops_extra.h
    @type File
    @args \ 

   Special, internally-used operator classes.

   There is a minimalist front end.  I.e.,
   the classes themselves are hidden.

   E.g., a specialized operator to compare "proc int" to a 
   constant integer, used to determine if a Petri net
   transition is enabled or not.

   Right now, this is used only by model formalisms.

 */

//@{

/**
    Build a specialized comparison.
    Technically, it is a unary operation, since we are comparing to
    a constant.

    @param	left	The constant left-hand side
    @param	op	The operator (as defined in smart.tab.h)
    @param	right	The right-hand expression.
			Must be of type PROC_INT.

    @param	file	Source file, or NULL for internal
    @param	line	Line number, or -1 for internal

    @return	Appropriate expression of type PROC_BOOL
*/
expr* MakeConstCompare(int left, int op, expr* right, const char* f, int ln);

/**
    Build a specialized bounding comparison.
    I.e., determine if one expression is between upper and lower bounds.
    Technically, it is a unary operation, since we are comparing to
    constants.

    @param	lower	The constant lower bound
    @param	opnd	The expression to check.
			Must be of type PROC_INT.
    @param	upper	The constant upper bound

    @param	file	Source file, or NULL for internal
    @param	line	Line number, or -1 for internal

    @return	Appropriate expression of type PROC_BOOL that is true if
		lower <= opnd <= upper	
*/
expr* MakeConstBounds(int lower, expr* opnd, int upper, const char* f, int ln);


#endif

//@}

