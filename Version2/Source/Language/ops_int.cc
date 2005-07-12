
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

void int_neg::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(r, st, 0, x);

  if (x.isNormal() || x.isInfinity()) {
    // This is the right thing to do even for infinity.
    x.ivalue = -x.ivalue;
  }
}

// ******************************************************************
// *                                                                *
// *                        int_add  methods                        *
// *                                                                *
// ******************************************************************

void int_add::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==a);
  DCASSERT(operands[0]);
  operands[0]->Compute(r, st, 0, x);
  if (x.isNull() || x.isError()) return;  // short circuit
  bool unknown = false;
  if (x.isUnknown()) {
    unknown = true;
    x.Clear();
  }
  int i=0;
  if (!x.isInfinity()) {
    // Sum until we run out of operands or hit an infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(r, st, 0, foo);
      if (foo.isNormal()) {
	// normal finite addition
        x.ivalue += foo.ivalue;  
	continue;
      }
      if (foo.isInfinity()) {
	// infinite addition
	unknown = false;  // infinity + ? = infinity
        x.setInfinity();
        x.ivalue = foo.ivalue;
        break;  
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
      }
      // must be an error
      x.setError();
      return;  // error...short circuit
    } // for i
  }


  // sum so far is +/- infinity, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity - infinity.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(x.isInfinity());
    DCASSERT(operands[i]);
    result foo;
    operands[i]->Compute(r, st, 0, foo);
    if (foo.isNormal() || foo.isUnknown()) continue;  // most likely case
    if (foo.isInfinity()) {
      // check operand for opposite sign for infinity
      if ( (x.ivalue>0) != (foo.ivalue>0) ) {
	  Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
	  Error << "Undefined operation (infty-infty) due to ";
	  Error << operands[i];
	  Error.Stop();
	  x.setError();
	  return;
      }
    } // infinity
    if (foo.isNull()) {
      x.setNull();
      return;  // null...short circuit
    }
    // must be an error
    x.setError();
    return;  // error...short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        int_sub  methods                        *
// *                                                                *
// ******************************************************************

void int_sub::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result lv;
  result rv;
  x.Clear();
  left->Compute(r, st, 0, lv);
  right->Compute(r, st, 0, rv);

  if (lv.isNormal() && rv.isNormal()) {
    // ordinary integer subtraction
    x.ivalue = lv.ivalue - rv.ivalue;
    return;
  }
  if (lv.isInfinity() && rv.isInfinity()) {
    // both infinity
    if ((lv.ivalue > 0) != (rv.ivalue >0)) {
      x.setInfinity();
      x.ivalue = lv.ivalue;
      return;
    }
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (infty-infty) due to " << right;
    Error.Stop();
    x.setError();
    return;
  }
  if (lv.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = lv.ivalue;
    return;
  }
  if (rv.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = -rv.ivalue;
    return;
  }
  if (lv.isUnknown() || rv.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (lv.isNull() || rv.isNull()) {
    x.setNull();
    return;
  }
  x.setError();
}

// ******************************************************************
// *                                                                *
// *                        int_mult methods                        *
// *                                                                *
// ******************************************************************

