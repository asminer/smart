
#include "rss_enum.h"
#include "proc_mclib.h"
#include "enum_hlm.h"

#include "../Options/options.h"

#include "../Utils/init_opts.h"
#include "../Utils/splay.h"

#include "../ExprLib/formalism.h"
#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"

#include "../Formlsms/phase_hlm.h"

// Explicit Markov chain library
#include "../_MCLib/mclib.h"
#include "../_LSLib/lslib.h"


// **************************************************************************
// *                                                                        *
// *                           state_weight class                           *
// *                                                                        *
// **************************************************************************

class state_weight : public shared_object {
    public:
        const model_enum_value* state;
        double weight;

        state_weight(const model_enum_value* s, double w) {
            state = s;
            weight = w;
        }

        virtual bool Print(std::ostream &s, int w) const {
            return false;
        }
        virtual int Compare(const shared_object* o) const {
            const model_enum_value* s = dynamic_cast <const model_enum_value*> (o);
            if (s) {
                return state->GetIndex() - s->GetIndex();
            }
            const state_weight* sw = dynamic_cast <const state_weight*> (o);
            if (sw) {
                return state->GetIndex() - sw->state->GetIndex();
            }
            DCASSERT(0);
        }

        class visitor : public shared_visitor {
                long* indexes;
                float* weights;
                double  total;
                unsigned size;
                unsigned i;
                bool first_pass;
            public:
                visitor(long* ndx, float* w, unsigned n) {
                    indexes = ndx;
                    weights = w;
                    size = n;
                    total = 0.0;
                    i = 0;
                    first_pass = true;
                }
                virtual void visit(shared_object* item) {
                    if (i>=size) return;
                    state_weight *s = dynamic_cast <state_weight*> (item);
                    DCASSERT(s);
                    if (first_pass) {
                        total += s->weight;
                    } else {
                        weights[i] = s->weight / total;
                        indexes[i] = s->state->GetIndex();
                    }
                    ++i;
                }
                inline void secondPass() {
                    first_pass = false;
                    i=0;
                }
        };
};

// **************************************************************************
// *                                                                        *
// *                            markov_def class                            *
// *                                                                        *
// **************************************************************************

/** Smart support for the Markov chain "formalism".
    I.e., front-end stuff for Markov chain formalism.
*/
class markov_def : public model_def {
  symbol* statelist;
  int state_count;

  splayOfShared *initial;

  GraphLib::dynamic_summable<double>* mymc;
  // Old_MCLib::Markov_chain* mymc;

  bool error;
  bool discrete;

  static debugging_msg mc_debug;
  static warning_msg dup_init;
  static warning_msg no_init;
  static warning_msg dup_arc;
  friend class init_mcform;
public:
  markov_def(const location &W, const type* t, bool d, char*n,
      formal_param **pl, int np);

  virtual ~markov_def();

  // Required for models:
  virtual model_var* MakeModelVar(const symbol* wrap, shared_object* bnds);

  // For model construction:
  void AddInitial(const expr* cause, model_enum_value* st, double weight);

  void AddEdge(const expr* cause,
    model_enum_value* from, model_enum_value* to, double weight);

  inline bool isDiscrete() const { return discrete; }
protected:
  virtual void InitModel();
  virtual void FinalizeModel(outputStream &ds);

};

debugging_msg markov_def::mc_debug;
warning_msg markov_def::dup_init;
warning_msg markov_def::no_init;
warning_msg markov_def::dup_arc;

// ******************************************************************
// *                       markov_def methods                       *
// ******************************************************************

markov_def::markov_def(const location &W, const type* t, bool d,
   char*n, formal_param **pl, int np) : model_def(W, t, n, pl, np)
{
  statelist = 0;
  state_count = 0;
  mymc = 0;
  initial = 0;
  discrete = d;
  error = 0;
}

markov_def::~markov_def()
{
  // traverse and delete statelist here, or not?
}

