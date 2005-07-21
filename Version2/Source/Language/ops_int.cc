
// $Id$

#include "ops_int.h"

//@Include: ops_int.h

/** @name ops_int.cc
    @type File
    @args \ 

   Implementation of operator classes, for integers.

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                                                                *
// *                        int_neg  methods                        *
// *                                                                *
// ******************************************************************

void int_neg::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  if (x.answer->isNormal() || x.answer->isInfinity()) {
    // This is the right thing to do even for infinity.
    x.answer->ivalue = -x.answer->ivalue;
  }
}

// ******************************************************************
// *                                                                *
// *                        int_add  methods                        *
// *                                                                *
// ******************************************************************

void int_add::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* sum = x.answer;

  operands[0]->Compute(x);
  if (sum->isNull() || sum->isError()) return;  // short circuit
  bool unknown = false;
  if (sum->isUnknown()) {
    unknown = true;
    sum->Clear();
  }
  int i=0;
  result foo;
  x.answer = &foo;
  if (!sum->isInfinity()) {
    // Sum until we run out of operands or hit an infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
	// normal finite addition
        sum->ivalue += foo.ivalue;  
	continue;
      }
      if (foo.isInfinity()) {
	// infinite addition
	unknown = false;  // infinity + ? = infinity
        (*sum) = foo;
        break;  
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (foo.isNull()) {
        sum->setNull();
	x.answer = sum;
        return;  // null...short circuit
      }
      // must be an error
      sum->setError();
      x.answer = sum;
      return;  // error...short circuit
    } // for i
  }


  // sum so far is +/- infinity, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity - infinity.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(sum->isInfinity());
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (foo.isNormal() || foo.isUnknown()) continue;  // most likely case
    if (foo.isInfinity()) {
      // check operand for opposite sign for infinity
      if ( (sum->ivalue>0) != (foo.ivalue>0) ) {
	  Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
	  Error << "Undefined operation (infty-infty) due to ";
	  Error << operands[i];
	  Error.Stop();
	  sum->setError();
	  x.answer = sum;
	  return;
      }
    } // infinity
    if (foo.isNull()) {
      sum->setNull();
      x.answer = sum;
      return;  // null...short circuit
    }
    // must be an error
    sum->setError();
    x.answer = sum;
    return;  // error...short circuit
  } // for i
  if (unknown) sum->setUnknown();
  x.answer = sum;
}

// ******************************************************************
// *                                                                *
// *                        int_sub  methods                        *
// *                                                                *
// ******************************************************************

void int_sub::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
  result lv;
  result rv;
  result* answer = x.answer;
  answer->Clear();
 
  x.answer = &lv;
  left->Compute(x);
  x.answer = &rv;
  right->Compute(x);
  x.answer = answer;

  if (lv.isNormal() && rv.isNormal()) {
    // ordinary integer subtraction
    x.answer->ivalue = lv.ivalue - rv.ivalue;
    return;
  }
  if (lv.isInfinity() && rv.isInfinity()) {
    // both infinity
    if ((lv.ivalue > 0) != (rv.ivalue >0)) {
      x.answer->setInfinity();
      x.answer->ivalue = lv.ivalue;
      return;
    }
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (infty-infty) due to " << right;
    Error.Stop();
    x.answer->setError();
    return;
  }
  if (lv.isInfinity()) {
    // one infinity
    x.answer->setInfinity();
    x.answer->ivalue = lv.ivalue;
    return;
  }
  if (rv.isInfinity()) {
    // one infinity
    x.answer->setInfinity();
    x.answer->ivalue = -rv.ivalue;
    return;
  }
  if (lv.isUnknown() || rv.isUnknown()) {
    x.answer->setUnknown();
    return;
  }
  if (lv.isNull() || rv.isNull()) {
    x.answer->setNull();
    return;
  }
  x.answer->setError();
}

// ******************************************************************
// *                                                                *
// *                        int_mult methods                        *
// *                                                                *
// ******************************************************************

void int_mult::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* prod = x.answer;

  operands[0]->Compute(x);
  if (prod->isError() || prod->isNull()) return;
  bool unknown = prod->isUnknown();
  if (unknown) {
    prod->Clear();
    prod->ivalue = 1;
  }
  int i=0;

  //
  // Three states of computation: finite multiply, zero, infinity
  //  
  // finite * zero -> zero,
  // finite * infinity -> infinity,
  // zero * infinity -> error,
  // 
  // infinity multiply: only keep track of sign, check for errors
  //
  // zero multiply: check only for errors 
  //
  result foo;
  x.answer = &foo;

  if (prod->isNormal() && prod->ivalue) {
    // Multiply until we run out of operands or change state
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
	if (0==foo.ivalue) {
	  prod->ivalue = 0;
          unknown = false;
	  break;  // change state
	} else {
	  // normal finite multiply
	  prod->ivalue *= foo.ivalue;
	}
	continue;
      }
      if (foo.isInfinity()) {
	// fix sign and change state
	prod->ivalue = SIGN(prod->ivalue) * SIGN(foo.ivalue);
	prod->setInfinity();
	break;
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (foo.isNull()) {
	prod->setNull();
     	x.answer = prod;
	return; // short circuit
      }
      // must be an error
      prod->setError();
      x.answer = prod;
      return; // short circuit
    } // for i
  }

  // The infinity case
  if (prod->isInfinity()) {
    // Keep multiplying, only worry about sign and make sure we don't hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal() || foo.isInfinity()) {
	if (0==foo.ivalue) {
	  // 0 * infinity, error
          Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
          Error << "Undefined operation (0 * infty) due to ";
          Error << operands[i];
          Error.Stop();
          prod->setError();
   	  x.answer = prod;
	  return;
	} else {
	  // fix sign
	  prod->ivalue *= SIGN(foo.ivalue);
	}
	continue;
      }
      if (foo.isUnknown()) {
	unknown = true;  // can't be sure of sign
	continue;
      }
      if (foo.isNull()) {
	prod->setNull();
        x.answer = prod;
	return; // short circuit
      }
      // must be an error
      prod->setError();
      x.answer = prod;
      return;
    } // for i
  } 

  // The zero case
  if (prod->ivalue == 0) {
    // Check the remaining operands, if any, and throw an
    // error if we have infinity * 0.
    for (i++; i<opnd_count; i++) {
      DCASSERT(prod->ivalue==0);
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal() || foo.isUnknown()) continue;

      // check for infinity
      if (foo.isInfinity()) {
        Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
        Error << "Undefined operation (0 * infty) due to ";
        Error << operands[i];
        Error.Stop();
        prod->setError();
	x.answer = prod;
        return;
      }
      if (foo.isNull()) {
        prod->setNull();
	x.answer = prod;
        return;  // null...short circuit
      }
      prod->setError();
      x.answer = prod;
      return;  // error...short circuit
    } // for i
  }

  // Only thing to worry about:
  if (unknown) prod->setUnknown();
  x.answer = prod;
}

// ******************************************************************
// *                                                                *
// *                        int_div  methods                        *
// *                                                                *
// ******************************************************************

void int_div::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
  result* answer = x.answer;
  result l;
  result r;
  answer->Clear();
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (l.isNormal() && r.isNormal()) {
    answer->rvalue = l.ivalue;
    if (0==r.ivalue) {
      Error.Start(right->Filename(), right->Linenumber());
      Error << "Undefined operation (divide by 0) due to " << right;
      Error.Stop();
      answer->setError();
    } else {
      answer->rvalue /= r.ivalue;
    }
    return;
  }

  if (l.isInfinity() && r.isInfinity()) {
      Error.Start(right->Filename(), right->Linenumber());
      Error << "Undefined operation (infty / infty) due to ";
      Error << left << "/" << right;
      Error.Stop();
      answer->setError();
      return;
  }

  if (l.isInfinity()) {
    // infinity / finite, check sign only
    answer->setInfinity();
    answer->ivalue = SIGN(l.ivalue) * SIGN(r.ivalue);
    return;
  }
  if (r.isInfinity()) {
    // finite / infinity = 0  
    answer->rvalue = 0.0;
    return;
  }

  if (l.isUnknown() || r.isUnknown()) {
    answer->setUnknown();
    return;
  }
  if (l.isNull() || r.isNull()) {
    answer->setNull();
    return;
  }

  answer->setError();
}

// ******************************************************************
// *                                                                *
// *                       int_equal  methods                       *
// *                                                                *
// ******************************************************************

void int_equal::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue == r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        int_neq  methods                        *
// *                                                                *
// ******************************************************************

void int_neq::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue != r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_gt methods                         *
// *                                                                *
// ******************************************************************

void int_gt::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue > r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_ge methods                         *
// *                                                                *
// ******************************************************************

void int_ge::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue >= r.ivalue);
  }
}


// ******************************************************************
// *                                                                *
// *                         int_lt methods                         *
// *                                                                *
// ******************************************************************

void int_lt::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue < r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_le methods                         *
// *                                                                *
// ******************************************************************

void int_le::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;
  result* answer = x.answer;
  x.answer = &l;
  left->Compute(x);
  x.answer = &r;
  right->Compute(x);
  x.answer = answer;

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.answer->bvalue = (l.ivalue <= r.ivalue);
  }
}

//@}

