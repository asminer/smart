
// $Id$

#ifndef STRINGS_H
#define STRINGS_H

/** @name strings.h
    @type File
    @args \ 

    String operators.
 */

#include "exprs.h"

//@{
  
// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/**
    Build an associative sum of strings expression.

    @param	opnds	The operands as expressions,
    			all with type STRING already.
    @param	n	Number of operands.
    			Must be at least 1.
    @param	file	Source file where this is defined
    @param	line	Line number where this is defined

    @return 	The appropriate expression, or NULL if we couldn't build it.
*/
expr* MakeStringAdd(expr** opnds, int n, const char* file, int line);

/**
     Build a binary expression for strings.

     @param	left	The left-hand expression (type STRING).
     @param	op	The operator (as defined in smart.tab.h)
     			Must be one of:
			EQUALS
			NEQUAL
			GT
			GE
			LT
			LE
			
     @param	right	The right-hand expression (type STRING).
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

*/
expr* MakeStringBinary(expr* left, int op, expr* right, const char* file, int
line);

//@}

#endif

