
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

// virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2rand(Filename(), Linenumber(), Type(0), x);
  }
};

/*
void determ2rand::Sample(Rng &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
}
*/

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

  // virtual void Compute(const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2proc(Filename(), Linenumber(), Type(0), x);
  }
};

/*
void determ2proc::Compute(const state &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
}
*/

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

//  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new determ2procrand(Filename(), Linenumber(), Type(0), x);
  }
};

/*
void determ2procrand::Sample(Rng &, const state &, int i, result &x) 
{
  SafeCompute(opnd, i, x);
}
*/

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

/**  Type promotion from integer to various reals.
*/   

class int2real : public typecast {
public:
  int2real(const char* fn, int line, type newt, expr* x)
  : typecast(fn, line, newt, x) { }

  virtual void Compute(int i, result &x);
  virtual void Sample(Rng &, int i, result &x);
  virtual void Compute(const state &, int i, result &x);
  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  inline void compute(int i, result &x) {
    SafeCompute(opnd, i, x);
    if (x.isNormal()) {
      x.rvalue = x.ivalue;
    }
  }
  virtual expr* MakeAnother(expr* x) { 
    return new int2real(Filename(), Linenumber(), Type(0), x);
  }
};

void int2real::Compute(int i, result &x)
{
  compute(i, x);
}

void int2real::Sample(Rng &, int i, result &x)
{
  compute(i, x);
}

void int2real::Compute(const state &, int i, result &x)
{
  compute(i, x);
}

