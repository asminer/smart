
// $Id$

#include "mcgen.h"
#include "../Chains/procs.h"
#include "../States/reachset.h"
#include "../States/trees.h"
#include "../Base/timers.h"
#include "../Templates/listarray.h"

#define DEBUG_IMMED

option* MarkovStorage;

const int num_mc_options = 2;

option_const sparse_mc("SPARSE", "\aSparse, row-wise or column-wise storage");
option_const kronecker_mc("KRONECKER", "\aUsing Kronecker algebra");

struct pair {
  int index;
  float weight;
};

// *******************************************************************
// *                                                                 *
// *                          RG  generation                         *
// *                                                                 *
// *******************************************************************

template <class REACHSET>
bool GenerateRG(state_model *dsm, REACHSET *S, digraph* rg)
{
  DCASSERT(dsm);
  DCASSERT(S);

  bool error = false;
  int e;

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  // Allocate list of enabled events
  List <event> enabled(16);
  
  result x;
  x.Clear();
  
  // ok, build the graph (assumes tangible only)
  int from; 
  rg->ResizeNodes(S->NumTangible());
  for (from=0; from < S->NumTangible(); from++) {
    rg->AddNode();
  }
  rg->ResizeEdges(4);
  for (from=0; from < S->NumTangible(); from++) {
    S->GetTangible(from, current);
    // what is enabled?
    if (!dsm->GetEnabledList(current, &enabled)) {
      error = true;
      break;
    }

    for (e=0; e<enabled.Length(); e++) {
      event* t = enabled.Item(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      // t is enabled, fire and get new state

      // set reached = current
      S->GetTangible(from, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during state graph generation";
	Error.Stop();
	error = true;
	break;
      }

      // Which state is reached?
      int to = S->FindTangible(reached);
      if (to<0) {
        Internal.Start(__FILE__, __LINE__, dsm->Filename(), dsm->Linenumber());
        Internal << "Couldn't find reachable state ";
        dsm->ShowState(Internal, reached);
        Internal << " in tangible reachability set\n";
        Internal.Stop();
      }

      rg->AddEdgeInOrder(from, to);

    } // for e
    if (error) break;
  } // for from

  // cleanup
  FreeState(reached);
  FreeState(current);
  return !error; 
}

void CompressAndAffix(state_model* dsm, digraph *rg)
{
  if (NULL==rg) {
    dsm->rg = new reachgraph;
    dsm->rg->CreateError();
    return;
  }
  dsm->rg = new reachgraph;
  dsm->rg->CreateExplicit(rg);
}

void SparseRG(state_model *dsm)
{
  DCASSERT(NULL==dsm->rg);
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability graph generation using sparse storage\n";
    Verbose.flush();
  }

  digraph* rg = NULL;
  bool ok = false;

  // Build CTMC based on state space type
  switch (dsm->statespace->Storage()) {
    case RT_None:
    case RT_Error:
	break;

    case RT_Explicit:
        rg = new digraph;
	ok = GenerateRG(dsm, dsm->statespace->Explicit(), rg);
	break;
			
    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Unhandled storage type for reachability graph generation\n";
	Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Done generating reachability graph; compressing\n";
    Verbose.flush();
  }

  // An error occurred during generation
  if (!ok) {
    delete rg;
    rg = NULL;
  } else {

    // "generate" initial probability vector here...

    // transpose if necessary
    if (!MatrixByRows->GetBool()) rg->Transpose();
  }

  // attach to model
  CompressAndAffix(dsm, rg);
}


// *******************************************************************
// *                                                                 *
// *                         CTMC  generation                        *
// *                                                                 *
// *******************************************************************

inline bool IllegalRateCheck(result &x, state_model *dsm, event *t)
{
  if (!x.isNormal() || x.rvalue <= 0) {
    Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
    if (x.isInfinity()) 
      Error << "Infinite rate";
    else if (x.isUnknown()) 
      Error << "Unknown rate";
    else if (x.isError() || x.isNull())
      Error << "Undefined rate";
    else 
      Error << "Illegal rate (" << x.rvalue << ")";

    Error << " for event " << t << " during CTMC generation";
    Error.Stop();
    return true;
  }  // if error
  return false;
}

inline bool IllegalWeightCheck(result &x, state_model *dsm, event *t)
{
  if (!x.isNormal() || x.rvalue <= 0) {
    Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
    if (x.isInfinity()) 
      Error << "Infinite weight";
    else if (x.isUnknown()) 
      Error << "Unknown weight";
    else if (x.isError() || x.isNull())
      Error << "Undefined weight";
    else 
      Error << "Illegal weight (" << x.rvalue << ")";

    Error << " for event " << t << " during CTMC generation";
    Error.Stop();
    return true;
  }  // if error
  return false;
}

