
// $Id$

#ifndef OPERATORS_H
#define OPERATORS_H

/** @name operators.h
    @type File
    @args \ 

  Classes to deal with binary and unary operators.

 */

#include "baseops.h"

//@{
  

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

// eventually these will be in smart.tab.h

const int NOT = 1;
const int NEG = 2;
const int PLUS = 3;
const int MINUS = 4;
const int TIMES = 5;
const int DIVIDE = 6;
const int EQUALS = 7;
const int NEQ = 8;
const int GT = 9;
const int GE = 10;
const int LT = 11;
const int LE = 12;

/**
     Build a unary expression.

     @param	op	The operator (as defined in smart.tab.h)
     @param	opnd	The operand, already of the proper type.
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

     Note: the only unary operators currently are NOT and unary minus.
*/
expr* MakeUnaryOp(int op, expr *opnd, const char* file=NULL, int line=0);

/**
     Build a binary expression.

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
expr* MakeBinaryOp(expr *left, int op, expr *right, 
                     const char* file=NULL, int line=0);



//@}

#endif

