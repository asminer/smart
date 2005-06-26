
// $Id$

#include "ssmcgen.h"
#include "ssgen.h"
#include "mcgen.h"

#include "../Base/timers.h"
#include "../States/trees.h"
#include "../States/reachset.h"
#include "../Chains/procs.h"
#include "../Templates/listarray.h"
#include "linear.h"
#include "sccs.h"


// *******************************************************************
// *                                                                 *
// *                          RG  generation                         *
// *                                                                 *
// *******************************************************************

// which are pairwise compatible
bool IsReachsetAndRGCompatible(const option_const* ssgen, 
				const option_const* mcgen)
{
  if (mcgen == &sparse_mc) {
    return (ssgen == &debug_ss) ||
           (ssgen == &hash_ss) ||
           (ssgen == &redblack_ss) ||
	   (ssgen == &splay_ss);
  }
  // kronecker... not done yet
  return false;
}

template <class SSTYPE>
bool GenerateSandRG(state_model *dsm, 
		    state_array *states,
	   	    SSTYPE *tree, 
		    digraph *rg,
		    bool debugging)
{
  DCASSERT(dsm);
  DCASSERT(states);
  DCASSERT(tree);
  DCASSERT(rg);

  bool error = false;
  int e;

  rg->ResizeNodes(4);
  rg->ResizeEdges(4);

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
    rg->AddNode();
    if (debugging) {
      Output << "Added initial state: ";
      dsm->ShowState(Output, current);
      Output << "\n";
      Output.flush();
    }
  }
  
  // Allocate list of enabled events
  List <event> enabled(16);
  
  result x;
  x.Clear();
  
  // ok, build the rg
  int from;
  for (from=0; from < states->NumStates(); from++) {
    states->GetState(from, current);
    if (debugging) {
      Output << "\nExploring state#";
      Output.Put(from, 5);
      Output << " : ";
      dsm->ShowState(Output, current);
      Output << "\n\n";
      Output.flush();
    }
    
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
      if (to == rg->NumNodes()) rg->AddNode();

      rg->AddEdgeInOrder(from, to);

      if (debugging) {
        Output << "\t" << t;
        Output << " --> ";
        dsm->ShowState(Output, reached);
        Output << "\n";
        Output.flush();
      }

    } // for e
    if (error) break;
  } // for from

  // cleanup
  FreeState(reached);
  FreeState(current);
  return !error; 
}



void DebugRGgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set & CTMC generation in debug mode\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  digraph *rg = new digraph;
  bool ok = GenerateSandRG(dsm, states, tree, rg, true);
  if (!ok) {
    delete states;
    states = NULL;
    delete rg;
    rg = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
  CompressAndAffix(dsm, rg);
}

void HashRGgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using Hash & sparse\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  hash_states* tree = new hash_states(states);
  digraph *rg = new digraph;
  bool ok = GenerateSandRG(dsm, states, tree, rg, false);
  if (!ok) {
    delete states;
    states = NULL;
    delete rg;
    rg = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
  CompressAndAffix(dsm, rg);
}


void SplayRGgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using Splay & sparse\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  digraph *rg = new digraph;
  bool ok = GenerateSandRG(dsm, states, tree, rg, false);
  if (!ok) {
    delete states;
    states = NULL;
    delete rg;
    rg = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
  CompressAndAffix(dsm, rg);
}


void RedBlackRGgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using red-black tree & sparse\n";
    Verbose.flush();
  }
  state_array* states = new state_array(true);
  splay_state_tree* tree = new splay_state_tree(states);
  digraph *rg = new digraph;
  bool ok = GenerateSandRG(dsm, states, tree, rg, false);
  if (!ok) {
    delete states;
    states = NULL;
    delete rg;
    rg = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, states, tree, NULL, NULL);
  CompressAndAffix(dsm, rg);
}



// *******************************************************************
// *                                                                 *
// *                         CTMC  generation                        *
// *                                                                 *
// *******************************************************************

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



