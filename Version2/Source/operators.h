
// $Id$

#ifndef OPERATORS_H
#define OPERATORS_H

/** @name operators.h
    @type File
    @args \ 

  Classes to deal with binary and unary operators,
  for types: bool, int, real, string.

 */

#include "exprs.h"

//@{
  

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/**
     Build a unary expression for simple types.
     The simple types supported are:
     bool, int, real.

     @param	op	The operator (as defined in smart.tab.h)
     @param	opnd	The operand, already of the proper type.
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

     Note: the only unary operators currently are NOT and unary minus.
*/
expr* SimpleUnaryOp(int op, expr *opnd, const char* file=NULL, int line=0);

/**
     Build a binary expression for simple types.
     The left and right expressions already match types.
     The simple types supported are:
     bool, int, real.

     @param	left	The left-hand expression.
     @param	op	The operator (as defined in smart.tab.h)
     @param	right	The right-hand expression.
     			left and right are already typecast, if necessary,
			to perfectly match (e.g., both reals).
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

*/
expr* SimpleBinaryOp(expr *left, int op, expr *right, 
                     const char* file=NULL, int line=0);

//@}

#endif

