
// $Id$

#include "baseops.h"


/** @name ops_int.h
    @type File
    @args \ 

   Operator classes, for integers (int, proc int, rand int, proc rand int)

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                         int_neg  class                         *
// ******************************************************************

/** Negation of an integer expression.
 */
class int_neg : public negop {
public:
  int_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(opnd);
    return opnd->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new int_neg(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                         int_add  class                         *
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
    DCASSERT(operand[0]);
    return operand[0]->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new int_add(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                         int_sub  class                         *
// ******************************************************************

/** Subtraction of two integer expressions.
 */
class int_sub : public subop {
public:
  int_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return left->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         int_mult class                         *
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
    DCASSERT(operand[0]);
    return operand[0]->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new int_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                         int_div  class                         *
// ******************************************************************

/** Division of two integer expressions.
    Note that the result type is REAL.
 */
class int_div : public divop {
public:
  int_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), REAL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                        int_equal  class                        *
// ******************************************************************

/** Check equality of two integer expressions.
 */
class int_equal : public eqop {
public:
  int_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         int_neq  class                         *
// ******************************************************************

/** Check inequality of two integer expressions.
 */
class int_neq : public neqop {
public:
  int_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                          int_gt class                          *
// ******************************************************************

/** Check if one integer expression is greater than another.
 */
class int_gt : public gtop {
public:
  int_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                          int_ge class                          *
// ******************************************************************

/** Check if one integer expression is greater than or equal another.
 */
class int_ge : public geop {
public:
  int_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                          int_lt class                          *
// ******************************************************************

/** Check if one integer expression is less than another.
 */
class int_lt : public ltop {
public:
  int_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                          int_le class                          *
// ******************************************************************

/** Check if one integer expression is less than or equal another.
 */
class int_le : public leop {
public:
  int_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_le(Filename(), Linenumber(), l, r);
  }
};





//@}

