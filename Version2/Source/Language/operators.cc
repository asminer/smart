
// $Id$

#include "operators.h"
#include "strings.h"

//@Include: operators.h

/** @name operators.cc
    @type File
    @args \ 

   Implementation of operator classes.

   Also, some other expression things are here,
   since this is essentially the "top" of the
   expression stuff.

 */

//@{

//#define DEBUG_DEEP

inline bool Pos(int i) { return (i>0) ? 1 : 0; }
inline int Sign(int i) { return (i<0) ? -1 : Pos(i); }

const char* GetOp(int op)
{
  switch (op) {
    case NOT: 		return "!";
    case PLUS: 		return "+";
    case MINUS:		return "-";
    case TIMES:		return "*";
    case DIVIDE:	return "/";
    case EQUALS:	return "==";
    case NEQUAL:	return "!=";
    case GT:		return ">";
    case GE:		return ">=";
    case LT:		return "<";
    case LE:		return "<=";
    default:
			return "unknown_op";
  }
  return "error";  // keep compilers happy
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                      Operators for bool                      **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                         bool_not class                         *
// *                                                                *
// ******************************************************************

/** Negation of a boolean expression.
 */
class bool_not : public negop {
public:
  bool_not(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new bool_not(Filename(), Linenumber(), x);
  }
};

void bool_not::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x); 

  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                         bool_or  class                         *
// *                                                                *
// ******************************************************************

/** Or of boolean expressions.
 */
class bool_or : public addop {
public:
  bool_or(const char* fn, int line, expr **x, int n): addop(fn, line, x, n) { }
  bool_or(const char* fn, int line, expr *l, expr* r): addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new bool_or(Filename(), Linenumber(), x, n);
  }
};

void bool_or::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (x.error) 	return; // error...short circuit
    if (x.isNull()) 	return;	// null...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (x.bvalue) 	return;	// true...short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                         bool_and class                         *
// *                                                                *
// ******************************************************************

/** And of two boolean expressions.
 */
class bool_and : public multop {
public:
  bool_and(const char* fn, int line, expr **x, int n) 
  : multop(fn, line, x, n) { }

  bool_and(const char* fn, int line, expr *l, expr *r) 
  : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new bool_and(Filename(), Linenumber(), x, n);
  }
};

void bool_and::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (x.error) 	return; // error...short circuit
    if (x.isNull()) 	return;	// null...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.bvalue) 	return;	// false...short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                        bool_equal class                        *
// *                                                                *
// ******************************************************************

/** Check equality of two boolean expressions.
 */
class bool_equal : public consteqop {
public:
  bool_equal(const char* fn, int line, expr *l, expr *r) 
    : consteqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr* r) {
    return new bool_equal(Filename(), Linenumber(), l, r);
  }
};

void bool_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();

  left->Compute(0, l); 
  right->Compute(0, r); 

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  x.bvalue = (l.bvalue == r.bvalue);
}

// ******************************************************************
// *                                                                *
// *                         bool_neq class                         *
// *                                                                *
// ******************************************************************

/** Check inequality of two boolean expressions.
 */
class bool_neq : public constneqop {
public:
  bool_neq(const char* fn, int line, expr *l, expr *r)
    : constneqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new bool_neq(Filename(), Linenumber(), l, r);
  }
};

void bool_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Compute(0, l);
  right->Compute(0, r);

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  x.bvalue = (l.bvalue != r.bvalue);
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                   Operators for  rand bool                   **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                       randbool_not class                       *
// *                                                                *
// ******************************************************************

/** Negation of a random boolean expression.
 */
class randbool_not : public negop {
public:
  randbool_not(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new randbool_not(Filename(), Linenumber(), x);
  }
};

void randbool_not::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Sample(seed, 0, x);

  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                       randbool_or  class                       *
// *                                                                *
// ******************************************************************

/** Or of two random boolean expressions.
 */
class randbool_or : public addop {
public:
  randbool_or(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }

  randbool_or(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new randbool_or(Filename(), Linenumber(), x, n);
  }
};

void randbool_or::Sample(long &seed, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
    if (x.error) 	return; // error...short circuit
    if (x.isNull()) 	return;	// null...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (x.bvalue) 	return;	// true...short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                       randbool_and class                       *
// *                                                                *
// ******************************************************************

/** And of two random boolean expressions.
 */
class randbool_and : public multop {
public:
  randbool_and(const char* fn, int line, expr **x, int n) 
    : multop(fn, line, x, n) { }
  
  randbool_and(const char* fn, int line, expr *l, expr *r) 
    : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new randbool_and(Filename(), Linenumber(), x, n);
  }
};

