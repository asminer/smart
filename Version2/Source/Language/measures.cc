
// $Id$

#include "measures.h"
//@Include: measures.cc

/** @name measures.cc
    @type File
    @args \ 

   Implementation of measures.

 */

//@{


// ******************************************************************
// *                                                                *
// *                        measure  methods                        *
// *                                                                *
// ******************************************************************

measure::measure(const char *fn, int line, type t, char *n)
  : constfunc(fn, line, t, n)
{
  eng = NULL;
}

measure::~measure()
{
  delete eng;
}

void measure::Compute(int i, result &x)
{
  DCASSERT(i==0);
  if (state != CS_Computed) {
    if (return_expr) {
      return_expr->Compute(i, x);
    } else {
      x.setNull();
    }
  } else {
    x = value;
  }
}

void measure::ShowHeader(OutputStream &s) const
{
  if (NULL==Name()) return; // hidden?
  s << GetType(Type(0)) << " " << Name() << " := ";
  if (state != CS_Computed) s << return_expr;
  else PrintResult(s, Type(0), value);
}


Engine_type measure::GetEngine(engineinfo *e)
{
  if (NULL==eng) {
    eng = new engineinfo;
    if (return_expr) return_expr->GetEngine(eng);
    else eng->setNone();
  }
  if (e) *e = *eng;
  return eng->engine;
}

expr* measure::SplitEngines(List <measure> *mlist)
{
  DCASSERT(eng);
  if (eng->engine == ENG_Mixed) {
    // mixed engine, perform an actual split
    DCASSERT(return_expr);
    expr* newreturn = return_expr->SplitEngines(mlist);
    Delete(return_expr);
    return_expr = newreturn;
  }
  return Copy(this);
}


//@}

