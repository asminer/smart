
// $Id$

#ifndef BASEOPS_H
#define BASEOPS_H

/** @name baseops.h
    @type File
    @args \ 

  Generic operator base classes.
  Derived classes are type specific.
 
  Useful to prevent 8-20 cut and pastes of the same function...
  
 */

#include "exprs.h"

//@{
  

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class for negation.
 
      Note: this includes binary "not"
*/  

class negop : public unary {
public:
  negop(const char* fn, int line, expr* x) : unary (fn, line, x) {}
  virtual void show(ostream &s) const;
};

// ******************************************************************
// *                                                                *
// *                          addop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of addition classes.
 
      This saves you from having to implement a few of
      the virtual functions, because they are all the
      same for addition.

      Note: this includes logical or
*/  

class addop : public assoc {
public:
  addop(const char* fn, int line, expr** x, int n) : assoc(fn, line, x, n) {}
  addop(const char* fn, int line, expr* l, expr* r) : assoc(fn, line, l, r) {}
  virtual void show(ostream &s) const;
  virtual int GetSums(int i, expr **sums=NULL, int N=0, int offset=0);
};

// ******************************************************************
// *                                                                *
// *                          subop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of subtraction classes.
*/  

class subop : public binary {
public:
  subop(const char* fn, int line, expr* l, expr* r) : binary(fn, line, l, r) {}
  virtual void show(ostream &s) const { binary_show(s, "-"); }
};

// ******************************************************************
// *                                                                *
// *                          multop class                          *
// *                                                                *
// ******************************************************************

/**   The base class of multiplication classes.
 
      This saves you from having to implement a few of
      the virtual functions, because they are all the
      same for multiplication.

      Note: this includes logical and
*/  

class multop : public assoc {
public:
  multop(const char* fn, int line, expr** x, int n) : assoc(fn, line, x, n){}
  multop(const char* fn, int line, expr* l, expr* r) : assoc(fn, line, l, r){}
  virtual void show(ostream &s) const;
  virtual int GetProducts(int i, expr **prods=NULL, int N=0, int offset=0);
};

// ******************************************************************
// *                                                                *
// *                          divop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of division classes.
*/  

class divop : public binary {
public:
  divop(const char* fn, int line, expr* l, expr* r) : binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, "/"); }
};


// ******************************************************************
// *                                                                *
// *                        consteqop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for equality check, for constants (i.e., deterministic).
*/  

class consteqop : public binary {
protected:
  /** The common part of consteqop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign, definitely not equal
      x.bvalue = false;
      return false;
    }
    if (l.infinity || r.infinity) {
      // one infinity, one not: definitely not equal
      x.bvalue = false;
      return false;
    }
    return true;
  }
public:
  consteqop(const char* fn, int line, expr* l, expr* r) : binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, "=="); }
};


// ******************************************************************
// *                                                                *
// *                        constneqop class                        *
// *                                                                *
// ******************************************************************

/**   The base class for inequality check, for constants.
*/  

class constneqop : public binary {
protected:
  /** The common part of constneqop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign, definitely not equal
      x.bvalue = true;
      return false;
    }
    if (l.infinity || r.infinity) {
      // one infinity, one not: definitely not equal
      x.bvalue = true;
      return false;
    }
    return true;
  }
public:
  constneqop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, "!="); }
};


// ******************************************************************
// *                                                                *
// *                        constgtop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than check, for constants.
*/  

class constgtop : public binary {
protected:
  /** The common part of constgtop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.bvalue = (l.ivalue>0);
      return false;
    }
    if (l.infinity) {
      // first is infinity, the other isn't; definitely greater.
      x.bvalue = true;
      return false;
    }
    if (r.infinity) {
      // second is infinity, the other isn't; definitely not greater.
      x.bvalue = false;
      return false;
    }
    return true;
  }
public:
  constgtop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, ">"); }
};


// ******************************************************************
// *                                                                *
// *                        constgeop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than or equal check, for constants.
*/  

class constgeop : public binary {
protected:
  /** The common part of constgeop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.bvalue = (l.ivalue>0);
      return false;
    }
    if (l.infinity) {
      // first is infinity, the other isn't; definitely greater.
      x.bvalue = true;
      return false;
    }
    if (r.infinity) {
      // second is infinity, the other isn't; definitely not greater.
      x.bvalue = false;
      return false;
    }
    return true;
  }
public:
  constgeop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, ">="); }
};


// ******************************************************************
// *                                                                *
// *                        constltop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for less than check, for constants.
*/  

class constltop : public binary {
protected:
  /** The common part of constltop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign.  
      x.bvalue = (r.ivalue>0);
      return false;
    }
    if (l.infinity) {
      // first is infinity, the other isn't; definitely greater.
      x.bvalue = false;
      return false;
    }
    if (r.infinity) {
      // second is infinity, the other isn't; definitely not greater.
      x.bvalue = true;
      return false;
    }
    return true;
  }
public:
  constltop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, "<"); }
};


// ******************************************************************
// *                                                                *
// *                        constleop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than or equal check, for constants.
*/  

class constleop : public binary {
protected:
  /** The common part of constleop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    DCASSERT(left);
    DCASSERT(right);
    x.Clear();
    left->Compute(0, l);
    right->Compute(0, r);

    if (l.error) {
      // some option about error tracing here, I guess...
      x.error = l.error;
      return false;
    }
    if (r.error) {
      x.error = r.error;
      return false;
    }
    if (l.null || r.null) {
      x.null = true;
      return false;
    }
    if (l.infinity && r.infinity) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.error = CE_Undefined;
        x.null = true;
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.bvalue = (r.ivalue>0);
      return false;
    }
    if (l.infinity) {
      // first is infinity, the other isn't; definitely greater.
      x.bvalue = false;
      return false;
    }
    if (r.infinity) {
      // second is infinity, the other isn't; definitely not greater.
      x.bvalue = true;
      return false;
    }
    return true;
  }
public:
  constleop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r){}
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const { binary_show(s, "<="); }
};



// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for equality check, for non-constants 
*/  

class eqop : public binary {
public:
  eqop(const char* fn, int line, expr* l, expr* r): binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, "=="); }
};



// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class for inequality check, for non-constants 
*/  

class neqop : public binary {
public:
  neqop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, "!="); }
};



// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater than check, for non-constants 
*/  

class gtop : public binary {
public:
  gtop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, ">"); }
};



// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater-equal check, for non-constants 
*/  

class geop : public binary {
public:
  geop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, ">="); }
};



// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less than check, for non-constants 
*/  

class ltop : public binary {
public:
  ltop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, "<"); }
};



// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less-equal check, for non-constants 
*/  

class leop : public binary {
public:
  leop(const char* fn, int line, expr* l, expr* r):binary(fn,line,l,r) {}
  virtual void show(ostream &s) const { binary_show(s, "<="); }
};





//@}

#endif

