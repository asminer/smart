
// $Id$

#include "baseops.h"


/** @name ops_proc.h
    @type File
    @args \ 

   Operator classes, for procs

 */

//@{


// ******************************************************************
// *                                                                *
// *                                                                *
// *                    Operators for  proc_bool                    *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      proc_bool_not  class                      *
// ******************************************************************

/** Negation of a proc boolean expression.
 */
class proc_bool_not : public negop {
public:
  proc_bool_not(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_bool_not(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                       proc_bool_or class                       *
// ******************************************************************

/** Or of proc boolean expressions.
 */
class proc_bool_or : public addop {
public:
  proc_bool_or(const char* fn, int line, expr **x, int n): addop(fn, line, x, n) { }
  proc_bool_or(const char* fn, int line, expr *l, expr* r): addop(fn, line, l, r) { }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_bool_or(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                      proc_bool_and  class                      *
// ******************************************************************

/** And of two proc boolean expressions.
 */
class proc_bool_and : public multop {
public:
  proc_bool_and(const char* fn, int line, expr **x, int n) 
  : multop(fn, line, x, n) { }

  proc_bool_and(const char* fn, int line, expr *l, expr *r) 
  : multop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_bool_and(Filename(), Linenumber(), x, n);
  }
};

// ******************************************************************
// *                     proc_bool_equal  class                     *
// ******************************************************************

/** Check equality of two proc boolean expressions.
 */
class proc_bool_equal : public eqop {
public:
  proc_bool_equal(const char* fn, int line, expr *l, expr *r) 
    : eqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr* r) {
    return new proc_bool_equal(Filename(), Linenumber(), l, r);
  }
};

// ******************************************************************
// *                      proc_bool_neq  class                      *
// ******************************************************************

/** Check inequality of two proc boolean expressions.
 */
class proc_bool_neq : public neqop {
public:
  proc_bool_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_bool_neq(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                     Operators for proc_int                     *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       proc_int_neg class                       *
// ******************************************************************

/** Negation of a proc integer expression.
 */
class proc_int_neg : public negop {
public:
  proc_int_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_INT;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_int_neg(Filename(), Linenumber(), x);
  }
};

// ******************************************************************
// *                       proc_int_add class                       *
// ******************************************************************

/** Addition of proc integer expressions.
 */
class proc_int_add : public addop {
public:
  proc_int_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  proc_int_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_INT;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_int_add(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                       proc_int_sub class                       *
// ******************************************************************

/** Subtraction of two proc integer expressions.
 */
class proc_int_sub : public subop {
public:
  proc_int_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_INT;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      proc_int_mult  class                      *
// ******************************************************************

/** Multiplication of proc integer expressions.
 */
class proc_int_mult : public multop {
public:
  proc_int_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  proc_int_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_INT;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_int_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                       proc_int_div class                       *
// ******************************************************************

/** Division of two proc integer expressions.
    Note that the result type is PROC_REAL.
 */
class proc_int_div : public divop {
public:
  proc_int_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      proc_int_equal class                      *
// ******************************************************************

/** Check equality of two proc integer expressions.
 */
class proc_int_equal : public eqop {
public:
  proc_int_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_int_neq class                       *
// ******************************************************************

/** Check inequality of two proc integer expressions.
 */
class proc_int_neq : public neqop {
public:
  proc_int_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                       proc_int_gt  class                       *
// ******************************************************************

/** Check if one proc integer expression is greater than another.
 */
class proc_int_gt : public gtop {
public:
  proc_int_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_int_ge  class                       *
// ******************************************************************

/** Check if one proc integer expression is greater than or equal another.
 */
class proc_int_ge : public geop {
public:
  proc_int_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_int_lt  class                       *
// ******************************************************************

/** Check if one proc integer expression is less than another.
 */
class proc_int_lt : public ltop {
public:
  proc_int_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_int_le  class                       *
// ******************************************************************

/** Check if one proc_integer expression is less than or equal another.
 */
class proc_int_le : public leop {
public:
  proc_int_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_int_le(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                    Operators for  proc_real                    *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      proc_real_neg  class                      *
// ******************************************************************

/** Negation of a proc_real expression.
 */
class proc_real_neg : public negop {
public:
  proc_real_neg(const char* fn, int line, expr *x) : negop(fn, line, x) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new proc_real_neg(Filename(), Linenumber(), x);
  }
};


// ******************************************************************
// *                      proc_real_add  class                      *
// ******************************************************************

/** Addition of proc_real expressions.
 */
class proc_real_add : public addop {
public:
  proc_real_add(const char* fn, int line, expr **x, int n) 
    : addop(fn, line, x, n) { }
  
  proc_real_add(const char* fn, int line, expr *l, expr *r) 
    : addop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_real_add(Filename(), Linenumber(), x, n);
  }
};



// ******************************************************************
// *                      proc_real_sub  class                      *
// ******************************************************************

/** Subtraction of two proc_real expressions.
 */
class proc_real_sub : public subop {
public:
  proc_real_sub(const char* fn, int line, expr *l, expr *r) : subop(fn,line,l,r) {}
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_sub(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      proc_real_mult class                      *
// ******************************************************************

/** Multiplication of proc_real expressions.
 */
class proc_real_mult : public multop {
public:
  proc_real_mult(const char* fn, int line, expr **x, int n) 
    : multop(fn,line,x,n) { }
  
  proc_real_mult(const char* fn, int line, expr *l, expr *r)
    : multop(fn,line,l,r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_real_mult(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                      proc_real_div  class                      *
// ******************************************************************

/** Division of two proc_real expressions.
 */
class proc_real_div : public divop {
public:
  proc_real_div(const char* fn, int line, expr *l, expr *r) 
    : divop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_REAL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_div(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                     proc_real_equal  class                     *
// ******************************************************************

/** Check equality of two proc_real expressions.
    To do still: check precision option, etc.
 */
class proc_real_equal : public eqop {
public:
  proc_real_equal(const char* fn, int line, expr *l, expr *r)
    : eqop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_equal(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                      proc_real_neq  class                      *
// ******************************************************************

/** Check inequality of two proc_real expressions.
 */
class proc_real_neq : public neqop {
public:
  proc_real_neq(const char* fn, int line, expr *l, expr *r)
    : neqop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_neq(Filename(), Linenumber(), l, r);
  }
};



// ******************************************************************
// *                       proc_real_gt class                       *
// ******************************************************************

/** Check if one proc_real expression is greater than another.
 */
class proc_real_gt : public gtop {
public:
  proc_real_gt(const char* fn, int line, expr *l, expr *r)
    : gtop(fn, line, l, r) { }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_gt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_real_ge class                       *
// ******************************************************************

/** Check if one proc_real expression is greater than or equal another.
 */
class proc_real_ge : public geop {
public:
  proc_real_ge(const char* fn, int line, expr *l, expr *r)
    : geop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_ge(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_real_lt class                       *
// ******************************************************************

/** Check if one proc_real expression is less than another.
 */
class proc_real_lt : public ltop {
public:
  proc_real_lt(const char* fn, int line, expr *l, expr *r)
    : ltop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_lt(Filename(), Linenumber(), l, r);
  }
};


// ******************************************************************
// *                       proc_real_le class                       *
// ******************************************************************

/** Check if one proc_real expression is less than or equal another.
 */
class proc_real_le : public leop {
public:
  proc_real_le(const char* fn, int line, expr *l, expr *r)
    : leop(fn, line, l, r) { }
  
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *l, expr *r) {
    return new proc_real_le(Filename(), Linenumber(), l, r);
  }
};

// ******************************************************************
// *                                                                *
// *                    Operators for proc_state                    *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      proc_state_seq class                      *
// ******************************************************************

/** Sequence of state changes, separated by semicolon.
    Want to consider this as a list of products
    (for "kronecker consistency" and such)
    so it is derived from class "multop".
 */
class proc_state_seq : public multop {
public:
  proc_state_seq(const char* fn, int ln, expr** x, int n) : multop(fn, ln, x, n) { }
  virtual void show(OutputStream &s) const {
    assoc_show(s, ";");
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_STATE;
  }
  virtual void NextState(const state &c, state &n, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_state_seq(Filename(), Linenumber(), x, n);
  }
};




//@}

