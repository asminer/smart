
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

void bool_not::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x); 

  if (!x.answer->isNormal()) return;

  x.answer->bvalue = !x.answer->bvalue;
}

// ******************************************************************
// *                                                                *
// *                        bool_or  methods                        *
// *                                                                *
// ******************************************************************

void bool_or::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (x.answer->isNormal() && x.answer->bvalue) 	return;	//short circuit
    if (x.answer->isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.answer->isNormal())  return; // error or null, short circuit
  } // for i
  if (unknown) x.answer->setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        bool_and methods                        *
// *                                                                *
// ******************************************************************

void bool_and::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (x.answer->isNormal() && !x.answer->bvalue) 	return;	//short circuit
    if (x.answer->isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.answer->isNormal())  return; // error or null, short circuit
  } // for i
  if (unknown) x.answer->setUnknown();
}

// ******************************************************************
// *                                                                *
// *                       bool_equal methods                       *
// *                                                                *
// ******************************************************************

void bool_equal::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);

  result* answer = x.answer;
  result lv;
  result rv;

  x.answer = &lv; 
  left->Compute(x); 
  x.answer = &rv;
  right->Compute(x); 
  x.answer = answer;

  if (CheckOpnds(lv, rv, x)) {
    x.answer->bvalue = (lv.bvalue == rv.bvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        bool_neq methods                        *
// *                                                                *
// ******************************************************************

void bool_neq::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);

  result* answer = x.answer;
  result lv;
  result rv;
  
  x.answer = &lv;
  left->Compute(x);
  x.answer = &rv;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(lv, rv, x)) {
    x.answer->bvalue = (lv.bvalue != rv.bvalue);
  }
}


//@}

