
// $Id$

#include "arrays.h"
//@Include: arrays.h

/** @name arrays.cc
    @type File
    @args \ 

   Implementation of user-defined arrays.

 */

//@{

// ******************************************************************
// *                                                                *
// *                      array_index  methods                      *
// *                                                                *
// ******************************************************************

/*

iterator::iterator(const char *fn, int line, type t, char*n)
  : symbol(fn, line, t, n)
{
}

void iterator::Compute(int i, result &x) const
{
  DCASSERT(i==0);
  x.ivalue = value;
  // Need to check for legal bounds, etc, and set to null otherwise
}

expr* iterator::Substitute(int i)
{
  DCASSERT(i==0);
  return MakeConstExpr(value, Filename(), Linenumber());
}

void iterator::show(ostream &s) const
{
  if (NULL==Name()) return;  // hidden?
  s << GetType(Type(0)) << " " << Name();
}

*/

//@}

