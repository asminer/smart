
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
// *                       constfunc  methods                       *
// *                                                                *
// ******************************************************************

constfunc::constfunc(const char *fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  return_expr = NULL;
  SetSubstitution(false);  
}

constfunc::~constfunc()
{
  Delete(return_expr);
}

void constfunc::show(OutputStream &s) const
{
  s << Name();
}

void constfunc::ShowHeader(OutputStream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := " << return_expr;
}

// ******************************************************************
// *                                                                *
// *                        determfunc class                        *
// *                                                                *
// ******************************************************************


/** Constant functions that are true constants.
    Thus, eventually, the value will be computed,
    and once it has, we can trash the return expression.
 */
class determfunc : public constfunc {
  bool computed_already;
  result value;
public:
  determfunc(const char *fn, int line, type t, char *n);
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
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
  computed_already = false;
}

void determfunc::Compute(int i, result &x)
{
  DCASSERT(i==0);
  if (!computed_already) {
    if (return_expr) {
      return_expr->Compute(0, value);
      // check for errors here
      Delete(return_expr);
      return_expr = NULL;
    } else {
      value.setNull();
    }
    computed_already = true;
  }
  x = value;
}

void determfunc::Sample(long &s, int i, result &x)
{
  // we are deterministic... just compute the result
  DCASSERT(i==0);
  if (!computed_already) {
    if (return_expr) {
      return_expr->Compute(0, value);
      // check for errors here
      Delete(return_expr);
      return_expr = NULL;
    } else {
      value.setNull();
    }
    computed_already = true;
  }
  x = value;
}

void determfunc::ShowHeader(OutputStream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := ";
  if (!computed_already) s << return_expr;
  else PrintResult(s, Type(0), value);
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
  // Check if we are deterministic or not...
  return new determfunc(file, line, t, id);
}

//@}

