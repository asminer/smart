
// $Id$

#include "modelfuncs.h"

#include "dsm.h"
#include "../States/reachset.h"
#include "../Chains/procs.h"
#include "../Engines/api.h"

// ********************************************************
// * options and option constants used by model functions *
// ********************************************************

option* StateDisplayOrder;

option_const lexical_sdo("LEXICAL", "\aStates are sorted by lexical ordering (definition order for formalisms dtmc and ctmc)");
option_const natural_sdo("NATURAL", "\aThe most natural order for the selected state space data structure");

// ********************************************************
// *                  prob_ss and avg_ss                  *
// ********************************************************

Engine_type prob_ss_engine(expr **pp, int np, engineinfo *e)
{
  DCASSERT(pp);
  DCASSERT(np==2);  // params are: (model, reward)
  if (e) {
    e->engine = ENG_SS_Inst;
    e->starttime.setInfinity();
    e->stoptime.setInfinity();
  }
  return ENG_SS_Inst;
}

void Add_prob_ss(PtrTable *fns)
{
  const char* helpdoc = "Computes the steady-state probability of b.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_BOOL, "b");
  engine_wrapper *p = new engine_wrapper(REAL, "prob_ss", 
	prob_ss_engine, pl, 2, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

void Add_avg_ss(PtrTable *fns)
{
  const char* helpdoc = "Computes the steady-state average of x.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_REAL, "x");
  engine_wrapper *p = new engine_wrapper(REAL, "avg_ss", 
	prob_ss_engine, pl, 2, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                  prob_at and avg_at                  *
// ********************************************************

Engine_type prob_at_engine(expr **pp, int np, engineinfo *e)
{
  DCASSERT(pp);
  DCASSERT(np==3);  // params are: (model, reward, time)
  result x;
  x.Clear();
  SafeCompute(pp[2], 0, x);  // get the time
  // check for errors here...

  if (e) {
    e->starttime = x;
    e->stoptime = x;
    e->engine = (x.isInfinity()) ? ENG_SS_Inst : ENG_T_Inst;
  }
  return (x.isInfinity()) ? ENG_SS_Inst : ENG_T_Inst;
}

void Add_prob_at(PtrTable *fns)
{
  const char* helpdoc = "Computes the probability of b at time t.";

  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_BOOL, "b");
  pl[2] = new formal_param(REAL, "t");
  engine_wrapper *p = new engine_wrapper(REAL, "prob_at", 
	prob_at_engine, pl, 3, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

void Add_avg_at(PtrTable *fns)
{
  const char* helpdoc = "Computes the average of x at time t.";

  formal_param **pl = new formal_param*[3];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_REAL, "x");
  pl[2] = new formal_param(REAL, "t");
  engine_wrapper *p = new engine_wrapper(REAL, "avg_at", 
	prob_at_engine, pl, 3, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                 prob_acc and avg_acc                 *
// ********************************************************

Engine_type prob_acc_engine(expr **pp, int np, engineinfo *e)
{
  DCASSERT(pp);
  DCASSERT(np==4);  // params are: (model, reward, starttime, stoptime)
  result t1, t2;
  t1.Clear();
  t2.Clear();
  SafeCompute(pp[2], 0, t1);  // get the start time
  SafeCompute(pp[3], 0, t2);  // get the stop time

  // check for errors here...

  if (e) {
    e->starttime = t1;
    e->stoptime = t2;
    e->engine = (t2.isInfinity()) ? ENG_SS_Acc : ENG_T_Acc;
  }
  return (t2.isInfinity()) ? ENG_SS_Acc : ENG_T_Acc;
}

void Add_prob_acc(PtrTable *fns)
{
  const char* helpdoc = "Computes the accumulated probability of b from time t1 to time t2.";

  formal_param **pl = new formal_param*[4];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_BOOL, "b");
  pl[2] = new formal_param(REAL, "t1");
  pl[3] = new formal_param(REAL, "t2");
  engine_wrapper *p = new engine_wrapper(REAL, "prob_acc", 
	prob_acc_engine, pl, 4, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

void Add_avg_acc(PtrTable *fns)
{
  const char* helpdoc = "Computes the accumulated average of x from time t1 to time t2.";

  formal_param **pl = new formal_param*[4];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(PROC_REAL, "x");
  pl[2] = new formal_param(REAL, "t1");
  pl[3] = new formal_param(REAL, "t2");
  engine_wrapper *p = new engine_wrapper(REAL, "avg_acc", 
	prob_acc_engine, pl, 4, 1, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);
}

// ********************************************************
// *                      num_states                      *
// ********************************************************

struct statehandle {
  int handle;
  state_array* states;
  state* tempfull;
};

bool operator > (statehandle &a, statehandle &b)
{
  b.states->GetState(b.handle, *(b.tempfull));
  return a.states->Compare(a.handle, *(b.tempfull)) > 0;
}

void SortStates(int* order, int N, state_array* states, state& s)
{
  if (N<2) return;
  // probably overkill, but it works and this is only for 
  // displaying states in lexical order.
  statehandle* foo = new statehandle[N];
  for (int i=0; i<N; i++) {
    foo[i].handle = order[i];
    foo[i].states = states;
    foo[i].tempfull = &s;
  }
  HeapOfObjects <statehandle> bar(foo, N);
  bar.Sort();
  for (int i=0; i<N; i++) 
    order[i] = foo[i].handle;
}

// for displays only
void BuildStateOrder(state_model *dsm, int* order, int* redro, int N)
{
  DCASSERT(dsm);
  reachset* ss = dsm->statespace;
  DCASSERT(ss);
  bool natural_order = (StateDisplayOrder->GetEnum() == &natural_sdo);
  if (natural_order) {
    for (int i=0; i<N; i++) 
      order[i] = redro[i] = i;
    return;
  }

  flatss* fs;
  state s;
  DCASSERT(dsm->UsesConstantStateSize());		
  AllocState(s, dsm->GetConstantStateSize());
  switch (ss->Storage()) {
    case RT_Enumerated:
	for (int i=0; i<ss->Enumerated(); i++) {
	  dsm->GetState(i, s);
	  order[i] = s[0].ivalue;
	  redro[s[0].ivalue] = i;
	} // for i	
    break;
    case RT_Explicit:
    	fs = ss->Explicit();
	for (int i=0; i<fs->NumTangible(); i++)
	  order[i] = i;
	for (int i=0; i<fs->NumVanishing(); i++)
	  order[i+fs->NumTangible()] = i;
	SortStates(order, fs->NumTangible(), fs->t_states, s);
	SortStates(order+fs->NumTangible(), fs->NumVanishing(), fs->v_states, s);
	for (int i=0; i<fs->NumTangible() + fs->NumVanishing(); i++) 
	  redro[order[i]] = i;
    break;
    default:
    	DCASSERT(0);
  } // switch
  FreeState(s);
}

void compute_num_states(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(2==np);
  DCASSERT(pp);
  model *m = dynamic_cast<model*> (pp[0]);
  DCASSERT(m);

  BuildReachset(m);

  state_model *dsm = dynamic_cast<state_model*> (m->GetModel());
  if (NULL==dsm) {
    // problem with model construction
    x.setNull();
    return;
  }
  DCASSERT(dsm);
  reachset* ss = dsm->statespace;
  DCASSERT(ss);  // should be set to something
  DCASSERT(ss->Storage() != RT_None);

  if (ss->Storage() == RT_Error) {
    // is an error message necessary?
    x.setNull();
    return;
  }
  
  x.ivalue = ss->NumTangible() + ss->NumVanishing();

  // should we show the state space?
  result show;
  SafeCompute(pp[1], 0, show);
  if (!show.isNormal()) return;
  if (!show.bvalue) return;
  
  // We must display the state space...
  int i;
  state s;
  // This may change in the future...
  DCASSERT(dsm->UsesConstantStateSize());		
  AllocState(s, dsm->GetConstantStateSize());
  int* order = new int[x.ivalue];
  int* redro = new int[x.ivalue];
  BuildStateOrder(dsm, order, redro, x.ivalue);
  flatss *ess;
  switch (ss->Storage()) {
    case RT_Enumerated:
	s[0].Clear();
	for (i=0; i<ss->Enumerated(); i++) {
	  Output << "State " << i << ": ";
	  s[0].ivalue = order[i];
	  dsm->ShowState(Output, s);
	  Output << "\n";
	  Output.flush();
	} // for i	
    break;
   
    case RT_Explicit:
        ess = ss->Explicit();
	DCASSERT(ess);
	if (ess->NumVanishing()) Output << "Tangible states:\n";
	for (i=0; i<ess->NumTangible(); i++) {
	  bool ok = ess->GetTangible(order[i], s);
	  DCASSERT(ok);
	  if (ess->NumVanishing())
	    Output << "Tangible state " << i << ": ";
	  else
	    Output << "State " << i << ": ";
	  dsm->ShowState(Output, s);
	  Output << "\n";
	  Output.flush();
	} // for i	
	if (ess->NumVanishing()) Output << "Vanishing states:\n";
	for (i=0; i<ess->NumVanishing(); i++) {
	  bool ok = ess->GetVanishing(order[i+ess->NumTangible()], s);
	  DCASSERT(ok);
	  Output << "Vanishing state " << i << ": ";
	  dsm->ShowState(Output, s);
	  Output << "\n";
	  Output.flush();
	} // for i	
    break; 

    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Unhandled state space type\n";
	Internal.Stop();
  }
  FreeState(s);
  delete[] order;
  delete[] redro;
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
  p->setWithinModel();
  InsertFunction(fns, p);

  // NOTE: change this to "bigint" soon.
}


// ********************************************************
// *                       num_arcs                       *
// ********************************************************

void State2MCOrder(state_model *dsm, int* order, int* redro, int N)
{
  DCASSERT(dsm->mc->Storage() == MC_Explicit);
  classified_chain <float> *R = dsm->mc->Explicit();
  DCASSERT(R);
  if (!R->NeedsRenumbering()) return;

  for (int i=0; i<N; i++) {
    order[i] = R->Renumber(order[i]);
    redro[order[i]] = i;
  }
}

void fsm_arcs(state_model *dsm, bool show, result &x)
{
  DCASSERT(dsm);
  // get number of arcs
  DCASSERT(dsm->rg);
  switch (dsm->rg->Storage()) {
    case MC_Explicit:
	x.ivalue = dsm->rg->Explicit()->NumEdges();
	break;

    case MC_Kronecker:
	x.ivalue = 0;
	break;

    default:
	x.setNull();
	return;
  }
  if (!show) return;

  // display here...
  Output << "Reachability graph:\n";
  Output.flush();

  DCASSERT(dsm->rg->Storage() == MC_Explicit);
  // dump explicit graph
  digraph* G = dsm->rg->Explicit();
  DCASSERT(G);
  for (int i=0; i<G->NumNodes(); i++) {
    G->ShowNodeList(Output, i);
    Output.flush();
  }
}

void mc_arcs(state_model *dsm, bool show, result &x)
{
  DCASSERT(dsm);
  // get number of arcs
  switch (dsm->mc->Storage()) {
    case MC_Explicit:
	x.ivalue = dsm->mc->Explicit()->numArcs();
	break;

    case MC_Kronecker:
	x.ivalue = 0;
	break;

    default:
	x.setNull();
	return;
  }
  if (!show) return;

  // display here...
  Output << "Markov chain:\n";
  Output.flush();
  DCASSERT(dsm->mc->Storage() == MC_Explicit);
  // dump explicit graph
  classified_chain <float> *R = dsm->mc->Explicit();
  DCASSERT(R);
  // renumbering system, if necessary
  int N = R->numStates();
  int* order = new int[N];
  int* redro = new int[N];
  BuildStateOrder(dsm, order, redro, N);
  State2MCOrder(dsm, order, redro, N);
  sparse_vector <float> row(4);
  const char* rowlabel = (R->graph->isTransposed) ? "Column" : "Row";
  for (int i=0; i<R->numStates(); i++) {
    Output << rowlabel << " " << i << ":\n";
    row.Clear();
    R->GrabRow(order[i], &row);
    for (int z=0; z<row.nonzeroes; z++)
      row.index[z] = redro[row.index[z]];
    row.isSorted = false;
    row.Sort();
    for (int z=0; z<row.nonzeroes; z++)
      Output << "\t" << row.index[z] << " : " << row.value[z] << "\n"; 
    Output.flush();
  }
  delete[] order;
  delete[] redro;
}


void compute_num_arcs(expr **pp, int np, result &x)
{
  x.Clear();
  DCASSERT(2==np);
  DCASSERT(pp);
  model *m = dynamic_cast<model*> (pp[0]);
  DCASSERT(m);

  BuildProcess(m);

  // should we show the graph / MC?
  bool show = false;
  result s;
  SafeCompute(pp[1], 0, s);
  if (s.isNormal()) show = s.bvalue;

  // eventually: switch for different process types
  state_model *dsm = dynamic_cast<state_model*> (m->GetModel());
  if (NULL==dsm) {
    // problem with model construction
    x.setNull();
    return;
  }

  switch (dsm->proctype) {
    case Proc_FSM:
	fsm_arcs(dsm, show, x);
	return;

    case Proc_Dtmc:
    case Proc_Ctmc:
	mc_arcs(dsm, show, x);
	return;

    case Proc_Unknown:
	DCASSERT(0);	// should be impossible
    default:
	x.setError();
  };
}

void Add_num_arcs(PtrTable *fns)
{
  const char* helpdoc = "Returns number of arcs in the reachability graph or Markov chain.  If show is true, then as a side effect, the graph is displayed to the current output stream.";

  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(BOOL, "show");
  internal_func *p = new internal_func(INT, "num_arcs", 
	compute_num_arcs, NULL,
	pl, 2, helpdoc);
  p->setWithinModel();
  InsertFunction(fns, p);

  // NOTE: change this to "bigint" soon.
}


// ********************************************************
// *                          test                        *
// ********************************************************


// A hook for testing things
void compute_test(expr **pp, int np, result &x)
{
  DCASSERT(np==2);
  DCASSERT(pp);
  DCASSERT(pp[0]);
  model *mcmod = dynamic_cast<model*> (pp[0]);
  DCASSERT(mcmod);
  state_model *proc = dynamic_cast<state_model*> (mcmod->GetModel());
  DCASSERT(proc);
  Output << "Got state model " << proc << "\n";
  Output << "Testing EnabledExpr and such\n";
  Output.flush();

  List <symbol> deplist(16);
  List <expr> prodlist(16);

  for (int e=0; e<proc->NumEvents(); e++) {
    Output << "Event " << e << " is named ";
    event* foo = proc->GetEvent(e);
    Output << foo << "\n";
    Output.flush();

    //  CHECK enabling expression

    expr* enable = foo->isEnabled();
    Output << "\tenabling expression: " << enable << "\n";
    Output.flush();
    if (enable) {
      // Show dependencies
      deplist.Clear();
      enable->GetSymbols(0, &deplist);
      Output << "\t\tdepends on: ";
      for (int p=0; p<deplist.Length(); p++)
        Output << deplist.Item(p) << " ";
      Output << "\n";
      Output.flush();
      // Show products
      prodlist.Clear();
      enable->GetProducts(0, &prodlist);
      Output << "\t\tProducts:\n";     
      for (int p=0; p<prodlist.Length(); p++) {
        expr *e = prodlist.Item(p);
        Output << "\t\t\t" << e << "\n";
	Delete(e);
      }
    }

    //  CHECK next state expression

    expr* fire = foo->getNextstate();
    Output << "\tnext state expression: " << fire << "\n";
    Output.flush();
    
    if (fire) {
      // Show dependencies
      deplist.Clear();
      fire->GetSymbols(0, &deplist);
      Output << "\t\tdepends on: ";
      for (int p=0; p<deplist.Length(); p++)
        Output << deplist.Item(p) << " ";
      Output << "\n";
      Output.flush();
      // Show products
      prodlist.Clear();
      fire->GetProducts(0, &prodlist);
      Output << "\t\tProducts:\n";     
      for (int p=0; p<prodlist.Length(); p++) {
        expr *e = prodlist.Item(p);
        Output << "\t\t\t" << e << "\n";
	Delete(e);
      }
    }

    switch (foo->FireType()) {
      case E_Nondeterm:
	Output << "\tEvent is nondeterministic\n";
	Output.flush();
      	break;
      case E_Timed:
	Output << "\tEvent is timed\n";
      	Output << "\tfiring distribution type is: ";
    	Output << GetType(foo->DistroType()) << "\n";
    	Output << "\tfiring distribution is: ";
    	Output << foo->Distribution() << "\n";
	Output << "\tweight class is: " << foo->WeightClass() << "\n";
	if (foo->WeightClass()) {
      	  Output << "\tfiring weight type is: ";
    	  Output << GetType(foo->WeightType()) << "\n";
    	  Output << "\tfiring weight is: ";
    	  Output << foo->Weight() << "\n";
        }
    	Output.flush();
	break;
      case E_Immediate:
	Output << "\tEvent is immediate\n";
	Output << "\tweight class is: " << foo->WeightClass() << "\n";
	if (foo->WeightClass()) {
      	  Output << "\tfiring weight type is: ";
    	  Output << GetType(foo->WeightType()) << "\n";
    	  Output << "\tfiring weight is: ";
    	  Output << foo->Weight() << "\n";
        }
    	Output.flush();
	break;
      default:
	DCASSERT(0);
    }

  }

  x.Clear();
  x.ivalue = 0;
}

void Add_test(PtrTable *fns)
{
  formal_param **pl = new formal_param*[2];
  pl[0] = new formal_param(ANYMODEL, "m");
  pl[1] = new formal_param(BOOL, "dummy");
  internal_func *p = new internal_func(INT, "test",
	compute_test, NULL, pl, 2, "hidden test function");
  p->setWithinModel();
  p->HideDocs();
  InsertFunction(fns, p);
}


// ==================================================================
// |                                                                |
// |                           Front  end                           |
// |                                                                |
// ==================================================================

void InitGenericModelFunctions(PtrTable *t)
{
  Add_prob_ss(t);
  Add_avg_ss(t);

  Add_prob_at(t);
  Add_avg_at(t);

  Add_prob_acc(t);
  Add_avg_acc(t);

  Add_num_states(t);
  Add_num_arcs(t);

  Add_test(t);

  // ****************************************************************
  // Options
  // ****************************************************************

  // StateDisplayOrder option
  option_const **sdolist = new option_const*[2];
  // these must be alphabetical
  sdolist[0] = &lexical_sdo;
  sdolist[1] = &natural_sdo;
  StateDisplayOrder = MakeEnumOption("StateDisplayOrder", "The order to use for displaying states in functions num_states and num_arcs.  This does not affect the internal storage of the states, so the reordering is done as necessary only for display.", sdolist, 2, &natural_sdo);
  AddOption(StateDisplayOrder);
}

