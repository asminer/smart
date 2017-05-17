
// $Id$

#include "../ExprLib/startup.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/measures.h"

#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/dsde_hlm.h"
#include "../Formlsms/rss_meddly.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"

#include "meddly_expert.h"
#include "timerlib.h"

#include "basic_msr.h"

#include <vector>
#include <algorithm>

// #define ALLOW_SHOW_PARAMS

// ******************************************************************
// *                                                                *
// *                     proc_noengine  methods                     *
// *                                                                *
// ******************************************************************


engtype* proc_noengine::ProcGen = 0;

proc_noengine
::proc_noengine(eng_class ect, const type* t, const char* name, int np)
 : msr_noengine(ect, t, name, np)
{
}

state_lldsm* 
proc_noengine ::BuildProc(hldsm* hlm, bool states_only, const expr* err)
{
  if (0==hlm)  return 0;
  result so;
  so.setBool(states_only);

  try {
      lldsm* llm = hlm->GetProcess();
      if (llm) {
        subengine* gen = llm->getCompletionEngine();
        if (0==gen) return dynamic_cast <state_lldsm*> (llm);
        gen->RunEngine(hlm, so);
      } else {
        if (!ProcGen) throw subengine::No_Engine;
        ProcGen->runEngine(hlm, so);
      }
      return dynamic_cast <state_lldsm*> (hlm->GetProcess());
  } // try
    
  catch (subengine::error e) {
      if (em->startError()) {
        em->causedBy(err);
        em->cerr() << "Couldn't build ";
        em->cerr() << (states_only ? "state space: " : "underlying process: ");
        em->cerr() << subengine::getNameOfError(e);
        em->stopIO();
      }
      return 0;
  } // catch
}

/*
void proc_noengine::BuildPartition(hldsm* hlm, const expr* err)
{
  if (0==hlm)  return;
  result dummy;

  try {
      if (hlm->hasPartInfo()) return;
      if (!VarOrder) throw subengine::No_Engine;
      result dummy;
      VarOrder->runEngine(hlm, dummy);
      if (hlm->hasPartInfo()) return;
      throw subengine::Engine_Failed;
  } // try
    
  catch (subengine::error e) {
      if (em->startError()) {
        em->causedBy(err);
        em->cerr() << "Couldn't build variable order: ";
        em->cerr() << subengine::getNameOfError(e);
        em->stopIO();
      }
  } // catch
}
*/

// ******************************************************************
// *                           num_states                           *
// ******************************************************************


class numstates_si : public proc_noengine {
public:
  numstates_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};


numstates_si::numstates_si()
#ifdef ALLOW_SHOW_PARAMS
 : proc_noengine(Nothing, em->BIGINT, "num_states", 2) 
#else
 : proc_noengine(Nothing, em->BIGINT, "num_states", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of reachable states.  If show is true, then as a side effect, the reachability set is displayed to the current output stream (unless there are too many states).");
#else
  SetDocumentation("Computes if necessary, and returns the number of reachable states.");
#endif
}

void numstates_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const state_lldsm* llm = BuildProc(
    mi ? mi->GetCompiledModel() : 0, 1, x.parent
  );
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
#ifdef ALLOW_SHOW_PARAMS
  SafeCompute(pass[1], x);
  bool show = false;
  if (x.answer->isNormal()) show = x.answer->getBool();
  if (!em->hasIO()) show = false;
#endif
  llm->getNumStates(*x.answer);
  if (!x.answer->isNormal())  return;
  if (!x.answer->getPtr()) {
    long ns = x.answer->getInt();
    x.answer->setPtr(new bigint(ns));
  }
#ifdef ALLOW_SHOW_PARAMS
  if (show) llm->getNumStates(true);
#endif
}

// ******************************************************************
// *                            num_arcs                            *
// ******************************************************************

class numarcs_si : public proc_noengine {
public:
  numarcs_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

numarcs_si::numarcs_si()
#ifdef ALLOW_SHOW_PARAMS
 : proc_noengine(Nothing, em->BIGINT, "num_arcs", 2)
#else
 : proc_noengine(Nothing, em->BIGINT, "num_arcs", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of arcs in the reachability graph or Markov chain.  If show is true, then as a side effect, the graph is displayed to the current output stream (unless it is too large).");
#else
  SetDocumentation("Computes if necessary, and returns the number of arcs in the underlying process (reachability graph, Markov chain, etc.).");
#endif
}

void numarcs_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  const graph_lldsm* gllm = smart_cast<const graph_lldsm*>(llm);
  DCASSERT(gllm);

#ifdef ALLOW_SHOW_PARAMS
  SafeCompute(pass[1], x);
  bool show = false;
  if (x.answer->isNormal()) show = x.answer->getBool();
  if (!em->hasIO()) show = false;
#endif
  gllm->getNumArcs(*x.answer);
  if (!x.answer->isNormal())    return;
  if (!x.answer->getPtr()) {
    long na = x.answer->getInt();
    x.answer->setPtr(new bigint(na));
  }
#ifdef ALLOW_SHOW_PARAMS
  if (show) gllm->showArcs(false);
#endif
}

