
// $Id$

#include "baseops.h"


/** @name ops_const.h
    @type File
    @args \ 

   Operator classes, for constants.

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                                                                *
// *                       Operators for void                       *
// *                                                                *
// *         Yes, there is one, for "sequencing", i.e., ";"         *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         void_seq class                         *
// ******************************************************************

/** Sequence of void expressions, separated by semicolon.
 */
class void_seq : public assoc {
public:
  void_seq(const char* fn, int ln, expr** x, int n) : assoc(fn, ln, x, n) { }
  virtual void show(OutputStream &s) const {
    assoc_show(s, ";");
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return VOID;
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new void_seq(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Operators for bool                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         bool_not class                         *
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
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new bool_not(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                         bool_or  class                         *
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
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new bool_or(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                         bool_and class                         *
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
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new bool_and(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                        bool_equal class                        *
// ******************************************************************

/** Check equality of two boolean expressions.
 */
class bool_equal : public eqop {
public:
  bool_equal(const char* fn, int line, expr *l, expr *r) 
    : eqop(fn, line, l, r) { }
  
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

// ******************************************************************
// *                         bool_neq class                         *
// ******************************************************************

/** Check inequality of two boolean expressions.
 */
class bool_neq : public neqop {
public:
  bool_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new bool_neq(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Operators for  int                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         int_neg  class                         *
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
    DCASSERT(0==i);
    return INT;
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
    DCASSERT(0==i);
    return INT;
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
    DCASSERT(0==i);
    return INT;
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
    DCASSERT(0==i);
    return REAL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new int_le(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Operators for real                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         real_neg class                         *
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
    DCASSERT(0==i);
    return REAL;
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
    DCASSERT(0==i);
    return REAL;
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
    DCASSERT(0==i);
    return REAL;
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
    DCASSERT(0==i);
    return REAL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
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
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(Rng *r, const state *st, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new real_le(Filename(), Linenumber(), l, r);
  }
};





//@}

