
#include "rss_enum.h"
#include "rgr_grlib.h"
#include "enum_hlm.h"

#include "../Options/options.h"

#include "../Utils/library.h"
#include "../Utils/init_opts.h"
#include "../Utils/splay.h"

#include "../ExprLib/formalism.h"
#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"

// Explicit libraries
#include "../_GraphLib/graphlib.h"
#include "../_LSLib/lslib.h"

// **************************************************************************

class fsm_index_visitor : public shared_visitor {
        long* indexes;
        unsigned size;
        unsigned i;
    public:
        fsm_index_visitor(long* ndx, unsigned n) {
            indexes = ndx;
            size = n;
            i = 0;
        }
        virtual void visit(const shared_object* item) {
            if (i>=size) return;
            const model_enum_value* st
                = dynamic_cast <const model_enum_value*> (item);
            DCASSERT(st);
            indexes[i++] = st->GetIndex();
        }
};

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

  splayOfShared *initial;   // model_enum_value

  GraphLib::dynamic_digraph* mygr;

  bool error;

  static debugging_msg fsm_debug;
  static warning_msg dup_init;
  static warning_msg no_init;
  static warning_msg dup_arc;
  friend class init_fsms;
public:
  fsm_def(const location &W, const type* t, char*n,
      formal_param **pl, int np);

  virtual ~fsm_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  // For model construction:
  void AddInitial(const expr* c, model_enum_value* st);

  void AddEdge(const expr* c, model_enum_value* from, model_enum_value* to);

protected:
  virtual void InitModel();
  virtual void FinalizeModel(outputStream &ds);

};

debugging_msg fsm_def::fsm_debug;
warning_msg fsm_def::dup_init;
warning_msg fsm_def::no_init;
warning_msg fsm_def::dup_arc;

// ******************************************************************
// *                        fsm_def  methods                        *
// ******************************************************************

fsm_def::fsm_def(const location &W, const type* t,
   char*n, formal_param **pl, int np) : model_def(W, t, n, pl, np)
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
    model_def::errmsg E(this, wrap);
    E << e.getString() << " when adding state " << wrap->Name();
    error = true;
    return 0;
  }

  // Build a state in the frontend FSM
  model_var* s = new model_enum_value(wrap, current, state_count);
  state_count++;

  if (fsm_debug.start()) {
    fsm_debug << "adding state " << s->Name() << "\n";
    fsm_debug.stop();
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

  model_enum_value* find = dynamic_cast <model_enum_value*> (initial->insert(foo));
  if (find != foo) {
    if (StartWarning(dup_init, cause)) {
      dup_init << "Ignoring duplicate initialization of state ";
      dup_init << foo->Name();
      DoneWarning(dup_init);
    }
    return;
  }

  if (fsm_debug.start()) {
    fsm_debug << "adding " << foo->Name() << " to initial set\n";
    fsm_debug.stop();
  }

}

void fsm_def::AddEdge(const expr* c, model_enum_value* f, model_enum_value* t)
{
  if (error) return;
  if (fsm_debug.start()) {
    fsm_debug << "adding edge ";
    fsm_debug << f->Name() << " : " << t->Name() << "\n";
    fsm_debug.stop();
  }
  DCASSERT(mygr);
  if (!isVariableOurs(f, c, "ignoring arc")) return;
  if (!isVariableOurs(t, c, "ignoring arc")) return;

  try {
    bool dup = mygr->addEdge(f->GetIndex(), t->GetIndex());
    if (dup && StartWarning(dup_arc, c)) {
      dup_arc << "Ignoring duplicate arc from state ";
      dup_arc << f->Name() << " to " << t->Name();
      DoneWarning(dup_arc);
    }
  }
  catch (GraphLib::error e) {
    model_def::errmsg E(this, c);
    E << e.getString() << " when adding edge from " << f->Name();
    E << " to " << t->Name();
    error = true;
  }
}

void fsm_def::InitModel()
{
  statelist = 0;
  state_count = 0;
  DCASSERT(0==mygr);
  mygr = new GraphLib::dynamic_digraph(true);
  DCASSERT(mygr);
  DCASSERT(0==initial);
  initial = new splayOfShared(16, 0);
  error = false;
}

void fsm_def::FinalizeModel(outputStream &ds)
{
    model_enum* mcstate = new model_enum(0, current, statelist);
    statelist = nullptr;
    state_count = 0;

    if (error) {
        Delete(mcstate);
        delete mygr;
        mygr = 0;
        ConstructionError();
        return;
    }

    LS_Vector init;
    init.f_value = nullptr;
    init.d_value = nullptr;
    init.size = initial->numElements();
    if (init.size) {
        long* ndx = new long[init.size];
        init.index = ndx;
        fsm_index_visitor v(ndx, init.size);
        initial->traverse(v);
    } else {
        init.index = nullptr;
        if (StartWarning(no_init)) {
            no_init << "Empty set of initial states";
            DoneWarning(no_init);
        }
    }
    delete initial;
    initial = nullptr;

    enum_reachset* rss = new enum_reachset(mcstate);
    grlib_reachgraph* rgr = new grlib_reachgraph(mygr);
    rgr->setInitial(init);
    graph_lldsm* foo = new graph_lldsm(lldsm::FSM);
    foo->setRSS(rss);
    foo->setRGR(rgr);
    hldsm* bar = MakeEnumeratedModel(foo);
    foo->dumpDot(ds);
    ConstructionSuccess(bar);
    mygr = nullptr;
}