model_var* markov_def::MakeModelVar(const symbol* wrap, shared_object* bnds)
{
  if (error) return 0;
  DCASSERT(wrap);
  DCASSERT(0==bnds);
  DCASSERT(mymc);

  // Add state to the backend MC
  try {
    mymc->addNode();
  }
  catch (GraphLib::error e) {
    model_def::errmsg E(this, wrap);
    E << e.getString() << " when adding state " << wrap->Name();
    error = true;
    return 0;
  }

  // Build a state in the frontend MC
  model_var* s = new model_enum_value(wrap, current, state_count);
  state_count++;

  // add to statelist (reverse order)
  s->LinkTo(statelist);
  statelist = s;
  return s;
}

void markov_def::AddInitial(const expr* cause,
      model_enum_value* foo, double weight)
{
  if (error) return;
  DCASSERT(initial);
  if (!isVariableOurs(foo, cause, "ignoring initial weight")) return;

  shared_object* find = initial->find(foo);
  if (find) {
    if (StartWarning(dup_init, cause)) {
      dup_init << "Ignoring duplicate initial probability for state ";
      dup_init << foo->Name();
      DoneWarning(dup_init);
    }
    return;
  }
  state_weight* sw = new state_weight(foo, weight);
  initial->insert(sw);

  if (mc_debug.start()) {
    mc_debug << "adding state " << foo->Name();
    mc_debug << " to initial set with weight " << weight << "\n";
    mc_debug.stop();
  }
}

void markov_def::AddEdge(const expr* cause,
      model_enum_value* f, model_enum_value* t, double wt)
{
  if (error) return;
  DCASSERT(mymc);
  if (!isVariableOurs(f, cause, "ignoring arc")) return;
  if (!isVariableOurs(t, cause, "ignoring arc")) return;

  try {
    bool dup = mymc->addEdge(f->GetIndex(), t->GetIndex(), wt);
    if (dup && StartWarning(dup_arc, cause)) {
      dup_arc << "Summing duplicate arc from state ";
      dup_arc << f->Name() << " to " << t->Name();
      DoneWarning(dup_arc);
    }
  }
  catch (GraphLib::error e) {
    model_def::errmsg E(this, cause);
    E << e.getString() << " when adding edge from ";
    E << f->Name() << " to " << t->Name();
    error = true;
  }
}

void markov_def::InitModel()
{
  statelist = 0;
  state_count = 0;
  DCASSERT(0==mymc);
  mymc = new GraphLib::dynamic_summable<double> (isDiscrete(), true);
  DCASSERT(mymc);
  DCASSERT(0==initial);
  initial = new splayOfShared (16, 0);
  error = false;
}

void markov_def::FinalizeModel(outputStream &ds)
{
    model_enum* mcstate = new model_enum(0, current, statelist);
    statelist = nullptr;
    state_count = 0;

    if (error) {
        Delete(mcstate);
        delete mymc;
        mymc = nullptr;
        ConstructionError();
        return;
    }

    DCASSERT(mcstate);

    //
    // build initial distribution
    //
    unsigned size = initial->numElements();
    long* indexes = size ? new long[size] : nullptr;
    float* probs = size ? new float[size] : nullptr;
    state_weight::visitor v(indexes, probs, size);
    initial->traverse(v);   // calculate total weights
    v.secondPass();
    initial->traverse(v);   // get probabilities

    //
    // Copy into format for linear solvers
    //
    LS_Vector init;
    init.size = size;
    init.index = indexes;
    init.f_value = probs;
    init.d_value = nullptr;


    //
    // Build reachable states
    //
    enum_reachset* rss = new enum_reachset(mcstate);

    //
    // Build process
    //
    mclib_process* proc = new mclib_process(isDiscrete(), mymc);

    //
    // Package everything
    //
    stochastic_lldsm* foo = new stochastic_lldsm(
        isDiscrete() ? lldsm::DTMC : lldsm::CTMC
    );

    foo->setRSS(rss);
    foo->setPROC(init, proc);
    hldsm* bar = MakeEnumeratedModel(foo);
    foo->dumpDot(ds);
    ConstructionSuccess(bar);
    mymc = nullptr;
}


