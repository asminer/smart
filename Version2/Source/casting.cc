
// $Id$

#include "casting.h"

#include "infinity.h"

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

class typecast : public unary {
protected:
  type newtype;  
public:
  typecast(const char* fn, int line, type nt, expr* x) : unary(fn, line, x) {
    newtype = nt;
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
// *                       determ2rand  class                       *
// *                                                                *
// ******************************************************************

/**  Type promotion from const X to rand X.
*/   

class determ2rand : public typecast {
public:
  determ2rand(const char* fn, int line, type nt, expr* x) 
    : typecast(fn, line, nt, x) { }

  virtual void Sample(long &seed, int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2rand(Filename(), Linenumber(), Type(0), x);
  }
};

void determ2rand::Sample(long &, int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;
  if (x.error) {
    // Do error tracing here?
  }
}

// ******************************************************************
// *                                                                *
// *                      bool2procbool  class                      *
// *                                                                *
// ******************************************************************

/**  Type promotion from bool to proc bool.
*/   

class bool2procbool : public typecast {
public:
  bool2procbool(const char* fn, int line, expr* x) 
    : typecast(fn, line, PROC_BOOL, x) { }

  virtual void Compute(int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new bool2procbool(Filename(), Linenumber(), x);
  }
};

void bool2procbool::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  DCASSERT(false==x.infinity);  // Can this fail?

  expr *answer = MakeConstExpr(x.bvalue, Filename(), Linenumber());
  x.other = answer;
}

// ******************************************************************
// *                                                                *
// *                    bool2procrandbool  class                    *
// *                                                                *
// ******************************************************************

/**  Type promotion from bool to proc rand bool.
*/   

class bool2procrandbool : public typecast {
public:
  bool2procrandbool(const char* fn, int line, expr* x) 
    : typecast(fn, line, PROC_RAND_BOOL, x) { }

  virtual void Sample(long &seed, int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new bool2procrandbool(Filename(), Linenumber(), x);
  }
};

void bool2procrandbool::Sample(long &seed, int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  DCASSERT(false==x.infinity);  // Can this fail?

  expr *answer = MakeConstExpr(x.bvalue, Filename(), Linenumber());
  x.other = answer;
}

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

  virtual void Compute(int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new int2real(Filename(), Linenumber(), x);
  }
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
// *                       int2procint  class                       *
// *                                                                *
// ******************************************************************

/**  Type promotion from int to proc int.
*/   

class int2procint : public typecast {
public:
  int2procint(const char* fn, int line, expr* x) 
    : typecast(fn, line, PROC_INT, x) { }

  virtual void Compute(int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new int2procint(Filename(), Linenumber(), x);
  }
};

void int2procint::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  expr *answer = NULL;
  if (x.infinity) {
    answer = MakeInfinityExpr(x.ivalue, Filename(), Linenumber());
  } else {
    answer = MakeConstExpr(x.ivalue, Filename(), Linenumber());
  }
  x.other = answer;
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

  virtual void Compute(int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new real2int(Filename(), Linenumber(), x);
  }
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
// *                      rand2procrand  class                      *
// *                                                                *
// ******************************************************************

/**  Type promotion from rand X to proc rand X.
*/   

class rand2procrand : public typecast {
public:
  rand2procrand(const char* fn, int line, type nt, expr* x) 
    : typecast(fn, line, nt, x) { }

  virtual void Sample(long &seed, int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new rand2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

void rand2procrand::Sample(long &, int i, result &x) const
{
  DCASSERT(0==i);
  x.other = Copy(opnd);
}

// ******************************************************************
// *                                                                *
// *                      proc2procrand  class                      *
// *                                                                *
// ******************************************************************

/**  Type promotion from proc X to proc rand X.
*/   

class proc2procrand : public typecast {
public:
  proc2procrand(const char* fn, int line, type nt, expr* x) 
    : typecast(fn, line, nt, x) { }

  virtual void Sample(long &seed, int i, result &x) const;
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new proc2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

void proc2procrand::Sample(long &, int i, result &x) const
{
  DCASSERT(0==i);
  opnd->Compute(i, x);
  // check for errors?
}

// ******************************************************************
// ******************************************************************
// **                                                              **
// **                                                              **
// **                                                              **
// **            Global functions  to build expressions            **
// **                                                              **
// **                                                              **
// **                                                              **
// ******************************************************************
// ******************************************************************

expr* MakeTypecast(expr *e, type newtype, const char* file=NULL, int line=0)
{
  if (newtype == e->Type(0)) return e;

  // Note... it is assumed that e is promotable to "newtype".
  switch (e->Type(0)) {
    
    case BOOL: 

      switch (newtype) {
	case RAND_BOOL:   
	  return new determ2rand(file, line, RAND_BOOL, e);

	case PROC_BOOL:
	  return new bool2procbool(file, line, e);
      }

      return NULL;   



    case INT:

      switch (newtype) {
	case REAL:
	  return new int2real(file, line, e);

	// add PH_INT eventually
	
	case RAND_INT:
	  return new determ2rand(file, line, RAND_INT, e);

	case PROC_INT:
	  return new int2procint(file, line, e);
      }

      return NULL;


    // lots of others go here...
      
    case RAND_BOOL:
      if (newtype==PROC_RAND_BOOL) 
	return new rand2procrand(file, line, PROC_RAND_BOOL, e);
      return NULL;

    case RAND_INT:
      switch (newtype) {
	case PROC_RAND_INT:
	  return new rand2procrand(file, line, PROC_RAND_INT, e);

	// add RAND_REAL, PROC_RAND_REAL, PROC_PH_INT, ....
      }
      return NULL;

    case PROC_BOOL:
      if (newtype==PROC_RAND_BOOL)
	return new proc2procrand(file, line, PROC_RAND_BOOL, e);
      return NULL;

    case PROC_INT:
      switch (newtype) {
	case PROC_RAND_INT:
	  return new proc2procrand(file, line, PROC_RAND_INT, e);

	// Add PROC_REAL, ...
      }
      return NULL;
  }

  // Still here?  Slipped through the cracks.
  return NULL;
}

//@}

