
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

/**  The basic class for most type conversions.  

     A handful of conversions require a derived class.
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
  // unfortunately, this is necessary
  virtual void Compute(Rng *r, const state *st, int a, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new typecast(Filename(), Linenumber(), newtype, x);
  }
};

void typecast::Compute(Rng *r, const state *st, int a, result &x)
{
  DCASSERT(opnd);
  opnd->Compute(r, st, a, x);
}

// ******************************************************************
// *                                                                *
// *                         int2real class                         *
// *                                                                *
// ******************************************************************

/**  Type promotion from various integer to various reals.
*/   

class int2real : public typecast {
public:
  int2real(const char* fn, int line, type newt, expr* x)
  : typecast(fn, line, newt, x) { }

  virtual void Compute(Rng *r, const state *st, int a, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new int2real(Filename(), Linenumber(), Type(0), x);
  }
};

void int2real::Compute(Rng *r, const state *st, int a, result &x) 
{
  DCASSERT(0==a);
  DCASSERT(opnd);
  opnd->Compute(r, st, 0, x);
  if (x.isNormal()) {
    x.rvalue = x.ivalue;
  }
}

// ******************************************************************
// *                                                                *
// *                         real2int class                         *
// *                                                                *
// ******************************************************************

/**  Type casting from various real to various integer.
*/   

class real2int : public typecast {
public:
  real2int(const char* fn, int line, type newt, expr* x)
  : typecast(fn, line, newt, x) { }

  virtual void Compute(Rng *r, const state *st, int a, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new real2int(Filename(), Linenumber(), Type(0), x);
  }
};

void real2int::Compute(Rng *r, const state *st, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(r, st, 0, x);
  if (x.isNormal()) {
    x.ivalue = int(x.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                      expo2randreal  class                      *
// *                                                                *
// ******************************************************************

/**  Type promotion from expo to rand real.
     Also works for proc expo to proc rand real.
*/   
class expo2randreal : public typecast {
public:
  expo2randreal(const char* fn, int line, expr* x) 
    : typecast(fn, line, RAND_REAL, x) { }

  virtual void Compute(Rng *r, const state *st, int a, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new expo2randreal(Filename(), Linenumber(), x);
  }
};

void expo2randreal::Compute(Rng *r, const state *st, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(opnd);
  opnd->Compute(r, st, 0, x);

  if (x.isNormal()) {
    if (x.rvalue>0) {
      x.rvalue = - log(r->uniform()) / x.rvalue;
      return;
    } 
    if (x.rvalue<0) {
      Error.Start(Filename(), Linenumber());
      Error << "expo with parameter " << x.rvalue << ", must be non-negative";
      Error.Stop();
      x.setError();
      return;
    } 
    // still here?  Must be expo(0) = const(infinity)
    x.setInfinity();  
    x.ivalue = 1;
    return;
  }
  if (x.isInfinity()) {  // expo(infintity) has mean 0
    if (x.ivalue < 0) {
      Error.Start(Filename(), Linenumber());
      Error << "expo with parameter ";
      PrintResult(Error, REAL, x);
      Error.Stop();
      x.setError();
      return; 
    }
    x.Clear();
    x.rvalue = 0.0;
    return;
  }
  // some other error, just propogate it
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

expr* BadTypecast(expr *e, type newtype, const char* file, int line)
{
  Error.Start(file, line);
  Error << "Bad typecast from type " << GetType(e->Type(0));
  Error << " to type " << GetType(newtype);
  Error.Stop();
  return ERROR;
}

expr* MakeTypecast(expr *e, type newtype, const char* file, int line)
{
  if (NULL==e || ERROR==e || DEFLT==e) return e;

  // Should catch SPN->ANYMODEL, DTMC->MARKOV, etc.
  if (MatchesTemplate(newtype, e->Type(0))) return e;

  if (IsModelType(newtype)) {
    // still here?  Problem...
    Internal.Start(__FILE__, __LINE__, file, line);
    Internal << "Bad model typecast attempt\n";
    Internal.Stop();
    // shouldn't get here
    return ERROR;
  }

  // Note... it is assumed that e is promotable to "newtype".
  switch (e->Type(0)) {
    
    // --------------------------------------------------------------
    case BOOL: 

      switch (newtype) {
	case RAND_BOOL:   
	  return new typecast(file, line, RAND_BOOL, e);

	case PROC_BOOL:
	  return new typecast(file, line, PROC_BOOL, e);

	case PROC_RAND_BOOL:
	  return new typecast(file, line, PROC_RAND_BOOL, e);
      }

      return BadTypecast(e, newtype, file, line);


    // --------------------------------------------------------------
    case INT:

      switch (newtype) {
	case REAL:
	  return new int2real(file, line, REAL, e);

	// add PH_INT eventually

	case RAND_INT:
	  return new typecast(file, line, RAND_INT, e);

	case RAND_REAL:
	  return new int2real(file, line, RAND_REAL, e);

	case PROC_INT:
	  return new typecast(file, line, PROC_INT, e);

	case PROC_REAL:
	  return new int2real(file, line, PROC_REAL, e);

	case PROC_RAND_INT:
	  return new typecast(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new int2real(file, line, PROC_RAND_REAL, e);
      }

      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case REAL:
      switch (newtype) {
	case INT:
	  return new real2int(file, line, INT, e);

	case RAND_REAL:
	  return new typecast(file, line, RAND_REAL, e);

	case PROC_REAL:
	  return new typecast(file, line, PROC_REAL, e);

	case PROC_RAND_REAL:
	  return new typecast(file, line, PROC_RAND_REAL, e);
      }

      return BadTypecast(e, newtype, file, line);


    // --------------------------------------------------------------
    case EXPO:
      switch (newtype) {

	// add PH_REAL eventually

	case RAND_REAL:
	  return new expo2randreal(file, line, e);
      }
      return BadTypecast(e, newtype, file, line);
      
    // --------------------------------------------------------------
    case RAND_BOOL:
      if (newtype==PROC_RAND_BOOL) 
	return new typecast(file, line, PROC_RAND_BOOL, e);
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case RAND_INT:
      switch (newtype) {
	case RAND_REAL:
	  return new int2real(file, line, RAND_REAL, e);

	case PROC_RAND_INT:
	  return new typecast(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new int2real(file, line, PROC_RAND_REAL, e);
      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case RAND_REAL:
      switch (newtype) {
	case RAND_INT:
	  return new real2int(file, line, RAND_INT, e);

	case PROC_RAND_REAL:
	  return new typecast(file, line, PROC_RAND_REAL, e);

      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_BOOL:
      if (newtype==PROC_RAND_BOOL)
	return new typecast(file, line, PROC_RAND_BOOL, e);
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_INT:
      switch (newtype) {
	case PROC_REAL:
	  return new int2real(file, line, PROC_REAL, e);

	case PROC_RAND_INT:
	  return new typecast(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new int2real(file, line, PROC_RAND_REAL, e);

	// Add PROC_PH_INT...
      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_REAL:
      switch (newtype) {
	case PROC_INT:
	  return new real2int(file, line, PROC_INT, e);

	case PROC_RAND_REAL:
	  return new typecast(file, line, PROC_RAND_REAL, e);

      }
      return BadTypecast(e, newtype, file, line);
  }

  // Still here?  Slipped through the cracks.
  Internal.Start(__FILE__, __LINE__, file, line);
  Internal << "Bad typecast from " << GetType(e->Type(0));
  Internal << " to " << GetType(newtype);
  Internal.Stop();
  return ERROR;
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

