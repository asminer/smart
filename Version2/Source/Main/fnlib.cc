
// $Id$

#include "fnlib.h"

void computecond(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  if (NULL==pp[0]) {
    // some type of error stuff?
    x.null = true;
    return;
  }
  result b;
  b.Clear();
  pp[0]->Compute(0, b);
  if (b.null || b.error) {
    // error stuff?
    x = b;
    return;
  }
  if (b.bvalue) pp[1]->Compute(0, x);
  else pp[2]->Compute(0, x); 
}

void samplecond(long &seed, expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(pp);
  DCASSERT(np==3);
  if (NULL==pp[0]) {
    x.null = true;
    return;
  }
  result b;
  b.Clear();
  pp[0]->Sample(seed, 0, b);
  if (b.null || b.error) {
    x = b;
    return;
  }
  if (b.bvalue) pp[1]->Sample(seed, 0, x);
  else pp[2]->Sample(seed, 0, x); 
}

void AddCond(type t, PtrTable *fns)
{
  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(NULL, -1, BOOL, "b");
  pl[1] = new formal_param(NULL, -1, t, "t");
  pl[2] = new formal_param(NULL, -1, t, "f");

  internal_func *cnd = 
    new internal_func(t, "cond", computecond, samplecond, pl, 3, -1);

  InsertFunction(fns, cnd);
}

void InitBuiltinFunctions(PtrTable *t)
{
  // Conditionals
  type i;
  for (i=FIRST_SIMPLE; i<=LAST_SIMPLE; i++)	AddCond(i, t);
  for (i=FIRST_PROC; i<=LAST_PROC; i++)		AddCond(i, t);
  for (i=FIRST_VOID; i<=LAST_VOID; i++)		AddCond(i, t);
}