// **************************************************************************
// *                                                                        *
// *                          fsm_formalism  class                          *
// *                                                                        *
// **************************************************************************

class fsm_formalism : public formalism {
public:
  fsm_formalism(const char* n, const char* sd, const char* ld);

  virtual model_def* makeNewModel(const location &W, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
};

// ******************************************************************
// *                     fsm_formalism  methods                     *
// ******************************************************************


fsm_formalism
::fsm_formalism(const char* n, const char* sd, const char* ld)
 : formalism(n, sd, ld)
{
    includeCTL();
}

model_def* fsm_formalism::makeNewModel(const location &W, char* name,
          symbol** formals, int np) const
{
  // TBD: check formals?
  return new fsm_def(W, this, name, (formal_param**) formals, np);
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

fsm_init::fsm_init() : model_internal(type::find("void"), "init", 2)
{
  SetFormal(1, type::find(true, false, DETERM, "state"), "s");
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

fsm_arcs::fsm_arcs() : model_internal(type::find("void"), "arcs", 2)
{
  typelist* tl = new typelist(2);
  const type* state = type::find("state");
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
    x.aggregate = 1;
    x.answer = &from;
    pass[i]->Compute(x);
    x.answer = &to;
    x.aggregate = 2;
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

fsm_instate::fsm_instate() : model_internal(type::find(false, true, DETERM, "bool"), "in_state", 2)
{
  const type* state = type::find("state");
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
 : model_internal(type::find(false, true, DETERM, "bool"), "is_absorbed", 1)
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
 : model_internal(type::find(false, true, DETERM, "bool"), "is_deadlocked", 1)
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
  const grlib_reachgraph* RG = smart_cast <const grlib_reachgraph*> (cruft->getRGR());
  DCASSERT(RG);

  x.answer->setBool(RG->isDeadlocked(x.current_state_index));
}

// ******************************************************************
// *                                                                *
// *                         fsm_lib  class                         *
// *                                                                *
// ******************************************************************

class fsm_lib : public library {
public:
  fsm_lib();
  virtual void printVersion(std::ostream &s) const;
};
static fsm_lib the_fsm_lib;

fsm_lib::fsm_lib() : library(false, false)
{
    registerLibrary(this);
}

void fsm_lib::printVersion(std::ostream &s) const
{
    s << GraphLib::Version();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_fsms : public initializer {
    public:
        init_fsms();
    protected:
        virtual void execute();
};
static init_fsms the_fsm_initializer;

init_fsms::init_fsms() : initializer(__FILE__, 4)
{
    builds_resource("fsm");
    needs_resource("Warning");
    needs_resource("Debug");
    needs_resource("CML");
}

void init_fsms::execute()
{
    //
    // Types
    //
    simple_type* t_state = type::registerNew(new void_type("state", "Discrete state", "State of a model (finite state machine or Markov chain)"));
    type::allowSetsOf(t_state);

    //
    // Register formalism
    //
    formalism* fsm = new fsm_formalism(
        "fsm",
        "Finite state machine",
        "The finite state machine formalism fsm allows for direct specification of a finite state machine. States of the finite state machine are declared, and transitions between states are specified \"by hand\"."
    );
    if (type::registerNew(fsm) != fsm) {
        internal_error E(__FILE__, __LINE__);
        E << "fsm type already exists?";
        return;
    }

    //
    // Add symbols to formalism, and finish it
    //
    fsm->addSymbol( new fsm_init       );
    fsm->addSymbol( new fsm_arcs       );
    fsm->addSymbol( new fsm_instate    );
    fsm->addSymbol( new fsm_absorbing  );
    fsm->addSymbol( new fsm_deadlocked );
    fsm->finish();

    //
    // Set up options
    //
    initialize_msg(fsm_def::fsm_debug,
        "fsms",
        "When set, diagnostic messages are displayed regarding FSM model construction.",
        get_object("Debug")
    );

    initialize_msg(fsm_def::dup_init,
        "fsm_dup_init",
        "For duplicatation of initial states in finite state machine models",
        get_object("Warning")
    );
    initialize_msg(fsm_def::no_init,
        "fsm_no_init",
        "For absence of initial states in finite state machine models",
        get_object("Warning")
    );
    initialize_msg(fsm_def::dup_arc,
        "fsm_dup_arc",
        "For duplicate arcs in finite state machine models",
        get_object("Warning")
    );
}

