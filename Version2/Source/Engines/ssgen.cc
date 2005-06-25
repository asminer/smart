
// $Id$

#include "ssgen.h"
#include "../States/reachset.h"
#include "../States/flatss.h"
#include "../States/stateheap.h"
#include "../States/trees.h"
#include "../States/hash_ss.h"
#include "../Base/timers.h"

option* StateStorage;
option* EliminateVanishing;

const int num_ss_options = 4;

option_const debug_ss("DEBUG", "\aUse splay tree and display states as they are generated");
option_const hash_ss("HASH", "\aHash table");
option_const redblack_ss("RED_BLACK", "\ared-black tree");
option_const splay_ss("SPLAY", "\aSplay tree");

// return true on success
template <class SSTYPE>
bool Explore_Indexed(state_model *dsm, bool eliminate,
			state_array *tst, SSTYPE* ttr,
			state_array *vst, SSTYPE* vtr, bool debugging)
{
  DCASSERT(dsm);
  DCASSERT(tst);
  DCASSERT(ttr);
  DCASSERT(vst);
  DCASSERT(vtr);

  bool error = false;
  int e;
  result x;
  x.Clear();

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);
  // Allocate list of enabled events
  List <event> enabled(16);
  
  // Find and insert the initial state(s)
  for (int i=0; i<dsm->NumInitialStates(); i++) {
    dsm->GetInitialState(i, current);
    dsm->isVanishing(current, x);
    if (!x.isNormal()) {
      Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
      Error << "Bad enabling expression during state space generation";
      Error.Stop();
      error = true;
      break;
    }
    if (x.bvalue) vtr->AddState(current);
    else ttr->AddState(current);
    if (debugging) {
      Output << "Added initial state: ";
      dsm->ShowState(Output, current);
      Output << "\n";
      Output.flush();
    }
  }

  int v_exp = 0;
  int t_exp = 0;

  bool current_is_vanishing = false;

  // New tangible + vanishing explore loop!
  while (!error) {

    // Get next state to explore, with priority to vanishings.
    if (v_exp < vst->NumStates()) {
      // explore next vanishing
      vst->GetState(v_exp, current);
      current_is_vanishing = true;
    } else {
      // No unexplored vanishing; safe to trash them, if desired
      if (current_is_vanishing && eliminate) {
	if (debugging) {
	  Output << "Eliminating " << vst->NumStates() << " vanishing states\n";
	}
	vst->Clear();
	vtr->Clear();
	v_exp = 0;
      }
      current_is_vanishing = false;
      // find next tangible to explore; if none, break out
      if (t_exp < tst->NumStates()) {
        tst->GetState(t_exp, current);
      } else {
	break;
      }
    }
    
    if (debugging) {
      if (current_is_vanishing) {
        Output << "\nExploring vanishing state#";
        Output.Put(v_exp, 5);
      } else {
        Output << "\nExploring tangible state#";
        Output.Put(t_exp, 5);
      }
      Output << " : ";
      dsm->ShowState(Output, current);
      Output << "\n\n";
      Output.flush();
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
        vst->GetState(v_exp, reached);
	DCASSERT(t->FireType() == E_Immediate);
      } else {
        tst->GetState(t_exp, reached);
	DCASSERT(t->FireType() != E_Immediate);
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

      if (x.bvalue) {
	// new state is vanishing
	int vindex = vtr->AddState(reached);
	if (debugging) {
          Output << "\t" << t;
          Output << " --> ";
          dsm->ShowState(Output, reached);
          Output << " (vanishing index " << vindex << ")\n";
          Output.flush();
	}
      } else {
	// new state is tangible
	int tindex = ttr->AddState(reached);
	if (debugging) {
          Output << "\t" << t;
          Output << " --> ";
          dsm->ShowState(Output, reached);
          Output << " (tangible index " << tindex << ")\n";
          Output.flush();
	}
      }

    } // for e


    // advance appropriate ptr
    if (current_is_vanishing) v_exp++; else t_exp++;
  } // while !error

  // cleanup
  FreeState(reached);
  FreeState(current);

  return !error; 
}


template <class SSDATA>
void TemplCompressAndAffix(state_model* dsm, 
			state_array* tst, SSDATA* ttr,
			state_array* vst, SSDATA* vtr)
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

