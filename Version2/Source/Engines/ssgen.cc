
// $Id$

#include "ssgen.h"
#include "../States/reachset.h"
#include "../States/flatss.h"
#include "../States/stateheap.h"
#include "../States/trees.h"
#include "../Base/timers.h"

option* StateStorage;

const int num_ss_options = 3;

option_const debug_ss("DEBUG", "\aUse splay tree and display states as they are generated");
option_const redblack_ss("RED_BLACK", "\ared-black tree");
option_const splay_ss("SPLAY", "\aSplay tree");

// return true on success
template <class SSTYPE>
bool Debug_Explore_Indexed(state_model *dsm, state_array *states, SSTYPE* tree)
{
  DCASSERT(dsm);
  DCASSERT(states);
  DCASSERT(tree);

  bool error = false;
  int e;

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

  // Allocate list of enabled events
  List <event> enabled(16);
  
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
      states->GetState(explore, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
	Error << "Bad next-state expression during state space generation";
	Error.Stop();
	error = true;
	break;
      }
      
      int rindex = tree->AddState(reached);

      // debug part...
      Output << "\t" << t;
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

  return !error; 
}

// return true on success
template <class SSTYPE>
bool Explore_Indexed(state_model *dsm, state_array *states, SSTYPE* tree)
{
  DCASSERT(dsm);
  DCASSERT(states);
  DCASSERT(tree);

  bool error = false;
  int e;

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
  
  // Allocate list of enabled events
  List <event> enabled(16);

  result x;
  x.Clear();
  int explore; 
  for (explore=0; explore < states->NumStates(); explore++) {
    states->GetState(explore, current);
    
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
      states->GetState(explore, reached);
      // do the firing
      t->getNextstate()->NextState(current, reached, x); 
      if (!x.isNormal()) {
	Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
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

  return !error; 
}

void CompressAndAffix(state_model* dsm, 
			state_array* tst, binary_tree* ttr,
			state_array* vst, binary_tree* vtr)
{
  DCASSERT(dsm);
  DCASSERT(NULL==dsm->statespace);
  dsm->statespace = new reachset;
  if (NULL==tst || 0==tst->NumStates()) {
    // Error during generation, reflect that fact
    dsm->statespace->CreateError();
    delete tst;
    delete ttr;
    delete vst;
    delete vtr;
    return;
  } 
  // Compress tangibles, trash tree
  int* torder = new int[tst->NumStates()];
  ttr->FillOrderList(torder);
  delete ttr;

  // Do the same for vanishing, if we have any
  if (vst && 0==vst->NumStates()) {
    delete vst;
    vst = NULL;
  }
  int* vorder = NULL;
  if (vst) {
    vorder = new int[vst->NumStates()];
    vtr->FillOrderList(vorder);
  }
  delete vtr;

  // attach everything
  flatss* ss = new flatss(tst, torder, vst, vorder);
  dsm->statespace->CreateExplicit(ss);
}

void DebugReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation in debug mode\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  bool ok = Debug_Explore_Indexed(dsm, states, tree);
  if (!ok) {
    delete states;
    states = NULL;
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
}

void SplayReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using Splay tree\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  bool ok = Explore_Indexed(dsm, states, tree);
  if (Report.IsActive()) {
    Report << "Splay tree:\n";
    tree->Report(Report);
    states->Report(Report);
    Report.flush();
  }
  if (!ok) {
    delete states;
    states = NULL;
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
}

void RedBlackReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using red-black tree\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  red_black_tree* tree = new red_black_tree(states);
  bool ok = Explore_Indexed(dsm, states, tree);
  if (Report.IsActive()) {
    Report << "red-black tree:\n";
    tree->Report(Report);
    states->Report(Report);
    Report.flush();
  }
  if (!ok) {
    delete states;
    states = NULL;
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
}

// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void BuildReachset(state_model *dsm)
{
  if (NULL==dsm) return;
  if (dsm->statespace) return;  // already built

  const option_const* ss_option = StateStorage->GetEnum();

  timer watch;

  watch.Start();
  if (ss_option == &debug_ss)		DebugReachset(dsm); 
  if (ss_option == &splay_ss)		SplayReachset(dsm); 
  if (ss_option == &redblack_ss)	RedBlackReachset(dsm); 
  watch.Stop();

  if (NULL==dsm->statespace) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "StateStorage option " << ss_option << " not handled";
    Internal.Stop();
  }

  if (Verbose.IsActive()) {
    Verbose << "Generation took " << watch << "\n";
    Verbose.flush();
  }
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
  sslist[1] = &redblack_ss;
  sslist[2] = &splay_ss;
  StateStorage = MakeEnumOption("StateStorage", "Algorithm and data structure to use for state space generation", sslist, num_ss_options, &splay_ss);
  AddOption(StateStorage);
}