// ******************************************************************
// *                           num_classes                          *
// ******************************************************************

class numclasses_si : public proc_noengine {
public:
  numclasses_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

numclasses_si::numclasses_si()
#ifdef ALLOW_SHOW_PARAMS
 : proc_noengine(Nothing, em->BIGINT, "num_classes", 2)
#else
 : proc_noengine(Nothing, em->BIGINT, "num_classes", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of terminal strongly-connected components in the reachability graph (equivalently, the number of recurrent classes in the Markov chain).  If show is true, then as a side effect, the states in each TSCC are displayed to the current output stream.");
#else 
  SetDocumentation("Computes if necessary, and returns the number of terminal strongly-connected components in the reachability graph (equivalently, the number of recurrent classes in the Markov chain).");
#endif
}

void numclasses_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
#ifdef ALLOW_SHOW_PARAMS
  SafeCompute(pass[1], x);
  bool show = false;
  if (x.answer->isNormal()) show = x.answer->getBool();
#endif
  const stochastic_lldsm* sllm = smart_cast <const stochastic_lldsm*> (llm);
  DCASSERT(sllm);
  sllm->getNumClasses(*x.answer);
  if (!x.answer->isNormal())    return;
  if (x.answer->getInt() < 0)   return;
  if (!x.answer->getPtr()) {
    long nc = x.answer->getInt();
    x.answer->setPtr(new bigint(nc));
  }
#ifdef ALLOW_SHOW_PARAMS
  long foo = sllm->getNumClasses(show);
  if (show && foo<0) {
    em->cout() << "Graph is too large\n";
    em->cout().flush();
  }
#endif
}

// ******************************************************************
// *                variable_order_transformation                   *
// ******************************************************************

// Junaid, Chuan, Ben: experiments on transforming variable orders

#define VAR_PARAMS

#if 0
std::vector< std::vector<int> > getNorders(dsde_hlm& smartModel, int numOrders, int maxIter);
#endif

class var_order_transform : public proc_noengine {
public:
  var_order_transform();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};


var_order_transform::var_order_transform()
#ifdef VAR_PARAMS
 : proc_noengine(Nothing, em->BIGINT, "var_order_transform", 2) 
#else
 : proc_noengine(Nothing, em->BIGINT, "var_order_transform", 1)
#endif
{
#ifdef VAR_PARAMS
  SetFormal(1, em->INT, "heuristic");
  SetDocumentation("Variable Ordering Heuristic. Side effect: computes the reachability set and displays it, and returns the number of states.");
#else
  SetDocumentation("Variable Ordering Experiment. Side effect: computes the reachability set and returns the number of states.");
#endif
}

void var_order_transform::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  hldsm* hi = mi ? mi->GetCompiledModel() : 0;
  const state_lldsm* llm = BuildProc(hi, 1, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  long heuristic = 0l;
#ifdef VAR_PARAMS
  SafeCompute(pass[1], x);
  if (x.answer->isNormal()) heuristic = x.answer->getInt();
  printf("Heuristic %ld\n", heuristic);
#endif
  llm->getNumStates(*x.answer);
  if (!x.answer->isNormal())  return;
  if (!x.answer->getPtr()) {
    long ns = x.answer->getInt();
    x.answer->setPtr(new bigint(ns));
  }

  // Call Ben's code for computing variable orders
  if (hi != 0) {
    dsde_hlm *dsde_instance = dynamic_cast<dsde_hlm*> (hi);
    if (dsde_instance != 0) {
#if 0
      const meddly_reachset* mrs =
        dynamic_cast<const meddly_reachset*>(llm->getRSS());
      if (mrs != 0) {
        std::vector< std::vector<int> > orders =
          getNorders(*dsde_instance, 10, 100);

        printf("Size of orders: %lu\n", orders.size());

        // The following needs C++11
        bool even = true;
        for (auto i:orders) {
          even = !even;
          if (!even) continue;
          printf("Order: [");
          for (auto j:i) { printf("%d ", j); }
          printf("]\n");

          // Call Chuan's code
          timer t;
          ((MEDDLY::expert_forest*)f)->reorderVariables(&i[0]);
          double s = t.elapsed_microseconds() / 1000000.0;
          printf("Time for reordering variables: %f seconds\n", s);
        }
      }
#endif

    } else {
      printf("Could not convert to dsde_instance.\n");
    }
  } else {
    printf("Could not convert to hldsm.\n");
  }
}

// *****************************************************************
// *                           num_levels                          *
// *****************************************************************