void randbool_and::Sample(long &seed, int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
    if (x.error) 	return; // error...short circuit
    if (x.isNull()) 	return;	// null...short circuit
    if (x.isUnknown()) {
      unknown = true;
      continue;
    }
    if (!x.bvalue) 	return;	// false...short circuit
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                      randbool_equal class                      *
// *                                                                *
// ******************************************************************

/** Check equality of two random boolean expressions.
 */
class randbool_equal : public eqop {
public:
  randbool_equal(const char* fn, int line, expr *l, expr *r) 
    : eqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_equal(Filename(), Linenumber(), l, r);
  }
};

void randbool_equal::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Sample(seed, 0, l); 
  right->Sample(seed, 0, r);

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  x.bvalue = (l.bvalue == r.bvalue);
}

// ******************************************************************
// *                                                                *
// *                       randbool_neq class                       *
// *                                                                *
// ******************************************************************

/** Check inequality of two random boolean expressions.
 */
class randbool_neq : public neqop {
public:
  randbool_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_neq(Filename(), Linenumber(), l, r);
  }
};

void randbool_neq::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result l;
  result r;
  x.Clear();
  left->Sample(seed, 0, l);
  right->Sample(seed, 0, r);

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  x.bvalue = (l.bvalue != r.bvalue);
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                      Operators for  int                      **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                         int_neg  class                         *
// *                                                                *
// ******************************************************************

/** Negation of an integer expression.
 */
class int_neg : public negop {
public:
  int_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new int_neg(Filename(), Linenumber(), x);
  }
};

void int_neg::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x);

  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;

  // This is the right thing to do even for infinity.
  x.ivalue = -x.ivalue;
}

// ******************************************************************
// *                                                                *
// *                         int_add  class                         *
// *                                                                *
// ******************************************************************

/** Addition of integer expressions.
 */
class int_add : public addop {
public:
  int_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  int_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new int_add(Filename(), Linenumber(), x, n);
  }
};


void int_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.error) return;
  if (x.isNull()) return;  // short circuit
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
      if (foo.error) {
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.isNull()) {
	  x.setNull();
	  return;  // null...short circuit
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      // new operand is not null or an error, add to x
      if (foo.isInfinity()) {
	unknown = false;  // infinity + ? = infinity
        x.setInfinity();
        x.ivalue = foo.ivalue;
        break;  
      } else {
        x.ivalue += foo.ivalue;  // normal finite addition
      }
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
    if (foo.error) {
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
    }
    // check operand for opposite sign for infinity
    if (foo.isInfinity()) {
      if ( (x.ivalue>0) != (foo.ivalue>0) ) {
	  Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
	  Error << "Undefined operation (infty-infty) caused by ";
	  Error << operands[i];
	  Error.Stop();
	  x.error = CE_Undefined;
	  x.setNull();
	  return;
      }
    }
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                         int_sub  class                         *
// *                                                                *
// ******************************************************************

/** Subtraction of two integer expressions.
 */
class int_sub : public subop {
public:
  int_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_sub(Filename(), Linenumber(), l, r);
  }
};

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

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
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
    Error << "Undefined operation (infty-infty) caused by " << right;
    Error.Stop();
    x.error = CE_Undefined;
    x.setNull();
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
  // ordinary integer subtraction
  x.ivalue = l.ivalue - r.ivalue;
}

// ******************************************************************
// *                                                                *
// *                         int_mult class                         *
// *                                                                *
// ******************************************************************

/** Multiplication of integer expressions.
 */
class int_mult : public multop {
public:
  int_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  int_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new int_mult(Filename(), Linenumber(), x, n);
  }
};

