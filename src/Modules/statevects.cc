
// $Id$

#include "../Options/options.h"
#include "../ExprLib/exprman.h"
// #include "../ExprLib/unary.h"
// #include "../ExprLib/binary.h"
// #include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"
// #include "biginttype.h"
#include "../ExprLib/mod_vars.h"
// #include "../ExprLib/dd_front.h"

#include "../Formlsms/stoch_llm.h"

#include "statevects.h"
#include "statesets.h"

// External
#include "intset.h"

// ******************************************************************
// *                                                                *
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class sparse_vect_printer : public lldsm::state_visitor {
  OutputStream &out;
  const double* dist;
  bool print_indexes;
  bool comma;
public:
  sparse_vect_printer(const hldsm* mdl, OutputStream &s, const double* d, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

sparse_vect_printer
::sparse_vect_printer(const hldsm* m, OutputStream &s, const double* d, bool pi)
 : state_visitor(m), out(s)
{
  dist = d;
  print_indexes = pi;
  comma = false;
}

bool sparse_vect_printer::canSkipIndex()
{
  return (0==dist[x.current_state_index] );
}

bool sparse_vect_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  if (print_indexes) {
    out.Put(x.current_state_index); 
  } else {
    x.current_state->Print(out, 0);
  }
  out << ":" << dist[x.current_state_index];
  return false;
}

// ******************************************************************
// *                                                                *
// *                       statevect  methods                       *
// *                                                                *
// ******************************************************************

int statevect::display_style;

statevect::statevect(const stochastic_lldsm* p, const double* d, long N)
: shared_object()
{
  parent = p;
  vect = new double[N];
  memcpy(vect, d, N*sizeof(double));
  numStates = N;
}

statevect::statevect(const stochastic_lldsm* p, double* d, long N, bool own)
: shared_object()
{
  parent = p;
  if (own) {
    vect = d;
  } else {
    vect = new double[N];
    memcpy(vect, d, N*sizeof(double));
  }
  numStates = N;
}


statevect::~statevect()
{
  // printf("Destroying state vector\n");  
  delete[] vect;
}

bool statevect::Print(OutputStream &s, int width) const
{
  DCASSERT(parent);
  DCASSERT(vect);
  if (FULL==display_style) {
    s.Put('[');
    s << vect[0];
    for (int i=1; i<numStates; i++) {
      s << ", " << vect[i];
    }
    s.Put(']');
  } else {
    sparse_vect_printer foo(parent->GetParent(), s, vect, SINDEX==display_style);
    s.Put('(');
    parent->visitStates(foo);
    s.Put(')');
  }
  return true;
}

bool statevect::Equals(const shared_object *o) const
{
  const statevect* b = dynamic_cast <const statevect*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  
  DCASSERT(numStates == b->numStates);

  for (long i=0; i<numStates; i++) {
    if (vect[i] != b->vect[i]) return false;
    // TBD - check within epsilon?
  }
  return true;
}

// ******************************************************************
// *                                                                *
// *                       statedist  methods                       *
// *                                                                *
// ******************************************************************

statedist::statedist(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

statedist::statedist(const stochastic_lldsm *p, double *d, long N, bool own)
 : statevect(p, d, N, own)
{
}

// ******************************************************************
// *                                                                *
// *                       stateprobs methods                       *
// *                                                                *
// ******************************************************************

stateprobs::stateprobs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

stateprobs::stateprobs(const stochastic_lldsm *p, double *d, long N, bool own)
 : statevect(p, d, N, own)
{
}

// ******************************************************************
// *                                                                *
// *                       statemsrs  methods                       *
// *                                                                *
// ******************************************************************

statemsrs::statemsrs(const stochastic_lldsm *p, const double *d, long N)
 : statevect(p, d, N)
{
}

statemsrs::statemsrs(const stochastic_lldsm *p, double *d, long N, bool own)
 : statevect(p, d, N, own)
{
}

// ******************************************************************
// *                                                                *
// *                      statedist_type class                      *
// *                                                                *
// ******************************************************************

class statedist_type : public simple_type {
public:
  statedist_type();
};

statedist_type::statedist_type() : simple_type("statedist", "Distribution over states", "A probability distribution over a set of model states.  Elements will sum to one.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                     stateprobs_type  class                     *
// *                                                                *
// ******************************************************************

class stateprobs_type : public simple_type {
public:
  stateprobs_type();
};

stateprobs_type::stateprobs_type() : simple_type("stateprobs", "Probabilities for states", "A vector of probabilities over states, specifying a probability value for each state.  Note that this is not a distribution, but rather a special case of measures for each state, so probabilities may not sum to one.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                      statemsrs_type class                      *
// *                                                                *
// ******************************************************************

class statemsrs_type : public simple_type {
public:
  statemsrs_type();
};

statemsrs_type::statemsrs_type() : simple_type("statemsrs", "Measures for states", "A vector of real--valued measures over states, specifying a measure for each state.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       condition_si class                       *
// ******************************************************************

class condition_si : public simple_internal {
public:
  condition_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

condition_si::condition_si() : simple_internal(em->STATEDIST, "condition", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEDIST);
  SetFormal(0, em->STATEDIST, "p");
  SetFormal(1, em->STATESET, "e");
  SetDocumentation("Given a state distribution p, build a new distribution conditioned on the fact that a state belongs to the set e.  Will return null if the probability (according to p) of e is 0.");
}

void condition_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  statedist* p = smart_cast <statedist*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }

  stateset* e = smart_cast <stateset*>(Share(x.answer->getPtr()));
  DCASSERT(e);

  if (p->getParent() != e->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "State distribution and set parameters to condition()";
      em->newLine();
      em->cerr() << "are from different model instances";
      em->stopIO();
    }
    Delete(p);
    Delete(e);
    x.answer->setNull();
    return;
  }

  if (!e->isExplicit()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, condition() doesn't work yet with DD statesets";
      em->stopIO();
    }
    Delete(p);
    Delete(e);
    x.answer->setNull();
    return;
  }

  //
  // Ready to do actual computation
  //

  //
  // Set up new distribution
  //
  double* qv = new double[p->size()];
  for (long i=0; i<p->size(); i++) qv[i] = 0;

  //
  // Copy stuff over, and compute total
  //
  double prob = 0.0;
  const intset eis = e->getExplicit();  
  long i = -1;
  while ( (i=eis.getSmallestAfter(i)) >= 0) {
    prob += (qv[i] = p->read(i));
  }

  //
  // Determine result
  //
  if (prob <= 0) {
    // 0 prob - that's a null
    x.answer->setNull();
    delete[] qv;
  } else
  if (prob >= 1) {
    // 1 prob - we can simply copy p
    x.answer->setPtr(Share(p));
    delete[] qv;
  } else {
    // generic case
    // (1) normalize
    for (long i=0; i<p->size(); i++) qv[i] /= prob;
    // (2) set result
    x.answer->setPtr(new statedist(p->getParent(), qv, p->size(), true));
  }

  //
  // Cleanup 
  //
  Delete(p);
  Delete(e);
}

// ******************************************************************
// *                         prob_si  class                         *
// ******************************************************************

class prob_si : public simple_internal {
public:
  prob_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

prob_si::prob_si() : simple_internal(em->REAL, "prob", 2)
{
  DCASSERT(em->STATESET);
  DCASSERT(em->STATEDIST);
  SetFormal(0, em->STATEDIST, "p");
  SetFormal(1, em->STATESET, "e");
  SetDocumentation("Determine the probability (according to distribution p) of a state being in set e.");
}

void prob_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  statedist* p = smart_cast <statedist*>(Share(x.answer->getPtr()));
  DCASSERT(p);

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) {
    Delete(p);
    return;
  }

  stateset* e = smart_cast <stateset*>(Share(x.answer->getPtr()));
  DCASSERT(e);

  if (p->getParent() != e->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "State distribution and set parameters to prob()";
      em->newLine();
      em->cerr() << "are from different model instances";
      em->stopIO();
    }
    Delete(p);
    Delete(e);
    x.answer->setNull();
    return;
  }

  if (!e->isExplicit()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Sorry, prob() doesn't work yet with DD statesets";
      em->stopIO();
    }
    Delete(p);
    Delete(e);
    x.answer->setNull();
    return;
  }

  //
  // Ready to do actual computation
  //
  double prob = 0.0;
  const intset eis = e->getExplicit();  
  long i = -1;
  while ( (i=eis.getSmallestAfter(i)) >= 0) {
    prob += p->read(i);
  }
  Delete(p);
  Delete(e);
  x.answer->setReal(prob);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************


