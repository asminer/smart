
// $Id$

#include "casting.h"

#include "infinity.h"
#include "../Rng/rng.h"
#include <math.h>

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
  virtual void show(OutputStream &s) const {
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

  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2rand(Filename(), Linenumber(), Type(0), x);
  }
};

void determ2rand::Sample(Rng &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
}

// ******************************************************************
// *                                                                *
// *                       determ2proc  class                       *
// *                                                                *
// ******************************************************************

/**  Type promotion from const X to proc X.
*/   

class determ2proc : public typecast {
public:
  determ2proc(const char* fn, int line, type nt, expr* x) 
    : typecast(fn, line, nt, x) { }

  virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2proc(Filename(), Linenumber(), Type(0), x);
  }
};

void determ2proc::Compute(const state &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
}

// ******************************************************************
// *                                                                *
// *                     determ2procrand  class                     *
// *                                                                *
// ******************************************************************

/**  Type promotion from const X to proc rand X.
*/   

class determ2procrand : public typecast {
public:
  determ2procrand(const char* fn, int line, type nt, expr* x) 
    : typecast(fn, line, nt, x) { }

  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

void determ2procrand::Sample(Rng &, const state &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
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

  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new rand2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

void rand2procrand::Sample(Rng &seed, const state &, int i, result &x) 
{
  SafeSample(opnd, seed, i, x);
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

  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new proc2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

void proc2procrand::Sample(Rng &, const state &s, int i, result &x) 
{
  SafeCompute(opnd, s, i, x);
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

  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new int2real(Filename(), Linenumber(), x);
  }
};

void int2real::Compute(int i, result &x)
{
  SafeCompute(opnd, i, x);
  if (!x.isNormal()) return;
  x.rvalue = x.ivalue;
}

// ******************************************************************
// *                                                                *
// *                         int2expo class                         *
// *                                                                *
// ******************************************************************

/**  Type promotion from integer to expo.
*/   

class int2expo : public typecast {
public:
  int2expo(const char* fn, int line, expr* x) : typecast(fn, line, EXPO, x) { }

  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new int2expo(Filename(), Linenumber(), x);
  }
};

void int2expo::Compute(int i, result &x)
{
  SafeCompute(opnd, i, x);
  if (x.isNormal()) {
    x.rvalue = x.ivalue;
    return;
  } 
  // if (x.isInfinity()) {
  // x.rvalue = 0.0;
  // }
  // the rest are safely propogated
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

  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new real2int(Filename(), Linenumber(), x);
  }
};

void real2int::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(0, x); 

  if (!x.isNormal()) return;

  x.ivalue = int(x.rvalue);
}

// ******************************************************************
// *                                                                *
// *                        real2expo  class                        *
// *                                                                *
// ******************************************************************

/**  Type casting from real to expo.
*/   
class real2expo : public typecast {
public:
  real2expo(const char* fn, int line, expr* x) : typecast(fn, line, EXPO, x) { }

  virtual void Compute(int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new real2int(Filename(), Linenumber(), x);
  }
};

void real2expo::Compute(int i, result &x)
{
  SafeCompute(opnd, i, x);
}

// ******************************************************************
// *                                                                *
// *                      expo2randreal  class                      *
// *                                                                *
// ******************************************************************

/**  Type promotion from expo to rand real.
*/   
class expo2randreal : public typecast {
public:
  expo2randreal(const char* fn, int line, expr* x) 
    : typecast(fn, line, RAND_REAL, x) { }

  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new expo2randreal(Filename(), Linenumber(), x);
  }
};