class numlevels_si : public msr_noengine {
public:
  numlevels_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

numlevels_si::numlevels_si()
#ifdef ALLOW_SHOW_PARAMS
 : msr_noengine(Nothing, em->INT, "num_levels", 2)
#else
 : msr_noengine(Nothing, em->INT, "num_levels", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of levels of a model. This is used for hierarchical state representations, including MDDs. Assignment of state variables to levels can be done using the appropriate functions for each formalism.  If parameter show is true, then as a side effect, the state variables in each level are displayed to the current output stream.");
#else
  SetDocumentation("Returns the number of levels of a model. This is used for hierarchical state representations, including MDDs.");
#endif
}

void numlevels_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  hldsm* hlm = mi ? mi->GetCompiledModel() : 0;
  
#ifdef ALLOW_SHOW_PARAMS
  SafeCompute(pass[1], x);
  bool show = false;
  if (x.answer->isNormal()) show = x.answer->getBool();
#endif

  switch (hlm->Type()) {
    case hldsm::Enumerated:
        x.answer->setInt(1);
#ifdef ALLOW_SHOW_PARAMS
        if (show) {
          em->cout() << "Level 1:\n\tstate\n";
          em->cout().flush();
        }
#endif
        return;

    case hldsm::Asynch_Events:
    case hldsm::Synch_Events:
        break;  // deal with this below

    default:
        DCASSERT(0);
        return;
  }
  
  if (!hlm->buildPartInfo()) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't build variable order";
      em->stopIO();
    }
  }

  const dsde_hlm* dsm = smart_cast <const dsde_hlm*> (hlm);
  DCASSERT(dsm);

  if (!dsm->hasPartInfo()) {
    x.answer->setInt(1);
#ifdef ALLOW_SHOW_PARAMS
    if (!show) return;
    em->cout() << "Level 1:\n\t";
    bool printed = false;
    for (int i=0; i<dsm->getNumStateVars(); i++) {
      if (printed)  em->cout() << ", ";
      else          printed = true;
      em->cout() << dsm->readStateVar(i)->Name();
    } // for i
    em->cout() << "\n";
    em->cout().flush();
#endif
  } else {
    const hldsm::partinfo& foo = dsm->getPartInfo();
    x.answer->setInt(foo.num_levels);
#ifdef ALLOW_SHOW_PARAMS
    if (!show) return;
    for (int k=foo.num_levels; k; k--) {
      em->cout() << "Level " << k << ":\n\t";
      bool printed = false;
      for (int p=foo.pointer[k]; p>foo.pointer[k-1]; p--) {
        if (printed)  em->cout() << ", ";
        else          printed = true;
        em->cout() << foo.variable[p]->Name();
      }  // for p
      em->cout() << "\n";
      em->cout().flush();
    } // for k
#endif
  }
}

// *****************************************************************
// *                           num_events                          *
// *****************************************************************

class numevents_si : public msr_noengine {
public:
  numevents_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

numevents_si::numevents_si()
#ifdef ALLOW_SHOW_PARAMS
 : msr_noengine(Nothing, em->INT, "num_events", 2)
#else
 : msr_noengine(Nothing, em->INT, "num_events", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of events of a model. This is useful for debugging a model. If parameter show is true, then as a side effect, information for each event is displayed to the current output stream.");
#else
  SetDocumentation("Returns the number of events of a model.");
#endif
}

void numevents_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const dsde_hlm* hlm = 
    smart_cast<const dsde_hlm*> (mi ? mi->GetCompiledModel() : 0);
  
  if (hlm) {
    long ans = hlm->getNumEvents();
    DCASSERT(ans>=0);
    x.answer->setInt(ans);

#ifdef ALLOW_SHOW_PARAMS
    SafeCompute(pass[1], x);
    if (x.answer->isNormal()) {
      if (x.answer->getBool()) {
        hlm->showEvents(em->cout());
      }
    }
#endif
  } else {
    x.answer->setNull();
  }
}

// *****************************************************************
// *                            num_vars                           *
// *****************************************************************

class numvars_si : public msr_noengine {
public:
  numvars_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

numvars_si::numvars_si()
#ifdef ALLOW_SHOW_PARAMS
 : msr_noengine(Nothing, em->INT, "num_vars", 2)
#else
 : msr_noengine(Nothing, em->INT, "num_vars", 1)
#endif
{
#ifdef ALLOW_SHOW_PARAMS
  SetFormal(1, em->BOOL, "show");
  SetDocumentation("Returns the number of state variables in a model. This is useful for debugging a model. If parameter show is true, then as a side effect, information for each state variable is displayed to the current output stream.");
#else
  SetDocumentation("Returns the number of state variables in a model.");
#endif
}

void numvars_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const dsde_hlm* hlm = 
    smart_cast<const dsde_hlm*> (mi ? mi->GetCompiledModel() : 0);
  
  if (hlm) {
    long ans = hlm->getNumStateVars();
    DCASSERT(ans>=0);
    x.answer->setInt(ans);

#ifdef ALLOW_SHOW_PARAMS
    SafeCompute(pass[1], x);
    if (x.answer->isNormal() && x.answer->getBool()) {
      size_t nw = 14;
      for (long i=0; i<ans; i++) {
        const model_statevar* sv = hlm->readStateVar(i);
        nw = MAX(nw, strlen(sv->Name()));
      }
      em->cout().Put("State variable", nw);
      em->cout().Put("index", 10);
      em->cout().Put("substate", 10);
      em->cout().Put('\n');
      for (long i=0; i<ans; i++) {
        const model_statevar* sv = hlm->readStateVar(i);
        em->cout().Put(sv->Name(), nw);
        em->cout().Put(long(sv->GetIndex()), 10);
        em->cout().Put(long(sv->GetPart()), 10);
        em->cout().Put('\n');
      } // for i
    }
#endif
  } else {
    x.answer->setNull();
  }
}

