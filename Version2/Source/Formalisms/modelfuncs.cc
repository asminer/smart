
// $Id$

#include "modelfuncs.h"


// ********************************************************
// *                      num_states                      *
// ********************************************************

void compute_num_states(expr **pp, int np, result &x)
{
  DCASSERT(2==np);
  DCASSERT(pp);

  Output << "Inside num_states\n";
  Output.flush();

  model *m = dynamic_cast<model*> (pp[0]);
  DCASSERT(m);

  Output << "Got model: " << m << "\n";
  Output.flush();

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

