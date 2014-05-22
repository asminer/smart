
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
// *                    statevect_printer  class                    *
// *                                                                *
// ******************************************************************

class statevect_printer : public lldsm::state_visitor {
  OutputStream &out;
  const statevect* dist;
  int display_style;
private:
  long nextz;
  bool comma;
public:
  statevect_printer(const hldsm* mdl, OutputStream &s, const statevect* d, 
    int style);
  virtual bool canSkipIndex();
  virtual bool visit();
};

statevect_printer::statevect_printer(const hldsm* m, OutputStream &s, 
  const statevect* d, int style) : state_visitor(m), out(s)
{
  dist = d;
  display_style = style;
  comma = false;
  nextz = 0;
}

bool statevect_printer::canSkipIndex()
{
  if (statevect::FULL == display_style) return false;
  if (dist->isSparse()) {
    return dist->readSparseIndex(nextz) != x.current_state_index;
  }
  return (0==dist->readFull(x.current_state_index));
}

bool statevect_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  
  if (statevect::SINDEX==display_style) {
    out.Put(x.current_state_index); 
    out.Put(':');
  }
  if (statevect::SSTATE==display_style) {
    x.current_state->Print(out, 0);
    out.Put(':');
  }

  if (dist->isSparse()) {
    if (x.current_state_index == nextz) {
      out << dist->readSparseValue(nextz);
      nextz++;
      return (nextz >= dist->size());
    } else {
      // must be FULL display style
      out << 0;
      return false;
    }
  }
  if (x.current_state_index < dist->size()) {
    out << dist->readFull(x.current_state_index);
  } else {
    out << 0;
  }
  if (statevect::FULL == display_style) return false;
  return (x.current_state_index >= dist->size()-1);
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
  // determine number of nonzeroes
  long nnz = 0;
  for (long i=0; i<N; i++) if (d[i]) nnz++;
  if (0==nnz) {
    // easy case
    indexes = 0;
    vect = 0;
    vectsize = 0;
    return;
  }

  // determine truncation point
  long tsize;
  for (tsize=N-1; tsize>=0; tsize--) {
    if (d[tsize]) break;
  }
  tsize++;

  //
  // Decide how to store
  //
  if (tsize * sizeof(double) <= nnz * (sizeof(double)+sizeof(long))) {
    //
    // Truncated
    //
    indexes = 0;
    vect = new double[tsize];
    memcpy(vect, d, tsize*sizeof(double));
    vectsize = tsize;
  } else {
    //
    // Sparse
    //
    indexes = new long[nnz];
    vect = new double[nnz];
    vectsize = nnz;
    long z = 0;
    for (long i=0; i<N; i++) if (d[i]) {
      indexes[z] = i;
      vect[z] = d[i];
      z++;
    }
  }
}

statevect::statevect(const stochastic_lldsm* p, long* I, double* D, long N)
: shared_object()
{
  parent = p;
  indexes = I;
  vect = D;
  vectsize = N;
}

statevect::~statevect()
{
  // printf("Destroying state vector\n");  
  delete[] indexes;
  delete[] vect;
}

bool statevect::Print(OutputStream &s, int width) const
{
  DCASSERT(parent);

  if (FULL==display_style)  s.Put('[');
  else                      s.Put('(');

  statevect_printer foo(parent->GetParent(), s, this, display_style);
  parent->visitStates(foo);

  if (FULL==display_style)  s.Put(']');
  else                      s.Put(')');

  return true;
}

bool statevect::Equals(const shared_object *o) const
{
  const statevect* b = dynamic_cast <const statevect*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  
  DCASSERT(vectsize == b->vectsize);

  for (long i=0; i<vectsize; i++) {
    if (vect[i] != b->vect[i]) return false;
    // TBD - check within epsilon?
  }
  return true;
}

void statevect::copyRestricted(const statevect* sv, const intset* e)
{
  DCASSERT(sv);

  long nnz = 0;
  long tsize = 0;

  if (sv->isSparse()) {
    for (long z=0; z<sv->size(); z++) {
      long i = sv->readSparseIndex(z);
      if (e && !e->contains(i)) continue;
      if (0==sv->readSparseValue(z)) continue;
      nnz++;
      tsize = MAX(tsize, 1+i);
    }
  } else {
    for (long i=0; i<sv->size(); i++) {
      if (0==sv->readFull(i)) continue;
      if (e && !e->contains(i)) continue;
      nnz++;
      tsize = MAX(tsize, 1+i);
    }
  }

  if (tsize * sizeof(double) <= nnz * (sizeof(double)+sizeof(long))) {
    //
    // Store as truncated
    //
    indexes = 0;
    vect = new double[tsize];
    vectsize = tsize;
    //
    if (sv->isSparse()) {
      //
      // Trunc <- sparse
      //
      for (long i=0; i<tsize; i++) vect[i] = 0;
      for (long z=0; z<nnz; z++) {
        long i = sv->readSparseIndex(z);
        if (e && !e->contains(i)) continue;
        vect[i] = sv->readSparseValue(z);
      }
    } else {
      //
      // Trunc <- trunc
      //
      for (long i=0; i<tsize; i++) {
        if (e && !e->contains(i))   vect[i] = 0;
        else                        vect[i] = sv->readFull(i);
      }
    }
  } else {
    //
    // Store as sparse
    //
    indexes = new long[nnz];
    vect = new double[nnz];
    vectsize = nnz;
    //
    if (sv->isSparse()) {
      //
      // Sparse <- sparse
      //
      long zp = 0;
      for (long z=0; z<sv->size(); z++) {
        long i = sv->readSparseIndex(z);
        if (e && !e->contains(i)) continue;
        if (0==sv->readSparseValue(z)) continue;
        indexes[zp] = i;
        vect[zp] = sv->readSparseValue(z);
        zp++;
      }
      DCASSERT(zp == nnz);
    } else {
      //
      // Sparse <- trunc
      //
      long zp = 0;
      for (long i=0; i<sv->size(); i++) {
        if (0==sv->readFull(i)) continue;
        if (e && !e->contains(i)) continue;
        indexes[zp] = i;
        vect[zp] = sv->readFull(i);
        zp++;
      }
      DCASSERT(zp == nnz);
    }
  }
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

statedist::statedist(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
{
}

double statedist::normalize()
{
  //
  // This works both for sparse and truncated full :^)
  //
  double total = 0.0;
  for (long i=0; i<vectsize; i++) total += vect[i];
  if (total <= 0.0) return total;
  if (1.0 == total) return total;
  for (long i=0; i<vectsize; i++) vect[i] /= total;
  return total;
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

stateprobs::stateprobs(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
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

statemsrs::statemsrs(const stochastic_lldsm* p, long* I, double* D, long N)
 : statevect(p, I, D, N)
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

  statedist* q = new statedist(p->getParent(), 0, 0, 0);
  const intset& eis = e->getExplicit();
  q->copyRestricted(p, &eis);

  double total = q->normalize();

  if (total <= 0) {
    // that's a null
    Delete(q);
    x.answer->setNull();
  } else if (total == 1) {
    // q is equal to p
    Delete(q);
    x.answer->setPtr(Share(p));
  } else {
    // generic case
    x.answer->setPtr(q);
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
    prob += p->readFull(i);
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