// ******************************************************************
// *                           show_states                          *
// ******************************************************************

class showstates_si : public proc_noengine {
public:
  showstates_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};


showstates_si::showstates_si()
 : proc_noengine(Nothing, em->VOID, "show_states", 2)
{
  SetDocumentation("Displays the reachability set to the current output stream.  The reachability set will be constructed first, if necessary.  If parameter `internal' is true, then the internal representation of the states is displayed; otherwise, a storage-independent list of states is displayed (unless there are too many).");
  result def;
  def.setBool(false);
  SetFormal(1, em->BOOL, "internal", 
    em->makeLiteral(0, -1, em->BOOL, def)
  );
}

void showstates_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const state_lldsm* llm = BuildProc(
    mi ? mi->GetCompiledModel() : 0, 1, x.parent
  );
  if (0==llm || lldsm::Error == llm->Type()) return;

  bool internal = false;
  SafeCompute(pass[1], x);
  if (x.answer->isNormal()) {
    internal = x.answer->getBool();
  }
  llm->showStates(internal);
}

// ******************************************************************
// *                            show_arcs                           *
// ******************************************************************

class showarcs_si : public proc_noengine {
public:
  showarcs_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showarcs_si::showarcs_si()
 : proc_noengine(Nothing, em->VOID, "show_arcs", 2)
{
  SetDocumentation("Display the underlying reachability graph to the current output stream.  The process will be constructed first, if necessary.  If parameter `internal' is true, then the internal representation of the process is displayed; otherwise, a storage-independent enumeration of the process is displayed (unless it is too large).");
  result def;
  def.setBool(false);
  SetFormal(1, em->BOOL, "internal", 
    em->makeLiteral(0, -1, em->BOOL, def)
  );
}

void showarcs_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    return;
  }
  const graph_lldsm* gllm = smart_cast<const graph_lldsm*>(llm);
  DCASSERT(gllm);

  bool internal = false;
  SafeCompute(pass[1], x);
  if (x.answer->isNormal()) {
    internal = x.answer->getBool();
  }
  gllm->showArcs(internal); 
}

// ******************************************************************
// *                            show_proc                           *
// ******************************************************************

class showproc_si : public proc_noengine {
public:
  showproc_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showproc_si::showproc_si()
 : proc_noengine(Nothing, em->VOID, "show_proc", 2)
{
  SetDocumentation("Display the underlying process (reachability graph, Markov chain, etc.) to the current output stream.  The process will be constructed first, if necessary.  If parameter `internal' is true, then the internal representation of the process is displayed; otherwise, a storage-independent enumeration of the process is displayed (unless it is too large).");
  result def;
  def.setBool(false);
  SetFormal(1, em->BOOL, "internal", 
    em->makeLiteral(0, -1, em->BOOL, def)
  );
}

void showproc_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    return;
  }

  bool internal = false;
  SafeCompute(pass[1], x);
  if (x.answer->isNormal()) {
    internal = x.answer->getBool();
  }

  const stochastic_lldsm* sllm = dynamic_cast<const stochastic_lldsm*> (llm);
  if (sllm) {
    sllm->showProc(internal);
    return;
  }

  // Not stochastic.  Show the reachability graph instead.

  const graph_lldsm* gllm = smart_cast<const graph_lldsm*>(llm);
  DCASSERT(gllm);

  gllm->showArcs(internal); 
}

// ******************************************************************
// *                           show_classes                         *
// ******************************************************************

class showclasses_si : public proc_noengine {
public:
  showclasses_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showclasses_si::showclasses_si()
 : proc_noengine(Nothing, em->VOID, "show_classes", 1)
{
  SetDocumentation("Shows the classification of states into terminal strongly-connected components in the reachability graph (equivalently, the number of recurrent classes in the Markov chain) to the current output stream.");
}

void showclasses_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    return;
  }

  const stochastic_lldsm* sllm = smart_cast <const stochastic_lldsm*> (llm);
  DCASSERT(sllm);
  sllm->showClasses();
  /*
  long foo = sllm->getNumClasses(true);
  if (foo<0) {
    em->cout() << "Graph is too large\n";
    em->cout().flush();
  }
  */
}

// *****************************************************************
// *                           show_levels                         *
// *****************************************************************

class showlevels_si : public msr_noengine {
public:
  showlevels_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showlevels_si::showlevels_si()
 : msr_noengine(Nothing, em->VOID, "show_levels", 1)
{
  SetDocumentation("Display the state variables by level to the current output stream.  This is used for hierarchical state representations, including MDDs.  Assignment of state variables to levels can be done using the appropriate functions for each formalism.");
}

void showlevels_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  hldsm* hlm = mi ? mi->GetCompiledModel() : 0;
  