void int_mult::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.error) return;
  if (x.isNull()) return;  // short circuit
  bool unknown = x.isUnknown();
  if (unknown) {
    x.Clear();
    x.ivalue = 1;
  }
  int i=0;
  if (x.ivalue) {
    // Multiply until we run out of operands or hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.error) {
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.isNull()) {
	  x.setNull();
	  return;  // null...short circuit
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (0==foo.ivalue) {
	// we have zero
	if (x.isInfinity()) {
	  // 0 * infinity, error
          Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
          Error << "Undefined operation (0 * infty) caused by ";
          Error << operands[i];
          Error.Stop();
	  x.error = CE_Undefined;
          x.setNull();
	  return;
	}
	x.ivalue = 0;
	unknown = false;  // 0 * ? = 0.
	break;
      }
      if (foo.isInfinity()) {
	// we have infinity
        x.setInfinity();
        x.ivalue = Sign(x.ivalue) * Sign(foo.ivalue);
      } else {
	// normal finite integer multiplication
        x.ivalue *= foo.ivalue;
      }
    } // for i
  }


  // product so far is zero, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity * 0.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(x.ivalue==0);
    DCASSERT(operands[i]);
    result foo;
    operands[i]->Compute(0, foo);
    if (foo.error) {
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
    }
    // check for infinity
    if (foo.isInfinity()) {
      Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
      Error << "Undefined operation (0 * infty) caused by ";
      Error << operands[i];
      Error.Stop();
      x.error = CE_Undefined;
      x.setNull();
      return;
    }
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                         int_div  class                         *
// *                                                                *
// ******************************************************************

/** Division of two integer expressions.
    Note that the result type is REAL.
 */
class int_div : public divop {
public:
  int_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_div(Filename(), Linenumber(), l, r);
  }
};

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

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  x.rvalue = l.ivalue;
  if (0==r.ivalue) {
    x.error = CE_ZeroDivide;
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (divide by 0) caused by " << right;
    Error.Stop();
  } else {
    x.rvalue /= r.ivalue;
  }
}

// ******************************************************************
// *                                                                *
// *                        int_equal  class                        *
// *                                                                *
// ******************************************************************

/** Check equality of two integer expressions.
 */
class int_equal : public consteqop {
public:
  int_equal(const char* fn, int line, expr *l, expr *r)
    : consteqop(fn, line, l, r) { }
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_equal(Filename(), Linenumber(), l, r);
  }
};

void int_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.ivalue == r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         int_neq  class                         *
// *                                                                *
// ******************************************************************

/** Check inequality of two integer expressions.
 */
class int_neq : public constneqop {
public:
  int_neq(const char* fn, int line, expr *l, expr *r)
    : constneqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_neq(Filename(), Linenumber(), l, r);
  }
};

void int_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.ivalue != r.ivalue);
  }
}


// ******************************************************************
// *                                                                *
// *                          int_gt class                          *
// *                                                                *
// ******************************************************************

/** Check if one integer expression is greater than another.
 */
class int_gt : public constgtop {
public:
  int_gt(const char* fn, int line, expr *l, expr *r)
    : constgtop(fn, line, l, r) { }
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_gt(Filename(), Linenumber(), l, r);
  }
};

void int_gt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
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
// *                          int_ge class                          *
// *                                                                *
// ******************************************************************

/** Check if one integer expression is greater than or equal another.
 */
class int_ge : public constgeop {
public:
  int_ge(const char* fn, int line, expr *l, expr *r)
    : constgeop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_ge(Filename(), Linenumber(), l, r);
  }
};

void int_ge::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue >= r.ivalue);
  }
}
// ******************************************************************
// *                                                                *
// *                          int_lt class                          *
// *                                                                *
// ******************************************************************

/** Check if one integer expression is less than another.
 */
class int_lt : public constltop {
public:
  int_lt(const char* fn, int line, expr *l, expr *r)
    : constltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_lt(Filename(), Linenumber(), l, r);
  }
};

void int_lt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue < r.ivalue);
  }
}

// ******************************************************************
// *                                                                *
// *                          int_le class                          *
// *                                                                *
// ******************************************************************

/** Check if one integer expression is less than or equal another.
 */
class int_le : public constleop {
public:
  int_le(const char* fn, int line, expr *l, expr *r)
    : constleop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_le(Filename(), Linenumber(), l, r);
  }
};

void int_le::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.ivalue <= r.ivalue);
  }
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                      Operators for real                      **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                         real_neg class                         *
// *                                                                *
// ******************************************************************

/** Negation of a real expression.
 */
class real_neg : public negop {
public:
  real_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new real_neg(Filename(), Linenumber(), x);
  }
};

void real_neg::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x);

  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;

  if (x.isInfinity()) {
    x.ivalue = -x.ivalue;
  } else {
    x.rvalue = -x.rvalue;
  }
}

// ******************************************************************
// *                                                                *
// *                         real_add class                         *
// *                                                                *
// ******************************************************************

