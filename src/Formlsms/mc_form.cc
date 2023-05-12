
#include "mc_form.h"
#include "rss_enum.h"
#include "proc_mclib.h"
#include "enum_hlm.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/formalism.h"

#include "../ExprLib/sets.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/mod_vars.h"

#include "../include/splay.h"
#include "../Modules/statevects.h"
#include "../Modules/expl_ssets.h"
#include "../Formlsms/phase_hlm.h"

// Explicit Markov chain library
#include "../_MCLib/mclib.h"
#include "../_LSLib/lslib.h"


// **************************************************************************
// *                                                                        *
// *                           state_weight class                           *
// *                                                                        *
// **************************************************************************

struct state_weight {
  const model_enum_value* state;
  double weight;

  state_weight(const model_enum_value* s, double w) {
    state = s;
    weight = w;
  }

  inline int Compare(const model_enum_value* s) {
    return state->GetIndex() - s->GetIndex();
  }
  inline int Compare(const state_weight* sw) {
    return sw ? Compare(sw->state) : Compare((const model_enum_value*)0);
  }
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

  SplayOfPointers <state_weight> *initial;

  GraphLib::dynamic_summable<double>* mymc;
  // Old_MCLib::Markov_chain* mymc;

  bool error;
  bool discrete;

  static named_msg mc_debug;
  static named_msg dup_init;
  static named_msg no_init;
  static named_msg dup_arc;
  friend class init_mcform;
public:
  markov_def(const char* fn, int line, const type* t, bool d, char*n, 
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
  virtual void FinalizeModel(OutputStream &ds);

};

named_msg markov_def::mc_debug;
named_msg markov_def::dup_init;
named_msg markov_def::no_init;
named_msg markov_def::dup_arc;

// ******************************************************************
// *                       markov_def methods                       *
// ******************************************************************

markov_def::markov_def(const char* fn, int line, const type* t, bool d,
   char*n, formal_param **pl, int np) : model_def(fn, line, t, n, pl, np)
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
    if (StartError(wrap)) {
      em->cerr() << e.getString() << " when adding state " << wrap->Name();
      DoneError();
    }
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

  state_weight* find = initial->Find(foo);
  if (find) {
    if (StartWarning(dup_init, cause)) {
      em->warn() << "Ignoring duplicate initial probability for state ";
      em->warn() << foo->Name();
      DoneWarning();
    }
    return;
  }
  state_weight* sw = new state_weight(foo, weight);
  initial->Insert(sw);

  if (mc_debug.startReport()) {
    mc_debug.report() << "adding state " << foo->Name();
    mc_debug.report() << " to initial set with weight " << weight << "\n";
    mc_debug.stopIO();
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
      em->warn() << "Summing duplicate arc from state ";
      em->warn() << f->Name() << " to " << t->Name();
      DoneWarning();
    }
  } 
  catch (GraphLib::error e) {
    if (StartError(cause)) {
      em->cerr() << e.getString() << " when adding edge from ";
      em->cerr() << f->Name() << " to " << t->Name();
      DoneError();
    }
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
  initial = new SplayOfPointers <state_weight> (16, 0);
  error = false;
}

void markov_def::FinalizeModel(OutputStream &ds)
{
  model_enum* mcstate = new model_enum(0, current, statelist);
  statelist = 0;
  state_count = 0;

  if (error) {
    Delete(mcstate);
    delete mymc;
    mymc = 0;
    ConstructionError();
    return;
  }

  DCASSERT(mcstate);

  //
  // build initial distribution
  //
  state_weight** init_data;
  long size = initial->NumElements();
  if (size) {
    init_data = new state_weight*[size];
    initial->CopyToArray(init_data);
  } else {
    init_data = 0;
    if (StartWarning(no_init, 0)) {
      em->warn() << "Empty initial distribution";
      DoneWarning();
    }
  }
  delete initial;
  initial = 0;

  double total = 0;
  for (long i=0; i<size; i++) {
    DCASSERT(init_data[i]);
    total += init_data[i]->weight;
  }
  for (long i=0; i<size; i++) {
    DCASSERT(total > 0);
    init_data[i]->weight /= total;
  }
  long* indexes = new long[size];
  float* probs = new float[size];
  for (long i=0; i<size; i++) {
    indexes[i] = init_data[i]->state->GetIndex();
    probs[i] = init_data[i]->weight;
    delete init_data[i];
  }
  delete[] init_data;

  LS_Vector init;
  init.size = size;
  init.index = indexes;
  init.f_value = probs;
  init.d_value = 0;

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
  if (ds.IsActive()) foo->dumpDot(ds);
  ConstructionSuccess(bar);
  mymc = 0;

  //foo->getPROC()->computeTransient(...);
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

  virtual model_def* makeNewModel(const char* fn, int ln, char* name,
          symbol** formals, int np) const;

  virtual bool canDeclareType(const type* vartype) const;
  virtual bool canAssignType(const type* vartype) const;

  virtual bool includeCTL() const { return true; }
  virtual bool includeStochastic() const { return true; }
};