  switch (hlm->Type()) {
    case hldsm::Enumerated:
        em->cout() << "Level 1:\n\tstate\n";
        em->cout().flush();
        return;

    case hldsm::Asynch_Events:
    case hldsm::Synch_Events:
        break;  // deal with this below

    default:
        DCASSERT(0);
        return;
  }
  
  if (!hlm->buildPartInfo()) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't build variable order";
      em->stopIO();
    }
  }

  const dsde_hlm* dsm = smart_cast <const dsde_hlm*> (hlm);
  DCASSERT(dsm);

  if (!dsm->hasPartInfo()) {
    em->cout() << "Level 1:\n\t";
    bool printed = false;
    for (int i=0; i<dsm->getNumStateVars(); i++) {
      if (printed)  em->cout() << ", ";
      else          printed = true;
      em->cout() << dsm->readStateVar(i)->Name();
    } // for i
    em->cout() << "\n";
    em->cout().flush();
  } else {
    const hldsm::partinfo& foo = dsm->getPartInfo();
    for (int k=foo.num_levels; k; k--) {
      em->cout() << "Level " << k << ":\n\t";
      bool printed = false;
      for (int p=foo.pointer[k]; p>foo.pointer[k-1]; p--) {
        if (printed)  em->cout() << ", ";
        else          printed = true;
        em->cout() << foo.variable[p]->Name();
      }  // for p
      em->cout() << "\n";
      em->cout().flush();
    } // for k
  }
}

// *****************************************************************
// *                           show_events                         *
// *****************************************************************

class showevents_si : public msr_noengine {
public:
  showevents_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showevents_si::showevents_si()
 : msr_noengine(Nothing, em->VOID, "show_events", 1)
{
  SetDocumentation("Information for each model event is displayed to the current output stream.  This is useful for debugging a model.");
}

void showevents_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const dsde_hlm* hlm = 
    smart_cast<const dsde_hlm*> (mi ? mi->GetCompiledModel() : 0);
  
  if (hlm) {
    hlm->showEvents(em->cout());
  }
}

// *****************************************************************
// *                            show_vars                          *
// *****************************************************************

class showvars_si : public msr_noengine {
public:
  showvars_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

showvars_si::showvars_si()
 : msr_noengine(Nothing, em->VOID, "show_vars", 1)
{
  SetDocumentation("Information for each state variable is displayed to the current output stream.  This is useful for debugging a model.");
}

void showvars_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const dsde_hlm* hlm = 
    smart_cast<const dsde_hlm*> (mi ? mi->GetCompiledModel() : 0);
  
  if (hlm) {
      long ans = hlm->getNumStateVars();
      size_t nw = 14;
      for (long i=0; i<ans; i++) {
        const model_statevar* sv = hlm->readStateVar(i);
        nw = MAX(nw, strlen(sv->Name()));
      }
      em->cout().Put("State variable", nw);
      em->cout().Put("index", 10);
      em->cout().Put("substate", 10);
      em->cout().Put('\n');
      for (long i=0; i<ans; i++) {
        const model_statevar* sv = hlm->readStateVar(i);
        em->cout().Put(sv->Name(), nw);
        em->cout().Put(long(sv->GetIndex()), 10);
        em->cout().Put(long(sv->GetPart()), 10);
        em->cout().Put('\n');
      } // for i
  } 
}

// *****************************************************************
// *                           run_for_MCC                         *
// *****************************************************************

class run_for_MCC_si : public proc_noengine {
public:
  run_for_MCC_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

run_for_MCC_si::run_for_MCC_si()
 : proc_noengine(Nothing, em->VOID, "run_for_MCC", 1)
{
  SetDocumentation("Specialized function for running experiments for the annual Model Checking Competition.");
}

void run_for_MCC_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);

  model_instance* mi = grabModelInstance(x, pass[0]);
  const state_lldsm* llm = BuildProc(
    mi ? mi->GetCompiledModel() : 0, 1, x.parent
  );
  if (0==llm || lldsm::Error == llm->Type()) return;

  // actual code here!

#if 0
  // TBD - call a virtual function within llm to display this
  em->cout() << "Method: decision diagram (TBD - fix the format please!\n";
  em->cout().flush();
  //
  // Display number of states
  //
  result numstates;
  llm->getNumStates(numstates);
  if (!numstates.isNormal()) {
    //
    // TBD: Error, can we print something and exit cleanly here?

    return;
  }

  em->cout() << "Number of states (TBD fix format please!): ";  
  shared_object* bigns = numstates.getPtr();
  if (bigns) {
    bigns->Print(em->cout(), 0);
  } else {
    long ns = numstates.getInt();
    em->cout() << ns;
  }
  em->cout().Put('\n');
  em->cout().flush();
#endif

  //
  // TBD - other things to display here
  //

}


