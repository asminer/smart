
// $Id$

#include "casting.h"
//@Include: casting.h

/** @name casting.cc
    @type File
    @args \ 

   Implementation of type casting between simple types.

 */

//@{

// ******************************************************************
// *                                                                *
// *                         typecast class                         *
// *                                                                *
// ******************************************************************

/**  The base class for most type conversions.  

     Actual conversion is handled in derived classes.
     So are the things like Copying.
*/   

class typecast : public expr {
protected:
  expr* opnd;
  type newtype;  
public:
  typecast(const char* fn, int line, type nt, expr* x) : expr(fn, line) {
    newtype = nt;
    opnd = x;
  }
  virtual ~typecast() {
    delete opnd;
  }
  virtual void show(ostream &s) const {
    s << GetType(newtype) << "(" << opnd << ")";
  }
  virtual type Type(int i) const {
    DCASSERT(i==0);
    return newtype;
  }
};

// ******************************************************************
// *                                                                *
// *                         int2real class                         *
// *                                                                *
// ******************************************************************

/**  Type promotion from integer to real.
*/   

class int2real : public typecast {
public:
  int2real(const char* fn, int line, expr* x) : typecast(fn, line, REAL, x) { }

  virtual expr* Copy() const { 
    return new int2real(Filename(), Linenumber(), CopyExpr(opnd));
  }
  virtual void Compute(int i, result &x) const;
};

void int2real::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;
  if (x.infinity) return;

  x.rvalue = x.ivalue;
}

// ******************************************************************
// *                                                                *
// *                         real2int class                         *
// *                                                                *
// ******************************************************************

/**  Type casting from real to integer.
*/   

class real2int : public typecast {
public:
  real2int(const char* fn, int line, expr* x) : typecast(fn, line, INT, x) { }

  virtual expr* Copy() const { 
    return new real2int(Filename(), Linenumber(), CopyExpr(opnd));
  }
  virtual void Compute(int i, result &x) const;
};

void real2int::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;
  if (x.infinity) return;

  // error checking here
  x.ivalue = int(x.rvalue);
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* SimpleTypecast(expr *e, type newtype, const char* file=NULL, int line=0)
{
  if (newtype == e->Type(0)) return e;

  // Note... it is assumed that e is promotable to "newtype".
  switch (e->Type(0)) {
    
    case BOOL : 
      return NULL;   
  }

  // Still here?  Slipped through the cracks.
  return NULL;
}

//@}

