
// $Id$

#include "ssgen.h"
#include "../States/flatss.h"
#include "../States/stateheap.h"
#include "../States/trees.h"
#include "../Base/timers.h"

option* StateStorage;

const int num_ss_options = 2;

option_const debug_ss("DEBUG", "Use splay tree and display states as they are generated");
option_const splay_ss("SPLAY", "Splay tree");

// return true on success
template <class SSTYPE>
bool Debug_Explore_Indexed(state_model *dsm, state_array *states, SSTYPE* tree)
{
  DCASSERT(dsm);
  DCASSERT(dsm->NumEvents() > 0);
  DCASSERT(states);
  DCASSERT(tree);

  bool error = false;
  int e;
  // build event enabling and next-state expressions
  expr** enabled = new expr*[dsm->NumEvents()];
  expr** next = new expr*[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    enabled[e] = dsm->EnabledExpr(e);
    DCASSERT(enabled[e] != NULL);
    DCASSERT(enabled[e] != ERROR);
    next[e] = dsm->NextStateExpr(e);
    DCASSERT(next[e] != ERROR);
  }

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
    // debug part...
    Output << "Added initial state: ";
    dsm->ShowState(Output, current);
    Output << "\n";
    Output.flush();
    // ...to here
  }
  
  result x;
  x.Clear();
  int explore; 
  for (explore=0; explore < states->NumStates(); explore++) {
    states->GetState(explore, current);
    // debug part...
    Output << "\nExploring state#";
    Output.Put(explore, 5);
    Output << " : ";
    dsm->ShowState(Output, current);
    Output << "\n\n";
    Output.flush();
    // ...to here
    
    // what is enabled?
    for (e=0; e<dsm->NumEvents(); e++) {
      enabled[e]->Compute(current, 0, x);
      if (!x.isNormal()) {
	Error.Start();
	if (x.isUnknown()) 
	  Error << "Unknown if event is enabled";
	else
	  Error << "Bad enabling expression";
	Error << " during state space generation";
	Error.Stop();
	error = true;
	break;
      }
      if (!x.bvalue) continue;  // event is not enabled

      // e is enabled, fire and get new state
      if (NULL==next[e]) continue;  // firing is "no-op"

      // set reached = current
      states->GetState(explore, reached);
      // do the firing
      next[e]->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.Start();
	Error << "Bad next-state expression during state space generation";
	Error.Stop();
	error = true;
	break;
      }
      
      int rindex = tree->AddState(reached);

      // debug part...
      Output << "\t";
      dsm->ShowEventName(Output, e);
      Output << " --> ";
      dsm->ShowState(Output, reached);
      Output << " (state index " << rindex << ")\n";
      Output.flush();
      // ... to here

    } // for e
    if (error) break;
  } // for explore

  // cleanup
  FreeState(reached);
  FreeState(current);
  for (e=0; e<dsm->NumEvents(); e++) {
    Delete(enabled[e]);
    Delete(next[e]);
  }
  delete[] enabled;
  delete[] next;

  return !error; 
}

// return true on success
template <class SSTYPE>
bool Explore_Indexed(state_model *dsm, state_array *states, SSTYPE* tree)
{
  DCASSERT(dsm);
  DCASSERT(dsm->NumEvents() > 0);
  DCASSERT(states);
  DCASSERT(tree);

  bool error = false;
  int e;
  // build event enabling and next-state expressions
  expr** enabled = new expr*[dsm->NumEvents()];
  expr** next = new expr*[dsm->NumEvents()];
  for (e=0; e<dsm->NumEvents(); e++) {
    enabled[e] = dsm->EnabledExpr(e);
    DCASSERT(enabled[e] != NULL);
    DCASSERT(enabled[e] != ERROR);
    next[e] = dsm->NextStateExpr(e);
    DCASSERT(next[e] != ERROR);
  }

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
  }
  
  result x;
  x.Clear();
  int explore; 
  for (explore=0; explore < states->NumStates(); explore++) {
    states->GetState(explore, current);
    
    // what is enabled?
    for (e=0; e<dsm->NumEvents(); e++) {
      enabled[e]->Compute(current, 0, x);
      if (!x.isNormal()) {
	Error.Start();
	if (x.isUnknown()) 
	  Error << "Unknown if event is enabled";
	else
	  Error << "Bad enabling expression";
	Error << " during state space generation";
	Error.Stop();
	error = true;
	break;
      }
      if (!x.bvalue) continue;  // event is not enabled

      // e is enabled, fire and get new state
      if (NULL==next[e]) continue;  // firing is "no-op"

      // set reached = current
      states->GetState(explore, reached);
      // do the firing
      next[e]->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.Start();
	Error << "Bad next-state expression during state space generation";
	Error.Stop();
	error = true;
	break;
      }
      tree->AddState(reached);
    } // for e
    if (error) break;
  } // for explore

  // cleanup
  FreeState(reached);
  FreeState(current);
  for (e=0; e<dsm->NumEvents(); e++) {
    Delete(enabled[e]);
    Delete(next[e]);
  }
  delete[] enabled;
  delete[] next;

  return !error; 
}

bool DebugReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation in debug mode\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  bool ok = Debug_Explore_Indexed(dsm, states, tree);
  return false; // temporary

  // affix to state model here...

  // return ok;
}

bool SplayReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using Splay tree\n";
    Verbose.flush();
  }
  timer watch;
  watch.Start();
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  bool ok = Explore_Indexed(dsm, states, tree);
  
  Output << states->NumStates() << " states generated\n";
  Output.flush();
  
  watch.Stop();
  if (Report.IsActive()) {
    Report << "Generation took " << watch << "\n";
    Report << "Splay tree:\n";
    tree->Report(Report);
    states->Report(Report);
    Report.flush();
  }


  return false; // temporary

  // affix to state model here...

  // return ok;
}

bool BuildReachset(state_model *dsm)
{
  const option_const* ss_option = StateStorage->GetEnum();
  if (ss_option == &debug_ss)
    return DebugReachset(dsm); 
  if (ss_option == &splay_ss)
    return SplayReachset(dsm); 

  Internal.Start(__FILE__, __LINE__);
  Internal << "StateStorage option " << ss_option << " not handled";
  Internal.Stop();
  return false;
}

//#define DEBUG

void InitSSGen()
{
#ifdef DEBUG
  Output << "Initializing state space generation options\n";
#endif
  // StateStorage option
  option_const **sslist = new option_const*[num_ss_options];
  // these must be alphabetical
  sslist[0] = &debug_ss;
  sslist[1] = &splay_ss;
  StateStorage = MakeEnumOption("StateStorage", "Algorithm and data structure to use for state space generation", sslist, num_ss_options, &splay_ss);
  AddOption(StateStorage);
}

