
// $Id$

#ifndef INFINITY_H
#define INFINITY_H

/** @name infinity.h
    @type File
    @args \ 

    Constant infinity expression.
    (Infinity is type int).
 */

#include "exprs.h"

//@{
  
// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

/// Build an infinity constant (type=integer).
expr* MakeInfinityExpr(int sign, const char* file=NULL, int line=0);

//@}

#endif

