
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
// *                       randbool_not class                       *
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
  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new randbool_not(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                       randbool_or  class                       *
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
  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new randbool_or(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                       randbool_and class                       *
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
  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new randbool_and(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                      randbool_equal class                      *
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
  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_equal(Filename(), Linenumber(), l, r);
  }
};

// ******************************************************************
// *                       randbool_neq class                       *
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
  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new randbool_neq(Filename(), Linenumber(), l, r);
  }
};




//@}

