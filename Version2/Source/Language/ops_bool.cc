
// $Id$

#include "ops_bool.h"

//@Include: ops_bool.h

/** @name ops_bool.cc
    @type File
    @args \ 

   Implementation of operator classes, for bool variables.

 */

//@{

// ******************************************************************
// *                                                                *
// *                        bool_not methods                        *
// *                                                                *
// ******************************************************************

void bool_not::Compute(Rng *r, const state *st, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(r, st, 0, x); 

  if (!x.isNormal()) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                        bool_or  methods                        *
// *                                                                *
// ******************************************************************

void bool_or::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(r, st, 0, x);
    if (x.isNormal() && x.bvalue) 	return;	// true...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.isNormal())  return; // error or null, short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        bool_and methods                        *
// *                                                                *
// ******************************************************************

void bool_and::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(r, st, 0, x);
    if (x.isNormal() && !x.bvalue) 	return;	// false...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.isNormal())  return; // error or null, short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                       bool_equal methods                       *
// *                                                                *
// ******************************************************************

void bool_equal::Compute(Rng *r, const state *st, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);

  result lv;
  result rv;

  left->Compute(r, st, 0, lv); 
  right->Compute(r, st, 0, rv); 

  if (CheckOpnds(l, rv, xv)) {
    x.bvalue = (lv.bvalue == rv.bvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        bool_neq methods                        *
// *                                                                *
// ******************************************************************

void bool_neq::Compute(Rng *r, const state *st, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);

  result lv;
  result rv;
  
  left->Compute(r, st, 0, lv);
  right->Compute(r, st, 0, rv);

  if (CheckOpnds(lv, rv, x)) {
    x.bvalue = (lv.bvalue != rv.bvalue);
  }
}


//@}