/** Addition of real expressions.
 */
class real_add : public addop {
public:
  real_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  real_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new real_add(Filename(), Linenumber(), x, n);
  }
};


void real_add::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.error) return;
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
      if (foo.error) {
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.isNull()) {
	  x.setNull();
	  return;  // null...short circuit
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      // new operand is not null or an error, add to x
      if (foo.isInfinity()) {
        x.setInfinity();
        x.ivalue = foo.ivalue;
	unknown = false;
        break;  
      } else {
        x.rvalue += foo.rvalue;  // normal finite addition
      }
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
    if (foo.error) {
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
    }
    // check operand for opposite sign for infinity
    if (foo.isInfinity()) {
      if ( (x.ivalue>0) != (foo.ivalue>0) ) {
	  Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
	  Error << "Undefined operation (infty-infty) caused by ";
	  Error << operands[i];
	  Error.Stop();
	  x.error = CE_Undefined;
	  x.setNull();
	  return;
      }
    }
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                         real_sub class                         *
// *                                                                *
// ******************************************************************

/** Subtraction of two real expressions.
 */
class real_sub : public subop {
public:
  real_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_sub(Filename(), Linenumber(), l, r);
  }
};

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

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
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
    Error << "Undefined operation (infty-infty) caused by " << right;
    Error.Stop();
    x.error = CE_Undefined;
    x.setNull();
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
  // ordinary subtraction
  x.rvalue = l.rvalue - r.rvalue;
}

// ******************************************************************
// *                                                                *
// *                        real_mult  class                        *
// *                                                                *
// ******************************************************************

/** Multiplication of real expressions.
 */
class real_mult : public multop {
public:
  real_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  real_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new real_mult(Filename(), Linenumber(), x, n);
  }
};

void real_mult::Compute(int a, result &x)
{
  DCASSERT(0==a);
  x.Clear();
  DCASSERT(operands[0]);
  operands[0]->Compute(0, x);
  if (x.error) return;
  if (x.isNull()) return;  // short circuit
  bool unknown = x.isUnknown();
  if (unknown) {
    x.Clear();
    x.rvalue = 1.0;
  }
  int i=0;
  if (x.rvalue) {
    // Multiply until we run out of operands or hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.error) {
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.isNull()) {
	  x.setNull();
	  return;  // null...short circuit
      }
      if (foo.isUnknown()) {
	unknown = true;
	continue;
      }
      if (0.0==foo.rvalue) {
	// we have zero
	if (x.isInfinity()) {
	  // 0 * infinity, error
          Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
          Error << "Undefined operation (0 * infty) caused by ";
          Error << operands[i];
          Error.Stop();
	  x.error = CE_Undefined;
          x.setNull();
	  return;
	}
	x.rvalue = 0.0;
	unknown = false;
	break;
      }
      if (foo.isInfinity()) {
	// we have infinity
        x.setInfinity();
        x.ivalue = Sign(x.ivalue) * Sign(foo.ivalue);
      } else {
	// normal finite real multiplication
        x.rvalue *= foo.rvalue;
      }
    } // for i
  }


  // product so far is zero, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity * 0.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(x.rvalue==0.0);
    DCASSERT(operands[i]);
    result foo;
    operands[i]->Compute(0, foo);
    if (foo.error) {
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.isNull()) {
        x.setNull();
        return;  // null...short circuit
    }
    // check for infinity
    if (foo.isInfinity()) {
      Error.Start(operands[i]->Filename(), operands[i]->Linenumber());
      Error << "Undefined operation (0 * infty) caused by ";
      Error << operands[i];
      Error.Stop();
      x.error = CE_Undefined;
      x.setNull();
      return;
    }
  } // for i
  if (unknown) x.setUnknown();
}

// ******************************************************************
// *                                                                *
// *                         real_div class                         *
// *                                                                *
// ******************************************************************

/** Division of two real expressions.
 */
class real_div : public divop {
public:
  real_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_div(Filename(), Linenumber(), l, r);
  }
};

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

  if (l.error) {
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.isNull() || r.isNull()) {
    x.setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.setUnknown();
    return;
  }
  if (0.0==r.rvalue) {
    x.error = CE_ZeroDivide;
    Error.Start(right->Filename(), right->Linenumber());
    Error << "Undefined operation (divide by 0) caused by " << right;
    Error.Stop();
  } else {
    x.rvalue = l.rvalue / r.rvalue;
  }
}