void expo2randreal::Sample(Rng &seed, int i, result &x)
{
  SafeCompute(opnd, i, x);

  if (x.isNormal()) {
    if (x.rvalue) {
      double mean = 1.0 / x.rvalue;
      x.rvalue = -mean * log(seed.uniform());
    } else {
      x.setInfinity();  // expo(0) has mean infinity...
    }
    return;
  }
  if (x.isInfinity()) {  // expo(infintity) has mean 0
    x.Clear();
    x.rvalue = 0.0;
    return;
  }
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

expr* MakeTypecast(expr *e, type newtype, const char* file, int line)
{
  if (NULL==e || ERROR==e || DEFLT==e) return e;
  if (newtype == e->Type(0)) return e;

  // Note... it is assumed that e is promotable to "newtype".
  switch (e->Type(0)) {
    
    // --------------------------------------------------------------
    case BOOL: 

      switch (newtype) {
	case RAND_BOOL:   
	  return new determ2rand(file, line, RAND_BOOL, e);

	case PROC_BOOL:
	  return new determ2proc(file, line, PROC_BOOL, e);

	case PROC_RAND_BOOL:
	  return new determ2procrand(file, line, PROC_RAND_BOOL, e);
      }

      return NULL;   


    // --------------------------------------------------------------
    case INT:

      switch (newtype) {
	case REAL:
	  return new int2real(file, line, e);

	// add PH_INT eventually

	case EXPO:
	  return new int2expo(file, line, e);
	
	case RAND_INT:
	  return new determ2rand(file, line, RAND_INT, e);

	case PROC_INT:
	  return new determ2proc(file, line, PROC_INT, e);

	case PROC_RAND_INT:
	  return new determ2procrand(file, line, PROC_RAND_INT, e);
      }

      return NULL;

    // --------------------------------------------------------------
    case REAL:
      switch (newtype) {
	case INT:
	  return new real2int(file, line, e);

	case EXPO:
	  return new real2expo(file, line, e);
	
	case RAND_REAL:
	  return new determ2rand(file, line, RAND_REAL, e);

	case PROC_REAL:
	  return new determ2proc(file, line, PROC_REAL, e);

	case PROC_RAND_REAL:
	  return new determ2procrand(file, line, PROC_RAND_REAL, e);
      }

      return NULL;


    // --------------------------------------------------------------
    case EXPO:
      switch (newtype) {
	case RAND_REAL:
	  return new expo2randreal(file, line, e);
      }
      return NULL;
      
    // --------------------------------------------------------------
    case RAND_BOOL:
      if (newtype==PROC_RAND_BOOL) 
	return new rand2procrand(file, line, PROC_RAND_BOOL, e);
      return NULL;

    // --------------------------------------------------------------
    case RAND_INT:
      switch (newtype) {
	case PROC_RAND_INT:
	  return new rand2procrand(file, line, PROC_RAND_INT, e);

	// add RAND_REAL, PROC_RAND_REAL, PROC_PH_INT, ....
      }
      return NULL;

    // --------------------------------------------------------------
    case RAND_REAL:
      switch (newtype) {
	case PROC_RAND_REAL:
	  return new rand2procrand(file, line, PROC_RAND_REAL, e);

      }
      return NULL;

    // --------------------------------------------------------------
    case PROC_BOOL:
      if (newtype==PROC_RAND_BOOL)
	return new proc2procrand(file, line, PROC_RAND_BOOL, e);
      return NULL;

    // --------------------------------------------------------------
    case PROC_INT:
      switch (newtype) {
	case PROC_RAND_INT:
	  return new proc2procrand(file, line, PROC_RAND_INT, e);

	// Add PROC_REAL, ...
      }
      return NULL;

    // --------------------------------------------------------------
    case PROC_REAL:
      switch (newtype) {
	case PROC_RAND_REAL:
	  return new proc2procrand(file, line, PROC_RAND_REAL, e);

      }
      return NULL;
  }

  // Still here?  Slipped through the cracks.
  return NULL;
}

expr* MakeTypecast(expr *e, const expr* fp, const char* file, int line)
{
  if (NULL==e || ERROR==e || DEFLT==e) return e;
  if (NULL==fp) return NULL;
  DCASSERT(ERROR!=fp);
  
  DCASSERT(fp->NumComponents() == e->NumComponents());
  int nc = e->NumComponents();
  expr** newagg = new expr*[nc];
  bool same = true;
  bool null = false;
  int i;
  for (i=0; i<nc; i++) {
    expr* thisone = Copy(e->GetComponent(i));
    newagg[i] = MakeTypecast(thisone, fp->Type(i), file, line);
    if (newagg[i] != thisone) same = false;
    if (thisone) if (NULL==newagg[i]) {
      // we couldn't typecast this component
      null = true;
      break;
    }
  }
  if (!same && !null) { 
    Delete(e);
    return MakeAggregate(newagg, nc, file, line);
  }
  delete[] newagg;
  if (same) return e;
  // must be null
  return NULL;
}

//@}

