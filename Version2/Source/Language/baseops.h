
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
  virtual void show(OutputStream &s) const;
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
  virtual void show(OutputStream &s) const;
  virtual int GetSums(int i, List <expr> *sums=NULL);
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
  virtual void show(OutputStream &s) const { binary_show(s, "-"); }
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
  virtual void show(OutputStream &s) const;
  virtual int GetProducts(int i, List <expr> *prods=NULL);
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
  virtual void show(OutputStream &s) const { binary_show(s, "/"); }
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
  virtual void show(OutputStream &s) const { binary_show(s, "=="); }
protected:
  /** Common to all eqops. 
      @param l	The value of the left operand (already computed).
      @param r	The value of the right operand (already computed).
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(const result &l, const result &r, compute_data &x) const {
    x.answer->Clear();
    // Most common case first
    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign, definitely not equal
      x.answer->bvalue = false;
      return false;
    }
    if (l.isInfinity() || r.isInfinity()) {
      // one infinity, one not: definitely not equal
      x.answer->bvalue = false;
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }

    // Can we get here?
    DCASSERT(0);
    return false;
  }
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
  virtual void show(OutputStream &s) const { binary_show(s, "!="); }
protected:
  /** Common to all neqops.
      @param l	The value of the left operand (already computed)
      @param r	The value of the right operand (already computed)
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(result &l, result &r, compute_data &x) const {
    x.answer->Clear();

    // Most common case first
    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign, definitely not equal
      x.answer->bvalue = true;
      return false;
    }
    if (l.isInfinity() || r.isInfinity()) {
      // one infinity, one not: definitely not equal
      x.answer->bvalue = true;
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }
    // can we get here?
    DCASSERT(0);
    return false;
  }
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
  virtual void show(OutputStream &s) const { binary_show(s, ">"); }
protected:
  /** Common to all gtops.
      @param l	The value of the left operand (already computed).
      @param r	The value of the right operand (already computed).
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(const result &l, const result &r, compute_data &x) const {
    x.answer->Clear();

    // most common case first
    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->bvalue = (l.ivalue>0);
      return false;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->bvalue = (l.ivalue>0);
      return false;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->bvalue = (r.ivalue<0);
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }

    // Still here?
    DCASSERT(0);
    // keep compiler happy
    return false;
  }
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
  virtual void show(OutputStream &s) const { binary_show(s, ">="); }
protected:
  /** Common to all geops.
      @param l	The value of the left operand (already computed)
      @param r	The value of the right operand (already computed)
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(const result &l, const result &r, compute_data &x) const {
    x.answer->Clear();

    // most common case
    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->bvalue = (l.ivalue>0);
      return false;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->bvalue = (l.ivalue>0);
      return false;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->bvalue = (r.ivalue<0);
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }
    DCASSERT(0);
    return false;
  }
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
  virtual void show(OutputStream &s) const { binary_show(s, "<"); }
protected:
  /** Common to all ltops.
      @param l	The value of the left operand (already computed)
      @param r	The value of the right operand (already computed)
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(const result &l, const result &r, compute_data &x) const {
    x.answer->Clear();

    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign.  
      x.answer->bvalue = (r.ivalue>0);
      return false;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->bvalue = (l.ivalue<0);
      return false;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->bvalue = (r.ivalue>0);
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }
    DCASSERT(0);
    return true;
  }
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
  virtual void show(OutputStream &s) const { binary_show(s, "<="); }
protected:
  /** Common to all leops.
      @param l	The value of the left operand (already computed)
      @param r	The value of the right operand (already computed)
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool CheckOpnds(result &l, result &r, compute_data &x) const {
    x.answer->Clear();
    
    if (l.isNormal() && r.isNormal()) return true;

    if (l.isError() || r.isError()) {
      x.answer->setError();
      return false;
    }
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return false;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if ((l.ivalue > 0) == (r.ivalue >0)) {
        // same sign infinity, this is undefined
        x.answer->setError();
        return false;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->bvalue = (r.ivalue>0);
      return false;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->bvalue = (l.ivalue<0);
      return false;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->bvalue = (r.ivalue>0);
      return false;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return false;
    }
    DCASSERT(0);
    return false;
  }
};





//@}

#endif

