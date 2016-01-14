
// $Id$

#include "../ExprLib/engine.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/measures.h"

#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/dsde_hlm.h"

#include "../Modules/biginttype.h"
#include "../Modules/statesets.h"

#include "basic_msr.h"

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
  const hldsm* hlm = mi ? mi->GetCompiledModel() : 0;
  
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
  SetDocumentation("Display the underlying process (reachability graph, Markov chain, etc.) to the current output stream.  The process will be constructed first, if necessary.  If parameter `internal' is true, then the internal representation of the process is displayed; otherwise, a storage-independent enumeration of the process is displayed (unless it is too large).");
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
  const hldsm* hlm = mi ? mi->GetCompiledModel() : 0;
  
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
  SetDocumentation("Returns the set of model states satisfying p.  Note that this set could contain states thate are not reachable from the initial state(s) of the model.");
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
// *                           front  end                           *
// *                                                                *
// ******************************************************************

void InitBasicMeasureFuncs(exprman* em, List <msr_func> *common)
{
  if (0==common)  return;

  // Process or model properties
  common->Append(new numstates_si);
  common->Append(new numarcs_si);
  common->Append(new numclasses_si);
  common->Append(new numlevels_si);
  common->Append(new numevents_si);
  common->Append(new numvars_si);

  // Process or model display
  common->Append(new showstates_si);
  common->Append(new showarcs_si);
  common->Append(new showclasses_si);
  common->Append(new showlevels_si);
  common->Append(new showevents_si);
  common->Append(new showvars_si);

  // Statesets
  common->Append(new initial_si);
  common->Append(new reachable_si);
  common->Append(new potential_si);

  // Miscellaneous
  common->Append(new writedot_si);

  // Engine types
  proc_noengine::ProcGen = em->findEngineType("ProcessGeneration");
}


