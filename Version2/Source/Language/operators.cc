
// $Id$

#include "operators.h"
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
    case NEG: 		return "-";
    case PLUS: 		return "+";
    case MINUS:		return "-";
    case TIMES:		return "*";
    case DIVIDE:	return "/";
    case EQUALS:	return "==";
    case NEQ:		return "!=";
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
  if (x.null) return;

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
  bool_or(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }

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
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (x.error) 	return; // error...short circuit
    if (x.null) 	return;	// null...short circuit
    if (x.bvalue) 	return;	// true...short circuit
  } // for i
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
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(0, x);
    if (x.error) 	return; // error...short circuit
    if (x.null) 	return;	// null...short circuit
    if (!x.bvalue) 	return;	// false...short circuit
  } // for i
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
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
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
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
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
  if (x.null) return;

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
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
    if (x.error) 	return; // error...short circuit
    if (x.null) 	return;	// null...short circuit
    if (x.bvalue) 	return;	// true...short circuit
  } // for i
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
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Sample(seed, 0, x);
    if (x.error) 	return; // error...short circuit
    if (x.null) 	return;	// null...short circuit
    if (!x.bvalue) 	return;	// false...short circuit
  } // for i
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
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
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
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
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
  if (x.null) return;

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
  if (x.null) return;  // short circuit
  int i=0;
  if (!x.infinity) {
    // Sum until we run out of operands or hit an infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.error) {
	  // error tracing options here
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.null) {
	  x.null = true;
	  return;  // null...short circuit
      }
      // new operand is not null or an error, add to x
      if (foo.infinity) {
        x.infinity = true;
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
    DCASSERT(x.infinity);
    DCASSERT(operands[i]);
    result foo;
    operands[i]->Compute(0, foo);
    if (foo.error) {
        // error tracing options here
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.null) {
        x.null = true;
        return;  // null...short circuit
    }
    // check operand for opposite sign for infinity
    if (foo.infinity) {
      if ( (x.ivalue>0) != (foo.ivalue>0) ) {
	  // deal with error tracing here
	  x.error = CE_Undefined;
	  x.null = true;
	  return;
      }
    }
  } // for i
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
#ifdef DEBUG_DEEP
  cout << "Adding " << left << " to " << right << "\n";
#endif
  left->Compute(0, l);
  right->Compute(0, r);
#ifdef DEBUG_DEEP
  cout << "Got " << left << " = " << l.ivalue << "\n";
  cout << "Got " << right << " = " << r.ivalue << "\n";
#endif

  if (l.error) {
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
    return;
  }
  if (l.infinity && r.infinity) {
    // both infinity
    if ((l.ivalue > 0) != (r.ivalue >0)) {
      x.infinity = true;
      x.ivalue = l.ivalue;
      return;
    }
    // summing infinities with different signs, print error message here
    x.error = CE_Undefined;
    x.null = true;
    return;
  }
  if (l.infinity) {
    // one infinity
    x.infinity = true;
    x.ivalue = l.ivalue;
    return;
  }
  if (r.infinity) {
    // one infinity
    x.infinity = true;
    x.ivalue = -r.ivalue;
    return;
  }
  // ordinary integer subtraction
  x.ivalue = l.ivalue - r.ivalue;
#ifdef DEBUG_DEEP
  cout << "So their difference is " << x.ivalue << "\n";
#endif
}

// ******************************************************************
// *                                                                *
// *                         int_mult class                         *
// *                                                                *
// ******************************************************************

/** Multiplication of two integer expressions.
 */
class int_mult : public multop {
public:
  int_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
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
  if (x.null) return;  // short circuit
  int i=0;
  if (x.ivalue) {
    // Multiply until we run out of operands or hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      result foo;
      operands[i]->Compute(0, foo);
      if (foo.error) {
	  // error tracing options here
	  x.error = foo.error;
	  return;  // error...short circuit
      }
      if (foo.null) {
	  x.null = true;
	  return;  // null...short circuit
      }
      if (0==foo.ivalue) {
	// we have zero
	if (x.infinity) {
	  // 0 * infinity, error
	  x.error = CE_Undefined;
          x.null = true;
	  return;
	}
	x.ivalue = 0;
	break;
      }
      if (foo.infinity) {
	// we have infinity
        x.infinity = true;
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
        // error tracing options here
	x.error = foo.error;
	return;  // error...short circuit
    }
    if (foo.null) {
        x.null = true;
        return;  // null...short circuit
    }
    // check for infinity
    if (foo.infinity) {
      x.error = CE_Undefined;
      x.null = true;
      return;
    }
  } // for i
}

// ******************************************************************
// *                                                                *
// *                         int_div  class                         *
// *                                                                *
// ******************************************************************