// *****************************************************************
// *                            initial                            *
// *****************************************************************

class initial_si : public proc_noengine {
public:
  initial_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

initial_si::initial_si()
 : proc_noengine(Nothing, em->STATESET, "initial", 1)
{
  SetDocumentation("Returns the set of initial states within a model.");
}

void initial_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 0, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  const state_lldsm* sllm = smart_cast <const state_lldsm*>(llm);
  DCASSERT(sllm);
  stateset* ss = sllm->getInitialStates();
  if (ss) {
    x.answer->setPtr(ss);
  } else {
    x.answer->setNull();
  }
}


// *****************************************************************
// *                           reachable                           *
// *****************************************************************

class reachable_si : public proc_noengine {
public:
  reachable_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

reachable_si::reachable_si()
 : proc_noengine(Nothing, em->STATESET, "reachable", 1)
{
  SetDocumentation("Returns the set of reachable states within a model.");
}

void reachable_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 1, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  const graph_lldsm* gllm = smart_cast <const graph_lldsm*>(llm);
  DCASSERT(gllm);
  stateset* rss = gllm->getReachable();
  if (rss) {
    x.answer->setPtr(rss);
  } else {
    x.answer->setNull();
  }
}


// *****************************************************************
// *                           potential                           *
// *****************************************************************

class potential_si : public proc_noengine {
public:
  potential_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

potential_si::potential_si()
 : proc_noengine(Nothing, em->STATESET, "potential", 2)
{
  SetFormal(1, em->BOOL->addProc(), "p");
  SetDocumentation("Returns the set of model states satisfying p.  Note that this set could contain states that are not reachable from the initial state(s) of the model.");
}

void potential_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  model_instance* mi = grabModelInstance(x, pass[0]);
  const lldsm* llm = BuildProc(mi ? mi->GetCompiledModel() : 0, 1, x.parent);
  if (0==llm || lldsm::Error == llm->Type()) {
    x.answer->setNull();
    return;
  }
  const graph_lldsm* gllm = smart_cast <const graph_lldsm*>(llm);
  DCASSERT(gllm);
  stateset* ss = gllm->getPotential(pass[1]);
  if (ss) {
    x.answer->setPtr(ss);
  } else {
    x.answer->setNull();
  }
}


// *****************************************************************
// *                           write_dot                           *
// *****************************************************************

class writedot_si : public msr_noengine {
public:
  writedot_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

writedot_si::writedot_si()
 : msr_noengine(Nothing, em->VOID, "write_dot", 2)
{
  SetFormal(1, em->STRING, "filename");
  SetDocumentation("Writes the graph representation of the model to a file named filename (or not at all if this is null), in the format of the dot graph visualization tool.  This is done upon instantiation of the model.  The file is not overwritten if it already exists.");
}

void writedot_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  model_def* mdl = smart_cast<model_def*> (pass[0]);
  DCASSERT(mdl);

  if (x.stopExecution())  return;
  result* answer = x.answer;
  result foo;
  x.answer = &foo;

  SafeCompute(pass[1], x);
  mdl->SetDotFile(foo);

