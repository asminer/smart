
// $Id$

#ifndef CASTING_H
#define CASTING_H

/** @name casting.h
    @type File
    @args \ 

    Expressions that deal with typecasting between types.
 */

#include "exprs.h"

//@{
  


// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/** 
     Build a typecast expression.
     If e is not promotable to the specified type, we return NULL.
     If e is already of the specified type, we return it.

     @param	e		The original expression.
     @param	newtype		The desired new type.
     @param	file		Source file where this is defined
     @param	line		Line number where this is defined
     
     @return	An expression casting the original one into the new type.
     		If this is impossible we return NULL.
		If the original expression has the same type,
		it is returned unchanged.
     			
 */
expr* MakeTypecast(expr *e, type newtype, const char* file, int line);


/** 
     Build a typecast expression for aggregates.
     If a component of e is not promotable as requested, we return NULL.
     If all components of e are already of the specified type, we return it.

     @param	e		The original expression.
     @param	fp		The expression to match;
     				both e and fp must have the same number
				of components.
     @param	file		Source file where this is defined
     @param	line		Line number where this is defined
     
     @return	An expression casting the original one into the new type.
     		If this is impossible we return NULL.
		If the original expression has the same type,
		it is returned unchanged.
     			
 */
expr* MakeTypecast(expr *e, const expr* fp, const char* file, int line);


//@}

#endif

