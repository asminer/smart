
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
  split_return = NULL;
  dependencies = NULL;
  num_dependencies = 0;
}

measure::~measure()
{
  delete eng;
}

void measure::Compute(int i, result &x)
{
  DCASSERT(i==0);
  if (state != CS_Computed) {
    if (split_return) {
      split_return->Compute(i, x);
    } else if (return_expr) {
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
  return Copy(this);
}

void measure::ComputeDependencies(List <measure> *Glist)
{
  if (dependencies) return;  // already computed
  if (GetEngine(NULL) != ENG_Mixed) return;

  List <measure> deps(16);
  DCASSERT(return_expr);
  split_return = return_expr->SplitEngines(&deps);
  num_dependencies = deps.Length();
  dependencies = deps.MakeArray();
  DCASSERT(num_dependencies > 0);

  if (Glist) {
    for (int i=0; i<num_dependencies; i++) {
      Glist->Append(dependencies[i]);
    }
  }
}

//@}

