
// $Id$

#include "ssmcgen.h"
#include "ssgen.h"
#include "mcgen.h"

#include "../Base/timers.h"
#include "../States/trees.h"
#include "../States/reachset.h"
#include "../Chains/procs.h"

// which are pairwise compatible
bool IsReachsetAndCTMCCompatible(const option_const* ssgen, 
				const option_const* mcgen)
{
  if (mcgen == &sparse_mc) {
    return (ssgen == &debug_ss) ||
           (ssgen == &redblack_ss) ||
	   (ssgen == &splay_ss);
  }
  // kronecker... not done yet
  return false;
}

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

template <class SSTYPE>
bool Debug_GenerateSandCTMC(state_model *dsm, 
			    state_array *states,
		   	    SSTYPE *tree, 
			    labeled_digraph<float>* mc)
{
  DCASSERT(dsm);
  DCASSERT(dsm->NumEvents() > 0);
  DCASSERT(states);
  DCASSERT(tree);
  DCASSERT(mc);

  bool error = false;
  int e;

  mc->ResizeNodes(4);
  mc->ResizeEdges(4);

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  // Find and insert the initial state(s)
  for (int i=0; i<dsm->NumInitialStates(); i++) {
    dsm->GetInitialState(i, current);
    tree->AddState(current);
    mc->AddNode();
    // debug part...
    Output << "Added initial state: ";
    dsm->ShowState(Output, current);
    Output << "\n";
    Output.flush();
    // ...to here
  }
  
  result x;
  x.Clear();
  // compute the constant rates once before starting; use -1 if not const.
  float* rates = new float[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    if (EXPO==t->DistroType()) {
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
  } 
  
  // ok, build the ctmc
  int from;
  for (from=0; from < states->NumStates(); from++) {
    states->GetState(from, current);
    // debug part...
    Output << "\nExploring state#";
    Output.Put(from, 5);
    Output << " : ";
    dsm->ShowState(Output, current);
    Output << "\n\n";
    Output.flush();
    // ...to here
    
    // what is enabled?
    for (e=0; e<dsm->NumEvents(); e++) {
      event* t = dsm->GetEvent(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      t->isEnabled()->Compute(current, 0, x);
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	if (x.isUnknown()) 
	  Error << "Unknown if event " << t << " is enabled";
	else
	  Error << "Bad enabling expression for event " << t;
	Error << " during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }
      if (!x.bvalue) continue;  // event is not enabled

      // e is enabled, fire and get new state

      // set reached = current
      states->GetState(from, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }

      // Which state is reached?
      int to = tree->AddState(reached);
      if (to == mc->NumNodes()) mc->AddNode();

      // determine the rate
      double printrate = 0.0;
      if (rates[e]<0) {
	// we must have a PROC_EXPO distribution
        SafeCompute(t->Distribution(), current, 0, x);
        if (IllegalRateCheck(x, dsm, t)) error = true;
	else mc->AddEdgeInOrder(from, to, x.rvalue);
        printrate = x.rvalue; 
      } else {
        mc->AddEdgeInOrder(from, to, rates[e]);
        printrate = rates[e];
      }

      // debug part...
      Output << "\t" << t;
      Output << " --> ";
      dsm->ShowState(Output, reached);
      Output << "\n";
      Output << "\tproduced MC entry [" << from << ", ";
      Output << to << "] = " << printrate << "\n";
      Output.flush();
      // ... to here

    } // for e
    if (error) break;
  } // for from

  // cleanup
  FreeState(reached);
  FreeState(current);
  delete[] rates;
  return !error; 
}


template <class SSTYPE>
bool GenerateSandCTMC(state_model *dsm, 
			    state_array *states,
		   	    SSTYPE *tree, 
			    labeled_digraph<float>* mc)
{
  DCASSERT(dsm);
  DCASSERT(dsm->NumEvents() > 0);
  DCASSERT(states);
  DCASSERT(tree);
  DCASSERT(mc);

  bool error = false;
  int e;

  mc->ResizeNodes(4);
  mc->ResizeEdges(4);

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);

  // Find and insert the initial state(s)
  for (int i=0; i<dsm->NumInitialStates(); i++) {
    dsm->GetInitialState(i, current);
    tree->AddState(current);
    mc->AddNode();
  }
  
  result x;
  x.Clear();
  // compute the constant rates once before starting; use -1 if not const.
  float* rates = new float[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    if (EXPO==t->DistroType()) {
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
  } 
  
  // ok, build the ctmc
  int from;
  for (from=0; from < states->NumStates(); from++) {
    states->GetState(from, current);
    
    // what is enabled?
    for (e=0; e<dsm->NumEvents(); e++) {
      event* t = dsm->GetEvent(e);
      DCASSERT(t);
      if (NULL==t->getNextstate())
        continue;  // firing is "no-op", don't bother

      t->isEnabled()->Compute(current, 0, x);
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	if (x.isUnknown()) 
	  Error << "Unknown if event " << t << " is enabled";
	else
	  Error << "Bad enabling expression for event " << t;
	Error << " during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }
      if (!x.bvalue) continue;  // event is not enabled

      // e is enabled, fire and get new state

      // set reached = current
      states->GetState(from, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during CTMC generation";
	Error.Stop();
	error = true;
	break;
      }

      // Which state is reached?
      int to = tree->AddState(reached);
      if (to == mc->NumNodes()) mc->AddNode();

      // determine the rate
      double printrate = 0.0;
      if (rates[e]<0) {
	// we must have a PROC_EXPO distribution
        SafeCompute(t->Distribution(), current, 0, x);
        if (IllegalRateCheck(x, dsm, t)) error = true;
	else mc->AddEdgeInOrder(from, to, x.rvalue);
        printrate = x.rvalue; 
      } else {
        mc->AddEdgeInOrder(from, to, rates[e]);
        printrate = rates[e];
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


void DebugCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set & CTMC generation in debug mode\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = Debug_GenerateSandCTMC(dsm, states, tree, mc);
  if (!ok) {
    delete states;
    states = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree);
  CompressAndAffix(dsm, mc);
}

void SplayCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using Splay & sparse\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, states, tree, mc);
  if (!ok) {
    delete states;
    states = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree);
  CompressAndAffix(dsm, mc);
}


void RedBlackCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using red-black tree & sparse\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, states, tree, mc);
  if (!ok) {
    delete states;
    states = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree);
  CompressAndAffix(dsm, mc);
}


// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void BuildReachsetAndCTMC(state_model *dsm)
{
  if (NULL==dsm) return;
  if (dsm->statespace) {
    BuildCTMC(dsm);
    return;
  }
  DCASSERT(NULL==dsm->mc);
  const option_const* ss_option = StateStorage->GetEnum();
  DCASSERT(ss_option);
  const option_const* mc_option = MarkovStorage->GetEnum();
  DCASSERT(mc_option);
  if (!IsReachsetAndCTMCCompatible(ss_option, mc_option)) {
    BuildReachset(dsm);
    BuildCTMC(dsm);
    return;
  }
  DCASSERT(mc_option == &sparse_mc);  // only one implemented so far
  
  timer watch;

  watch.Start();
  if (ss_option == &debug_ss)		DebugCTMCgen(dsm); 
  if (ss_option == &splay_ss)		SplayCTMCgen(dsm); 
  if (ss_option == &redblack_ss)	RedBlackCTMCgen(dsm); 
  watch.Stop();

  if (NULL==dsm->statespace) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "StateStorage option " << ss_option->name << " not handled";
    Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Generation took " << watch << "\n";
    Verbose.flush();
  }
}

