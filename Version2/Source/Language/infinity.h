
// $Id$

#ifndef INFINITY_H
#define INFINITY_H

/** @name infinity.h
    @type File
    @args \ 

    Constant infinity expression.
    (Infinity is type int).

    The only reason this is separate from "exprs"
    is that we need options (for the #InfinityString option).
 */

#include "exprs.h"

//@{
  
// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

extern option* infinity_string;

/// Build an infinity constant (type=integer).
expr* MakeInfinityExpr(int sign, const char* file, int line);

//@}

#endif

