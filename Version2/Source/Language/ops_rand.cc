
// $Id$

#include "ops_rand.h"

//@Include: ops_rand.h

/** @name ops_rand.cc
    @type File
    @args \ 

   Implementation of operator classes, for random variables.

 */

//@{

// ******************************************************************
// *                                                                *
// *                      randbool_not methods                      *
// *                                                                *
// ******************************************************************

void randbool_not::Sample(Rng &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Sample(seed, 0, x);

  if (!x.isNormal()) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                      randbool_or  methods                      *
// *                                                                *
// ******************************************************************

void randbool_or::Sample(Rng &seed, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
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
// *                      randbool_and methods                      *
// *                                                                *
// ******************************************************************

void randbool_and::Sample(Rng &seed, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
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
// *                     randbool_equal methods                     *
// *                                                                *
// ******************************************************************

void randbool_equal::Sample(Rng &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Sample(seed, 0, l); 
  right->Sample(seed, 0, r);

  if (l.isNormal() && r.isNormal()) {
    x.bvalue = (l.bvalue == r.bvalue);
    return;
  }

  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  x.setError();
}

// ******************************************************************
// *                                                                *
// *                      randbool_neq methods                      *
// *                                                                *
// ******************************************************************

void randbool_neq::Sample(Rng &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Sample(seed, 0, l);
  right->Sample(seed, 0, r);

  if (l.isNormal() && r.isNormal()) {
    x.bvalue = (l.bvalue != r.bvalue);
    return;
  }

  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  x.setError();
}


//@}

