
// $Id$

#include "api.h"

// Probably these will be divided up, so I'm guessing
// lots of includes here.

void 	SolveSteady(model *m, List <measure> *mlist)
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
}

