
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

class negop : public expr {
protected:
  expr* opnd;
public:
  negop(const char* fn, int line, expr* x);
  virtual ~negop();
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

      Note: this includes binary or
*/  

class addop : public expr {
protected:
  expr* left;
  expr* right;
public:
  addop(const char* fn, int line, expr* l, expr* r);
  virtual ~addop();
  virtual void show(ostream &s) const;
  virtual int GetSums(expr **sums=NULL, int N=0, int offset=0);
};

// ******************************************************************
// *                                                                *
// *                          subop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of subtraction classes.
*/  

class subop : public expr {
protected:
  expr* left;
  expr* right;
public:
  subop(const char* fn, int line, expr* l, expr* r);
  virtual ~subop();
  virtual void show(ostream &s) const;
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

      Note: this includes binary and
*/  

class multop : public expr {
protected:
  expr* left;
  expr* right;
public:
  multop(const char* fn, int line, expr* l, expr* r);
  virtual ~multop();
  virtual void show(ostream &s) const;
  virtual int GetProducts(expr **prods=NULL, int N=0, int offset=0);
};

// ******************************************************************
// *                                                                *
// *                          divop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of division classes.
*/  

class divop : public expr {
protected:
  expr* left;
  expr* right;
public:
  divop(const char* fn, int line, expr* l, expr* r);
  virtual ~divop();
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        consteqop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for equality check, for constants (i.e., deterministic).
*/  

class consteqop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of consteqop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  consteqop(const char* fn, int line, expr* l, expr* r);
  virtual ~consteqop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        constneqop class                        *
// *                                                                *
// ******************************************************************

/**   The base class for inequality check, for constants.
*/  

class constneqop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of constneqop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  constneqop(const char* fn, int line, expr* l, expr* r);
  virtual ~constneqop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        constgtop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than check, for constants.
*/  

class constgtop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of constgtop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  constgtop(const char* fn, int line, expr* l, expr* r);
  virtual ~constgtop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        constgeop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than or equal check, for constants.
*/  

class constgeop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of constgeop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  constgeop(const char* fn, int line, expr* l, expr* r);
  virtual ~constgeop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        constltop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for less than check, for constants.
*/  

class constltop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of constltop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  constltop(const char* fn, int line, expr* l, expr* r);
  virtual ~constltop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};


// ******************************************************************
// *                                                                *
// *                        constleop  class                        *
// *                                                                *
// ******************************************************************

/**   The base class for greater than or equal check, for constants.
*/  

class constleop : public expr {
protected:
  expr* left;
  expr* right;
  /** The common part of constleop for derived classes.
      @param l	The value of the left operand.
      @param r	The value of the right operand.
      @param x	The result.  (Can be set on error conditions, etc.)
      @return	true if the computation should continue after calling this.
  */
  inline bool ComputeOpnds(result &l, result &r, result &x) const {
    if (left) left->Compute(0, l); else l.null = true;
    if (right) right->Compute(0, r); else r.null = true;

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
  constleop(const char* fn, int line, expr* l, expr* r);
  virtual ~constleop();
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for equality check, for non-constants 
*/  

class eqop : public expr {
protected:
  expr* left;
  expr* right;
public:
  eqop(const char* fn, int line, expr* l, expr* r);
  virtual ~eqop();
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class for inequality check, for non-constants 
*/  

class neqop : public expr {
protected:
  expr* left;
  expr* right;
public:
  neqop(const char* fn, int line, expr* l, expr* r);
  virtual ~neqop();
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater than check, for non-constants 
*/  

class gtop : public expr {
protected:
  expr* left;
  expr* right;
public:
  gtop(const char* fn, int line, expr* l, expr* r);
  virtual ~gtop();
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater-equal check, for non-constants 
*/  

class geop : public expr {
protected:
  expr* left;
  expr* right;
public:
  geop(const char* fn, int line, expr* l, expr* r);
  virtual ~geop();
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less than check, for non-constants 
*/  

class ltop : public expr {
protected:
  expr* left;
  expr* right;
public:
  ltop(const char* fn, int line, expr* l, expr* r);
  virtual ~ltop();
  virtual void show(ostream &s) const;
};



// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less-equal check, for non-constants 
*/  

class leop : public expr {
protected:
  expr* left;
  expr* right;
public:
  leop(const char* fn, int line, expr* l, expr* r);
  virtual ~leop();
  virtual void show(ostream &s) const;
};





//@}

#endif

