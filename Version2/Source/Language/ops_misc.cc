
// $Id$

#include "ops_misc.h"

//@Include: ops_misc.h

/** @name ops_misc.cc
    @type File
    @args \ 

   Implementation of operator classes, for other types

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                                                                *
// *                        void_seq methods                        *
// *                                                                *
// ******************************************************************

void void_seq::Compute(Rng *, const state *, int a, result &x)
{
  DCASSERT(0==a);
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], NULL, NULL, 0, x);
    // check for errors and bail?
  }
}



//@}