// **************************************************************************
// *                                                                        *
// *                         markov_formalism class                         *
// *                                                                        *
// **************************************************************************

class markov_formalism : public formalism {
  bool discrete;
public:
  markov_formalism(const char* n, const char* sd, const char* ld, bool d);

  virtual model_def* makeNewModel(const location &W, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;
};

// ******************************************************************
// *                    markov_formalism methods                    *
// ******************************************************************


markov_formalism
::markov_formalism(const char* n, const char* sd, const char* ld, bool d)
 : formalism(n, sd, ld)
{
  discrete = d;
  includeCTL();
  includeStochastic();
}

model_def* markov_formalism::makeNewModel(const location &W, char* name,
          symbol** formals, int np) const
{
  // TBD: check formals?
  return new markov_def(W, this, discrete,
      name, (formal_param**) formals, np);
}

bool markov_formalism::canDeclareType(const type* vartype) const
{
  if (0==vartype)                 return 0;
  if (vartype->matches("state"))  return 1;
  return 0;
}

bool markov_formalism::canAssignType(const type* vartype) const
{
  return 0;
}

// **************************************************************************
// *                                                                        *
// *                         Markov Chain Functions                         *
// *                                                                        *
// **************************************************************************

// **************************************************************************
// *                             mc_init  class                             *
// **************************************************************************

class mc_init : public model_internal {
public:
  mc_init();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

mc_init::mc_init() : model_internal(type::find("void"), "init", 2)
{
  typelist* t = new typelist(2);
  t->SetItem(0, type::find("state"));
  t->SetItem(1, type::find("real"));
  SetFormal(1, t, "s:w");
  SetRepeat(1);
  SetDocumentation("Sets the initial state(s) with probabilities for a Markov chain model.");
}

void mc_init::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  markov_def* mdl = smart_cast<markov_def*>(pass[0]);
  DCASSERT(mdl);

  if (model_debug.start()) {
    model_debug << "Calling init in model " << mdl->Name() << "\n";
    model_debug.stop();
  }

  if (x.stopExecution())  return;
  result* answer = x.answer;
  result state;
  result weight;
  for (int i=1; i<np; i++) {
    if (0==pass[i])  continue;
    x.aggregate = 0;
    x.answer = &state;
    pass[i]->Compute(x);
    DCASSERT(state.isNormal());
    if (!state.isNormal())  continue;
    DCASSERT(state.getPtr());
    model_enum_value* st = smart_cast<model_enum_value*> (state.getPtr());
    DCASSERT(st);
    x.answer = &weight;
    x.aggregate = 1;
    pass[i]->Compute(x);

    if (weight.isNormal()) {
      mdl->AddInitial(pass[i], st, weight.getReal());
      continue;
    }
    // something bizarre happened...
    const type* R = pass[i]->Type(1);
    if (0==R)  continue;
    unnamed_warning E(pass[i]->Where());
    E << "Ignoring weight: ";
    R->print(E.stream(), weight);
    E << " for state " << st->Name() << " in init";
  } // for i
  x.answer = answer;
  x.aggregate = 0;
}


// **************************************************************************
// *                             mc_arcs  class                             *
// **************************************************************************

class mc_arcs : public model_internal {
public:
  mc_arcs();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

mc_arcs::mc_arcs() : model_internal(type::find("void"), "arcs", 2)
{
  typelist* tl = new typelist(3);
  const type* state = type::find("state");
  tl->SetItem(0, state);
  tl->SetItem(1, state);
  tl->SetItem(2, type::find("real"));
  SetFormal(1, tl, "from:to:w");
  SetRepeat(1);
  SetDocumentation("Adds a set of arcs to the Markov chain.");
}

void mc_arcs::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(pass);
  DCASSERT(pass[0]);
  markov_def* mdl = smart_cast<markov_def*>(pass[0]);
  DCASSERT(mdl);

