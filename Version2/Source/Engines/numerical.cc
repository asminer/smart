
// $Id$

#include "numerical.h"

#include "../Language/measures.h"

#define DEBUG

bool 	NumericalSteadyInst(model *m, List <measure> *mlist)
{
#ifdef DEBUG
  Output << "Using numerical solution\n";
  Output.flush();
#endif
  return false;
}

void InitNumerical()
{
#ifdef DEBUG
  Output << "Initializing numerical options\n";
#endif
}

