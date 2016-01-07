
// $Id$

#include "fsm_form.h"
#include "rss_enum.h"
#include "rgr_expl.h"
#include "enum_hlm.h"
#include "graph_llm.h"

#include "../ExprLib/exprman.h"
#include "../ExprLib/formalism.h"

#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"

#include "../include/splay.h"

// Explicit libraries
#include "graphlib.h"
#include "lslib.h"


// **************************************************************************
// *                                                                        *
// *                             fsm_def  class                             *
// *                                                                        *
// **************************************************************************

/** Smart support for the finite state machine "formalism".
    I.e., front-end stuff for FSM formalism.
*/
class fsm_def : public model_def {
  symbol* statelist;
  int state_count;

  SplayOfPointers <model_enum_value> *initial;

  GraphLib::digraph* mygr;

  bool error;

  static named_msg fsm_debug;
  static named_msg dup_init;
  static named_msg no_init;
  static named_msg dup_arc;
  friend void InitializeFSMs(exprman* em, List <msr_func> *);
public:
  fsm_def(const char* fn, int line, const type* t, char*n, 
      formal_param **pl, int np);

  virtual ~fsm_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  // For model construction:
  void AddInitial(const expr* c, model_enum_value* st);

  void AddEdge(const expr* c, model_enum_value* from, model_enum_value* to);

protected:
  virtual void InitModel();
  virtual void FinalizeModel(OutputStream &ds);

};

named_msg fsm_def::fsm_debug;
named_msg fsm_def::dup_init;
named_msg fsm_def::no_init;
named_msg fsm_def::dup_arc;

// ******************************************************************
// *                        fsm_def  methods                        *
// ******************************************************************

fsm_def::fsm_def(const char* fn, int line, const type* t,
   char*n, formal_param **pl, int np) : model_def(fn, line, t, n, pl, np)
{
  statelist = 0; 
  state_count = 0;
  mygr = 0;
  initial = 0;
  error = 0;
}

fsm_def::~fsm_def()
{
  // traverse and delete statelist here, or not?
}

model_var* fsm_def::MakeModelVar(const symbol* wrap, shared_object* bnds)
{
  if (error) return 0;
  DCASSERT(wrap);
  DCASSERT(0==bnds);
  DCASSERT(mygr);

  // Add state to the backend FSM
  try {
    mygr->addNode();
  }
  catch (GraphLib::error e) {
    if (StartError(wrap)) {
      em->cerr() << e.getString() << " when adding state " << wrap->Name();
      DoneError();
    }
    error = true;
    return 0;
  }

  // Build a state in the frontend FSM
  model_var* s = new model_enum_value(wrap, current, state_count);
  state_count++;

  if (fsm_debug.startReport()) {
    fsm_debug.report() << "adding state " << s->Name() << "\n";
    fsm_debug.stopIO();
  }
  
  // add to statelist (reverse order)
  s->LinkTo(statelist);
  statelist = s;
  return s;
}

void fsm_def::AddInitial(const expr* cause, model_enum_value* foo)
{
  if (error) return;
  DCASSERT(initial);
  if (!isVariableOurs(foo, cause, "ignoring as initial state")) return;

  model_enum_value* find = initial->Insert(foo);
  if (find != foo) {
    if (StartWarning(dup_init, cause)) {
      em->warn() << "Ignoring duplicate initialization of state ";
      em->warn() << foo->Name();
      DoneWarning();
    }
    return;
  }

  if (fsm_debug.startReport()) {
    fsm_debug.report() << "adding " << foo->Name() << " to initial set\n";
    fsm_debug.stopIO();
  }
  
}

void fsm_def::AddEdge(const expr* c, model_enum_value* f, model_enum_value* t)
{
  if (error) return;
  if (fsm_debug.startReport()) {
    fsm_debug.report() << "adding edge ";
    fsm_debug.report() << f->Name() << " : " << t->Name() << "\n";
    fsm_debug.stopIO();
  }
  DCASSERT(mygr);
  if (!isVariableOurs(f, c, "ignoring arc")) return;
  if (!isVariableOurs(t, c, "ignoring arc")) return;

  try {
    bool dup = mygr->addEdge(f->GetIndex(), t->GetIndex());
    if (dup && StartWarning(dup_arc, c)) {
      em->warn() << "Ignoring duplicate arc from state ";
      em->warn() << f->Name() << " to " << t->Name();
      DoneWarning();
    }
  }
  catch (GraphLib::error e) {
    if (StartError(c)) {
      em->cerr() << e.getString() << " when adding edge from " << f->Name();
      em->cerr() << " to " << t->Name();
      DoneError();
    }
    error = true;
  }
}