// ******************************************************************
// *                                                                *
// *                        real_equal class                        *
// *                                                                *
// ******************************************************************

/** Check equality of two real expressions.
    To do still: check precision option, etc.
 */
class real_equal : public consteqop {
public:
  real_equal(const char* fn, int line, expr *l, expr *r)
    : consteqop(fn, line, l, r) { }
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_equal(Filename(), Linenumber(), l, r);
  }
};

void real_equal::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.rvalue == r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         real_neq class                         *
// *                                                                *
// ******************************************************************

/** Check inequality of two real expressions.
 */
class real_neq : public constneqop {
public:
  real_neq(const char* fn, int line, expr *l, expr *r)
    : constneqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_neq(Filename(), Linenumber(), l, r);
  }
};

void real_neq::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.rvalue != r.rvalue);
  }
}


// ******************************************************************
// *                                                                *
// *                         real_gt  class                         *
// *                                                                *
// ******************************************************************

/** Check if one real expression is greater than another.
 */
class real_gt : public constgtop {
public:
  real_gt(const char* fn, int line, expr *l, expr *r)
    : constgtop(fn, line, l, r) { }
  
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_gt(Filename(), Linenumber(), l, r);
  }
};

void real_gt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l, r, x)) {
    // normal comparison
    x.bvalue = (l.rvalue > r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         real_ge  class                         *
// *                                                                *
// ******************************************************************

/** Check if one real expression is greater than or equal another.
 */
class real_ge : public constgeop {
public:
  real_ge(const char* fn, int line, expr *l, expr *r)
    : constgeop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_ge(Filename(), Linenumber(), l, r);
  }
};

void real_ge::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue >= r.rvalue);
  }
}
// ******************************************************************
// *                                                                *
// *                         real_lt  class                         *
// *                                                                *
// ******************************************************************

/** Check if one real expression is less than another.
 */
class real_lt : public constltop {
public:
  real_lt(const char* fn, int line, expr *l, expr *r)
    : constltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_lt(Filename(), Linenumber(), l, r);
  }
};

void real_lt::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue < r.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                         real_le  class                         *
// *                                                                *
// ******************************************************************

/** Check if one real expression is less than or equal another.
 */
class real_le : public constleop {
public:
  real_le(const char* fn, int line, expr *l, expr *r)
    : constleop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_le(Filename(), Linenumber(), l, r);
  }
};

void real_le::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  if (ComputeOpnds(l,r,x)) {
    // normal comparison
    x.bvalue = (l.rvalue <= r.rvalue);
  }
}



// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **                     Operators for  procs                     **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        proc_unary class                        *
// *                                                                *
// ******************************************************************

/** Unary operators for procs.
 */
class proc_unary : public unary {
protected:
  type returntype;
  int oper;
public:
  proc_unary(const char* fn, int line, type rt, int op, expr *x)
    : unary(fn, line, x) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { unary_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_unary(Filename(), Linenumber(), returntype, oper, x);
  }
};

void proc_unary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  x.Clear();
  opnd->Compute(0, x); 

  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;

  x.other = MakeUnaryOp(oper, (expr*)x.other, Filename(), Linenumber());
}

// ******************************************************************
// *                                                                *
// *                       proc_binary  class                       *
// *                                                                *
// ******************************************************************

/** Binary operations for procs.
 */
class proc_binary : public binary {
protected:
  type returntype;
  int oper;
public:
  proc_binary(const char* fn, int line, type rt, int op, expr *l, expr *r)
    : binary(fn, line, l, r) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { binary_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_binary(Filename(), Linenumber(), returntype, oper, l, r);
  }
};

void proc_binary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(left);
  DCASSERT(right);
  result r;
  x.Clear();
  left->Compute(0, x); 
  x.other = NULL;
  if (x.error) return;
  if (x.isNull()) return;
  if (x.isUnknown()) return;
  right->Compute(0, r); 
  if (r.error || r.isNull()) {
    Delete((expr *)x.other);
    x.setNull();
    x.error = r.error;
    return;
  }

  x.other = MakeBinaryOp((expr*)x.other, oper, (expr*)r.other,
	                 Filename(), Linenumber());
}


// ******************************************************************
// *                                                                *
// *                        proc_assoc class                        *
// *                                                                *
// ******************************************************************