/** Division of two integer expressions.
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
  if (l.null || r.null) {
    x.null = true;
    return;
  }
  x.rvalue = l.ivalue;
  if (0==r.ivalue) {
    x.error = CE_ZeroDivide;
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


// cut and paste the same for reals

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
  virtual void show(ostream &s) const { unary_show(s, GetOp(oper)); }
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

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

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
  virtual void show(ostream &s) const { binary_show(s, GetOp(oper)); }
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
  if (x.null) return;
  right->Compute(0, r); 
  if (r.error || r.null) {
    Delete((expr *)x.other);
    x.null = true;
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
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return returntype;
  }
  virtual void Compute(int i, result &x);
  virtual void show(ostream &s) const { assoc_show(s, GetOp(oper)); }
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
    if (x.error || x.null) {
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

  type optype = opnd->Type(0);
  switch (optype) {
    case BOOL:
      if (op==NOT) return new bool_not(file, line, opnd);
      return NULL;

    case INT:
      if (op==NEG) return new int_neg(file, line, opnd);
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

expr* BinaryAssocError(type ltype, int op, type rtype)
{
  cerr << "INTERNAL: expression " << GetType(ltype) << " ";
  cerr << GetOp(op) << " " << GetType(rtype) << "\n\t";
  cerr << "should use associative expression\n";
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
  type ltype = left->Type(0);
  type rtype = right->Type(0);

  switch (ltype) {

    //===============================================================
    case BOOL:

      DCASSERT(rtype==BOOL);

      switch (op) {
          case PLUS:	
	  case TIMES:   return BinaryAssocError(ltype, op, rtype);

	  case EQUALS:	return new bool_equal(file, line, left, right);
	  case NEQ:	return new bool_neq(file, line, left, right);
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
          case PLUS:	
          case TIMES:	return BinaryAssocError(ltype, op, rtype);

          case MINUS:	return new int_sub(file, line, left, right);
          case DIVIDE:	return new int_div(file, line, left, right);
	  case EQUALS:	return new int_equal(file, line, left, right);
	  case NEQ:	return new int_neq(file, line, left, right);
	  case GT:	return new int_gt(file, line, left, right);
	  case GE:	return new int_ge(file, line, left, right);
	  case LT:	return new int_lt(file, line, left, right);
	  case LE:	return new int_le(file, line, left, right);
      }
      return NULL;
      

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

expr* IllegalAssocError(int op, type alltype)
{
  cerr << "INTERNAL: illegal associative operator " << GetOp(op);
  cerr << " for type " << GetType(alltype) << "\n";
  return NULL;
}


// Note: operand types must match properly already
expr* MakeAssocOp(int op, expr **opnds, int n, const char* file, int line)
{
  int i;
  for (i=0; i<n; i++) if (NULL==opnds[i]) {
    int j;
    for (j=0; j<n; j++) Delete(opnds[j]);
    delete[] opnds;
    return NULL;
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
          case PLUS:	return new bool_or(file, line, opnds, n);
	  case TIMES:   return new bool_and(file, line, opnds, n);

	  case EQUALS:	
	  case NEQ:	return IllegalAssocError(op, alltypes);
      }
      return NULL;

      
    //===============================================================
    case INT:

      switch (op) {
          case PLUS:	return new int_add(file, line, opnds, n);
          case TIMES:	return new int_mult(file, line, opnds, n);

          case MINUS:	
          case DIVIDE:	
	  case EQUALS:	
	  case NEQ:	
	  case GT:	
	  case GE:	
	  case LT:	
	  case LE:	return IllegalAssocError(op, alltypes);
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
  int i;
  // First... try to split us into sums
  int sumcount = e->GetSums(a, NULL, 0);
  if (sumcount>1) {
    // There is a sum below e
    expr **opnds = new expr*[sumcount];
    e->GetSums(a, opnds, sumcount);
    for (i=0; i<sumcount; i++) {
      opnds[i] = Copy(opnds[i]);
      Optimize(a, opnds[i]);
    }
    // replace with associative sum
    expr *ne = MakeAssocOp(PLUS, opnds, sumcount, e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
    return;  // done
  } 
  // Still here?  try to split into products
  int prodcount = e->GetProducts(a, NULL, 0);
  if (prodcount>1) {
    // There is a product below us
    expr **opnds = new expr*[prodcount];
    e->GetProducts(a, opnds, prodcount);
    for (i=0; i<prodcount; i++) {
      opnds[i] =Copy(opnds[i]);
      Optimize(a, opnds[i]);
    }
    // replace with associative product
    expr *ne = MakeAssocOp(TIMES, opnds, prodcount, e->Filename(), e->Linenumber());
    Delete(e);
    e = ne;
  } 
}


//@}

