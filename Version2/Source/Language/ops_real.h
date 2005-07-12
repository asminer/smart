
// $Id$

#include "baseops.h"


/** @name ops_real.h
    @type File
    @args \ 

   Operator classes, for reals.
   That means real, rand real, proc real, ...

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                         real_neg class                         *
// ******************************************************************

/** Negation of a real expression.
 */
class real_neg : public negop {
public:
  real_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(opnd);
    return opnd->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new real_neg(Filename(), Linenumber(), x);
  }
};


// ******************************************************************
// *                         real_add class                         *
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
    DCASSERT(operands[0]);
    return operands[0]->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new real_add(Filename(), Linenumber(), x, n);
  }
};



// ******************************************************************
// *                         real_sub class                         *
// ******************************************************************

/** Subtraction of two real expressions.
 */
class real_sub : public subop {
public:
  real_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return left->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                        real_mult  class                        *
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
    DCASSERT(operands[0]);
    return operands[0]->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new real_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                         real_div class                         *
// ******************************************************************

/** Division of two real expressions.
 */
class real_div : public divop {
public:
  real_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return left->Type(i);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                        real_equal class                        *
// ******************************************************************

/** Check equality of two real expressions.
    To do still: check precision option, etc.
 */
class real_equal : public eqop {
public:
  real_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         real_neq class                         *
// ******************************************************************

/** Check inequality of two real expressions.
 */
class real_neq : public neqop {
public:
  real_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                         real_gt  class                         *
// ******************************************************************

/** Check if one real expression is greater than another.
 */
class real_gt : public gtop {
public:
  real_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         real_ge  class                         *
// ******************************************************************

/** Check if one real expression is greater than or equal another.
 */
class real_ge : public geop {
public:
  real_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         real_lt  class                         *
// ******************************************************************

/** Check if one real expression is less than another.
 */
class real_lt : public ltop {
public:
  real_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                         real_le  class                         *
// ******************************************************************

/** Check if one real expression is less than or equal another.
 */
class real_le : public leop {
public:
  real_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(left);
    return ApplyPM(left->Type(i), BOOL);
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_le(Filename(), Linenumber(), l, r);
  }
};





//@}

