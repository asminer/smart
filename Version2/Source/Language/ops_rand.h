
// $Id$

#include "baseops.h"


/** @name ops_rand.h
    @type File
    @args \ 

   Operator classes, for random variables.

   Same as for constants, except replace "Compute" with "Sample".

 */

//@{


// ******************************************************************
// *                                                                *
// *                                                                *
// *                    Operators for  rand bool                    *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      rand_bool_not  class                      *
// ******************************************************************

/** Negation of a random boolean expression.
 */
class rand_bool_not : public negop {
public:
  rand_bool_not(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new rand_bool_not(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                       rand_bool_or class                       *
// ******************************************************************

/** Or of random boolean expressions.
 */
class rand_bool_or : public addop {
public:
  rand_bool_or(const char* fn, int line, expr **x, int n): addop(fn, line, x, n) { }
  rand_bool_or(const char* fn, int line, expr *l, expr* r): addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_bool_or(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                      rand_bool_and  class                      *
// ******************************************************************

/** And of two random boolean expressions.
 */
class rand_bool_and : public multop {
public:
  rand_bool_and(const char* fn, int line, expr **x, int n) 
  : multop(fn, line, x, n) { }

  rand_bool_and(const char* fn, int line, expr *l, expr *r) 
  : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_bool_and(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                     rand_bool_equal  class                     *
// ******************************************************************

/** Check equality of two random boolean expressions.
 */
class rand_bool_equal : public eqop {
public:
  rand_bool_equal(const char* fn, int line, expr *l, expr *r) 
    : eqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr* r) {
    return new rand_bool_equal(Filename(), Linenumber(), l, r);
  }
};

// ******************************************************************
// *                      rand_bool_neq  class                      *
// ******************************************************************

/** Check inequality of two random boolean expressions.
 */
class rand_bool_neq : public neqop {
public:
  rand_bool_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_bool_neq(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Operators for rand int                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       rand_int_neg class                       *
// ******************************************************************

/** Negation of an random integer expression.
 */
class rand_int_neg : public negop {
public:
  rand_int_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_INT;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new rand_int_neg(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                       rand_int_add class                       *
// ******************************************************************

/** Addition of random integer expressions.
 */
class rand_int_add : public addop {
public:
  rand_int_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  rand_int_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_INT;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_int_add(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                       rand_int_sub class                       *
// ******************************************************************

/** Subtraction of two random integer expressions.
 */
class rand_int_sub : public subop {
public:
  rand_int_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_INT;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      rand_int_mult  class                      *
// ******************************************************************

/** Multiplication of random integer expressions.
 */
class rand_int_mult : public multop {
public:
  rand_int_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  rand_int_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_INT;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_int_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                       rand_int_div class                       *
// ******************************************************************

/** Division of two random integer expressions.
    Note that the result type is RAND_REAL.
 */
class rand_int_div : public divop {
public:
  rand_int_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      rand_int_equal class                      *
// ******************************************************************

/** Check equality of two random integer expressions.
 */
class rand_int_equal : public eqop {
public:
  rand_int_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_int_neq class                       *
// ******************************************************************

/** Check inequality of two random integer expressions.
 */
class rand_int_neq : public neqop {
public:
  rand_int_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                       rand_int_gt  class                       *
// ******************************************************************

/** Check if one random integer expression is greater than another.
 */
class rand_int_gt : public gtop {
public:
  rand_int_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_int_ge  class                       *
// ******************************************************************

/** Check if one random integer expression is greater than or equal another.
 */
class rand_int_ge : public geop {
public:
  rand_int_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_int_lt  class                       *
// ******************************************************************

/** Check if one random integer expression is less than another.
 */
class rand_int_lt : public ltop {
public:
  rand_int_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_int_le  class                       *
// ******************************************************************

/** Check if one random integer expression is less than or equal another.
 */
class rand_int_le : public leop {
public:
  rand_int_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_int_le(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                    Operators for  rand real                    *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      rand_real_neg  class                      *
// ******************************************************************

/** Negation of a rand_real expression.
 */
class rand_real_neg : public negop {
public:
  rand_real_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new rand_real_neg(Filename(), Linenumber(), x);
  }
};


// ******************************************************************
// *                      rand_real_add  class                      *
// ******************************************************************

/** Addition of rand_real expressions.
 */
class rand_real_add : public addop {
public:
  rand_real_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  rand_real_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_real_add(Filename(), Linenumber(), x, n);
  }
};



// ******************************************************************
// *                      rand_real_sub  class                      *
// ******************************************************************

/** Subtraction of two rand_real expressions.
 */
class rand_real_sub : public subop {
public:
  rand_real_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      rand_real_mult class                      *
// ******************************************************************

/** Multiplication of rand_real expressions.
 */
class rand_real_mult : public multop {
public:
  rand_real_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  rand_real_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new rand_real_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                      rand_real_div  class                      *
// ******************************************************************

/** Division of two rand_real expressions.
 */
class rand_real_div : public divop {
public:
  rand_real_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_REAL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                     rand_real_equal  class                     *
// ******************************************************************

/** Check equality of two rand_real expressions.
    To do still: check precision option, etc.
 */
class rand_real_equal : public eqop {
public:
  rand_real_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      rand_real_neq  class                      *
// ******************************************************************

/** Check inequality of two rand_real expressions.
 */
class rand_real_neq : public neqop {
public:
  rand_real_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                       rand_real_gt class                       *
// ******************************************************************

/** Check if one rand_real expression is greater than another.
 */
class rand_real_gt : public gtop {
public:
  rand_real_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_real_ge class                       *
// ******************************************************************

/** Check if one rand_real expression is greater than or equal another.
 */
class rand_real_ge : public geop {
public:
  rand_real_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_real_lt class                       *
// ******************************************************************

/** Check if one rand_real expression is less than another.
 */
class rand_real_lt : public ltop {
public:
  rand_real_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       rand_real_le class                       *
// ******************************************************************

/** Check if one rand_real expression is less than or equal another.
 */
class rand_real_le : public leop {
public:
  rand_real_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return RAND_BOOL;
  }
  virtual void Sample(Rng &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new rand_real_le(Filename(), Linenumber(), l, r);
  }
};




//@}