template <class SSTYPE>
bool GenerateSandCTMC(state_model *dsm, bool eliminate,
		    state_array *tst, SSTYPE *ttr, 
		    state_array *vst, SSTYPE *vtr, 
		    labeled_digraph<float>* Rtt,
		    bool debugging)
{
  DCASSERT(dsm);
  DCASSERT(tst);
  DCASSERT(ttr);
  DCASSERT(vst);
  DCASSERT(vtr);
  DCASSERT(Rtt);

  DCASSERT(eliminate);

  bool error = false;
  int e;
  result x;
  x.Clear();

  // compute the constant rates/weights a priori; use -1 if not const.
  for (e=0; e<dsm->NumEvents(); e++) {
    event* t = dsm->GetEvent(e);
    // get rate or weight as appropriate
    switch (t->FireType()) {
      case E_Timed:
    	if (EXPO==t->DistroType()) {
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

  // allocate temporary (full) states
  DCASSERT(dsm->UsesConstantStateSize());
  int stsize = dsm->GetConstantStateSize();
  state current, reached;
  AllocState(current, stsize);
  AllocState(reached, stsize);
  // Allocate list of enabled events
  List <event> enabled(16);

  // tangible-tangible arcs
  Rtt->ResizeNodes(4);
  Rtt->ResizeEdges(4);
  // vanishing-vanishing arcs
  labeled_digraph <float> Pvv;
  Pvv.ResizeNodes(4);
  Pvv.ResizeEdges(4);
  // vanishing-tangible arcs
  listarray <pair> Pvt;
  // tangible-vanishing arcs
  sparse_vector <float> Rtv(4);

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
    if (x.bvalue) {
      vtr->AddState(current);
      Pvv.AddNode();
      Pvt.NewList();
    } else {
      ttr->AddState(current);
      Rtt->AddNode();
    }
    if (debugging) {
      Output << "Added initial state: ";
      dsm->ShowState(Output, current);
      Output << "\n";
      Output.flush();
    }
  }

  // for collapsing vanishing arcs
  int v_alloc = 0;
  float* h = NULL;
  double* n = NULL;
  int* sccmap = NULL;
  
  int v_exp = 0;
  int t_exp = 0;
  int t_row = -1;

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
   	// Collapse the vanishing arcs
        DCASSERT(v_exp == Pvv.NumNodes());
	DCASSERT(t_row>=0);
	// Enlarge solution vectors as necessary
 	if (v_alloc < v_exp) {
	  v_alloc = v_exp;
	  sccmap = (int*) realloc (sccmap, v_alloc * sizeof(int));
	  if (NULL==sccmap) OutOfMemoryError("Vector resize");
	  h = (float*) realloc(h, v_alloc * sizeof(float));
          if (NULL==h) OutOfMemoryError("Vector resize");
	  n = (double*) realloc(n, v_alloc * sizeof(double));
	  if (NULL==n) OutOfMemoryError("Vector resize");
        }	
	// "Partial" conversion to Static, but keep memory for later
	Pvv.Transpose();
	Pvv.CircularToTerminated();
	Pvv.Defragment(0);
	Pvv.isDynamic = false;  // slight hack ;^)

	// Compute the terminal SCCs, but ignore any vanishing
	// states with arcs to tangibles; if there is still
	// a terminal SCC, it is a vanishing loop with no escape.
	for (int vp=0; vp<v_exp; vp++) {
	  sccmap[vp] = (Pvt.list_pointer[vp]<0) ? 0 : 2*v_exp+1;
        }
	int num_tsccs = ComputeTSCCs(&Pvv, sccmap);
	if (num_tsccs>0) {
          Error.StartModel(dsm->Name(), dsm->Filename(), dsm->Linenumber());
          Error << "Inescapable loop of vanishing states";
          Error.Stop();
          error = true;
	  break;
	}
	// Ok, compute vector n = Rtv * (I + Pvv + Pvv^2 + ...) 
	for (int i=0; i<v_exp; i++) {
	  h[i] = 1.0; // FIX LATER: ASSUMING h = 1
	  n[i] = 0.0;
        }
	MTTASolve(n, &Pvv, h, &Rtv, 0, v_exp);
   	if (debugging) {
	  Output << "New TT entries from tangible# " << t_row << "\n";
        }
	// n * Pvt should give the new T-T entries
        DCASSERT(Pvt.IsDynamic());
	for (int i=0; i<Pvt.num_lists; i++) {
          CHECK_RANGE(0, i, v_exp);
          int ptr = Pvt.list_pointer[i];
          while (ptr>=0) {
	    if (t_row != Pvt.value[ptr].index) { // Don't add self loops
	      if (debugging) {
	        Output << "\tto tangible# " << Pvt.value[ptr].index;
	        Output << " rate " << n[i] * Pvt.value[ptr].weight << "\n";
	        Output.flush();
              }
              Rtt->AddEdgeInOrder(t_row, Pvt.value[ptr].index,
				  Pvt.value[ptr].weight * n[i]);
            } // if not self-loop
	    ptr = Pvt.next[ptr];
	    if (ptr==Pvt.list_pointer[i]) break;
          } // while ptr
	} // for i
        
	Pvv.isDynamic = true;  // part 2 of above hack ;^)
        Pvv.Clear();
  	Pvt.Clear();
  	Rtv.Clear();

        // Done, clear the actual vanishing states
	vst->Clear();
	vtr->Clear();
	v_exp = 0;
	t_row = -1;

      } // if current_is_vanishing
      current_is_vanishing = false;
      // find next tangible to explore; if none, break out
      if (t_exp < tst->NumStates()) {
        tst->GetState(t_exp, current);
      } else {
	break;
      }
    } // if (v_exp < vst.NumStates()) 
    
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
      bool next_is_vanishing = x.bvalue;

      // determine next state index
      int vindex = -1;
      int tindex = -1;
      if (next_is_vanishing) {
	// new state is vanishing
	vindex = vtr->AddState(reached);
	if (vindex >= Pvv.num_nodes) {
  	  Pvv.AddNode();
          Pvt.NewList();
        }
      } else {
	// new state is tangible
	tindex = ttr->AddState(reached);
	if (tindex >= Rtt->num_nodes) Rtt->AddNode();
      }
      if (debugging) {
        Output << "\t" << t;
        Output << " --> ";
        dsm->ShowState(Output, reached);
	if (next_is_vanishing) 
          Output << " (vanishing index " << vindex << ") ";
     	else
          Output << " (tangible index " << tindex << ") ";
      }

      // We have the from state index, to state index;
      // get the rate / weight and add the arc
      double value = 0.0;
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
	if (debugging) {
	  Output << " weight " << value << "\n";
	  Output.flush();
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
	  DCASSERT(t_row<0 || t_row == t_exp);
	  t_row = t_exp;
	  Rtv.SortedAppend(vindex, value);
	} else {
	  if (t_exp != tindex)  // don't add self-loops
            Rtt->AddEdgeInOrder(t_exp, tindex, value);
	}
	if (debugging) {
	  Output << " rate " << value << "\n";
	  Output.flush();
        }
      } // if (arc type)

    } // for e

    // If this is a vanishing state, normalize the weights.
    if (current_is_vanishing) {
      // 
      // Get sum of this row
      //
      double wtotal = 0.0;
      DCASSERT(Pvv.IsDynamic());
      CHECK_RANGE(0, v_exp, Pvv.num_nodes);
      int ptr = Pvv.row_pointer[v_exp];
      while (ptr>=0) {
	wtotal += Pvv.value[ptr];
	ptr = Pvv.next[ptr];
	if (ptr==Pvv.row_pointer[v_exp]) break;
      } // while ptr
      DCASSERT(Pvt.IsDynamic());
      CHECK_RANGE(0, v_exp, Pvt.num_lists);
      ptr = Pvt.list_pointer[v_exp];
      while (ptr>=0) {
	wtotal += Pvt.value[ptr].weight;
	ptr = Pvt.next[ptr];
	if (ptr==Pvt.list_pointer[v_exp]) break;
      } // while ptr
      //
      // Now, normalize the row
      //
      DCASSERT(wtotal>0);
      ptr = Pvv.row_pointer[v_exp];
      while (ptr>=0) {
	Pvv.value[ptr] /= wtotal;
	ptr = Pvv.next[ptr];
	if (ptr==Pvv.row_pointer[v_exp]) break;
      } // while ptr
      ptr = Pvt.list_pointer[v_exp];
      while (ptr>=0) {
	Pvt.value[ptr].weight /= wtotal;
	ptr = Pvt.next[ptr];
	if (ptr==Pvt.list_pointer[v_exp]) break;
      } // while ptr
    }


    // advance appropriate ptr
    if (current_is_vanishing) v_exp++; else t_exp++;
  } // while !error

  // cleanup
  free(sccmap);
  free(h);
  free(n);
  FreeState(reached);
  FreeState(current);

  return !error; 
}



void DebugCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting reachability set & CTMC generation in debug mode\n";
    Verbose.flush();
  }
  state_array* tst = new state_array(true);
  state_array* vst = new state_array(true);
  splay_state_tree* ttr = new splay_state_tree(tst);
  splay_state_tree* vtr = new splay_state_tree(vst);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, true, tst, ttr, vst, vtr, mc, true);
  delete vst;
  vst = NULL;
  delete vtr;
  vtr = NULL;
  if (!ok) {
    delete tst;
    tst = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, tst, ttr, vst, vtr);
  CompressAndAffix(dsm, mc);
}

void HashCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using Hash & sparse\n";
    Verbose.flush();
  }
  state_array* tst = new state_array(true);
  state_array* vst = new state_array(true);
  hash_states* ttr = new hash_states(tst);
  hash_states* vtr = new hash_states(vst);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, true, tst, ttr, vst, vtr, mc, false);
  delete vst;
  vst = NULL;
  delete vtr;
  vtr = NULL;
  if (!ok) {
    delete tst;
    tst = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, tst, ttr, vst, vtr);
  CompressAndAffix(dsm, mc);
}


void SplayCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using Splay & sparse\n";
    Verbose.flush();
  }
  state_array* tst = new state_array(true);
  state_array* vst = new state_array(true);
  splay_state_tree* ttr = new splay_state_tree(tst);
  splay_state_tree* vtr = new splay_state_tree(vst);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, true, tst, ttr, vst, vtr, mc, false);
  delete vst;
  vst = NULL;
  delete vtr;
  vtr = NULL;
  if (!ok) {
    delete tst;
    tst = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, tst, ttr, vst, vtr);
  CompressAndAffix(dsm, mc);
}