// ******************************************************************
// *                    markov_formalism methods                    *
// ******************************************************************


markov_formalism
::markov_formalism(const char* n, const char* sd, const char* ld, bool d)
 : formalism(n, sd, ld)
{
  discrete = d;
}

model_def* markov_formalism::makeNewModel(const char* fn, int ln, char* name, 
          symbol** formals, int np) const
{
  // TBD: check formals?
  return new markov_def(fn, ln, this, discrete, 
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

mc_init::mc_init() : model_internal(em->VOID, "init", 2)
{
  typelist* t = new typelist(2);
  t->SetItem(0, em->findType("state"));
  t->SetItem(1, em->REAL);
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

  if (model_debug.startReport()) {
    model_debug.report() << "Calling init in model " << mdl->Name() << "\n";
    model_debug.stopIO();
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
    if (em->startWarning()) {
      em->causedBy(pass[i]);
      em->warn() << "Ignoring weight: ";
      R->print(em->warn(), weight);
      em->warn() << " for state " << st->Name() << " in init";
      em->stopIO();
    }
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

mc_arcs::mc_arcs() : model_internal(em->VOID, "arcs", 2)
{
  typelist* tl = new typelist(3);
  const type* state = em->findType("state");
  tl->SetItem(0, state);
  tl->SetItem(1, state);
  tl->SetItem(2, em->REAL);
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

mc_instate::mc_instate() : model_internal(em->BOOL->addProc(), "in_state", 2)
{
  const type* state = em->findType("state");
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
 : model_internal(em->BOOL->addProc(), "transient", 1)
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
 : model_internal(em->BOOL->addProc(), "is_absorbed", 1)
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
    disc ? em->INT->modifyType(PHASE) : em->REAL->modifyType(PHASE), 
    "tta", 2
  )
{
  is_disc = disc;
  SetFormal(1, em->BOOL->addProc(), "stop");

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
// **************************************************************************
// *                              mc_reverse_tta class                              *
// **************************************************************************

class mc_reverse_tta : public model_internal {
  bool is_disc;
public:
  mc_reverse_tta(bool disc);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void ExtractParams(traverse_data &x, expr** pass, int np, 
    stochastic_lldsm* &cruft, shared_object* &ss) {

      DCASSERT(x.answer);
      DCASSERT(3==np);

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

mc_reverse_tta::mc_reverse_tta(bool disc)
: model_internal(
    em->STATEPROBS, "reverse_tta", 3
  )
{
  is_disc = disc;
  SetFormal(1,em->INT,"time");
  SetFormal(2, em->STATESET, "q");
  SetDocumentation("Returns the expected time to the reach absorbing q state, when starting from the transient states.");
}
#include <iostream>
void mc_reverse_tta::Compute(traverse_data &x, expr** pass, int np)
{
  stochastic_lldsm* proc = 0;
  shared_object* accept = 0;
  
  pass[1]->Compute(x);
  if (!x.answer->isNormal()) return ;
  long time = x.answer->getInt();
  std::cerr << "time: " << time << "\n";

  ExtractParams(x, pass, np, proc, accept);
  if (0==proc || 0==accept) {
    x.answer->setNull();
    return;
  }
  
  SafeCompute(pass[2], x);
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* ss = smart_cast <stateset*>(Share(x.answer->getPtr()));
  expl_stateset* e = dynamic_cast <expl_stateset*>(ss);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    //Delete(p);
    Delete(accept);
    x.answer->setNull();
    return;
  }
  const intset& eis = e->getExplicit();
  double* probs=new double[proc->getNumStates()];
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
     probs[p] =0;
  }
  for(long i=0;i<proc->getPROC()->getNumStates();i++){
    //how to assign the values to probs
    if(eis.contains(i)){
      probs[i]=1;
    }
  }

  double* aux=0;
  
  //statedist* init = proc->getInitialDistribution();
  //phase_hlm* foo = makeTTA(is_disc, init, accept, 0, proc->copyPROC());
  //proc->getPROC()->getNumStates();
  //
  // Set x to be all zeroes, except for the accepting state
  //
  /*
  for (long i=proc->getNumStates()-1; i>=0; i--){
  //long acc_state = proc->getAcceptingState();
  if(proc->getPROC()->isAbsorbing(i)){
  // DCASSERT(acc_state>0);
  // DCASSERT(acc_state<proc->getPROC()->getNumStates());
      probs[i] = 1;
    }
    else {
      probs[i] = 0;
    }
  }
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
    std::cerr << probs[p] << "\n";
  }
  */
  //FIND tr4
  //FIX formal param // use compute
  bool res= proc->getPROC()->reverseTransientUnbounded(time,probs,aux);

  // for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
  //   std::cerr << probs[p] << "\n";
  // }

  // std::cerr << acc_state << " " << probs[5] << "\n";
  //how to set the pointer ??
  stateprobs* sp= new stateprobs(proc,probs,proc->getPROC()->getNumStates());
  x.answer->setPtr(sp);
  
}

// **************************************************************************
// *                              mc_reverse_tta_timed class                              *
// **************************************************************************

class mc_reverse_tta_timed : public model_internal {
  bool is_disc;
public:
  mc_reverse_tta_timed(bool disc);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void ExtractParams(traverse_data &x, expr** pass, int np, 
    stochastic_lldsm* &cruft, shared_object* &ss) {

      DCASSERT(x.answer);
      DCASSERT(5==np);

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

mc_reverse_tta_timed::mc_reverse_tta_timed(bool disc)
: model_internal(
    em->STATEPROBS, "reverse_tta_timed", 5
  )
{
  is_disc = disc;
  SetFormal(1,em->INT,"h"); //start time
  SetFormal(2,em->INT,"k"); //end time
  SetFormal(3, em->STATESET, "q_abs");
  SetFormal(4, em->STATESET, "q_timed");
  SetDocumentation("Returns the expected time to the reach absorbing b state, when starting from the transient states and following a states for h times.");
}
#include <iostream>
void mc_reverse_tta_timed::Compute(traverse_data &x, expr** pass, int np)
{
  stochastic_lldsm* proc = 0;
  shared_object* accept = 0;
  
  pass[1]->Compute(x);
  if (!x.answer->isNormal()) return ;
  long h = x.answer->getInt();
  pass[2]->Compute(x);
  if (!x.answer->isNormal()) return ;
  long k = x.answer->getInt();
  //std::cerr << "time: " << time << "\n";

  ExtractParams(x, pass, np, proc, accept);
  if (0==proc || 0==accept) {
    x.answer->setNull();
    return;
  }
  SafeCompute(pass[3], x);
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* ss = smart_cast <stateset*>(Share(x.answer->getPtr()));
  expl_stateset* e = dynamic_cast <expl_stateset*>(ss);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    //Delete(p);
    Delete(accept);
    x.answer->setNull();
    return;
  }
  const intset& eis = e->getExplicit();
  double* probs=new double[proc->getPROC()->getNumStates()];
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
     probs[p] =0;
  }
  for(long i=0;i<proc->getPROC()->getNumStates();i++){
    //how to assign the values to probs
    if(eis.contains(i)){
      probs[i]=1;
    }
  }
  SafeCompute(pass[4], x);
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* ss_t = smart_cast <stateset*>(Share(x.answer->getPtr()));
  expl_stateset* et = dynamic_cast <expl_stateset*>(ss_t);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    //Delete(p);
    Delete(accept);
    x.answer->setNull();
    return;
  }
  const intset& etis = et->getExplicit();
  double* probs_t=new double[proc->getPROC()->getNumStates()];
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
     probs_t[p] =0;
  }
  for(long i=0;i<proc->getPROC()->getNumStates();i++){
    //how to assign the values to probs
    if(etis.contains(i)){
      probs_t[i]=1;
      
    }
  }
   for(long i=0;i<proc->getPROC()->getNumStates();i++){
    std::cout << "filter " << probs_t[i] << "\n";
   }

   double* aux=0;
  //statedist* init = proc->getInitialDistribution();
  //phase_hlm* foo = makeTTA(is_disc, init, accept, 0, proc->copyPROC());
  //proc->getPROC()->getNumStates();
  //
  // Set x to be all zeroes, except for the accepting state
  //
  /*
  for (long i=proc->getNumStates()-1; i>=0; i--){
  //long acc_state = proc->getAcceptingState();
  if(proc->getPROC()->isAbsorbing(i)){
  // DCASSERT(acc_state>0);
  // DCASSERT(acc_state<proc->getPROC()->getNumStates());
      probs[i] = 1;
    }
    else {
      probs[i] = 0;
    }
  }
  */
  // for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
  //   std::cerr << probs[p] << "\n";
  // }
  
  bool res= proc->getPROC()->reverseTransientBounded(h,k,probs,probs_t,aux);

  // for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
  //   std::cerr << probs[p] << "\n";
  // }

  // std::cerr << acc_state << " " << probs[5] << "\n";
  //how to set the pointer ??
  stateprobs* sp= new stateprobs(proc,probs,proc->getPROC()->getNumStates());
  x.answer->setPtr(sp);
  
}
// **************************************************************************
// *                              mc_conditional_tta class                              *
// **************************************************************************

class mc_conditional_tta : public model_internal {
  bool is_disc;
public:
  mc_conditional_tta(bool disc);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void ExtractParams(traverse_data &x, expr** pass, int np, 
    stochastic_lldsm* &cruft, shared_object* &ss) {

      DCASSERT(x.answer);
      DCASSERT(5==np);

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

mc_conditional_tta::mc_conditional_tta(bool disc)
: model_internal(
    em->STATEPROBS, "conditional_tta", 5
  )
{
  is_disc = disc;
  SetFormal(1,em->INT,"h time");
  SetFormal(2,em->INT,"k time");
  SetFormal(3,em->STATESET,"unbounded exp time");
  SetFormal(4, em->STATESET, "set of good states");
  SetDocumentation("Returns the conditional expected time to the reach absorbing q state, when starting from the transient states.");
}
#include<iostream>
void mc_conditional_tta::Compute(traverse_data &x, expr** pass, int np)
{
  stochastic_lldsm* proc = 0;
  shared_object* accept = 0;
  
  pass[1]->Compute(x);
  if (!x.answer->isNormal()) return ;
  long h = x.answer->getInt();
  std::cerr << "time: " << time << "\n";
  pass[2]->Compute(x);
  if (!x.answer->isNormal()) return ;
  long k = x.answer->getInt();

  ExtractParams(x, pass, np, proc, accept);
  if (0==proc || 0==accept) {
    x.answer->setNull();
    return;
  }
  //FIX here to get the probs from thee stateset
  SafeCompute(pass[3], x);
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* ss = smart_cast <stateset*>(Share(x.answer->getPtr()));
  expl_stateset* e = dynamic_cast <expl_stateset*>(ss);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    //Delete(p);
    Delete(accept);
    x.answer->setNull();
    return;
  }
  const intset& eis = e->getExplicit();
  double* probs=new double[proc->getPROC()->getNumStates()];
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
     probs[p] =0;
  }
  for(long i=0;i<proc->getPROC()->getNumStates();i++){
    //how to assign the values to probs
    if(eis.contains(i)){
      probs[i]=1;
    }
  }
  SafeCompute(pass[4], x);
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* ss_t = smart_cast <stateset*>(Share(x.answer->getPtr()));
  expl_stateset* et = dynamic_cast <expl_stateset*>(ss_t);
  if (!e) {
  
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() requires explicit statesets (for now)";
      em->stopIO();
    }
    //Delete(p);
    Delete(accept);
    x.answer->setNull();
    return;
  }
  const intset& etis = et->getExplicit();
  double* probs_t=new double[proc->getPROC()->getNumStates()];
  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
     probs_t[p] =0;
  }
  for(long i=0;i<proc->getPROC()->getNumStates();i++){
    //how to assign the values to probs
    if(etis.contains(i)){
      probs_t[i]=1;
      
    }
  }

  double* aux=0;
  
  //statedist* init = proc->getInitialDistribution();
  //phase_hlm* foo = makeTTA(is_disc, init, accept, 0, proc->copyPROC());
  //proc->getPROC()->getNumStates();
  //
  // Set x to be all zeroes, except for the accepting state
  //
/*
  for (long i=proc->getNumStates()-1; i>=0; i--){
  //long acc_state = proc->getAcceptingState();
  if(proc->getPROC()->isAbsorbing(i)){
  // DCASSERT(acc_state>0);
  // DCASSERT(acc_state<proc->getPROC()->getNumStates());
      probs[i] = 1;
    }
    else {
      probs[i] = 0;
    }
  }
  */
  double arr[4]={1,1,0,0};
  //probs=arr;

  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
    std::cerr << "is this " <<probs[p] << "\n";
  }
  //FIND tr4
  //FIX formal param // use compute
  bool res= proc->getPROC()->reverseTransientConditional(h, k, probs, probs_t, aux);

  for(long p = 0; p < proc->getPROC()->getNumStates(); ++p) {
    std::cerr << probs[p] << "\n";
  }

  // std::cerr << acc_state << " " << probs[5] << "\n";
  //how to set the pointer ??
  stateprobs* sp= new stateprobs(proc,probs,proc->getPROC()->getNumStates());
  x.answer->setPtr(sp);
  
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
    virtual bool execute();
  private:
    void FillSymbolTable(bool disc, formalism* mc);
};
init_mcform the_mcform_initializer;

init_mcform::init_mcform() : initializer("init_mcform")
{
  usesResource("em");
  usesResource("CML");
  buildsResource("formalisms");
}

bool init_mcform::execute()
{
  if (0==em) return false;

  bool ok;
  // Set up options
  option* debug = em->findOption("Debug");
  markov_def::mc_debug.Initialize(debug,
    "mcs",
    "When set, diagnostic messages are displayed regarding Markov chain (dtmc and ctmc formalism) model construction.",
    false
  );

  option* warning = em->findOption("Warning");
  markov_def::dup_init.Initialize(warning,
    "mc_dup_init",
    "For duplicatation of initial probabilities in Markov chain models",
    true
  );
  markov_def::no_init.Initialize(warning,
    "mc_no_init",
    "For absence of initial probabilities in Markov chain models",
    true
  );
  markov_def::dup_arc.Initialize(warning,
    "mc_dup_arc",
    "For duplicate arcs in Markov chain models",
    true
  );

  // Set up and register formalisms
  const char* longdocs = "The Markov chain formalisms dtmc and ctmc allow for direct specification of a discrete-time or continuous-time Markov chain. The two formalisms are nearly identical; the primary difference is that self-loops in a ctmc are ignored. States of the Markov chain are declared, and transition rates / probabilities are specified \"by hand\".";

  formalism* dtmc = new markov_formalism("dtmc", 
      "Discrete-time Markov chain", longdocs, true);
  formalism* ctmc = new markov_formalism("ctmc", 
      "Continuous-time Markov chain", longdocs, false);
  ok = em->registerType(dtmc);
  if (!ok) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Couldn't register dtmc type";
      em->stopIO();
    }
    return false;
  }
  ok = em->registerType(ctmc);
  if (!ok) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->noCause();
      em->internal() << "Couldn't register ctmc type";
      em->stopIO();
    }
    return false;
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

  // fill symbol tables
  FillSymbolTable(true,   dtmc);
  FillSymbolTable(false,  ctmc);

  return true;
}

