
// $Id$

#include "numerical.h"

#include "../Language/measures.h"
#include "../Formalisms/dsm.h"

#include "linear.h"

//#define DEBUG

/// Returns true on success
bool BuildProcess(state_model *dsm)
{
  if (NULL==dsm) return false;
  if (dsm->mc) return true;  // already built

  // actual construction here, someday

  return false;
}

bool 	NumericalSteadyInst(model *m, List <measure> *mlist)
{
#ifdef DEBUG
  Output << "Using numerical solution\n";
  Output.flush();
#endif
  if (NULL==m) return false;
  state_model *dsm = m->GetModel();
  if (!BuildProcess(dsm)) return false;

  DCASSERT(dsm->mc);
  // Solve MC, one class at a time.

  return false;
}

void InitNumerical()
{
#ifdef DEBUG
  Output << "Initializing numerical options\n";
#endif
  // Linear solver options
  InitLinear();
}