void int2real::Sample(Rng &, const state &, int i, result &x)
{
  compute(i, x);
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
  SafeCompute(opnd, i, x);
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
    if (x.rvalue>0) {
      x.rvalue = - log(seed.uniform()) / x.rvalue;
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
// *                                                                *
// *                    rand_int2rand_real class                    *
// *                                                                *
// ******************************************************************

/**  Type promotion from rand integer to (proc) rand real.
*/   

class rand_int2rand_real : public typecast {
public:
  rand_int2rand_real(const char* fn, int line, int nt, expr* x) 
  : typecast(fn, line, nt, x) { }

  virtual void Sample(Rng &seed, int i, result &x);
  virtual void Sample(Rng &seed, const state &, int i, result &x);
protected:
  inline void sample(Rng &seed, int i, result &x) {
    SafeSample(opnd, seed, i, x);
    if (x.isNormal()) {
      x.rvalue = x.ivalue;
    }
  }
  virtual expr* MakeAnother(expr* x) { 
    return new rand_int2rand_real(Filename(), Linenumber(), Type(0), x);
  }
};

void rand_int2rand_real::Sample(Rng &seed, int i, result &x)
{
  sample(seed, i, x);
}

void rand_int2rand_real::Sample(Rng &seed, const state &, int i, result &x)
{
  sample(seed, i, x);
}

// ******************************************************************
// *                                                                *
// *                    rand_real2rand_int class                    *
// *                                                                *
// ******************************************************************

/**  Type casting from rand_real to rand_integer.
*/   

class rand_real2rand_int : public typecast {
public:
  rand_real2rand_int(const char* fn, int line, expr* x) 
  : typecast(fn, line, RAND_INT, x) { }

  virtual void Sample(Rng &seed, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new rand_real2rand_int(Filename(), Linenumber(), x);
  }
};

void rand_real2rand_int::Sample(Rng &seed, int i, result &x)
{
  SafeSample(opnd, seed, i, x);
  if (x.isNormal()) {
    x.ivalue = int(x.rvalue);
  }
}

// ******************************************************************
// *                                                                *
// *                    proc_int2proc_real class                    *
// *                                                                *
// ******************************************************************

/**  Type promotion from proc integer to proc (rand) real.
*/   

class proc_int2proc_real : public typecast {
public:
  proc_int2proc_real(const char* fn, int line, int nt, expr* x) 
  : typecast(fn, line, nt, x) { }

  virtual void Compute(const state &, int i, result &x);
  virtual void Sample(Rng &, const state &, int i, result &x);
protected:
  inline void compute(const state &s, int i, result &x) {
    SafeCompute(opnd, s, i, x);
    if (x.isNormal()) {
      x.rvalue = x.ivalue;
    }
  }
  virtual expr* MakeAnother(expr* x) { 
    return new proc_int2proc_real(Filename(), Linenumber(), Type(0), x);
  }
};

void proc_int2proc_real::Compute(const state &s, int i, result &x)
{
  compute(s, i, x);
}

void proc_int2proc_real::Sample(Rng &, const state &s, int i, result &x)
{
  compute(s, i, x);
}

// ******************************************************************
// *                                                                *
// *                    proc_real2proc_int class                    *
// *                                                                *
// ******************************************************************

/**  Type casting from proc_real to proc_integer.
*/   

class proc_real2proc_int : public typecast {
public:
  proc_real2proc_int(const char* fn, int line, expr* x) 
  : typecast(fn, line, PROC_INT, x) { }

  virtual void Compute(const state &s, int i, result &x);
protected:
  virtual expr* MakeAnother(expr* x) { 
    return new proc_real2proc_int(Filename(), Linenumber(), x);
  }
};

void proc_real2proc_int::Compute(const state &s, int i, result &x)
{
  SafeCompute(opnd, s, i, x);
  if (x.isNormal()) {
    x.ivalue = int(x.rvalue);
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
	  return new determ2rand(file, line, RAND_BOOL, e);

	case PROC_BOOL:
	  return new determ2proc(file, line, PROC_BOOL, e);

	case PROC_RAND_BOOL:
	  return new determ2procrand(file, line, PROC_RAND_BOOL, e);
      }

      return BadTypecast(e, newtype, file, line);


    // --------------------------------------------------------------
    case INT:

      switch (newtype) {
	case REAL:
	  return new int2real(file, line, REAL, e);

	// add PH_INT eventually

	case RAND_INT:
	  return new determ2rand(file, line, RAND_INT, e);

	case RAND_REAL:
	  return new int2real(file, line, RAND_REAL, e);

	case PROC_INT:
	  return new determ2proc(file, line, PROC_INT, e);

	case PROC_REAL:
	  return new int2real(file, line, PROC_REAL, e);

	case PROC_RAND_INT:
	  return new determ2procrand(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new int2real(file, line, PROC_RAND_REAL, e);
      }

      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case REAL:
      switch (newtype) {
	case INT:
	  return new real2int(file, line, e);

	case RAND_REAL:
	  return new determ2rand(file, line, RAND_REAL, e);

	case PROC_REAL:
	  return new determ2proc(file, line, PROC_REAL, e);

	case PROC_RAND_REAL:
	  return new determ2procrand(file, line, PROC_RAND_REAL, e);
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
	return new rand2procrand(file, line, PROC_RAND_BOOL, e);
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case RAND_INT:
      switch (newtype) {
	case RAND_REAL:
	  return new rand_int2rand_real(file, line, RAND_REAL, e);

	case PROC_RAND_INT:
	  return new rand2procrand(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new rand_int2rand_real(file, line, PROC_RAND_REAL, e);
      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case RAND_REAL:
      switch (newtype) {
	case RAND_INT:
	  return new rand_real2rand_int(file, line, e);

	case PROC_RAND_REAL:
	  return new rand2procrand(file, line, PROC_RAND_REAL, e);

      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_BOOL:
      if (newtype==PROC_RAND_BOOL)
	return new proc2procrand(file, line, PROC_RAND_BOOL, e);
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_INT:
      switch (newtype) {
	case PROC_REAL:
	  return new proc_int2proc_real(file, line, PROC_REAL, e);

	case PROC_RAND_INT:
	  return new proc2procrand(file, line, PROC_RAND_INT, e);

	case PROC_RAND_REAL:
	  return new proc_int2proc_real(file, line, PROC_RAND_REAL, e);

	// Add PROC_PH_INT...
      }
      return BadTypecast(e, newtype, file, line);

    // --------------------------------------------------------------
    case PROC_REAL:
      switch (newtype) {
	case PROC_INT:
	  return new proc_real2proc_int(file, line, e);

	case PROC_RAND_REAL:
	  return new proc2procrand(file, line, PROC_RAND_REAL, e);

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