template <class REACHSET>
bool GenerateCTMC(state_model *dsm, REACHSET *S, labeled_digraph<float>* Rtt)
{
  DCASSERT(dsm);
  DCASSERT(S);

  bool error = false;
  int e;

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  // Allocate list of enabled events
  List <event> enabled(16);
  
  result x;
  x.Clear();
  // compute the constant rates/weights a priori; use -1 if not const.
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    // get rate or weight as appropriate
    switch (t->FireType()) {
      case E_Timed:
    	if (EXPO==t->DistroType()) {
#ifdef DEBUG_IMMED
	  Output << "Pre-computing rate for event " << t << "\n";
	  Output.flush();
#endif
      	  SafeCompute(t->Distribution(), 0, x);
      	  if (IllegalRateCheck(x, dsm, t)) {
       	    // bail out
	    return false;
      	  }
      	  t->value = x.rvalue;
    	} else {
      	  t->value = -1.0;
    	}
	break;

      case E_Immediate:
    	if (REAL==t->WeightType()) {
#ifdef DEBUG_IMMED
	  Output << "Pre-computing weight for event " << t << "\n";
	  Output.flush();
#endif
      	  SafeCompute(t->Weight(), 0, x);
	  if (x.isNormal())
	    t->value = x.rvalue;
          else
	    t->value = -1;  // signal to compute again later.
	  // Note: if this event is always enabled by itself,
	  // the weight is irrelevant.
    	} else {
      	  t->value = -1.0;
    	}
	break;

      default:
	Internal.Start(__FILE__, __LINE__);
        Internal << "Bad event type in MC generation";
        Internal.Stop();
    } // switch
  } // for e

  // Store vanishings in a splay tree (for now...)
  state_array vst(true);
  splay_state_tree vtr(&vst);
  // vanishing-vanishing arcs
  labeled_digraph <float> Pvv;
  Pvv.ResizeNodes(4);
  Pvv.ResizeEdges(4);
  // vanishing-tangible arcs
  listarray <pair> Pvt;
  // tangible-vanishing arcs
  sparse_vector <float> Rtv(4);

  int v_exp = 0;
  int t_exp = 0;

  // allocate the Rtt arcs...
  Rtt->ResizeNodes(S->NumTangible());
  for (t_exp=0; t_exp < S->NumTangible(); t_exp++) {
    Rtt->AddNode();
  }
  Rtt->ResizeEdges(4);

  bool current_is_vanishing = false;

  // New tangible + vanishing explore loop!
  while (!error) {

    // Get next state to explore, with priority to vanishings.
    if (v_exp < vst.NumStates()) {
      // explore next vanishing
      vst.GetState(v_exp, current);
      current_is_vanishing = true;
    } else {
      // No unexplored vanishing; safe to trash them, if desired
      if (current_is_vanishing) {
#ifdef DEBUG_IMMED
	Output << "Eliminating " << vst.NumStates() << " vanishing states\n";
#endif
	// TODO!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	// Collapse t-v, v-v, v-t arcs into t-t arcs
	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
	vst.Clear();
	vtr.Clear();
	v_exp = 0;
      }
      current_is_vanishing = false;
      // find next tangible to explore; if none, break out
      if (t_exp < S->NumTangible()) {
        S->GetTangible(t_exp, current);
      } else {
	break;
      }
    }
    
    // what is enabled?
    if (!dsm->GetEnabledList(current, &enabled)) {
      error = true;
      continue;
    }

    for (e=0; e<enabled.Length(); e++) {
      event* t = enabled.Item(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      // t is enabled, fire and get new state

      // set reached = current
      if (current_is_vanishing) {
        vst.GetState(v_exp, reached);
	DCASSERT(t->FireType() == E_Immediate);
      } else {
        S->GetTangible(t_exp, reached);
	DCASSERT(t->FireType() == E_Timed);
      }

      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during state space generation";
	Error.Stop();
	error = true;
	break;
      }

      // determine tangible or vanishing
      dsm->isVanishing(reached, x);
      if (!x.isNormal()) {
        Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
        Error << "Bad enabling expression during state space generation";
        Error.Stop();
        error = true;
        break;
      }
      bool next_is_vanishing = x.bvalue;

      // determine next state index
      int vindex = -1;
      int tindex = -1;
      if (next_is_vanishing) {
	vindex = vtr.AddState(reached);
	if (vindex == Pvv.NumNodes())	Pvv.AddNode();
#ifdef DEVELOPMENT_CODE
	int foo = Pvt.NewList();
	DCASSERT(foo==vindex);
#else
	Pvt.NewList();
#endif
      } else {
	tindex = S->FindTangible(reached);
        if (tindex<0) {
          Internal.Start(__FILE__, __LINE__, dsm->Filename(), dsm->Linenumber());
          Internal << "Couldn't find reachable state ";
          dsm->ShowState(Internal, reached);
          Internal << " in tangible reachability set\n";
          Internal.Stop();
        }
      }

      // We have the from state index, to state index;
      // get the rate / weight and add the arc
      double value;
      if (current_is_vanishing) {
	// figure the weight
	if (enabled.Length()==1) {
	  value = 1.0;
  	} else if (t->value>=0) {
	  value = t->value;
	} else {
	// we must have a PROC_REAL weight
          SafeCompute(t->Weight(), current, 0, x);
          if (IllegalWeightCheck(x, dsm, t)) error = true;
	  else value = x.rvalue;
	}
	// Add arc to either Pvv or Pvt
	if (next_is_vanishing) {
	  Pvv.AddEdgeInOrder(v_exp, vindex, value);
	} else {
	  pair arc;
	  arc.index = tindex;
	  arc.weight = value;
	  Pvt.AddItem(v_exp, arc);
	}
      } else {
	// Add arc to either Rtv or Rtt
	// figure the rate
        if (t->value<0) {
	  // we must have a PROC_EXPO distribution
          SafeCompute(t->Distribution(), current, 0, x);
          if (IllegalRateCheck(x, dsm, t)) error = true;
	  else value = x.rvalue;
	} else {
	  value = t->value;
	}
	if (next_is_vanishing) {
	  Rtv.SortedAppend(vindex, value);
	} else {
          Rtt->AddEdgeInOrder(t_exp, tindex, value);
	}
      } // if (arc type)

    } // for e

    // advance appropriate ptr
    if (current_is_vanishing) v_exp++; else t_exp++;
  } // while !error


  // cleanup
  FreeState(reached);
  FreeState(current);
  return !error; 
}
  