/** Associative operations for procs.
 */
class proc_assoc : public assoc {
protected:
  type returntype;
  int oper;
public:
  proc_assoc(const char* fn, int line, type rt, int op, expr **x, int n)
    : assoc(fn, line, x, n) 
  { 
    returntype = rt;
    oper = op;
  }

  proc_assoc(const char* fn, int line, type rt, int op, expr *l, expr *r)
    : assoc(fn, line, l, r) 
  { 
    returntype = rt;
    oper = op;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(OutputStream &s) const { assoc_show(s, GetOp(oper)); }
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_assoc(Filename(), Linenumber(), returntype, oper, x, n);
  }
};

void proc_assoc::Compute(int a, result &x)
{
  DCASSERT(0==a);
  expr** r = new expr* [opnd_count];
  int i;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (x.error || x.isNull() || x.isUnknown()) {
      // bail out
      int j;
      for (j=0; j<i; j++) Delete(r[j]);
      delete[] r;
      return;
    }
    r[i] = (expr *) x.other;
  }
  // all opnds computed successfully, build expression
  x.other = MakeAssocOp(oper, r, opnd_count, Filename(), Linenumber());
}


// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **            Global functions  to build expressions            **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************


expr* MakeUnaryOp(int op, expr *opnd, const char* file, int line)
{
  if (NULL==opnd) return NULL;
  if (ERROR==opnd) return ERROR;

  type optype = opnd->Type(0);
  switch (optype) {
    case BOOL:
      if (op==NOT) return new bool_not(file, line, opnd);
      return NULL;

    case INT:
      if (op==MINUS) return new int_neg(file, line, opnd);
      return NULL;

    case REAL:
      if (op==MINUS) return new real_neg(file, line, opnd);
      return NULL;

    case PROC_BOOL:
    case PROC_INT:
    case PROC_REAL:
    case PROC_PH_INT:
    case PROC_PH_REAL:
    case PROC_RAND_BOOL:
    case PROC_RAND_INT:
    case PROC_RAND_REAL:
      return new proc_unary(file, line, optype, op, opnd);
  }
  return NULL;
}



// Note: the types left and right must match properly already
expr* MakeBinaryOp(expr *left, int op, expr *right, const char* file, int line)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }
  if (ERROR==left || ERROR==right) {
    Delete(left);
    Delete(right);
    return ERROR;
  }
  type ltype = left->Type(0);
  type rtype = right->Type(0);

  switch (ltype) {

    //===============================================================
    case BOOL:

      DCASSERT(rtype==BOOL);

      switch (op) {
          case PLUS:
          case OR:	return new bool_or(file, line, left, right);

	  case TIMES:
	  case AND:   	return new bool_and(file, line, left, right);

	  case EQUALS:	return new bool_equal(file, line, left, right);
	  case NEQUAL:	return new bool_neq(file, line, left, right);
      }
      return NULL;

      
    //===============================================================
    case INT:

      if (rtype==PH_INT) {
	DCASSERT(op==TIMES);
	// do ph int times int here
	return NULL;
      }

      DCASSERT(rtype==INT);

      switch (op) {
          case PLUS:	return new int_add(file, line, left, right);
          case TIMES:	return new int_mult(file, line, left, right);
          case MINUS:	return new int_sub(file, line, left, right);
          case DIVIDE:	return new int_div(file, line, left, right);
	  case EQUALS:	return new int_equal(file, line, left, right);
	  case NEQUAL:	return new int_neq(file, line, left, right);
	  case GT:	return new int_gt(file, line, left, right);
	  case GE:	return new int_ge(file, line, left, right);
	  case LT:	return new int_lt(file, line, left, right);
	  case LE:	return new int_le(file, line, left, right);
      }
      return NULL;
      

    //===============================================================
    case REAL:

      if (rtype==PH_REAL) {
	DCASSERT(op==TIMES);
	// do ph real times real here
	return NULL;
      }

      DCASSERT(rtype==REAL);

      switch (op) {
          case PLUS:	return new real_add(file, line, left, right);
          case TIMES:	return new real_mult(file, line, left, right);
          case MINUS:	return new real_sub(file, line, left, right);
          case DIVIDE:	return new real_div(file, line, left, right);
	  case EQUALS:	return new real_equal(file, line, left, right);
	  case NEQUAL:	return new real_neq(file, line, left, right);
	  case GT:	return new real_gt(file, line, left, right);
	  case GE:	return new real_ge(file, line, left, right);
	  case LT:	return new real_lt(file, line, left, right);
	  case LE:	return new real_le(file, line, left, right);
      }
      return NULL;
      

    //===============================================================
    case STRING:
      
      DCASSERT(rtype==STRING);
      // Defined in strings.cc
      return MakeStringBinary(left, op, right, file, line);
    
    //===============================================================
    case PROC_INT:
      if (rtype==PROC_PH_INT) {
	DCASSERT(op==TIMES);
	return new proc_binary(file, line, rtype, op, left, right);
      }
      return new proc_binary(file, line, ltype, op, left, right);

      
    //===============================================================
    case PROC_REAL:
      if (rtype==PROC_PH_REAL) {
	DCASSERT(op==TIMES);
	return new proc_binary(file, line, rtype, op, left, right);
      }
      return new proc_binary(file, line, ltype, op, left, right);


    //===============================================================
    case PROC_BOOL:
    case PROC_PH_INT:
    case PROC_PH_REAL:
    case PROC_RAND_INT:
    case PROC_RAND_REAL:
      return new proc_binary(file, line, ltype, op, left, right); 


  }

  return NULL;
}

