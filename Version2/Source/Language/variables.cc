
// $Id$

#include "variables.h"
//@Include: variables.h

/** @name variables.cc
    @type File
    @args \ 

   Implementation of functions with no parameters.

 */

//@{


// ******************************************************************
// *                                                                *
// *                        variable methods                        *
// *                                                                *
// ******************************************************************

variable::variable(const char *fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  state = CS_Undefined;
  SetSubstitution(false);  
}

void variable::show(OutputStream &s) const
{
  s << Name();
}

// ******************************************************************
// *                                                                *
// *                       constfunc  methods                       *
// *                                                                *
// ******************************************************************

constfunc::constfunc(const char *fn, int line, type t, char* n)
  : variable(fn, line, t, n)
{
  return_expr = NULL;
  have_cached = false;
  cache.setNull();
}

constfunc::~constfunc()
{
  DeleteResult(mytype, cache);
  Delete(return_expr);
}

void constfunc::ShowHeader(OutputStream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := " << return_expr;
}

void constfunc::ClearCache()
{
  have_cached = false;
}

void constfunc::Compute(int i, result &x)
{
  DCASSERT(i==0);
  if (have_cached) {
    x = cache;
    x.notFreeable();
  } else { 
    SafeCompute(return_expr, i, x);
    cache = x;
    x.notFreeable();
    have_cached = true;
  }
}

void constfunc::Sample(Rng &seed, int i, result &x)
{
  DCASSERT(i==0);
  if (have_cached) {
    x = cache;
    x.notFreeable();
  } else {
    SafeSample(return_expr, seed, i, x);
    cache = x;
    x.notFreeable();
    have_cached = true;
  }
}

Engine_type constfunc::GetEngine(engineinfo *e)
{
  if (e) e->setNone();
  return ENG_None;
}

expr* constfunc::SplitEngines(List <measure> *mlist)
{
  return Copy(this);
}

// ******************************************************************
// *                                                                *
// *                        determfunc class                        *
// *                                                                *
// ******************************************************************


/** Constant functions that are true constants.
    Once the value has been computed and is cached,
    we trash the return expression (and we never clear the cache).
   
 */
class determfunc : public constfunc {
public:
  determfunc(const char *fn, int line, type t, char *n);
  virtual void ClearCache() { }  // DO NOT TRASH cache
  virtual void Compute(int i, result &x);
  virtual void ShowHeader(OutputStream &s) const;
};

// ******************************************************************
// *                                                                *
// *                       determfunc methods                       *
// *                                                                *
// ******************************************************************

determfunc::determfunc(const char *fn, int line, type t, char *n)
  : constfunc(fn, line, t, n)
{
  // nothing!
}

void determfunc::Compute(int i, result &x)
{
  DCASSERT(i==0);
  if (have_cached) {
    x = cache;
    x.notFreeable();
  } else { 
    SafeCompute(return_expr, i, x);
    cache = x;
    x.notFreeable();
    have_cached = true;
    Delete(return_expr);
    return_expr = NULL;
    state = CS_Computed;
  }
}

void determfunc::ShowHeader(OutputStream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := ";
  if (!have_cached) s << return_expr;
  else PrintResult(s, Type(0), cache);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

constfunc* MakeConstant(type t, char* id, const char* file, int line)
{
  switch (t) {
    case BOOL:
    case INT:
    case REAL:
  	return new determfunc(file, line, t, id);

    default:
    	return new constfunc(file, line, t, id);
  }
}

//@}

