
// $Id$

#include "modelfuncs.h"

#include "dsm.h"
#include "../States/reachset.h"


// ********************************************************
// *                      num_states                      *
// ********************************************************

void compute_num_states(expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);
  model *m = dynamic_cast<model*> (pp[0]);
  DCASSERT(m);
  state_model *dsm = m->GetModel();
  DCASSERT(dsm);

  // Generate state space here

  DCASSERT(dsm->statespace);

  x.Clear();
  x.ivalue = dsm->statespace->Size();

  // should we show the state space?
  result show;
  SafeCompute(pp[1], 0, show);
  if (!show.isNormal()) return;
  if (!show.bvalue) return;
  
  // We must display the state space...
  int i;
  state s;
  switch (dsm->statespace->Type()) {
    case RT_Enumerated:
	AllocState(s, 1);
	for (i=0; i<x.ivalue; i++) {
          s[0].ivalue = i; 
	  Output << "State " << i << ": ";
	  dsm->ShowState(Output, s);
	  Output << "\n";
	  Output.flush();
	} // for i	
	FreeState(s);
    return;
    
    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Unhandled state space type\n";
	Internal.Stop();
  }
}

void Add_num_states(PtrTable *fns)
{
  const char* helpdoc = "Returns number of reachable states.  If show is true, then as a side effect, the states are displayed to the current output stream.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(BOOL, "show");
  internal_func *p = new internal_func(INT, "num_states", 
	compute_num_states, NULL,
	pl, 2, helpdoc);
  InsertFunction(fns, p);

  // NOTE: change this to "bigint" soon.
}


// ==================================================================
// |                                                                |
// |                           Front  end                           |
// |                                                                |
// ==================================================================

void InitGenericModelFunctions(PtrTable *t)
{
  Add_num_states(t);
}