expr* IllegalAssocError(int op, type alltype, const char *fn, int ln)
{
  Internal.Start(__FILE__, __LINE__, fn, ln);
  Internal << "Illegal associative operator " << GetOp(op);
  Internal << " for type " << GetType(alltype);
  Internal.Stop();
  return NULL;
}


// Note: operand types must match properly already
expr* MakeAssocOp(int op, expr **opnds, int n, const char* file, int line)
{
  int i,j;
  for (i=0; i<n; i++) {
    if (NULL==opnds[i]) {
      for (j=0; j<n; j++) Delete(opnds[j]);
      delete[] opnds;
      return NULL;
    }
    if (ERROR==opnds[i]) {
      for (j=0; j<n; j++) Delete(opnds[j]);
      delete[] opnds;
      return ERROR;
    }
  }
  type alltypes = opnds[0]->Type(0);
#ifdef DEVELOPMENT_CODE
  for (i=1; i<n; i++) {
    type foo = opnds[1]->Type(0);
    DCASSERT(foo==alltypes);
  }
#endif
  switch (alltypes) {

    //===============================================================
    case BOOL:

      switch (op) {
          case PLUS:
          case OR:	return new bool_or(file, line, opnds, n);

	  case TIMES:
	  case AND:   	return new bool_and(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;

      
    //===============================================================
    case INT:

      switch (op) {
          case PLUS:	return new int_add(file, line, opnds, n);
          case TIMES:	return new int_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case REAL:

      switch (op) {
          case PLUS:	return new real_add(file, line, opnds, n);
          case TIMES:	return new real_mult(file, line, opnds, n);

	  default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;


    //===============================================================
    case STRING:
      
      switch (op) {
	case PLUS:	return MakeStringAdd(opnds, n, file, line);
			// Defined in strings.cc

	default:	return IllegalAssocError(op, alltypes, file, line);
      }
      return NULL;
      
  }

  return NULL;
}

// ******************************************************************
// *                    Expression  optimization                    *
// ******************************************************************

void Optimize(int a, expr* &e)
{
  static List <expr> optbuffer(128);
  if (NULL==e) return;
  if (ERROR==e) return;
  int i;
  // First... try to split us into sums
  optbuffer.Clear();
  int sumcount = e->GetSums(a, &optbuffer);
  if (sumcount>1) {
    // There is a sum below e
    expr **opnds = optbuffer.Copy();
    for (i=0; i<sumcount; i++) {
      opnds[i] = Copy(opnds[i]);  // make this our copy
      Optimize(a, opnds[i]);
    }
    // replace with associative sum
    expr *ne = MakeAssocOp(PLUS, opnds, sumcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
    return;  // done
  } 
  // Still here?  try to split into products
  optbuffer.Clear();
  int prodcount = e->GetProducts(a, &optbuffer);
  if (prodcount>1) {
    // There is a product below us
    expr **opnds = optbuffer.Copy();
    for (i=0; i<prodcount; i++) {
      opnds[i] =Copy(opnds[i]);
      Optimize(a, opnds[i]);
    }
    // replace with associative product
    expr *ne = MakeAssocOp(TIMES, opnds, prodcount, 
    				e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
  } 
}


//@}

