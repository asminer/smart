
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
// *                        iterator methods                        *
// *                                                                *
// ******************************************************************

iterator::iterator(const char *fn, int line, type t, char*n)
  : symbol(fn, line, t, n)
{
}

void iterator::Compute(int i, result &x) const
{
  DCASSERT(i==0);
  x.ivalue = value;
  // Need to check for legal bounds, etc, and set to null otherwise
}

expr* iterator::Substitute(int i)
{
  DCASSERT(i==0);
  return MakeConstExpr(value, Filename(), Linenumber());
}

void iterator::show(ostream &s) const
{
  if (NULL==Name()) return;  // hidden?
  s << GetType(Type(0)) << " " << Name();
}

// ******************************************************************
// *                                                                *
// *                       constfunc  methods                       *
// *                                                                *
// ******************************************************************

constfunc::constfunc(const char *fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  return_expr = NULL;
}

constfunc::~constfunc()
{
  Delete(return_expr);
}

expr* constfunc::Substitute(int i)
{
  DCASSERT(i==0);
  return Copy(this);
}

void constfunc::show(ostream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := " << return_expr;
}

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
      value.null = true;
    }
    computed_already = true;
  }
  x = value;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************



//@}