/* "Old"
template <class REACHSET>
bool GenerateCTMC(state_model *dsm, REACHSET *S, labeled_digraph<float>* mc)
{
  DCASSERT(dsm);
  DCASSERT(S);

  bool error = false;
  int e;

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  // Allocate list of enabled events
  List <event> enabled(16);
  
  result x;
  x.Clear();
  // compute the constant rates/weights a priori; use -1 if not const.
  float* rates = NULL;
  if (dsm->NumEvents()) rates = new float[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    // get rate or weight as appropriate
    switch (t->FireType()) {
      case E_Timed:
    	if (EXPO==t->DistroType()) {
#ifdef DEBUG_IMMED
	  Output << "Pre-computing rate for event " << t << "\n";
	  Output.flush();
#endif
      	  SafeCompute(t->Distribution(), 0, x);
      	  if (IllegalRateCheck(x, dsm, t)) {
       	    // bail out
	    delete[] rates;
	    return false;
      	  }
      	  rates[e] = x.rvalue;
    	} else {
      	  rates[e] = -1.0;
    	}
	break;

      case E_Immediate:
    	if (REAL==t->WeightType()) {
#ifdef DEBUG_IMMED
	  Output << "Pre-computing weight for event " << t << "\n";
	  Output.flush();
#endif
      	  SafeCompute(t->Weight(), 0, x);
	  if (x.isNormal())
	    rates[e] = x.rvalue;
          else
	    rates[e] = -1;  // signal to compute again later.
	  // Note: if this event is always enabled by itself,
	  // the weight is irrelevant.
    	} else {
      	  rates[e] = -1.0;
    	}
	break;

      default:
	Internal.Start(__FILE__, __LINE__);
        Internal << "Bad event type in MC generation";
        Internal.Stop();
    } // switch
  } // for e

  // Store vanishings in a splay tree (for now...)
  state_array vstates(true);
  splay_state_tree vtree(&vstates);
  // vanishing-vanishing arcs
  labeled_digraph <float> Pvv;
  Pvv.ResizeNodes(4);
  Pvv.ResizeEdges(4);
  // vanishing-tangible arcs
  listarray <pair> Pvt;
  
  // ok, build the ctmc (assumes tangible only; fix soon!)
  int from; 
  mc->ResizeNodes(S->NumTangible());
  for (from=0; from < S->NumTangible(); from++) {
    mc->AddNode();
  }
  mc->ResizeEdges(4);
  for (from=0; from < S->NumTangible(); from++) {
    S->GetTangible(from, current);
    // what is enabled?
    if (!dsm->GetEnabledList(current, &enabled)) {
      error = true;
      break;
    }

    for (e=0; e<enabled.Length(); e++) {
      event* t = enabled.Item(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      // t is enabled, fire and get new state

      // set reached = current
      S->GetTangible(from, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }

      dsm->isVanishing(reached, x);
      DCASSERT(x.isNormal());
      if (x.bvalue) {
        // Reached state is vanishing, explore...
        vtree.AddState(reached);
	Pvv.AddNode();
 	for (int v_exp = 0; v_exp<vtree.NumStates(); v_exp++) {
 	  vstates.GetState(v_exp, vcurrent); 	
	} // for v_exp
      } 

      // Reached state is tangible
      int to = S->FindTangible(reached);
      if (to<0) {
        Internal.Start(__FILE__, __LINE__, dsm->Filename(), dsm->Linenumber());
        Internal << "Couldn't find reachable state ";
        dsm->ShowState(Internal, reached);
        Internal << " in tangible reachability set\n";
        Internal.Stop();
      }

      // determine the rate
      if (rates[e]<0) {
	// we must have a PROC_EXPO distribution
        SafeCompute(t->Distribution(), current, 0, x);
        if (IllegalRateCheck(x, dsm, t)) error = true;
	else mc->AddEdgeInOrder(from, to, x.rvalue);
      } else {
        mc->AddEdgeInOrder(from, to, rates[e]);
      }

    } // for e
    if (error) break;
  } // for from

  // cleanup
  FreeState(reached);
  FreeState(current);
  delete[] rates;
  return !error; 
}
*/

