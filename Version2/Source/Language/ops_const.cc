
// $Id$

#include "ops_const.h"

//@Include: ops_const.h

/** @name ops_const.cc
    @type File
    @args \ 

   Implementation of operator classes, for constants.

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                                                                *
// *                        void_seq methods                        *
// *                                                                *
// ******************************************************************

void void_seq::Compute(int a, result &x)
{
  DCASSERT(0==a);
  for (int i=0; i<opnd_count; i++) {
    SafeCompute(operands[i], 0, x);
    // check for errors and bail?
  }
}

// ******************************************************************
// *                                                                *
// *                        bool_not methods                        *
// *                                                                *
// ******************************************************************

void bool_not::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(0, x); 

  if (!x.isNormal()) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                        bool_or  methods                        *
// *                                                                *
// ******************************************************************

void bool_or::Compute(int a, result &x)
{
  DCASSERT(0==a);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
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

void bool_and::Compute(int a, result &x)
{
  DCASSERT(0==a);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
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

void bool_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);

  result l;
  result r;

  left->Compute(0, l); 
  right->Compute(0, r); 

  x.Clear();
  if (CheckOpnds(l, r, x)) {
    x.bvalue = (l.bvalue == r.bvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        bool_neq methods                        *
// *                                                                *
// ******************************************************************

void bool_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);

  result l;
  result r;
  
  left->Compute(0, l);
  right->Compute(0, r);

  x.Clear();
  if (CheckOpnds(l, r, x)) {
    x.bvalue = (l.bvalue != r.bvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        int_neg  methods                        *
// *                                                                *
// ******************************************************************

void int_neg::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(0, x);

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

void int_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
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
      operands[i]->Compute(0, foo);
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
    operands[i]->Compute(0, foo);
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

void int_sub::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNormal() && r.isNormal()) {
    // ordinary integer subtraction
    x.ivalue = l.ivalue - r.ivalue;
    return;
  }
  if (l.isInfinity() && r.isInfinity()) {
    // both infinity
    if ((l.ivalue > 0) != (r.ivalue >0)) {
      x.setInfinity();
      x.ivalue = l.ivalue;
      return;
    }
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (infty-infty) due to " << right;
    Error.Stop();
    x.setError();
    return;
  }
  if (l.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = l.ivalue;
    return;
  }
  if (r.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = -r.ivalue;
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
// *                        int_mult methods                        *
// *                                                                *
// ******************************************************************

void int_mult::Compute(int a, result &x)
{
  DCASSERT(0==a);
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
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
      operands[i]->Compute(0, foo);
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
      operands[i]->Compute(0, foo);
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
      operands[i]->Compute(0, foo);
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

void int_div::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

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
      Error << "Undefined operation (infty / infty) due to " << left << "/" << right;
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

void int_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;

  left->Compute(0, l);
  right->Compute(0, r);

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

void int_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);

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

void int_gt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
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

void int_ge::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
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

void int_lt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
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

void int_le::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue <= r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_neg methods                        *
// *                                                                *
// ******************************************************************

void real_neg::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x);

  if (x.isNormal()) {
    x.rvalue = -x.rvalue;
    return;
  }
  if (x.isInfinity()) {
    x.ivalue = -x.ivalue;
  }
}

// ******************************************************************
// *                                                                *
// *                        real_add methods                        *
// *                                                                *
// ******************************************************************

void real_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.isError()) return;
  if (x.isNull()) return;  // short circuit
  bool unknown = x.isUnknown();
  if (unknown) x.Clear();
  int i=0;
  if (!x.isInfinity()) {
    // Sum until we run out of operands or hit an infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.isNormal()) {
	x.rvalue += foo.rvalue;  // normal finite addition
	continue;
      }
      if (foo.isInfinity()) {
	x.setInfinity();
	x.ivalue = foo.ivalue;
	unknown = false;
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
      x.setError();
      return;
    } // for i
  }

  // sum so far is +/- infinity, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity - infinity.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(x.isInfinity());
    DCASSERT(operands[i]);
    result foo;
    operands[i]->Compute(0, foo);
    if (foo.isNormal() || foo.isUnknown()) continue;

    // check operand for opposite sign for infinity
    if (foo.isInfinity()) {
      if ( (x.ivalue>0) != (foo.ivalue>0) ) {
	  Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
	  Error << "Undefined operation (infty-infty) due to ";
	  Error << operands[i];
	  Error.Stop();
	  x.setError();
	  return;
      }
    }
    if (foo.isNull()) {
      x.setNull();
      return;  // null...short circuit
    }
    x.setError();
    return;
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        real_sub methods                        *
// *                                                                *
// ******************************************************************

void real_sub::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNormal() && r.isNormal()) {
    // ordinary subtraction
    x.rvalue = l.rvalue - r.rvalue;
    return;
  }
  if (l.isInfinity() && r.isInfinity()) {
    // both infinity
    if ((l.ivalue > 0) != (r.ivalue >0)) {
      x.setInfinity();
      x.ivalue = l.ivalue;
      return;
    }
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (infty-infty) due to " << right;
    Error.Stop();
    x.setError();
    return;
  }
  if (l.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = l.ivalue;
    return;
  }
  if (r.isInfinity()) {
    // one infinity
    x.setInfinity();
    x.ivalue = -r.ivalue;
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
// *                       real_mult  methods                       *
// *                                                                *
// ******************************************************************

void real_mult::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.isError()) return;
  if (x.isNull()) return;  // short circuit
  bool unknown = x.isUnknown();
  if (unknown) {
    x.Clear();
    x.rvalue = 1.0;
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

  if (x.isNormal() && x.rvalue) {
    // Multiply until we run out of operands or change state
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.isNormal()) {
	if (0.0==foo.rvalue) {
	  x.rvalue = 0.0;
          unknown = false;
	  break;  // change state
	} else {
	  // normal finite multiply
	  x.rvalue *= foo.rvalue;
	}
	continue;
      }
      if (foo.isInfinity()) {
	// fix sign and change state
	x.ivalue = SIGN(x.rvalue) * SIGN(foo.ivalue);
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
      operands[i]->Compute(0, foo);
      if (foo.isNormal()) {
	if (0.0==foo.rvalue) {
	  // 0 * infinity, error
          Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
          Error << "Undefined operation (0 * infty) due to ";
          Error << operands[i];
          Error.Stop();
          x.setError();
	  return;
	} else {
	  // fix sign
	  x.ivalue *= SIGN(foo.rvalue);
	}
	continue;
      }
      if (foo.isInfinity()) {
	x.ivalue *= foo.ivalue;
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
  if (x.rvalue == 0.0) {
    // Check the remaining operands, if any, and throw an
    // error if we have infinity * 0.
    for (i++; i<opnd_count; i++) {
      DCASSERT(x.rvalue==0.0);
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
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
// *                        real_div methods                        *
// *                                                                *
// ******************************************************************

void real_div::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.isNormal() && r.isNormal()) {
    if (0.0==r.rvalue) {
      Error.Start(right->Filename(), right->Linenumber());
      Error << "Undefined operation (divide by 0) due to " << right;
      Error.Stop();
      x.setError();
    } else {
      x.rvalue = l.rvalue / r.rvalue;
    }
    return;
  }

  if (l.isInfinity() && r.isInfinity()) {
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (infty / infty) due to " << left << "/" << right;
    Error.Stop();
    x.setError();
    return;
  }

  if (l.isInfinity()) {
    // infinity / finite, check sign only
    x.setInfinity();
    x.ivalue = SIGN(l.ivalue) * SIGN(r.rvalue);
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
// *                       real_equal methods                       *
// *                                                                *
// ******************************************************************

void real_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
    
  result l;
  result r;

  left->Compute(0, l);
  right->Compute(0, r);

  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.ivalue == r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_neq methods                        *
// *                                                                *
// ******************************************************************

void real_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.rvalue != r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_gt  methods                        *
// *                                                                *
// ******************************************************************

void real_gt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.rvalue > r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_ge  methods                        *
// *                                                                *
// ******************************************************************

void real_ge::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue >= r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_lt  methods                        *
// *                                                                *
// ******************************************************************

void real_lt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue < r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                        real_le  methods                        *
// *                                                                *
// ******************************************************************

void real_le::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  left->Compute(0, l);
  right->Compute(0, r);
  if (CheckOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue <= r.rvalue);
  }
}




//@}