void fsm_def::InitModel()
{
  statelist = 0; 
  state_count = 0;
  DCASSERT(0==mygr);
  // mygr = startDirectedGraph(0);
  mygr = new GraphLib::digraph(true);
  DCASSERT(mygr);
  DCASSERT(0==initial);
  initial = new SplayOfPointers <model_enum_value> (16, 0);
  error = false;
}

void fsm_def::FinalizeModel(OutputStream &ds)
{
  model_enum* mcstate = new model_enum(0, current, statelist);
  statelist = 0;
  state_count = 0;

  if (error) {
    Delete(mcstate);
    delete mygr;
    mygr = 0;
    ConstructionError();
    return;
  }

  GraphLib::digraph::finish_options fo;
  fo.Store_By_Rows = false;  // need option?
  fo.Will_Clear = false;
  try {
    if (!error) mygr->finish(fo);
  }
  catch (GraphLib::error e) {
    if (StartError(0)) {
      em->cerr() << e.getString() << " when finalizing finite state machine";
      DoneError();
    }
    error = true;
  }

  LS_Vector init;
  init.f_value = 0;
  init.d_value = 0;
  init.size = initial->NumElements();
  if (init.size) {
    model_enum_value** init_data = new model_enum_value*[init.size];
    initial->CopyToArray(init_data);
    long* foo = new long[init.size];
    for (long i=0; i<init.size; i++) {
      DCASSERT(init_data[i]);
      foo[i] = init_data[i]->GetIndex();
    }
    init.index = foo;
    delete[] init_data;
  } else {
    init.index = 0;
    if (StartWarning(no_init, 0)) {
      em->warn() << "Empty set of initial states";
      DoneWarning();
    }
  }
  delete initial;
  initial = 0;

// #ifdef NEW_STATESETS
  enum_reachset* rss = new enum_reachset(mcstate);
  rss->setInitial(init);
  expl_reachgraph* rgr = new expl_reachgraph(mygr);
  graph_lldsm* foo = new graph_lldsm(lldsm::FSM);
  foo->setRSS(rss);
  foo->setRGR(rgr);
// #else
  // graph_lldsm* foo = MakeEnumeratedFSM(init, mcstate, mygr);
// #endif
  hldsm* bar = MakeEnumeratedModel(foo);
  if (ds.IsActive()) foo->dumpDot(ds);
  ConstructionSuccess(bar);
  mygr = 0;
}


// **************************************************************************
// *                                                                        *
// *                          fsm_formalism  class                          *
// *                                                                        *
// **************************************************************************

class fsm_formalism : public formalism {
public:
  fsm_formalism(const char* n, const char* sd, const char* ld);

  virtual model_def* makeNewModel(const char* fn, int ln, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
  virtual bool includeCTL() const { return true; }
};

// ******************************************************************
// *                     fsm_formalism  methods                     *
// ******************************************************************


fsm_formalism
::fsm_formalism(const char* n, const char* sd, const char* ld)
 : formalism(n, sd, ld)
{
}

model_def* fsm_formalism::makeNewModel(const char* fn, int ln, char* name, 
          symbol** formals, int np) const
{
  // TBD: check formals?
  return new fsm_def(fn, ln, this, name, (formal_param**) formals, np);
}

bool fsm_formalism::canDeclareType(const type* vartype) const
{
  if (0==vartype)                 return 0;
  if (vartype->matches("state"))  return 1;
  return 0;
}

bool fsm_formalism::canAssignType(const type* vartype) const
{
  return 0;
}

// **************************************************************************
// *                                                                        *
// *                     Finite State Machine Functions                     *
// *                                                                        *
// **************************************************************************

// **************************************************************************
// *                             fsm_init class                             *
// **************************************************************************

class fsm_init : public model_internal {
public:
  fsm_init();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

fsm_init::fsm_init() : model_internal(em->VOID, "init", 2)
{
  SetFormal(1, em->findType("{state}"), "s");
  SetRepeat(1);
  SetDocumentation("Sets the initial state(s) for a finite state machine model.");
}

void fsm_init::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  fsm_def* mdl = smart_cast<fsm_def*>(pass[0]);
  DCASSERT(mdl);

