
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

array_index::array_index(const char *fn, int line, type t, char *n, expr *v) 
  : symbol(fn, line, t, n)
{
  values = v;
  current = NULL;
}

array_index::~array_index()
{
  Delete(values);
  Delete(current);
}

void array_index::Compute(int i, result &x)
{
  DCASSERT(i==0);
  x.Clear();
  if (NULL==current) {
    x.null = true;
    // set error condition?
    return;
  }
  current->GetElement(index, x);
}

void array_index::show(ostream &s) const
{
  s << Name();
}


//@}

