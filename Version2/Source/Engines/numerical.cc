
// $Id$

#include "numerical.h"

#include "api.h"

#include "../Language/measures.h"
#include "../Formalisms/dsm.h"
#include "../States/reachset.h"
#include "../Chains/procs.h"

#include "linear.h"

//#define DEBUG

void Warnwrapper(bool ok)
{
  if (ok) return;
  Warning.Start();
  Warning << "Numerical solution did not achieve desired precision\n";
  Warning.Stop();
}

// Returns stationary vector or NULL on error
double* ComputeStationary(classified_chain <float> *mc, 
			  sparse_vector <float> *initial)
{
  bool aok = true;
  double* pi = new double[mc->numStates()];
  int s;
  for (s=mc->numStates()-1; s>=0; s--) pi[s] = 0.0;

  // build holding vector, but use pi to get row sums
  if (mc->numTransient()) {
    mc->UseTRMatrix();
    mc->graph->SumOutgoingEdges(pi);
  }
  mc->UseSelfMatrix();
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
  if (mc->numTransient()) {
    if (!MTTASolve(pi, mc->graph, h, initial, 0, mc->numTransient()))
      aok = false;
#ifdef DEBUG
    Output << "Time spent in each transient state=[";
    Output.PutArray(pi, mc->numStates());
    Output << "]\n";
    Output.flush();
#endif
  }

  // put initial probs in absorbing slots of pi
  for (int nnz=0; nnz<initial->nonzeroes; nnz++) {
    if (mc->isTransient(initial->index[nnz])) continue;
    pi[initial->index[nnz]] += initial->value[nnz];
  }

  if (mc->numTransient()) {
    mc->UseTRMatrix();
    // accumulate other probability of absorption
    // by multiplying pi * Q_TA
    if (mc->graph->isTransposed) {
      // Q_TA is by columns
      VectorColmatrixMultiply(pi, mc->graph, pi, mc->numTransient(), mc->numStates());
    } else {
      // Q_TA is by rows
      VectorRowmatrixMultiply(pi, mc->graph, pi, 0, mc->numTransient());
    }

    // normalize absorption probs, set stationary probs to 0 for transient
    double total = 0.0;
    for (int s=0; s<mc->numTransient(); s++) pi[s] = 0;
    for (int s=mc->numTransient(); s<mc->numStates(); s++) total += pi[s];
    if (total != 1.0)
      for (int s=mc->numTransient(); s<mc->numStates(); s++) 
	pi[s] /= total;

    mc->UseSelfMatrix();

#ifdef DEBUG
    Output << "Absorption probabilities=[";
    Output.PutArray(pi, mc->numStates());
    Output << "]\n";
    Output.flush();
#endif
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


// return true on success, false if some error
bool	EnumRewards(int start, int stop, double* x, List <measure> *mlist)
{
  state s;
  if (!AllocState(s, 1)) return false;
  s[0].Clear();  
  for (int m=0; m<mlist->Length(); m++) {
    result mval;
    mval.Clear();
    double mtotal = 0.0;
    bool mok = true;
    DCASSERT(mlist->Item(m));
    expr* mexpr = mlist->Item(m)->GetRewardExpr();
    DCASSERT(mexpr != ERROR);
    if (NULL==mexpr) {
      mval.setNull();
      mlist->Item(m)->SetValue(mval);
      continue;
    } 
    switch (mexpr->Type(0)) {
      case PROC_BOOL:
	// -------------------- BOOL measures --------------------
	for (int i=start; i<stop; i++) if (x[i]) {
          s[0].ivalue = i;
      	  mexpr->Compute(NULL, &s, 0, mval); 
      	  if (mval.isNormal()) {
	    if (mval.bvalue) mtotal += x[i];
          } else {
            mok = false;
	    break;
          }
    	} // for i
	// -------------------------------------------------------
      break;
      case PROC_REAL:
	// -------------------- REAL measures --------------------
	for (int i=start; i<stop; i++) if (x[i]) {
          s[0].ivalue = i;
      	  mexpr->Compute(NULL, &s, 0, mval); 
      	  if (mval.isNormal()) {
 	    mtotal += mval.rvalue * x[i];
          } else {
            mok = false;
            break;
	    // TO DO: check for infinity and keep going
            // to check for errors, e.g., infinity - infinity
          }
    	} // for i
	// -------------------------------------------------------
      break;
      default:
	Internal.Start(__FILE__, __LINE__);
        Internal << "Measure expression of type " << GetType(mexpr->Type(0));
        Internal.Stop();
    } // switch for measure type
    if (mok) mval.rvalue = mtotal;
    mlist->Item(m)->SetValue(mval);
  } // for m

  FreeState(s);
  return true;
}


// return true on success, false if some error
bool ExplicitRewards(flatss *SS, state &s, double* x, List <measure> *mlist)
{
  for (int m=0; m<mlist->Length(); m++) {
    result mval;
    mval.Clear();
    double mtotal = 0.0;
    bool mok = true;
    DCASSERT(mlist->Item(m));
    expr* mexpr = mlist->Item(m)->GetRewardExpr();
    DCASSERT(mexpr != ERROR);
    if (NULL==mexpr) {
      mval.setNull();
      mlist->Item(m)->SetValue(mval);
      continue;
    } 
    switch (mexpr->Type(0)) {
      case PROC_BOOL:
	// -------------------- BOOL measures --------------------
	for (int i=SS->NumTangible()-1; i>=0; i--) if (x[i]) {
#ifdef DEVELOPMENT_CODE
	  DCASSERT(SS->GetTangible(i, s));	
#else
	  SS->GetTangible(i, s);
#endif
      	  mexpr->Compute(NULL, &s, 0, mval); 
      	  if (mval.isNormal()) {
	    if (mval.bvalue) mtotal += x[i];
          } else {
            mok = false;
	    break;
          }
    	} // for i
	// -------------------------------------------------------
      break;
      case PROC_REAL:
	// -------------------- REAL measures --------------------
	for (int i=SS->NumTangible()-1; i>=0; i--) if (x[i]) {
#ifdef DEVELOPMENT_CODE
	  DCASSERT(SS->GetTangible(i, s));	
#else
	  SS->GetTangible(i, s);
#endif
      	  mexpr->Compute(NULL, &s, 0, mval); 
      	  if (mval.isNormal()) {
 	    mtotal += mval.rvalue * x[i];
          } else {
            mok = false;
            break;
	    // TO DO: check for infinity and keep going
            // to check for errors, e.g., infinity - infinity
          }
    	} // for i
	// -------------------------------------------------------
      break;
      default:
	Internal.Start(__FILE__, __LINE__);
        Internal << "Measure expression of type " << GetType(mexpr->Type(0));
        Internal.Stop();
    } // switch for measure type
    if (mok) mval.rvalue = mtotal;
    mlist->Item(m)->SetValue(mval);
  } // for m

  FreeState(s);
  return true;
}


bool 	NumericalSteadyInst(model *m, List <measure> *mlist)
{
#ifdef DEBUG
  Output << "Using numerical solution\n";
  Output.flush();
#endif
  if (NULL==m) return false;
  BuildProcess(m);

  state_model *dsm = dynamic_cast<state_model*> (m->GetModel());
  DCASSERT(dsm->proctype != Proc_Unknown);
  DCASSERT(dsm->mc);
  DCASSERT(dsm->mc->Explicit());
  double *pi = ComputeStationary(dsm->mc->Explicit(), dsm->mc->initial);
  if (NULL==pi) return false;

#ifdef DEBUG
  Output << "Got solution vector pi=[";
  Output.PutArray(pi, dsm->mc->Explicit()->numStates());
  Output << "]\n";
  Output.flush();
#endif
  // compute measures
  
  bool rwd_result;
  state s;
  switch (dsm->statespace->Storage()) {
    case RT_None:
 	rwd_result = false;
	break;

    case RT_Enumerated:
	rwd_result = EnumRewards(dsm->mc->Explicit()->numTransient(),
				 dsm->mc->Explicit()->numStates(),
				 pi, mlist);
	break;

    case RT_Explicit:
    	AllocState(s, dsm->GetConstantStateSize());
	rwd_result = ExplicitRewards(dsm->statespace->Explicit(), s,
				 pi, mlist);
	FreeState(s);
	break;


    default:
	rwd_result = false;
  };
 
  delete[] pi;
  return rwd_result;
}

void InitNumerical()
{
#ifdef DEBUG
  Output << "Initializing numerical options\n";
#endif
  // Linear solver options
  InitLinear();
  InitProcOptions();
}