  if (x.stopExecution())  return;
  result* answer = x.answer;
  result states;
  x.answer = &states;
  for (int i=1; i<np; i++) {
    if (0==pass[i])  continue;
    pass[i]->Compute(x);
    if (!states.isNormal())  continue;
    DCASSERT(states.getPtr());
    shared_set* ss = smart_cast<shared_set*> (states.getPtr());
    DCASSERT(ss);
    // enumerate the set
    for (long z=0; z<ss->Size(); z++) {
      result elem;
      ss->GetElement(z, elem);
      DCASSERT(elem.isNormal());
      model_enum_value* st = smart_cast<model_enum_value*> (elem.getPtr());
      DCASSERT(st);
      mdl->AddInitial(pass[i], st);
    }
  } // for i
  x.answer = answer;
}


// **************************************************************************
// *                             fsm_arcs class                             *
// **************************************************************************

class fsm_arcs : public model_internal {
public:
  fsm_arcs();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

fsm_arcs::fsm_arcs() : model_internal(em->VOID, "arcs", 2)
{
  typelist* tl = new typelist(2);
  const type* state = em->findType("state");
  tl->SetItem(0, state);
  tl->SetItem(1, state);
  SetFormal(1, tl, "from:to");
  SetRepeat(1);
  SetDocumentation("Adds a set of arcs to the finite state machine.");
}

void fsm_arcs::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  fsm_def* mdl = smart_cast<fsm_def*>(pass[0]);
  DCASSERT(mdl);
  
  if (x.stopExecution())  return;
  result* answer = x.answer;
  result from;
  result to;
  for (int i=1; i<np; i++) {
    if (0==pass[i])  continue;
    x.aggregate = 0;
    x.answer = &from;
    pass[i]->Compute(x);
    x.answer = &to;
    x.aggregate = 1;
    pass[i]->Compute(x);

    // TBD: check state for errors!
    if (!from.isNormal())  continue;  // error message?
    if (!to.isNormal())    continue;

    DCASSERT(from.getPtr());
    model_enum_value* frst = smart_cast<model_enum_value*> (from.getPtr());
    DCASSERT(frst);
    model_enum_value* tost = smart_cast<model_enum_value*> (to.getPtr());
    DCASSERT(tost);

    mdl->AddEdge(pass[i], frst, tost);
  } // for i
  x.answer = answer;
  x.aggregate = 0;
}

// **************************************************************************
// *                           fsm_instate  class                           *
// **************************************************************************

class fsm_instate : public model_internal {
public:
  fsm_instate();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

fsm_instate::fsm_instate() : model_internal(em->BOOL->addProc(), "in_state", 2)
{
  const type* state = em->findType("state");
  SetFormal(1, state->getSetOfThis(), "sset");
  SetDocumentation("Returns true iff the finite state machine is in one of the specified states.");
}

void fsm_instate::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(x.current_state);

  model_instance* mi = grabModelInstance(x, pass[0]);
  DCASSERT(mi);
  hldsm* foo = mi->GetCompiledModel();
  DCASSERT(foo);
  const state_lldsm* bar = dynamic_cast <const state_lldsm*> (foo->GetProcess());
  DCASSERT(bar);

  const enum_reachset* rss = dynamic_cast <const enum_reachset*> (bar->getRSS());
  DCASSERT(rss);

  result current(rss->getEnumeratedState(x.current_state_index));

  SafeCompute(pass[1], x);
  DCASSERT(x.answer->isNormal());
  shared_set* ss = smart_cast<shared_set*> (x.answer->getPtr());
  DCASSERT(ss);

  x.answer->setBool(ss->IndexOf(current) >= 0);
}

// **************************************************************************
// *                          fsm_absorbing  class                          *
// **************************************************************************

class fsm_absorbing : public model_internal {
public:
  fsm_absorbing();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

fsm_absorbing::fsm_absorbing() 
 : model_internal(em->BOOL->addProc(), "is_absorbed", 1)
{
  SetDocumentation("Returns true iff the finite state machine is in an absorbing state (this includes deadlocked states).");
}

void fsm_absorbing::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(1==np);
  DCASSERT(x.current_state);

