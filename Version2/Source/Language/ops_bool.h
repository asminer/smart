
// $Id$

#include "baseops.h"


/** @name ops_bool.h
    @type File
    @args \ 

   Operator classes, for boolean variables.
   This includes bool, rand bool, proc bool, proc rand bool.

 */

//@{


// ******************************************************************
// *                         bool_not class                         *
// ******************************************************************

/** Negation of a boolean expression.
 */
class bool_not : public negop {
public:
  bool_not(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    return opnd->Type(i);
  }
  virtual void Compute(compute_data &x);
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
    DCASSERT(operands[0]);
    return operands[0]->Type(i);
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new bool_or(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                         bool_and class                         *
// ******************************************************************

/** And of boolean expressions.
 */
class bool_and : public multop {
public:
  bool_and(const char* fn, int line, expr **x, int n) 
  : multop(fn, line, x, n) { }

  bool_and(const char* fn, int line, expr *l, expr *r) 
  : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(operands[0]);
    return operands[0]->Type(i);
  }
  virtual void Compute(compute_data &x);
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
    DCASSERT(left);
    return left->Type(i);
  }
  virtual void Compute(compute_data &x);
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
    DCASSERT(left);
    return left->Type(i);
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new bool_neq(Filename(), Linenumber(), l, r);
  }
};



//@}

