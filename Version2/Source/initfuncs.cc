
// $Id$

#include "initfuncs.h"
//@Include: initfuncs.h

/** @name initfuncs.cc
    @type File
    @args \ 

   Implementation of internal functions.

 */

//@{

void cond_c(const pos_param **pp, int np, result &x)
{
  DCASSERT(pp);
  result b;
  if (NULL==pp[0]) {
    x.null = true;
    return;
  } 
  pp[0]->Compute(0, b);
  if (b.bvalue) {
    if (NULL==pp[1]) x.null = true;
    else pp[1]->Compute(0, x);
  } else {
    if (NULL==pp[2]) x.null = true;
    else pp[2]->Compute(0, x);
  }
}

void cond_s(long &s, const pos_param **pp, int np, result &x)
{
  DCASSERT(pp);
  result b;
  if (NULL==pp[0]) {
    x.null = true;
    return;
  } 
  pp[0]->Sample(s, 0, b);
  if (b.bvalue) {
    if (NULL==pp[1]) x.null = true;
    else pp[1]->Sample(s, 0, x);
  } else {
    if (NULL==pp[2]) x.null = true;
    else pp[2]->Sample(s, 0, x);
  }
}

internal_func* MakeCond(type t)
{
  formal_param *pl[3];
  pl[0] = new formal_param(NULL, -1, BOOL, "b");
  pl[1] = new formal_param(NULL, -1, t, "then");
  pl[2] = new formal_param(NULL, -1, t, "else");
  internal_func *f = new internal_func(t, "cond", cond_c, cond_s, pl, 3, 4);

  return f;
}

void InitFuncs(internal_func **table, int tabsize)
{
  table[0] = MakeCond(INT);
}

//@}

