
// $Id$

#include "numerical.h"

#include "../Language/measures.h"
#include "../Formalisms/dsm.h"

#include "linear.h"

#define DEBUG

/// Returns true on success
bool BuildProcess(state_model *dsm)
{
  if (NULL==dsm) return false;
  if (dsm->mc) return true;  // already built

  // actual construction here, someday

  return false;
}

// Returns stationary vector or NULL on error
double* ComputeStationary(bool discrete, classified_chain <float> *mc, 
			  sparse_vector <float> *initial)
{
  double* pi = new double[mc->numStates()];
  int s;
  for (s=mc->numStates()-1; s>=0; s--) pi[s] = 0.0;

  // build holding vector, but use pi to get row sums
  if (mc->numTransient()) {
    mc->graph->row_pointer = mc->TRarcs;
    mc->graph->SumOutgoingEdges(pi);
  }
  mc->graph->row_pointer = mc->self_arcs;
  mc->graph->SumOutgoingEdges(pi);
#ifdef DEBUG
  Output << "Row sum vector: [";
  Output.PutArray(pi, mc->numStates());
  Output << "]\n";
  Output.flush();
#endif
  float *h = new float[mc->numStates()];
  for (s=mc->numStates()-1; s>=0; s--) {
    if (0==pi[s]) h[s] = 0.0;
    else h[s] = 1.0 / pi[s];
    pi[s] = 0.0;
  }

  // Only one recurrent class?  That's easy
  if (1==mc->numClasses()) {
    double oneoverN = 1.0 / (mc->numStates()-mc->numTransient());
    for(s=mc->numTransient(); s<mc->numStates(); s++) pi[s] = oneoverN;
    if (oneoverN<1.0)
      SSSolve(pi, mc->graph, h, mc->numTransient(), mc->numStates());
    return pi;
  }

  // Count number of visits for each transient state
  if (mc->numTransient()) 
    MTTASolve(pi, mc->graph, h, initial, 0, mc->numTransient());

  // put initial probs in absorbing slots of pi
  for (int nnz=0; nnz<initial->nonzeroes; nnz++) {
    if (mc->isTransient(initial->index[nnz])) continue;
    pi[initial->index[nnz]] += initial->value[nnz];
  }

  // accumulate other probability of absorption

  // for each class, compute its prob of absorption;
  // fill subvector with 1s, solve for the class, then scale by absorption prob
  // (until we get to the absorbing states)

  return pi;
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

  DCASSERT(dsm->proctype != Proc_Unknown);
  DCASSERT(dsm->mc);
  DCASSERT(dsm->mc->explicit_mc);
  
  bool disc = (dsm->proctype == Proc_Dtmc);
  
  double *pi = ComputeStationary(disc, dsm->mc->explicit_mc, dsm->mc->initial);
  if (NULL==pi) return false;

#ifdef DEBUG
  Output << "Got solution vector pi=[";
  Output.PutArray(pi, dsm->mc->explicit_mc->numStates());
  Output << "]\n";
  Output.flush();
#endif
  // compute measures
  
 
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