  if (x.stopExecution())  return;
  result* answer = x.answer;
  result from;
  result to;
  result weight;
  for (int i=1; i<np; i++) {
    if (0==pass[i])  continue;
    x.aggregate = 0;
    x.answer = &from;
    pass[i]->Compute(x);
    x.answer = &to;
    x.aggregate = 1;
    pass[i]->Compute(x);
    x.answer = &weight;
    x.aggregate = 2;
    pass[i]->Compute(x);

    // TBD: check state, weight for errors!
    if (!from.isNormal())   continue;  // error message?
    if (!to.isNormal())     continue;
    if (!weight.isNormal()) continue;

    DCASSERT(from.getPtr());
    model_enum_value* frst = smart_cast<model_enum_value*> (from.getPtr());
    DCASSERT(frst);
    model_enum_value* tost = smart_cast<model_enum_value*> (to.getPtr());
    DCASSERT(tost);

    mdl->AddEdge(pass[i], frst, tost, weight.getReal());
  } // for i
  x.answer = answer;
  x.aggregate = 0;
}

// **************************************************************************
// *                            mc_instate class                            *
// **************************************************************************

class mc_instate : public model_internal {
public:
  mc_instate();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

mc_instate::mc_instate() : model_internal(type::find(false, true, DETERM, "bool"), "in_state", 2)
{
  const type* state = type::find("state");
  SetFormal(1, state->getSetOfThis(), "sset");
  SetDocumentation("Returns true iff the Markov chain is in one of the specified states.");
}

void mc_instate::Compute(traverse_data &x, expr** pass, int np)
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
// *                           mc_transient class                           *
// **************************************************************************

class mc_transient : public model_internal {
public:
  mc_transient();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

mc_transient::mc_transient()
 : model_internal(type::find(false, true, DETERM, "bool"), "transient", 1)
{
  SetDocumentation("Returns true iff the Markov chain is in a transient state.");
}

void mc_transient::Compute(traverse_data &x, expr** pass, int np)
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
  const stochastic_lldsm* cruft = smart_cast<const stochastic_lldsm*>(bar);
  DCASSERT(cruft);
  const stochastic_lldsm::process* PROC = cruft->getPROC();
  DCASSERT(PROC);

  x.answer->setBool(PROC->isTransient(x.current_state_index));
}

// **************************************************************************
// *                           mc_absorbing class                           *
// **************************************************************************

class mc_absorbing : public model_internal {
public:
  mc_absorbing();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

mc_absorbing::mc_absorbing()
 : model_internal(type::find(false, true, DETERM, "bool"), "is_absorbed", 1)
{
  SetDocumentation("Returns true iff the Markov chain is in an absorbing state (this includes deadlocked states).");
}

void mc_absorbing::Compute(traverse_data &x, expr** pass, int np)
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
  const stochastic_lldsm* cruft = smart_cast<const stochastic_lldsm*>(bar);
  DCASSERT(cruft);

