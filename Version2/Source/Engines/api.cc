
// $Id$

#include "api.h"

#include "../Language/measures.h"

// Probably these will be divided up, so I'm guessing
// lots of includes here.

/// Sets the return result for all measures to be "error"
void ErrorList(List <measure> *mlist)
{
  if (NULL==mlist) return;
  result err;
  err.Clear();
  err.setError();
  int i;
  for(i=0; i<mlist->Length(); i++) {
    measure *m = mlist->Item(i);
    DCASSERT(m);
    m->SetValue(err);
  }
}

void 	SolveSteadyInst(model *m, List <measure> *mlist)
{
  Output << "Solving group of steady-state measures for model " << m << "\n";
  Output.flush();

  // EXTREMELY temporary
/*
  state m;
  AllocState(m, 1);
  int i;
  for (i=0; i<msteady->Length(); i++) {
    measure* foo = msteady->Item(i);
    expr* bar = foo->GetRewardExpr();
    Output << "\tMeasure " << foo << " has reward " << bar << "\n";
    for (int s=0; s<10; s++) {
      m[0].Clear();
      m[0].ivalue = s;
      result x;
      x.Clear();
      bar->Compute(m, 0, x);
      Output << "\t\t state " << s << " returned ";
      PrintResult(Output, BOOL, x);
      Output << "\n";
      Output.flush();
    }
  }
  FreeState(m);
*/

  ErrorList(mlist);
}

void 	SolveSteadyAcc(model *m, List <measure> *mlist)
{
  Output << "Solving group of steady-state accumulated measures for model " << m << "\n";
  Output.flush();
  ErrorList(mlist);
}

void 	SolveTransientInst(model *m, List <measure> *mlist)
{
  Output << "Solving group of transient measures for model " << m << "\n";
  Output.flush();
  ErrorList(mlist);
}

void 	SolveTransientAcc(model *m, List <measure> *mlist)
{
  Output << "Solving group of transient accumulated measures for model " << m << "\n";
  Output.flush();
  ErrorList(mlist);
}