void InitStatevects(exprman* em, symbol_table* st)
{
  if (0==em)  return;
  
  // Type registry
  simple_type* t_statedist = new statedist_type;
  em->registerType(t_statedist);

  simple_type* t_stateprobs = new stateprobs_type;
  em->registerType(t_stateprobs);

  simple_type* t_statemsrs = new statemsrs_type;
  em->registerType(t_statemsrs);

  em->setFundamentalTypes();

  // Operators

  // Options
  // ------------------------------------------------------------------
  radio_button** style = new radio_button*[3];
  style[statevect::FULL] = new radio_button(
      "FULL",
      "Vectors are displayed in (raw) full.",
      statevect::FULL
  );
  style[statevect::SINDEX] = new radio_button(
      "SPARSE_INDEX",
      "Vectors are displayed in sparse format, with state indexes.",
      statevect::SINDEX
  );
  style[statevect::SSTATE] = new radio_button(
      "SPARSE_STATE",
      "Vectors are displayed in sparse format, with states.",
      statevect::SSTATE
  );
  statevect::display_style = statevect::SINDEX; 
  em->addOption(
    MakeRadioOption("StatevectDisplayStyle",
      "Style to display a statedist, stateprobs, or statemsrs vector.",
      style, 3, statevect::display_style
    )
  );

  if (0==st) return;

  // Functions
  st->AddSymbol(  new prob_si         );
  st->AddSymbol(  new condition_si    );
}

