
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
  x.Clear();
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                         bool_or  class                         *
// *                                                                *
// ******************************************************************

/** Or of two boolean expressions.
 */
class bool_or : public addop {
public:
  bool_or(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new bool_or(Filename(), Linenumber(), l, r);
  }
};

void bool_or::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;

  // Should we short-circuit?  (i.e., don't compute right if left=true?)
  if (right) right->Compute(0, r); else r.null = true;

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
  x.bvalue = l.bvalue || r.bvalue;
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
  bool_and(const char* fn, int line, expr *l, expr *r) 
    : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new bool_and(Filename(), Linenumber(), l, r);
  }
};

void bool_and::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

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
  x.bvalue = l.bvalue && r.bvalue;
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
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

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
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

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
  x.Clear();
  if (opnd) opnd->Sample(seed, 0, x); else x.null = true;

  // Trace errors?
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
  randbool_or(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_or(Filename(), Linenumber(), l,r);
  }
};

void randbool_or::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Sample(seed, 0, l); else l.null = true;

  // Should we short-circuit?  (i.e., don't compute right if left=true?)
  if (right) right->Sample(seed, 0, r); else r.null = true;

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
  x.bvalue = l.bvalue || r.bvalue;
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
  randbool_and(const char* fn, int line, expr *l, expr *r) 
    : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(long &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_and(Filename(), Linenumber(), l, r);
  }
};

void randbool_and::Sample(long &seed, int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Sample(seed, 0, l); else l.null = true;
  if (right) right->Sample(seed, 0, r); else r.null = true;

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
  x.bvalue = l.bvalue && r.bvalue;
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
  result l;
  result r;
  x.Clear();
  if (left) left->Sample(seed, 0, l); else l.null = true;
  if (right) right->Sample(seed, 0, r); else r.null = true;

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
  result l;
  result r;
  x.Clear();
  if (left) left->Sample(seed, 0, l); else l.null = true;
  if (right) right->Sample(seed, 0, l); else r.null = true;

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
  x.Clear();
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
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

/** Addition of two integer expressions.
 */
class int_add : public addop {
public:
  int_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_add(Filename(), Linenumber(), l, r);
  }
};

void int_add::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
#ifdef DEBUG_DEEP
  cout << "Adding " << left << " to " << right << "\n";
#endif
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;
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
    if ((l.ivalue > 0) == (r.ivalue >0)) {
      x.infinity = true;
      x.ivalue = l.ivalue;
      return;
    }
    // different signs, print error message here
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
    x.ivalue = r.ivalue;
    return;
  }
  // ordinary integer addition
  x.ivalue = l.ivalue + r.ivalue;
#ifdef DEBUG_DEEP
  cout << "So their sum is " << x.ivalue << "\n";
#endif
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
  result l;
  result r;
  x.Clear();
#ifdef DEBUG_DEEP
  cout << "Adding " << left << " to " << right << "\n";
#endif
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;
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
  int_mult(const char* fn, int line, expr *l, expr *r) 
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }
  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_mult(Filename(), Linenumber(), l, r);
  }
};

void int_mult::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

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
  if (l.infinity || r.infinity) {
    if ((l.ivalue==0) || (r.ivalue==0)) {
      // 0 * infinity, undefined
      // print error message here
      x.error = CE_Undefined;
      x.null = true;
      return;
    }
    x.infinity = true;
    // check the signs
    bool lpos = l.ivalue > 0;
    bool rpos = r.ivalue > 0;
    if (lpos == rpos) {
      // either infinity*infinity or -infinity * -infinity
      x.ivalue = 1;
    } else {
      x.ivalue = -1;
    }
    return;
  }
  // Ordinary integer multiplication
  x.ivalue = l.ivalue * r.ivalue;
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
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

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
  x.rvalue = l.ivalue;
  if (0==r.ivalue) {
    x.error = CE_ZeroDivide;
    // spew out some error message here
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
  virtual void show(ostream &s) const;
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_unary(Filename(), Linenumber(), returntype, oper, x);
  }
};

void proc_unary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  x.Clear();
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  x.other = MakeUnaryOp(oper, (expr*)x.other, Filename(), Linenumber());
}

void proc_unary::show(ostream &s) const 
{
  switch (oper) {
    case NOT:
      s << "!";
      break;
    case NEG:
      s << "-";
      break;
    default:
      s << "(unknown unary operator)";
  }
  s << opnd;
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
  virtual void show(ostream &s) const;
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_binary(Filename(), Linenumber(), returntype, oper, l, r);
  }
};

void proc_binary::Compute(int i, result &x)
{
  DCASSERT(0==i);
  result l;
  result r;
  x.Clear();
  if (left) left->Compute(0, l); else { 
    l.null = true;
    l.other = NULL;
  }
  if (right) right->Compute(0, r); else {
    r.null = true;
    r.other = NULL;
  }
  x.other = NULL;
  if (l.error) {
    // some option about error tracing here, I guess...
    Delete((expr *)l.other);
    x.error = l.error;
    return;
  }
  if (r.error) {
    Delete((expr *)r.other);
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
    Delete((expr *)l.other);
    Delete((expr *)r.other);
    return;
  }

  x.other = MakeBinaryOp((expr*)l.other, oper, (expr*)r.other,
	                 Filename(), Linenumber());
}

void proc_binary::show(ostream &s) const 
{
  s << left;
  switch (oper) {
    case PLUS:		s << "+"; 	break;
    case MINUS:		s << "-"; 	break;
    case TIMES:		s << "*"; 	break;
    case DIVIDE:	s << "/"; 	break;
    case EQUALS:	s << "=="; 	break;
    case NEQ:		s << "!="; 	break;
    case GT:		s << ">"; 	break;
    case GE:		s << ">="; 	break;
    case LT:		s << "<"; 	break;
    case LE:		s << "<="; 	break;
    default:
      s << "(unknown binary operator)";
  }
  s << right;
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
          case PLUS:	return new bool_or(file, line, left, right);
	  case TIMES:   return new bool_and(file, line, left, right);
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
          case PLUS:	return new int_add(file, line, left, right);
          case MINUS:	return new int_sub(file, line, left, right);
          case TIMES:	return new int_mult(file, line, left, right);
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

//@}

