
// $Id$

#ifndef CASTING_H
#define CASTING_H

/** @name casting.h
    @type File
    @args \ 

    Expressions that deal with typecasting between types:
    bool, int, real, string.
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
     @param	e		The original expression.
     @param	newtype		The desired new type.
     				Specifically, one of:
				bool, int, real.
     @param	file		Source file where this is defined
     @param	line		Line number where this is defined
     
     @return	An expression casting the original one into the new type.
     		If this is impossible we return NULL.
		If the original expression has the same type,
		it is returned unchanged.
     			
 */
expr* SimpleTypecast(expr *e, type newtype, const char* file=NULL, int line=0);


//@}

#endif