void CompressAndAffix(state_model* dsm, labeled_digraph<float> *mc)
{
  if (NULL==mc) {
    dsm->mc = new markov_chain(NULL);
    dsm->mc->CreateError();
    return;
  }
  dsm->mc = new markov_chain(NULL);
  dsm->mc->CreateExplicit(new classified_chain<float>(mc));
}

void SparseCTMC(state_model *dsm)
{
  DCASSERT(NULL==dsm->mc);
  if (Verbose.IsActive()) {
    Verbose << "Starting CTMC generation using sparse storage\n";
    Verbose.flush();
  }

  labeled_digraph<float>* mc = NULL;
  bool ok = false;

  // Build CTMC based on state space type
  switch (dsm->statespace->Storage()) {
    case RT_None:
    case RT_Error:
	break;

    case RT_Explicit:
        mc = new labeled_digraph<float>;
	ok = GenerateCTMC(dsm, dsm->statespace->Explicit(), mc);
	break;
			
    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Unhandled storage type for CTMC generation\n";
	Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Done generating CTMC; classifying and compressing\n";
    Verbose.flush();
  }

  // An error occurred during generation
  if (!ok) {
    delete mc;
    mc = NULL;
  } else {

    // "generate" initial probability vector here...

    // transpose if necessary
    if (!MatrixByRows->GetBool()) mc->Transpose();
  }

  // attach to model
  CompressAndAffix(dsm, mc);
}

// *******************************************************************
// *                                                                 *
// *                           Front  ends                           *
// *                                                                 *
// *******************************************************************

void BuildRG(state_model *dsm)
{
  if (NULL==dsm) return;
  DCASSERT(dsm->statespace);
  if (dsm->rg) return;  // already built

  const option_const* mc_option = MarkovStorage->GetEnum();

  timer watch;

  watch.Start();
  if (mc_option == &sparse_mc)		SparseRG(dsm); 
  watch.Stop();

  if (NULL==dsm->rg) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "MarkovStorage option " << mc_option->name << " not handled";
    Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Generation took " << watch << "\n";
    Verbose.flush();
  }
}

void BuildCTMC(state_model *dsm)
{
  if (NULL==dsm) return;
  DCASSERT(dsm->statespace);
  if (dsm->mc) return;  // already built

  const option_const* mc_option = MarkovStorage->GetEnum();

  timer watch;

  watch.Start();
  if (mc_option == &sparse_mc)		SparseCTMC(dsm); 
  watch.Stop();

  if (NULL==dsm->mc) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "MarkovStorage option " << mc_option << " not handled";
    Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Generation took " << watch << "\n";
    Verbose.flush();
  }
}

//#define DEBUG

void InitMCGen()
{
#ifdef DEBUG
  Output << "Initializing Markov chain generation options\n";
#endif
  // StateStorage option
  option_const **mclist = new option_const*[num_mc_options];
  // these must be alphabetical
  mclist[0] = &kronecker_mc;
  mclist[1] = &sparse_mc;
  MarkovStorage = MakeEnumOption("MarkovStorage", "Algorithm and data structure to use for Markov chain generation", mclist, num_mc_options, &sparse_mc);
  AddOption(MarkovStorage);
}

