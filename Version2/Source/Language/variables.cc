
// $Id$

#include "variables.h"
#include "../Base/memtrack.h"
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
  ALLOC("variable", sizeof(variable));
  status = CS_Undefined;
  SetSubstitution(false);  
}

variable::~variable()
{
  FREE("variable", sizeof(variable));
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

void constfunc::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  if (!have_cached) {
    result* answer = x.answer;
    x.answer = &cache;
    SafeCompute(return_expr, x);
    x.answer = answer;
    have_cached = true;
  } 
  CopyResult(mytype, *(x.answer), cache);
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
  virtual void Compute(compute_data &x);
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

void determfunc::Compute(compute_data &x)
{
  DCASSERT(x.answer);
  if (!have_cached) {
    result* answer = x.answer;
    x.answer = &cache;
    SafeCompute(return_expr, x);
    x.answer = answer;
    have_cached = true;
    Delete(return_expr);
    return_expr = NULL;
    status = CS_Computed;
  }
  CopyResult(mytype, *(x.answer), cache);
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

