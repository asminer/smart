
// $Id$

#include "numerical.h"

#include "../Language/measures.h"
#include "../Formalisms/dsm.h"

#include "linear.h"

#define DEBUG

void Warnwrapper(bool ok)
{
  if (ok) return;
  Warning.Start();
  Warning << "Numerical solution did not achieve desired precision\n";
  Warning.Stop();
}

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
  bool aok = true;
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
    for(s=mc->numTransient(); s<mc->numStates(); s++) pi[s] = 1; 
    if (mc->recurrent) // otherwise there is 1 absorbing state...
      Warnwrapper(
        SSSolve(pi, mc->graph, h, mc->numTransient(), mc->numStates())
      );
    return pi;
  }

  // Count number of visits for each transient state
  if (mc->numTransient()) 
    if (!MTTASolve(pi, mc->graph, h, initial, 0, mc->numTransient()))
      aok = false;

  // put initial probs in absorbing slots of pi
  for (int nnz=0; nnz<initial->nonzeroes; nnz++) {
    if (mc->isTransient(initial->index[nnz])) continue;
    pi[initial->index[nnz]] += initial->value[nnz];
  }

  if (mc->numTransient()) {
    mc->graph->row_pointer = mc->TRarcs;
    // accumulate other probability of absorption
    // by multiplying pi * Q_TA
    if (mc->graph->isTransposed) {
      // Q_TA is by columns
      VectorColmatrixMultiply(pi, mc->graph, pi);
    } else {
      // Q_TA is by rows
      VectorRowmatrixMultiply(pi, mc->graph, pi);
    }

    // normalize absorption probs, set stationary probs to 0 for transient
    double total = 0.0;
    for (int s=0; s<mc->numTransient(); s++) pi[s] = 0;
    for (int s=mc->numTransient(); s<mc->numStates(); s++) total += pi[s];
    if (total != 1.0)
      for (int s=mc->numTransient(); s<mc->numStates(); s++) 
	pi[s] /= total;

    mc->graph->row_pointer = mc->self_arcs;
  }

  // for each class (with size>1), compute its prob of absorption;
  // fill subvector with 1s, solve for the class, then scale by absorption prob
  // (until we get to the absorbing states)
  for (int c=1; c<=mc->recurrent; c++) {  
    // Determine absorption prob for this class
    double absorb_prob = 0.0;
    for (int s=mc->blockstart[c]; s<mc->blockstart[c+1]; s++) {
      absorb_prob += pi[s];
      pi[s] = 1;
    }
    if (0.0==absorb_prob) continue;  
    
    // Get stationary distribution for this class
    if (!SSSolve(pi, mc->graph, h, mc->blockstart[c], mc->blockstart[c+1]))
      aok = false;

    // scale by absorption probability
    for (int s=mc->blockstart[c]; s<mc->blockstart[c+1]; s++) {
      pi[s] *= absorb_prob;
    }
  } // for c

  Warnwrapper(aok);
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