  x.answer->setBool(cruft->isAbsorbing(x.current_state_index));
}

// **************************************************************************
// *                              mc_tta class                              *
// **************************************************************************

class mc_tta : public model_internal {
  bool is_disc;
public:
  mc_tta(bool disc);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void ExtractParams(traverse_data &x, expr** pass, int np,
    stochastic_lldsm* &cruft, shared_object* &ss) {

      DCASSERT(x.answer);
      DCASSERT(2==np);

      model_instance* mi = grabModelInstance(x, pass[0]);
      DCASSERT(mi);
      hldsm* foo = mi->GetCompiledModel();
      DCASSERT(foo);
      lldsm* bar = foo->GetProcess();
      DCASSERT(bar);
      cruft = smart_cast<stochastic_lldsm*>(bar);
      DCASSERT(cruft);

      // make set of states
      ss = cruft->getPotential(pass[1]);
  }
};

mc_tta::mc_tta(bool disc)
: model_internal(
    type::find(false, false, PHASE, disc ? "int" : "real"),
    "tta", 2
  )
{
  is_disc = disc;
  SetFormal(1, type::find(false, true, DETERM, "bool"), "stop");

  SetDocumentation("Returns the distribution corresponding to the first time that stop becomes true, when starting from the initial distribution.");
}

void mc_tta::Compute(traverse_data &x, expr** pass, int np)
{
  stochastic_lldsm* proc = 0;
  shared_object* accept = 0;

  ExtractParams(x, pass, np, proc, accept);
  if (0==proc || 0==accept) {
    x.answer->setNull();
    return;
  }

  statedist* init = proc->getInitialDistribution();
  phase_hlm* foo = makeTTA(is_disc, init, accept, 0, proc->copyPROC());
  x.answer->setPtr(foo);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_mcform : public initializer {
    public:
        init_mcform();
        virtual void execute();
};
static init_mcform the_mcform_initializer;

init_mcform::init_mcform() : initializer(__FILE__, 1, 3)
{
    builds_resource(0, "mc_form.cc");
    needs_resource(1, "Warning");
    needs_resource(2, "Debug");
    needs_resource(3, "CML");
}

void init_mcform::execute()
{
    //
    // Register formalisms and state type
    //
    const char* longdocs = "The Markov chain formalisms dtmc and ctmc allow for direct specification of a discrete-time or continuous-time Markov chain. The two formalisms are nearly identical; the primary difference is that self-loops in a ctmc are ignored. States of the Markov chain are declared, and transition rates / probabilities are specified \"by hand\".";

    formalism* dtmc = new markov_formalism("dtmc",
        "Discrete-time Markov chain", longdocs, true);
    formalism* ctmc = new markov_formalism("ctmc",
        "Continuous-time Markov chain", longdocs, false);
    simple_type* t_state = type::registerNew(new void_type("state", "Discrete state", "State of a model (finite state machine or Markov chain)"));
    type::allowSetsOf(t_state);

    if (type::registerNew(dtmc) != dtmc) {
        internal_error E(__FILE__, __LINE__);
        E << "dtmc type already exists?";
        return;
    }
    if (type::registerNew(ctmc) != ctmc) {
        internal_error E(__FILE__, __LINE__);
        E << "ctmc type already exists?";
        return;
    }

    //
    // Build model functions for both discrete & continuous
    //
    symbol*  init       = new mc_init;
    symbol*  arcs       = new mc_arcs;
    symbol*  instate    = new mc_instate;
    symbol*  transient  = new mc_transient;
    symbol*  absorbing  = new mc_absorbing;

    //
    // Add symbols to dtmc model
    //
    dtmc->addSymbol(    init                );
    dtmc->addSymbol(    arcs                );
    dtmc->addSymbol(    instate             );
    dtmc->addSymbol(    transient           );
    dtmc->addSymbol(    absorbing           );
    dtmc->addSymbol(    new mc_tta(true)    );
    dtmc->finish();

    //
    // Add symbols to ctmc model
    //
    ctmc->addSymbol(    init                );
    ctmc->addSymbol(    arcs                );
    ctmc->addSymbol(    instate             );
    ctmc->addSymbol(    transient           );
    ctmc->addSymbol(    absorbing           );
    ctmc->addSymbol(    new mc_tta(false)   );
    ctmc->finish();

    //
    // Set up options
    //
    initialize_msg(markov_def::dup_init,
        "mc_dup_init",
        "For duplicatation of initial probabilities in Markov chain models",
        get_object(1, "Warning")
    );
    initialize_msg(markov_def::no_init,
        "mc_no_init",
        "For absence of initial probabilities in Markov chain models",
        get_object(1, "Warning")
    );
    initialize_msg(markov_def::dup_arc,
        "mc_dup_arc",
        "For duplicate arcs in Markov chain models",
        get_object(1, "Warning")
    );

    initialize_msg(markov_def::mc_debug,
        "mcs",
        "When set, diagnostic messages are displayed regarding Markov chain (dtmc and ctmc formalism) model construction.",
        get_object(2, "Debug")
    );

}