  model_instance* mi = grabModelInstance(x, pass[0]);
  DCASSERT(mi);
  hldsm* foo = mi->GetCompiledModel();
  DCASSERT(foo);
  const lldsm* bar = foo->GetProcess();
  DCASSERT(bar);
  const graph_lldsm* cruft = smart_cast <const graph_lldsm*>(bar);
  DCASSERT(cruft);

  x.answer->setBool(cruft->isAbsorbing(x.current_state_index));
}

// **************************************************************************
// *                          fsm_deadlocked class                          *
// **************************************************************************

class fsm_deadlocked : public model_internal {
public:
  fsm_deadlocked();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

fsm_deadlocked::fsm_deadlocked() 
 : model_internal(em->BOOL->addProc(), "is_deadlocked", 1)
{
  SetDocumentation("Returns true iff the finite state machine is in a deadlocked state (no outgoing edges).");
}

void fsm_deadlocked::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(1==np);
  DCASSERT(x.current_state);

  model_instance* mi = grabModelInstance(x, pass[0]);
  DCASSERT(mi);
  hldsm* foo = mi->GetCompiledModel();
  DCASSERT(foo);
  const lldsm* bar = foo->GetProcess();
  DCASSERT(bar);
  const graph_lldsm* cruft = smart_cast <const graph_lldsm*>(bar);
  DCASSERT(cruft);
  
  x.answer->setBool(cruft->isDeadlocked(x.current_state_index));
}

// ******************************************************************
// *                                                                *
// *                         fsm_lib  class                         *
// *                                                                *
// ******************************************************************

class fsm_lib : public library {
public:
  fsm_lib() : library(false) { }
  virtual const char* getVersionString() const {
    return GraphLib::Version();
  }
  virtual bool hasFixedPointer() const { 
    return true; 
  }

  static void Init(exprman* em);
};

void fsm_lib::Init(exprman* em)
{
  static fsm_lib* fsml = 0;
 
  if (0==fsml) {
    fsml = new fsm_lib;
    em->registerLibrary(fsml);
  }
}


// **************************************************************************
// *                                                                        *
// *                               Front  end                               *
// *                                                                        *
// **************************************************************************

void InitializeFSMs(exprman* em, List <msr_func> *common)
{
  bool ok;
  // Set up options
  option* debug = em->findOption("Debug");
  fsm_def::fsm_debug.Initialize(debug,
    "fsms",
    "When set, diagnostic messages are displayed regarding FSM model construction.",
    false
  );

  option* warning = em->findOption("Warning");
  fsm_def::dup_init.Initialize(warning,
    "fsm_dup_init",
    "For duplicatation of initial states in finite state machine models",
    true
  );
  fsm_def::no_init.Initialize(warning,
    "fsm_no_init",
    "For absence of initial states in finite state machine models",
    true
  );
  fsm_def::dup_arc.Initialize(warning,
    "fsm_dup_arc",
    "For duplicate arcs in finite state machine models",
    true
  );

  // Set up and register formalisms
  const char* longdocs = "The finite state machine formalism fsm allows for direct specification of a finite state machine. States of the finite state machine are declared, and transitions between states are specified \"by hand\".";

  formalism* fsm = new fsm_formalism("fsm", "Finite state machine", longdocs);
  ok = em->registerType(fsm);
  if (!ok) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Couldn't register fsm type";
      em->stopIO();
    }
    return;
  }

  // set up and register state type, if necessary
  if (!em->findType("state")) {
    DCASSERT(!em->findType("{state}"));
    simple_type* t_state = new void_type("state", "Discrete state", "State of a model (finite state machine or Markov chain)");
    em->registerType(t_state);
    type* t_set_state = newSetType("{state}", t_state);
    em->registerType(t_set_state);
  }
  DCASSERT(em->findType("state"));
  DCASSERT(em->findType("{state}"));

  // Grab functions into a symbol table
  symbol_table* mcsyms = MakeSymbolTable();
  mcsyms->AddSymbol( new fsm_init       );
  mcsyms->AddSymbol( new fsm_arcs       );
  mcsyms->AddSymbol( new fsm_instate    );
  mcsyms->AddSymbol( new fsm_absorbing  );
  mcsyms->AddSymbol( new fsm_deadlocked );

  // Set the symbol table
  fsm->setFunctions(mcsyms);
  fsm->addCommonFuncs(common);

  // register libs
  fsm_lib::Init(em);
}