void CompressAndAffix(state_model* dsm, 
			state_array* tst, binary_tree* ttr,
			state_array* vst, binary_tree* vtr)
{
  TemplCompressAndAffix(dsm, tst, ttr, vst, vtr);
}

void CompressAndAffix(state_model* dsm, 
			state_array* tst, hash_states* ttr,
			state_array* vst, hash_states* vtr)
{
  TemplCompressAndAffix(dsm, tst, ttr, vst, vtr);
}

void DebugReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation in debug mode\n";
    Verbose.flush();
  }
  state_array* tstates = new state_array(true);
  state_array* vstates = new state_array(true);
  splay_state_tree* ttree = new splay_state_tree(tstates);
  splay_state_tree* vtree = new splay_state_tree(vstates);
  bool ok = Explore_Indexed(dsm, EliminateVanishing->GetBool(), 
  				tstates, ttree, vstates, vtree, true);
  if (!ok) {
    delete tstates;
    tstates = NULL;
  }
  CompressAndAffix(dsm, tstates, ttree, vstates, vtree);
}

void SplayReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using Splay tree\n";
    Verbose.flush();
  }
  state_array* tstates = new state_array(true);
  state_array* vstates = new state_array(true);
  splay_state_tree* ttree = new splay_state_tree(tstates);
  splay_state_tree* vtree = new splay_state_tree(vstates);
  bool ok = Explore_Indexed(dsm, EliminateVanishing->GetBool(),
  				tstates, ttree, vstates, vtree, false);
  if (Report.IsActive()) {
    Report << "Tangible Splay tree:\n";
    ttree->Report(Report);
    tstates->Report(Report);
    Report.flush();
  }
  if (!ok) {
    delete tstates;
    tstates = NULL;
  }
  CompressAndAffix(dsm, tstates, ttree, vstates, vtree);
}

void RedBlackReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using red-black tree\n";
    Verbose.flush();
  }
  state_array* tstates = new state_array(true);
  state_array* vstates = new state_array(true);
  red_black_tree* ttree = new red_black_tree(tstates);
  red_black_tree* vtree = new red_black_tree(vstates);
  bool ok = Explore_Indexed(dsm, EliminateVanishing->GetBool(),
  				tstates, ttree, vstates, vtree, false);
  if (Report.IsActive()) {
    Report << "Tangible red-black tree:\n";
    ttree->Report(Report);
    tstates->Report(Report);
    Report.flush();
  }
  if (!ok) {
    delete tstates;
    tstates = NULL;
  }
  CompressAndAffix(dsm, tstates, ttree, vstates, vtree);
}

void HashReachset(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set generation using hash table\n";
    Verbose.flush();
  }
  state_array* tstates = new state_array(true);
  state_array* vstates = new state_array(true);
  hash_states* ttree = new hash_states(tstates);
  hash_states* vtree = new hash_states(vstates);
  bool ok = Explore_Indexed(dsm, EliminateVanishing->GetBool(),
  				tstates, ttree, vstates, vtree, true );
  if (Report.IsActive()) {
    Report << "Tangible hash table:\n";
    ttree->Report(Report);
    tstates->Report(Report);
    Report.flush();
  }
  if (!ok) {
    delete tstates;
    tstates = NULL;
  }
  CompressAndAffix(dsm, tstates, ttree, vstates, vtree);
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
  if (ss_option == &hash_ss)		HashReachset(dsm); 
  watch.Stop();

  if (NULL==dsm->statespace) {
    // we didn't do anything...
    Internal.Start(__FILE__, __LINE__);
    Internal << "StateStorage option ";
    ss_option->show(Internal);
    Internal << " not handled";
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
  sslist[1] = &hash_ss;
  sslist[2] = &redblack_ss;
  sslist[3] = &splay_ss;
  StateStorage = MakeEnumOption("StateStorage", "Algorithm and data structure to use for state space generation", sslist, num_ss_options, &splay_ss);
  AddOption(StateStorage);

  // EliminateVanishing option
  EliminateVanishing = MakeBoolOption("EliminateVanishing", "Should vanishing states be eliminated.", true);
  AddOption(EliminateVanishing);
}