  x.answer = answer;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_basicmsrs : public initializer {
  public:
    init_basicmsrs();
    virtual bool execute();
};
init_basicmsrs the_basicmsr_initializer;

init_basicmsrs::init_basicmsrs() : initializer("init_basicmsrs")
{
  usesResource("em");
  usesResource("procgen");
  usesResource("stringtype");
  usesResource("biginttype");
  buildsResource("CML");
}

bool init_basicmsrs::execute()
{
  if (0==em) return false;

  // Process or model properties
  CML.Append(new numstates_si);
  CML.Append(new numarcs_si);
  CML.Append(new numclasses_si);
  CML.Append(new numlevels_si);
  CML.Append(new numevents_si);
  CML.Append(new numvars_si);

  // Process or model display
  CML.Append(new showstates_si);
  CML.Append(new showarcs_si);
  CML.Append(new showproc_si);
  CML.Append(new showclasses_si);
  CML.Append(new showlevels_si);
  CML.Append(new showevents_si);
  CML.Append(new showvars_si);

  // Statesets
  CML.Append(new initial_si);
  CML.Append(new reachable_si);
  CML.Append(new potential_si);

  // Miscellaneous
  CML.Append(new writedot_si);

  // Junaid, Chuan, Ben: experiments on transforming variable orders.
  CML.Append(new var_order_transform);

  // Model Checking Competition
  CML.Append(new run_for_MCC_si);

  // Engine types
  proc_noengine::ProcGen = em->findEngineType("ProcessGeneration");
  return proc_noengine::ProcGen;
}


#if 0

// Variable Ordering Experiment
// using namespace std;

typedef size_t u64;
const double ALPHA = .5;
const double ALPHAM1 = 1.0 - ALPHA;

struct DoubleInt {
	double theDouble;
	int theInt;
};

struct OrderPair {
	int name;
	int item;
};

struct ARC {
	int source;
	int target;
	int cardinality;
};

struct MODEL {
	int numPlaces;
	int numTrans;
	int numArcs;
	int * placeInits;
	ARC * theArcs;
};

bool comparePair(DoubleInt a, DoubleInt b) {
	return (a.theDouble < b.theDouble);
}

bool comparePairName(OrderPair a, OrderPair b) {
	return (a.name < b.name);
}

bool comparePairItem(OrderPair a, OrderPair b) {
	return (a.item < b.item);
}

void randOrder(std::vector<OrderPair>& theOrder) {
	unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
	std::shuffle(theOrder.begin(), theOrder.end(), std::default_random_engine(seed));
	
	for (unsigned index = 0; index < theOrder.size(); index++) {
		theOrder[index].name = index;
	}
}

double getSpanTop(MODEL theModel, std::vector<OrderPair> theOrder) {
	int * eventMax = new int[theModel.numTrans + theModel.numPlaces];
	int * eventMin = new int[theModel.numTrans + theModel.numPlaces];
	for (int index = 0; index < (theModel.numTrans + theModel.numPlaces); index++) {
		eventMax[index] = 0;
		eventMin[index] = theModel.numPlaces;
	}	
	u64 tops = 0LL;
	u64 spans = 0LL;
	for (int index = 0; index < theModel.numArcs; index++) {
		int source = theModel.theArcs[index].source;
		int target = theModel.theArcs[index].target;
		if (source < target) {
			int currentPlace = theOrder[source].item;
			if (eventMax[target] < currentPlace) {
				tops += currentPlace - eventMax[target];
				spans += currentPlace - eventMax[target];
				eventMax[target] = currentPlace;
			}
			if (eventMin[target] > currentPlace) {
				spans += eventMin[target] - currentPlace;
				eventMin[target] = currentPlace;
			}
		} else {
			int currentPlace = theOrder[target].item;
			if (eventMax[source] < currentPlace) {
				tops += currentPlace - eventMax[source];
				spans += currentPlace - eventMax[source];
				eventMax[source] = currentPlace;
			}
			if (eventMin[source] > currentPlace) {
				spans += eventMin[source] - currentPlace;
				eventMin[source] = currentPlace;
			}
		}
	}
	delete [] eventMax;
	delete [] eventMin;
	double result = ALPHA * (double)spans + ALPHAM1 * (double)tops;
	return result;
}


u64 cogDiff(MODEL theModel, std::vector<OrderPair>& theOrder) {
	// find the center of gravity for every variable in the model
	int * counts = new int[theModel.numTrans + theModel.numPlaces];
	DoubleInt * totals = new DoubleInt[theModel.numTrans + theModel.numPlaces];
	for (int index = 0; index < (theModel.numTrans + theModel.numPlaces); index++) {
		if (index < theModel.numPlaces) {
			counts[index] = 1;//0;
			totals[index].theDouble = 0.0;//(double)theModel.numPlaces;//0.0;
		} else {
			counts[index] = 0;
			totals[index].theDouble = 0.0;
		}
		totals[index].theInt = index;	// "names" 0..numPlaces - 1 
	}	
	
	// for every arc, update the cog value
	for (int index = 0; index < theModel.numArcs; index++) {
		int source = theModel.theArcs[index].source;
		int target = theModel.theArcs[index].target;
		if (source < target) {
			// source is a place
			int currentPlace = theOrder[source].item;
			counts[target]++;
			totals[target].theDouble += (double)currentPlace;
		} else {
			// source is the transition
			int currentPlace = theOrder[target].item;
			counts[source]++;
			totals[source].theDouble += (double)currentPlace;
		}
	}
	// the cog value is the average (mean)
	for (int index = 0; index < theModel.numArcs; index++) {
		int source = theModel.theArcs[index].source;
		int target = theModel.theArcs[index].target;
		if (source < target) {
			counts[source] += counts[target];
			totals[source].theDouble += totals[target].theDouble;
		} else {
			counts[target] += counts[source];
			totals[target].theDouble += totals[source].theDouble;
		}
	}
	for (int index = 0; index < theModel.numPlaces; index++) {
		if (counts[index] != 0) {
			totals[index].theDouble /= (double)counts[index];
		} else {
			totals[index].theDouble = (double)totals[index].theInt;
		}
		totals[index].theInt = index;
	}	
	
	// sort by the cog values
	std::vector<DoubleInt> toBeSorted;
	toBeSorted.assign(totals, totals + theModel.numPlaces);
	stable_sort(toBeSorted.begin(), toBeSorted.end(), comparePair);
	
	// update the order, and calc a quick hash
	u64 base = theOrder.size();
	u64 result = 0LL;
	for (unsigned index = 0; index < theOrder.size(); index++) {
		theOrder[index].name = index;
		theOrder[index].item = toBeSorted[index].theInt;
		result *= base;
		result += theOrder[index].item;
	}
	
	delete [] totals;
	delete [] counts;
	
	return result;
}

u64 orderHash(std::vector<OrderPair> theOrder) {
	u64 base = theOrder.size();
	u64 result = 0LL;
	for (unsigned index = 0; index < theOrder.size(); index++) {
		result *= base;
		result += theOrder[index].item;
	}
	return result;
}

u64 orderHashInt(int * theOrder, int length) {
	u64 base = length;
	u64 result = 0LL;
	for (int index = 0; index < length; index++) {
		result *= base;
		result += theOrder[index];
	}
	return result;
}


void forceHalt(MODEL theModel, std::vector<OrderPair>& startOrder, int * resultOrder, int numIter) {
	std::vector<OrderPair> current (startOrder);
	double bestScore = (double)(theModel.numPlaces + 1) * (double)(theModel.numTrans + 1);
	const int maxCycle = 11;
	u64 * cycleCheck = new u64[maxCycle];
	for (int index = 0; index < maxCycle; index++) {
		cycleCheck[index] = 0;
	}
	int iter = 0;
	for (iter = 0; iter < numIter; iter++) {
		u64 theNew = cogDiff(theModel, current);
		int hash = theNew % maxCycle;
		if (theNew == cycleCheck[hash]) break;
		cycleCheck[hash] = theNew;
		
		double score = getSpanTop(theModel, current);
		if (score < bestScore) {
			bestScore = score;
			for (int index = 0; index < theModel.numPlaces; index++) {
				resultOrder[index] = current[index].item;
			}
		}
	}
	
	delete [] cycleCheck;
}

std::vector< std::vector<int> > generateVarOrders(MODEL theModel, int numOrders, int maxIter) {
  printf("In %s()\n", __func__);

	std::vector< std::vector<int> > result;
	int hashSize = numOrders * 23;
	u64 * foundCheck = new u64[hashSize];
	for (int index = 0; index < hashSize; index++) {
		foundCheck[index] = 0;
	}
	int * resultOrder = new int[theModel.numPlaces];
	int count = 0;
	while (count < numOrders) {
		std::vector<OrderPair> starter;
		for (int i = 0; i < theModel.numPlaces; i++) {
			OrderPair t;
			t.name = i;
			t.item = i;
			starter.push_back(t);
		}
		randOrder(starter);
		
		forceHalt(theModel, starter, resultOrder, maxIter);
		u64 newHash = orderHashInt(resultOrder, theModel.numPlaces);
		int hash = newHash % hashSize;
		if (foundCheck[hash] == 0) {
			foundCheck[hash] = newHash;
			std::vector<int> found;
      int max_found = 0;
			for (int index = 0; index < theModel.numPlaces; index++) {
        if (max_found < resultOrder[index] + 1)
          max_found = resultOrder[index] + 1;
				found.push_back(resultOrder[index] + 1);
			}
			result.push_back(found);

      std::vector<int> found_inverse(max_found + 1);
      found_inverse[0] = 0;
      for (unsigned index = 0; index < found.size(); index++) {
        found_inverse[found[index]] = index + 1;
			}
			result.push_back(found_inverse);

			//cout << "found an order (" << count << ") hash is " << newHash << " " << hash <<  endl;
			count++;
		}
	}
	
	delete [] resultOrder;
	delete [] foundCheck;
	return result;
}

MODEL translateModel(dsde_hlm& smartModel) {
	MODEL result;
	result.numPlaces = smartModel.getNumStateVars();
	result.numTrans = smartModel.getNumEvents();
	
	std::vector<ARC> vecArcs;
	
	for (int index = 0; index < result.numTrans; index++) {
		model_event * theEvent = smartModel.getEvent(index);
		int arcTarget = index + result.numPlaces;	// using target as the transition
		
		expr* enabling = theEvent->getEnabling();
		
		List <symbol> L;	// Using the magic "List" from SMART with O(1) random access?
		enabling->BuildSymbolList(traverse_data::GetSymbols, 0, &L);
		for (int i = 0; i < L.Length(); i++) {
			symbol* s = L.Item(i);
			model_statevar* mv = dynamic_cast <model_statevar*> (s);
			if (0 != mv) {
				ARC tempArc;
				tempArc.target = arcTarget;
				// tempArc.source = mv->GetIndex() - 1;	// - 1 due to local 0..n-1 convention vs 1..n
				tempArc.source = mv->GetIndex(); 
				vecArcs.push_back(tempArc);
			} 
		}
	}

	result.numArcs = vecArcs.size();
	result.theArcs = new ARC[vecArcs.size()];
	for (unsigned index = 0; index < vecArcs.size(); index++) {
		result.theArcs[index] = vecArcs[index];

    printf("p: %d, t: %d\n", vecArcs[index].source, vecArcs[index].target);
	}

  printf("In translateModel()\n");
	
	return result;
}

std::vector< std::vector<int> > getNorders(dsde_hlm& smartModel, int numOrders, int maxIter) {
  MODEL theModel = translateModel(smartModel);
  std::vector< std::vector<int> > result = generateVarOrders(theModel, numOrders, maxIter);
  delete [] theModel.theArcs;
  return result;
}
#endif
