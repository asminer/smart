
// $Id$

#ifndef OPERATORS_H
#define OPERATORS_H

/** @name operators.h
    @type File
    @args \ 

  Classes to deal with binary and unary operators.

 */

#include "baseops.h"

// These are necessary because they appear in smart.tab.h
class array_index;
class array;
class formal_param;
class user_func;
class statement;

#include "../Main/smart.tab.h"
/* For token identifiers such as PLUS, MINUS, etc... */



//@{
  

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

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

/**
     Build an associative expression.

     @param	op	The operator (as defined in smart.tab.h)
     @param	opnds	The operands as expressions, 
     			which must have been typecast already
			as necessary to perfectly match.
     @param	n	Number of operands.
     			Must be at least 1.
     @param	file	Source file where this is defined
     @param	line	Line number where this is defined

     @return	The appropriate new expression, or NULL if
     		we could not build the expression.

*/
expr* MakeAssocOp(int op, expr **opnds, int n, 
                     const char* file=NULL, int line=0);



/**  "Optimize" an expression.
     This takes a generic expression and tries to replace a series of 
     sums and products with an associative list of sums and products.
     @param 	i	The component to optimize
     @param	e	The expression to optimize
 */
void Optimize(int i, expr* &e);


//@}

#endif