void init_mcform::FillSymbolTable(bool disc, formalism* mc)
{
  // Build functions if necessary
  static symbol*  init = 0;
  static symbol*  arcs = 0;
  static symbol*  instate = 0;
  static symbol*  transient = 0;
  static symbol*  absorbing = 0;
  static symbol*  dTTA = 0;
  static symbol*  cTTA = 0;
  static symbol*  rev_dTTA=0;
  static symbol*  rev_timed_dTTA=0;
  static symbol*  rev_cond_dTTA=0;

  if (!init)      init = new mc_init;
  if (!arcs)      arcs = new mc_arcs;
  if (!instate)   instate = new mc_instate;
  if (!transient) transient = new mc_transient;
  if (!absorbing) absorbing = new mc_absorbing;

  // Grab functions into a symbol table
  symbol_table* mcsyms = MakeSymbolTable();
  mcsyms->AddSymbol(  init      );
  mcsyms->AddSymbol(  arcs      );
  mcsyms->AddSymbol(  instate   );
  mcsyms->AddSymbol(  transient );
  mcsyms->AddSymbol(  absorbing );

  if (disc) {
    if (!dTTA)  dTTA = new mc_tta(true);
    mcsyms->AddSymbol(  dTTA    );
    if(!rev_dTTA) rev_dTTA= new mc_reverse_tta(true);
    mcsyms->AddSymbol(rev_dTTA);
    if(!rev_timed_dTTA) rev_timed_dTTA= new mc_reverse_tta_timed(true);
    mcsyms->AddSymbol(rev_timed_dTTA);
    if(!rev_cond_dTTA) rev_cond_dTTA= new mc_conditional_tta(true);
    mcsyms->AddSymbol(rev_cond_dTTA);
  } else {
    if (!cTTA)  cTTA = new mc_tta(false);
    mcsyms->AddSymbol(  cTTA    );
  }

  // Set the symbol table
  mc->setFunctions(mcsyms);
  mc->addCommonFuncs(CML);
}