void RedBlackCTMCgen(state_model *dsm)
{
  if (Verbose.IsActive()) {
    Verbose << "Starting generation using red-black tree & sparse\n";
    Verbose.flush();
  }
  state_array* tst = new state_array(true);
  state_array* vst = new state_array(true);
  red_black_tree* ttr = new red_black_tree(tst);
  red_black_tree* vtr = new red_black_tree(vst);
  labeled_digraph<float> *mc = new labeled_digraph<float>;
  bool ok = GenerateSandCTMC(dsm, true, tst, ttr, vst, vtr, mc, false);
  delete vst;
  vst = NULL;
  delete vtr;
  vtr = NULL;
  if (!ok) {
    delete tst;
    tst = NULL;
    delete mc;
    mc = NULL;
  }
  if (Verbose.IsActive()) {
    Verbose << "Done generating, compressing\n";
    Verbose.flush();
  }
  CompressAndAffix(dsm, tst, ttr, vst, vtr);
  CompressAndAffix(dsm, mc);
}


// *******************************************************************
// *                           Front  ends                           *
// *******************************************************************

void BuildReachSetAndGraph(state_model *dsm)
{
  if (NULL==dsm) return;
  if (dsm->statespace) {
    BuildRG(dsm);
    return;
  }
  DCASSERT(NULL==dsm->rg);
  const option_const* ss_option = StateStorage->GetEnum();
  DCASSERT(ss_option);
  const option_const* mc_option = MarkovStorage->GetEnum();
  DCASSERT(mc_option);
  if (!IsReachsetAndRGCompatible(ss_option, mc_option)) {
    BuildReachset(dsm);
    BuildRG(dsm);
    return;
  }
  DCASSERT(mc_option == &sparse_mc);  // only one implemented so far
  
  timer watch;

  watch.Start();
  if (ss_option == &debug_ss)		DebugRGgen(dsm); 
  if (ss_option == &hash_ss)		HashRGgen(dsm); 
  if (ss_option == &splay_ss)		SplayRGgen(dsm); 
  if (ss_option == &redblack_ss)	RedBlackRGgen(dsm); 
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
  if (ss_option == &hash_ss)		HashCTMCgen(dsm); 
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