void int_mult::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(0==a);
  DCASSERT(operands[0]);
  operands[0]->Compute(r, st, 0, x);
  if (x.isError()) return;
  if (x.isNull()) return;  // short circuit
  bool unknown = x.isUnknown();
  if (unknown) {
    x.Clear();
    x.ivalue = 1;
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

  if (x.isNormal() && x.ivalue) {
    // Multiply until we run out of operands or change state
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(r, st, 0, foo);
      if (foo.isNormal()) {
	if (0==foo.ivalue) {
	  x.ivalue = 0;
          unknown = false;
	  break;  // change state
	} else {
	  // normal finite multiply
	  x.ivalue *= foo.ivalue;
	}
	continue;
      }
      if (foo.isInfinity()) {
	// fix sign and change state
	x.ivalue = SIGN(x.ivalue) * SIGN(foo.ivalue);
	x.setInfinity();
	break;
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (foo.isNull()) {
	x.setNull();
	return; // short circuit
      }
      // must be an error
      x.setError();
    } // for i
  }

  // The infinity case
  if (x.isInfinity()) {
    // Keep multiplying, only worry about sign and make sure we don't hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(r, st, 0, foo);
      if (foo.isNormal() || foo.isInfinity()) {
	if (0==foo.ivalue) {
	  // 0 * infinity, error
          Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
          Error << "Undefined operation (0 * infty) due to ";
          Error << operands[i];
          Error.Stop();
          x.setError();
	  return;
	} else {
	  // fix sign
	  x.ivalue *= SIGN(foo.ivalue);
	}
	continue;
      }
      if (foo.isUnknown()) {
	unknown = true;  // can't be sure of sign
	continue;
      }
      if (foo.isNull()) {
	x.setNull();
	return; // short circuit
      }
      // must be an error
      x.setError();
    } // for i
  } 

  // The zero case
  if (x.ivalue == 0) {
    // Check the remaining operands, if any, and throw an
    // error if we have infinity * 0.
    for (i++; i<opnd_count; i++) {
      DCASSERT(x.ivalue==0);
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(r, st, 0, foo);
      if (foo.isNormal() || foo.isUnknown()) continue;

      // check for infinity
      if (foo.isInfinity()) {
        Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
        Error << "Undefined operation (0 * infty) due to ";
        Error << operands[i];
        Error.Stop();
        x.setError();
        return;
      }
      if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
      }
      x.setError();
      return;  // error...short circuit
    } // for i
  }

  // Only thing to worry about:
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        int_div  methods                        *
// *                                                                *
// ******************************************************************

void int_div::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);

  if (l.isNormal() && r.isNormal()) {
    x.rvalue = l.ivalue;
    if (0==r.ivalue) {
      Error.Start(right->Filename(), right->Linenumber());
      Error << "Undefined operation (divide by 0) due to " << right;
      Error.Stop();
      x.setError();
    } else {
      x.rvalue /= r.ivalue;
    }
    return;
  }

  if (l.isInfinity() && r.isInfinity()) {
      Error.Start(right->Filename(), right->Linenumber());
      Error << "Undefined operation (infty / infty) due to ";
      Error << left << "/" << right;
      Error.Stop();
      x.setError();
      return;
  }

  if (l.isInfinity()) {
    // infinity / finite, check sign only
    x.setInfinity();
    x.ivalue = SIGN(l.ivalue) * SIGN(r.ivalue);
    return;
  }
  if (r.isInfinity()) {
    // finite / infinity = 0  
    x.rvalue = 0.0;
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
// *                       int_equal  methods                       *
// *                                                                *
// ******************************************************************

void int_equal::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;

  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.ivalue == r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        int_neq  methods                        *
// *                                                                *
// ******************************************************************

void int_neq::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.ivalue != r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_gt methods                         *
// *                                                                *
// ******************************************************************

void int_gt::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);
  if (CheckOpnds(l, r, x)) {
    // normal comparison
#ifdef DEBUG_DEEP
    cout << "Comparing " << left << " and " << right << "\n";
    cout << "Got " << left << " = " << l.ivalue << "\n";
    cout << "Got " << right << " = " << r.ivalue << "\n";
#endif
    x.bvalue = (l.ivalue > r.ivalue);
#ifdef DEBUG_DEEP
    cout << "So " << left << " > " << right << " is " << x.bvalue << "\n";
#endif
  }
}

// ******************************************************************
// *                                                                *
// *                         int_ge methods                         *
// *                                                                *
// ******************************************************************

void int_ge::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue >= r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_lt methods                         *
// *                                                                *
// ******************************************************************

void int_lt::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue < r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_le methods                         *
// *                                                                *
// ******************************************************************

void int_le::Compute(Rng *g, const state *st, int a, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(g, st, 0, l);
  right->Compute(g, st, 0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue <= r.ivalue);
  }
}


//@}

