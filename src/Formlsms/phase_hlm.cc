
// $Id$

#include "intset.h"

#include "phase_hlm.h"

#include "../ExprLib/mod_vars.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/engine.h"

// #include "../include/heap.h"

#include "stoch_llm.h"

#include "../Modules/statesets.h"

// ******************************************************************
// *                                                                *
// *                        phase_hlm methods                       *
// *                                                                *
// ******************************************************************

phase_hlm::phase_hlm(bool d) : hldsm(Phase_Type)
{
  is_discrete = d;
}

lldsm::model_type phase_hlm::GetProcessType() const
{
  return is_discrete ? lldsm::DTMC : lldsm::CTMC;
}

// ******************************************************************
// *                                                                *
// *                        phase_dist class                        *
// *                                                                *
// ******************************************************************

/** Class for concrete phase-type distributions.
    State is a single integer.
*/
class phase_dist : public phase_hlm {
protected:
  int state_index;
public:
  phase_dist(bool d);
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool*) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;
};

phase_dist::phase_dist(bool d) : phase_hlm(d)
{
  state_index = 0;
}

int phase_dist::NumStateVars() const
{
  return 1;
}

bool phase_dist::containsListVar() const 
{
  return false;
}

void phase_dist::determineListVars(bool* ilv) const
{
  ilv[0] = 0;
}

void phase_dist::reindexStateVars(int &start)
{
  state_index = start;
  start++;
}

void phase_dist::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(x);
  if (isAcceptingState(x)) {
    s.Put('a');
    return;
  }
  if (isTrapState(x)) {
    s.Put('t');
    return;
  }
  s << x->get(state_index);
}

// ******************************************************************
// *                                                                *
// *                         tta_dist class                         *
// *                                                                *
// ******************************************************************

/** Class for time to absorption of a given MC.

    If the MC has n states, then our states are:
      0..n-1  Correspond to MC states.
      n       Vanishing initial state.
      n+1     Unique accept state.
      n+2     Unique trap state.
*/
class tta_dist : public phase_dist {
  long* init_index;
  double* init_val;
  double* init_acc;
  int init_len;
  stochastic_lldsm* chain;
  stateset* accept;
  stateset* final;    // accept or trap states.
  long num_states;
  // for outgoing edges
  long* to_states;
  double* weights;
  long out_alloc;
protected:
  int source;
public:
  tta_dist(bool, long*, double*, int, stochastic_lldsm*, 
           stateset* a, const stateset* t);
protected:
  virtual ~tta_dist();
public:
  virtual bool Print(OutputStream  &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;

protected:
  inline void getAccept(shared_state* s) const {
    DCASSERT(s);
    s->set(state_index, 1+num_states);
  }
  inline void getTrap(shared_state* s) const {
    DCASSERT(s);
    DCASSERT(chain);
    s->set(state_index, 2+num_states);
  }
  inline void uniqueify(shared_state* s) const {
    DCASSERT(s);
    long x = s->get(state_index);
    if (accept->getExplicit().contains(x)) {
      getAccept(s);
      return;
    }
    if (final->getExplicit().contains(x)) {
      getTrap(s);
      return;
    }
  };
};

tta_dist
:: tta_dist(bool d, long* i, double* v, int l, stochastic_lldsm* mc, 
  stateset* a, const stateset* t) : phase_dist(d)
{
  chain = mc;
  accept = Share(a);

  DCASSERT(chain);
  DCASSERT(accept);

  num_states = chain->getNumStates(false);
  init_index = i;
  init_val = v;
  init_len = l;

  to_states = 0;
  weights = 0;
  out_alloc = 0;

  // accumulated initial probs, for simulation
  init_acc = new double[init_len+1];
  init_acc[0] = 0;
  for (int i=0; i<init_len; i++) {
    init_acc[i+1] = init_acc[i] + init_val[i];
  }

  // Initialize the final states
  intset* f = new intset(num_states);
  final = new stateset(mc, f);

  // final := trap
  if (t) {
    final->changeExplicit().assignFrom( t->getExplicit() );
  } else {
    final->changeExplicit().removeAll();
  }

  // Add to final states: ! E (!final) U accept

  // (1) invert final
  final->changeExplicit().complement();

  // (2) call EU engine to determine E (!final) U accept
  result answer;
  traverse_data x(traverse_data::Compute);
  x.answer = &answer;
  result pass[3];
  pass[0].setBool(false);
  pass[1].setPtr(Share(final));
  pass[2].setPtr(Share(accept));
  engtype* eu = em->findEngineType("ExplicitEU");  
  DCASSERT(eu); 
  eu->runEngine(pass, 3, x);
  pass[1].setPtr(0);
  pass[2].setPtr(0);
  // (3) final is still inverted, intersect with E (!final) U accept 
  stateset* good = smart_cast <stateset*> (answer.getPtr());
  DCASSERT(good);
  final->changeExplicit() *= good->getExplicit();
  // (4) invert again
  // final is now: ! (!final and good) = final or !good
  final->changeExplicit().complement();

  // Now, add the accepting states
  final->changeExplicit() += accept->getExplicit();
}

tta_dist::~tta_dist()
{
  delete[] init_index;
  delete[] init_val;
  delete[] init_acc;
  Delete(chain);
  Delete(accept);
  Delete(final);
  free(to_states);
  free(weights);
}

bool tta_dist::Print(OutputStream &s, int) const
{
  DCASSERT(chain);
  const hldsm* p = chain->GetParent();
  if (p)  s << "tta(" << p->Name() << ", ...)";
  else    s << "tta(...)";
  return true;
}

void tta_dist::Sample(traverse_data &x)
{
  // Determine start state
  DCASSERT(x.stream);
  double u = x.stream->Uniform32();
  int low = 0;
  int high = init_len;
  while (high-low>1) {
    int mid = (low+high)/2;
    if (u < init_acc[mid])  high = mid;
    else                    low = mid;
  }
  long state = init_index[low];
  const long timeout = 2000000000;
  if (isDiscrete()) {
    long dt;
    bool ok = chain->randomTTA(*x.stream, state, *final, timeout, dt);
    if (!ok) {
      x.answer->setNull();
      return;
    }
    if (accept->getExplicit().contains(state)) {
      x.answer->setInt(dt);
    } else {
      x.answer->setInfinity(1);
    }
  } else {
    double ct;
    bool ok = chain->randomTTA(*x.stream, state, *final, timeout, ct);
    if (!ok) {
      x.answer->setNull();
      return;
    }
    if (accept->getExplicit().contains(state)) {
      x.answer->setReal(ct);
    } else {
      x.answer->setInfinity(1);
    }
  }
}

void tta_dist::getInitialState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, num_states);
}

void tta_dist::getAcceptingState(shared_state* s) const
{
  getAccept(s);
}

void tta_dist::getTrapState(shared_state* s) const
{
  getTrap(s);
}

bool tta_dist::isVanishingState(const shared_state* s) const
{
  DCASSERT(s);
  DCASSERT(chain);
  return (s->get(state_index) == num_states);
}

bool tta_dist::isAcceptingState(const shared_state* s) const
{
  DCASSERT(s);
  DCASSERT(chain);
  return (s->get(state_index) == num_states+1);
}

bool tta_dist::isTrapState(const shared_state* s) const
{
  DCASSERT(s);
  DCASSERT(chain);
  return (s->get(state_index) == num_states+2);
}

long tta_dist::setSourceState(const shared_state* s)
{
  source = s->get(state_index);
  if (num_states == source) {
    // vanishing initial state
    return init_len;
  }
  if (source > num_states) {
    return 1; 
    // trap or accept
  }
  long foo = chain->getOutgoingWeights(source, to_states, weights, out_alloc);
  DCASSERT(foo>=0);
  if (foo > out_alloc) {
    long* nt = (long*) realloc(to_states, foo*sizeof(long));
    if (0==nt) return -1;
    to_states = nt;
    double* w = (double*) realloc(weights, foo*sizeof(double));
    if (0==w) return -1;
    weights = w;
    out_alloc = foo;
    chain->getOutgoingWeights(source, to_states, weights, out_alloc);
  }
  return foo;
}

void tta_dist::getSourceState(shared_state* s) const
{
  s->set(state_index, source);
}

double tta_dist::getOutgoingFromSource(long e, shared_state* t) const
{
  if (num_states == source) {
    // vanishing initial state
    CHECK_RANGE(0, e, init_len);
    t->set(state_index, init_index[e]);
    uniqueify(t);
    return init_val[e];
  }
  if (source > num_states) {
    t->set(state_index, source);
    return 1.0;
  }
  CHECK_RANGE(0, e, out_alloc);
  t->set(state_index, to_states[e]);
  uniqueify(t);
  return weights[e];
}

// ******************************************************************
// *                                                                *
// *                      inf_or_zero_ph class                      *
// *                                                                *
// ******************************************************************

/** Infinity or zero, as a discrete or continuous phase type.
    State 0: accepting
    State 1: trap
*/
class inf_or_zero_ph : public phase_dist {
  bool infty;
  int src;
public:
  inf_or_zero_ph(bool discrete, bool inf);
  virtual bool Print(OutputStream  &s, int) const;

  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

inf_or_zero_ph::inf_or_zero_ph(bool d, bool inf) : phase_dist(d)
{
  infty = inf;
}

bool inf_or_zero_ph::Print(OutputStream  &s, int) const
{
  if (infty)   s << type::getInfinityString();
  else         s << 0;
  return true;
}

void inf_or_zero_ph::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  if (infty)              x.answer->setInfinity(1);
  else if (isDiscrete())  x.answer->setInt(0);
  else                    x.answer->setReal(0.0);
}

void inf_or_zero_ph::getInitialState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, infty ? 1 : 0);
}

void inf_or_zero_ph::getAcceptingState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, 0);
}

void inf_or_zero_ph::getTrapState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, 1);
}

bool inf_or_zero_ph::isVanishingState(const shared_state*) const
{
  return false;
}

bool inf_or_zero_ph::isAcceptingState(const shared_state* s) const
{
  return s->get(state_index) == 0;
}

bool inf_or_zero_ph::isTrapState(const shared_state* s) const
{
  return s->get(state_index) == 1;
}

long inf_or_zero_ph::setSourceState(const shared_state* s)
{
  src = s->get(state_index);
  return 1;
}

void inf_or_zero_ph::getSourceState(shared_state* s) const
{
  s->set(state_index, src);
}

double inf_or_zero_ph::getOutgoingFromSource(long e, shared_state* t) const
{
  DCASSERT(t);
  t->set(state_index, src);
  return 1.0;
}

// ******************************************************************
// *                                                                *
// *                      geometric_dph  class                      *
// *                                                                *
// ******************************************************************

/** Geometric(p), as a discrete phase type.
      State 3: unused trap state
      State 2: accepting state
      State 1: 
      State 0: initial (vanishing) state
*/
class geometric_dph : public phase_dist {
  double p;
  int source;
public:
  geometric_dph(double _p);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

geometric_dph::geometric_dph(double _p) : phase_dist(true)
{
  p = _p;
}

bool geometric_dph::Print(OutputStream  &s, int) const
{
  s << "Geometric(" << p << ")";
  return true;
}

void geometric_dph::Sample(traverse_data &x)
{
  generateGeometric(p, x);
}

void geometric_dph::getInitialState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, 0);
}

void geometric_dph::getAcceptingState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, 2);
}

void geometric_dph::getTrapState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, 3);
}

bool geometric_dph::isVanishingState(const shared_state* s) const
{
  DCASSERT(s);
  return 0 == s->get(state_index);
}

bool geometric_dph::isAcceptingState(const shared_state* s) const
{
  DCASSERT(s);
  return 2 == s->get(state_index);
}

bool geometric_dph::isTrapState(const shared_state* s) const
{
  DCASSERT(s);
  return 3 == s->get(state_index);
}

long geometric_dph::setSourceState(const shared_state* s)
{
  DCASSERT(s);
  source = s->get(state_index);
  if (source>1) return 1;
  return 2;
}

void geometric_dph::getSourceState(shared_state* s) const
{
  s->set(state_index, source);
}

double geometric_dph::getOutgoingFromSource(long e, shared_state* t) const
{
  DCASSERT(t);
  if (source>1) {
    t->set(state_index, source);
    return 1.0;
  }
  if (e) {
    t->set(state_index, 2);
    return 1-p;
  }
  t->set(state_index, 1);
  return p;
}


// ******************************************************************
// *                                                                *
// *                        finite_dph class                        *
// *                                                                *
// ******************************************************************

/** Implicit discrete phase representation for an arbitrary,
    finite range discrete random variable.
    The range must be {a, a+1, ..., b} where 0 <= a <= b < infinity.
    State 0/1: initial state, depending on a.
    State b+1: accepting state
    State b+2: unused trap state
*/
class finite_dph : public phase_dist {
  int source;
protected:
  /// Maximum value of the random variable.
  long b;
public:
  finite_dph(long b_);
protected:
  virtual ~finite_dph();
public:
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;

  /** pdf of the distribution.
      Must be provided in derived classes.
      Must return the correct value for all integers in the range 0..b.
  */
  virtual double pdf(long x) const = 0;

  /** One minus the cdf of the distribution.
      Must be provided in derived classes.
      Must return the correct value for all integers in the range 0..b.
  */
  virtual double one_minus_cdf(long x) const = 0;
};

// ******************************************************************
// *                       finite_dph methods                       *
// ******************************************************************

finite_dph::finite_dph(long _b) : phase_dist(true)
{
  b = _b;
  DCASSERT(b>=0);
}

finite_dph::~finite_dph()
{
}

void finite_dph::getInitialState(shared_state* s) const
{
  DCASSERT(s);
  if (pdf(0))   s->set(state_index, 0);
  else          s->set(state_index, 1);
}

void finite_dph::getAcceptingState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, b+1);
}

void finite_dph::getTrapState(shared_state* s) const
{
  DCASSERT(s);
  s->set(state_index, b+2);
}

bool finite_dph::isVanishingState(const shared_state* s) const
{
  DCASSERT(s);
  return 0==s->get(state_index);
}

bool finite_dph::isAcceptingState(const shared_state* s) const
{
  DCASSERT(s);
  return b+1 == s->get(state_index);
}

bool finite_dph::isTrapState(const shared_state* s) const
{
  DCASSERT(s);
  return b+2 == s->get(state_index);
}

long finite_dph::setSourceState(const shared_state* s)
{
  DCASSERT(s);
  source = s->get(state_index);
  if (source>b) return 1;
  return 2;
}

void finite_dph::getSourceState(shared_state* s) const
{
  s->set(state_index, source);
}

double finite_dph::getOutgoingFromSource(long e, shared_state* t) const
{
  DCASSERT(t);
  if (source>b) {
    t->set(state_index, source);
    return 1.0;
  }
  double denom = source ? one_minus_cdf(source-1) : 1.0;
  if (e) {
    t->set(state_index, b+1);
    return pdf(source) / denom;
  } 
  t->set(state_index, source+1);
  return one_minus_cdf(source) / denom;
}

// ******************************************************************
// *                                                                *
// *                      bernoulli_dph  class                      *
// *                                                                *
// ******************************************************************

/** Bernoulli, as a phase type.
    Returns 1 with probability p, 0 with probability 1-p.
*/
class bernoulli_dph : public finite_dph {
  double p;
public:
  bernoulli_dph(double param);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual double pdf(long x) const;
  virtual double one_minus_cdf(long x) const;
};

bernoulli_dph::bernoulli_dph(double param) : finite_dph(1) 
{
  p = param;
}

bool bernoulli_dph::Print(OutputStream  &s, int) const
{
  s << "Bernoulli(" << p << ")";
  return true;
}

void bernoulli_dph::Sample(traverse_data &x)
{
  generateBernoulli(p, x);
}

double bernoulli_dph::pdf(long x) const 
{
  if (0==x) return 1-p;
  if (1==x) return p;
  return 0.0;
}

double bernoulli_dph::one_minus_cdf(long x) const
{
  if (0==x) return p;
  if (x>0) return 0.0;
  return 1.0;
}

// ******************************************************************
// *                                                                *
// *                      equilikely_dph class                      *
// *                                                                *
// ******************************************************************

/** Equilikely, as a phase type.
    Returns 1 with probability p, 0 with probability 1-p.
*/
class equilikely_dph : public finite_dph {
  long a;
public:
  equilikely_dph(long a, long b);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual double pdf(long x) const;
  virtual double one_minus_cdf(long x) const;
};

equilikely_dph::equilikely_dph(long _a, long b) : finite_dph(b) 
{
  a = _a;
  DCASSERT(a>=0);
  DCASSERT(a<=b);
}

bool equilikely_dph::Print(OutputStream  &s, int) const
{
  if (a==b) s << a;
  else      s << "Equilikely(" << a << ", " << b << ")";
  return true;
}

void equilikely_dph::Sample(traverse_data &x)
{
  generateEquilikely(a, b, x);
}

double equilikely_dph::pdf(long x) const 
{
  if (x<a)  return 0.0;
  if (x<=b) return 1.0 / (b-a+1.0);
  return 0.0;
}

double equilikely_dph::one_minus_cdf(long x) const
{
  if (x<a)  return 1.0;
  if (x<=b) return (b-x)/(b-a+1.0);
  return 0.0;
}

// ******************************************************************
// *                                                                *
// *                       binomial_dph class                       *
// *                                                                *
// ******************************************************************

/** Binomial, as a phase type.
*/
class binomial_dph : public finite_dph {
  double p;
  double* thePdf;
  double* oneMinusCdf;
public:
  binomial_dph(long n, double p);
  virtual ~binomial_dph();
  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual double pdf(long x) const;
  virtual double one_minus_cdf(long x) const;
};

binomial_dph::binomial_dph(long n, double _p) : finite_dph(n) 
{
  p = _p;
  DCASSERT(p>0);
  DCASSERT(p<1);

  thePdf = 0;
  oneMinusCdf = 0;

  //
  // Build the pdf
  //
  //  f(i) = C(n, i) * p^i * (1-p)^{n-1}
  //
  
  // But we use a trick:
  // Divide everything by f(0);
  // when we normalize at the end, everything works out

  thePdf = new double[n+1];
  thePdf[0] = 1;
  long nm = n;
  double pd1mp = p / (1-p);
  double total = 1;
  for (long i=1; i<=n; i++) {
    thePdf[i] = thePdf[i-1] * pd1mp * nm;
    thePdf[i] /= i;
    total += thePdf[i];
    nm--;
  }
  for (long i=0; i<=n; i++) thePdf[i] /= total;
  
  // Now build the cdf
  oneMinusCdf = new double[n+1];
  oneMinusCdf[n] = 0;
  for (long i=n-1; i>=0; i--) {
    oneMinusCdf[i] = oneMinusCdf[i+1] + thePdf[i+1];
  }
}

binomial_dph::~binomial_dph()
{
  delete[] thePdf;
  delete[] oneMinusCdf;
}

bool binomial_dph::Print(OutputStream  &s, int) const
{
  s << "Binomial(" << b << ", " << p << ")";
  return true;
}

void binomial_dph::Sample(traverse_data &x)
{
  generateBinomial(b, p, x);
}

double binomial_dph::pdf(long x) const 
{
  if (x<0) return 0;
  if (x>b) return 0;
  return thePdf[x];
}

double binomial_dph::one_minus_cdf(long x) const
{
  if (x<0) return 1;
  if (x>b) return 0;
  return oneMinusCdf[x];
}

// ******************************************************************
// *                                                                *
// *                        erlang_cph class                        *
// *                                                                *
// ******************************************************************

/** Erlang, as a phase type.
    State 0: initial state (vanishing)
    State n: accepting state
    State n+1: trap state
*/
class erlang_cph : public phase_dist {
  long n;
  double lambda;
  long source;
public:
  erlang_cph(long N, double L);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

erlang_cph::erlang_cph(long N, double L) : phase_dist(false)
{
  DCASSERT(N>=0);
  n = N;
  lambda = L;
}

bool erlang_cph::Print(OutputStream &s, int) const
{
  switch (n) {
    case 0:
      s.Put('0'); 
      break;

    case 1:
      s << "expo(" << lambda << ")";
      break;

    default:
      s << "erlang(" << n << ", " << lambda << ")";
  }
  return true;
}

void erlang_cph::Sample(traverse_data &x)
{
  if (n>1) {
    generateErlang(n, lambda, x);
    return;
  }
  if (n) {
    generateExpo(lambda, x);
    return;
  }
  DCASSERT(x.answer);
  x.answer->setReal(0.0);
}

void erlang_cph::getInitialState(shared_state* s) const 
{ 
  DCASSERT(s);
  s->set(state_index, 0);
}

void erlang_cph::getAcceptingState(shared_state* s) const 
{ 
  DCASSERT(s);
  s->set(state_index, n);
}

void erlang_cph::getTrapState(shared_state* s) const 
{ 
  DCASSERT(s);
  s->set(state_index, n+1);
}

bool erlang_cph::isVanishingState(const shared_state* s) const 
{ 
  return false; 
}
  
bool erlang_cph::isAcceptingState(const shared_state* s) const 
{ 
  DCASSERT(s);
  return s->get(state_index) == n;
}

bool erlang_cph::isTrapState(const shared_state* s) const 
{ 
  DCASSERT(s);
  return s->get(state_index) == n+1;
}

long erlang_cph::setSourceState(const shared_state* s) 
{
  DCASSERT(s);
  source = s->get(state_index);
  if (source>=n) return 0;
  return 1;
}

void erlang_cph::getSourceState(shared_state* s) const
{
  s->set(state_index, source);
}

double erlang_cph::getOutgoingFromSource(long e, shared_state* t) const 
{
  DCASSERT(t);
  DCASSERT(source<n);
  DCASSERT(0==e);
  t->set(state_index, source+1);
  return lambda;
}

// ******************************************************************
// *                                                                *
// *                       phase_cross  class                       *
// *                                                                *
// ******************************************************************

/** Abstract base class for hierarchical phase-type models.
    Use for models whose state space is the cross product
    of component state spaces.
      Initial state:  [i, i, i, ...]
      Accept state:   [a, a, a, ...]
      Trap state:     [t, t, t, ...]
*/
class phase_cross : public phase_hlm {
  /// Accumulated number of state vars; size is num_opnds+1.
  int* total_svs;
protected:
  /// The operands.
  phase_hlm** opnds;
  /// Number of operands.
  int num_opnds;
public:
  phase_cross(phase_hlm** the_opnds, int N);
protected:
  virtual ~phase_cross();
public:
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool*) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;

  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
protected:
  inline void makeInitial(shared_state* s) const {
    for (int i=0; i<num_opnds; i++) opnds[i]->getInitialState(s);
  }
  inline void makeTrap(shared_state* s) const {
    for (int i=0; i<num_opnds; i++) opnds[i]->getTrapState(s);
  }
  inline void makeAccept(shared_state* s) const {
    for (int i=0; i<num_opnds; i++) opnds[i]->getAcceptingState(s);
  }
};

phase_cross::phase_cross(phase_hlm** A, int N) : phase_hlm(A[0]->isDiscrete())
{
  opnds = A;
  num_opnds = N;
  total_svs = new int[N+1];
  total_svs[0] = 0;
  for (int i=0; i<N; i++) {
    total_svs[i+1] = total_svs[i] + A[i]->NumStateVars();
  }
}

phase_cross::~phase_cross()
{
  for (int i=0; i<num_opnds; i++) Delete(opnds[i]);
  delete[] opnds;
}

int phase_cross::NumStateVars() const
{
  return total_svs[num_opnds];
}

bool phase_cross::containsListVar() const 
{
  for (int i=0; i<num_opnds; i++)
    if (opnds[i]->containsListVar()) return true;
  return false;
}

void phase_cross::determineListVars(bool* ilv) const
{
  for (int i=0; i<num_opnds; i++) 
    opnds[i]->determineListVars(ilv + total_svs[i]);
}

void phase_cross::reindexStateVars(int &start)
{
  for (int i=0; i<num_opnds; i++)
    opnds[i]->reindexStateVars(start);
}

void phase_cross::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(x);
  s.Put('[');
  for (int i=0; i<num_opnds; i++) {
    if (i) s << ", ";
    opnds[i]->showState(s, x);
  }
  s.Put(']');
}

void phase_cross::getInitialState(shared_state* s) const
{
  makeInitial(s);
}

void phase_cross::getAcceptingState(shared_state* s) const
{
  makeAccept(s);
}

void phase_cross::getTrapState(shared_state* s) const
{
  makeTrap(s);
}

// ******************************************************************
// *                                                                *
// *                      phase_addition class                      *
// *                                                                *
// ******************************************************************

/** Class for constructing phase-type addition.
    We use a cross-product of states and "chain" them together.
    Note - because sampling is slightly different
    for discrete / continuous, this is an abstract base class.
*/
class phase_addition : public phase_cross {
  /// Active component; used by setSourceState, getOutgoingFromSource.
  int active;
  /// Source state is the trap state.
  bool from_trap;
public:
  phase_addition(phase_hlm** the_opnds, int N);
public:
  virtual bool Print(OutputStream &s, int) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual void getInitialState(shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

phase_addition::phase_addition(phase_hlm** A, int N) : phase_cross(A, N)
{
}

bool phase_addition::Print(OutputStream &s, int) const
{
  s.Put('(');
  for (int i=0; i<num_opnds; i++) {
    if (i) s << " + ";
    opnds[i]->Print(s, 0);
  }
  s.Put(')');
  return true;
}

bool phase_addition::isVanishingState(const shared_state* s) const
{
  int i;
  for (i=0; i<num_opnds; i++) {
    if (opnds[i]->isAcceptingState(s)) continue;
    // component i is the active one
    return opnds[i]->isVanishingState(s);
  }
  return false;  // must be the accepting state
}

bool phase_addition::isAcceptingState(const shared_state* s) const
{
#ifdef DEVELOPMENT_CODE
  if (opnds[num_opnds-1]->isAcceptingState(s)) {
    for (int i=0; i<num_opnds; i++) {
      DCASSERT(opnds[i]->isAcceptingState(s));
    }
  }
#endif
  return opnds[num_opnds-1]->isAcceptingState(s);
}

bool phase_addition::isTrapState(const shared_state* s) const
{
#ifdef DEVELOPMENT_CODE
  if (opnds[0]->isTrapState(s)) {
    for (int i=0; i<num_opnds; i++) {
      DCASSERT(opnds[i]->isTrapState(s));
    }
  }
#endif
  return opnds[0]->isTrapState(s);
}

void phase_addition::getInitialState(shared_state* s) const
{
  makeInitial(s);
  for (int i=0; i<num_opnds; i++) {
    if (opnds[i]->isTrapState(s)) {
      makeTrap(s);
      return;
    }
  }
}

long phase_addition::setSourceState(const shared_state* s)
{
  if (opnds[0]->isTrapState(s)) {
    from_trap = true;
    return opnds[0]->setSourceState(s);
  }
  from_trap = false;
  for (int i=0; i<num_opnds; i++) {
    if (opnds[i]->isAcceptingState(s)) continue;
    active = i;
    return opnds[i]->setSourceState(s);
  } // for i
  active = num_opnds-1;
  return opnds[num_opnds-1]->setSourceState(s);
}

void phase_addition::getSourceState(shared_state* s) const
{
  if (from_trap) {
    for (int i=0; i<num_opnds; i++) opnds[i]->getTrapState(s);
    return;
  }
  for (int i=0; i<active; i++)  opnds[i]->getAcceptingState(s);
  opnds[active]->getSourceState(s);
  for (int i=active+1; i<num_opnds; i++) opnds[i]->getInitialState(s);
}

double phase_addition::getOutgoingFromSource(long e, shared_state* t) const
{
  if (from_trap) {
    for (int i=1; i<num_opnds; i++) opnds[i]->getTrapState(t);
    return opnds[0]->getOutgoingFromSource(e, t);
  }
  for (int i=0; i<active; i++) opnds[i]->getAcceptingState(t);
  for (int i=active+1; i<num_opnds; i++) opnds[i]->getInitialState(t);
  double val = opnds[active]->getOutgoingFromSource(e, t);
  if (opnds[active]->isTrapState(t)) {
    makeTrap(t);
  }
  return val;
}

// ******************************************************************
// *                                                                *
// *                      phint_addition class                      *
// *                                                                *
// ******************************************************************

/** Class for constructing phase int addition.
*/
class phint_addition : public phase_addition {
public:
  phint_addition(phase_hlm** the_opnds, int N);
public:
  virtual void Sample(traverse_data &x);
};

phint_addition::phint_addition(phase_hlm** A, int N) : phase_addition(A, N)
{
}

void phint_addition::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  long total = 0;
  for (int i=0; i<num_opnds; i++) {
    opnds[i]->Sample(x);
    if (x.answer->isNormal()) {
      total += x.answer->getInt();
      continue;
    }
    return;  
  }
  x.answer->setInt(total);
}


// ******************************************************************
// *                                                                *
// *                     phreal_addition  class                     *
// *                                                                *
// ******************************************************************

/** Class for constructing phase real addition.
*/
class phreal_addition : public phase_addition {
public:
  phreal_addition(phase_hlm** the_opnds, int N);
public:
  virtual void Sample(traverse_data &x);
};

phreal_addition::phreal_addition(phase_hlm** A, int N) : phase_addition(A, N)
{
}

void phreal_addition::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  double total = 0.0;
  for (int i=0; i<num_opnds; i++) {
    opnds[i]->Sample(x);
    if (x.answer->isNormal()) {
      total += x.answer->getReal();
      continue;
    }
    return;  
  }
  x.answer->setReal(total);
}


// ******************************************************************
// *                                                                *
// *                       dph_multiply class                       *
// *                                                                *
// ******************************************************************

/** Class for constructing discrete phase-type multiplication.
    The model adds a new state variable to use N clock ticks
    for each tangible state.

    Initial state:  [i, N], if i is tangible;
                    [i, 0], if i is vanishing.

    Accept state:   [i, 1]
    Trap state:     [t, 1]

*/
class dph_multiply : public phase_hlm {
  /// Original model
  phase_hlm* original;

  /// Scaling factor
  long N;

  /// index of our counting state variable
  int state_index;

  /// Our source state variable
  int src;
public:
  dph_multiply(phase_hlm* orig, int n);
protected:
  virtual ~dph_multiply();
public:
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool*) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;

  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;

  inline void startInState(shared_state* t) const {
      if (original->isVanishingState(t)) {
        t->set(state_index, 0);
        return;
      }
      if (original->isAcceptingState(t) || original->isTrapState(t)) {
        t->set(state_index, 1);
        return;
      }
      t->set(state_index, N);
  };
};


dph_multiply::dph_multiply(phase_hlm* orig, int n) : phase_hlm(true)
{
  original = orig;
  N = n;
  state_index = 0;
}

dph_multiply::~dph_multiply()
{
  Delete(original);
}

int dph_multiply::NumStateVars() const
{
  return 1 + original->NumStateVars();
}

bool dph_multiply::containsListVar() const 
{
  return original->containsListVar();
}

void dph_multiply::determineListVars(bool* ilv) const
{
  original->determineListVars(ilv);
  ilv[original->NumStateVars()] = false;
}

void dph_multiply::reindexStateVars(int &start)
{
  original->reindexStateVars(start);
  state_index = start;
  start++;
}

void dph_multiply::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(x);
  s.Put('[');
  original->showState(s, x);
  s << ", " << x->get(state_index);
  s.Put(']');
}

bool dph_multiply::Print(OutputStream &s, int) const
{
  s << N << "*";
  original->Print(s, 0);
  return true;
}

void dph_multiply::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  original->Sample(x);
  if (x.answer->isNormal()) {
    x.answer->setInt( x.answer->getInt() * N );
  }
}

void dph_multiply::getInitialState(shared_state* s) const
{
  original->getInitialState(s);
  startInState(s);
}

void dph_multiply::getAcceptingState(shared_state* s) const
{
  original->getAcceptingState(s);
  startInState(s);
}

void dph_multiply::getTrapState(shared_state* s) const
{
  original->getTrapState(s);
  startInState(s);
}

bool dph_multiply::isVanishingState(const shared_state* s) const
{
  // nice, if it works...
  return (0==s->get(state_index));
}

bool dph_multiply::isAcceptingState(const shared_state* s) const
{
  return original->isAcceptingState(s);
}

bool dph_multiply::isTrapState(const shared_state* s) const
{
  return original->isTrapState(s);
}

long dph_multiply::setSourceState(const shared_state* s)
{
  src = s->get(state_index);
  long arcs = original->setSourceState(s);
  return (src>1) ? 1 : arcs;
}

void dph_multiply::getSourceState(shared_state* s) const
{
  s->set(state_index, src);
  original->getSourceState(s);
}

double dph_multiply::getOutgoingFromSource(long e, shared_state* t) const
{
  if (src>1) {
    t->set(state_index, src-1);
    original->getSourceState(t);
    return 1.0;
  }
  double v = original->getOutgoingFromSource(e, t);
  startInState(t);
  return v;
}


// ******************************************************************
// *                                                                *
// *                       cph_multiply class                       *
// *                                                                *
// ******************************************************************

/** Class for constructing continuous phase-type multiplication.
    Pretty easy: all outgoing rates from tangible states are
    divided by the appropriate factor; states are exactly the same.
*/
class cph_multiply : public phase_hlm {
  /// Original model
  phase_hlm* original;
  /// Scaling factor
  double R;
  /// Is the source state vanishing?
  bool src_is_vanishing;
public:
  cph_multiply(phase_hlm* orig, double r);
protected:
  virtual ~cph_multiply();
public:
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool*) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;

  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};


cph_multiply::cph_multiply(phase_hlm* orig, double r) : phase_hlm(false)
{
  original = orig;
  R = r;
}

cph_multiply::~cph_multiply()
{
  Delete(original);
}

int cph_multiply::NumStateVars() const
{
  return original->NumStateVars();
}

bool cph_multiply::containsListVar() const 
{
  return original->containsListVar();
}

void cph_multiply::determineListVars(bool* ilv) const
{
  original->determineListVars(ilv);
}

void cph_multiply::reindexStateVars(int &start)
{
  original->reindexStateVars(start);
}

void cph_multiply::showState(OutputStream &s, const shared_state* x) const
{
  original->showState(s, x);
}

bool cph_multiply::Print(OutputStream &s, int) const
{
  s << R << "*";
  original->Print(s, 0);
  return true;
}

void cph_multiply::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  original->Sample(x);
  if (x.answer->isNormal()) {
    x.answer->setReal( x.answer->getReal() * R );
  }
}

void cph_multiply::getInitialState(shared_state* s) const
{
  original->getInitialState(s);
}

void cph_multiply::getAcceptingState(shared_state* s) const
{
  original->getAcceptingState(s);
}

void cph_multiply::getTrapState(shared_state* s) const
{
  original->getTrapState(s);
}

bool cph_multiply::isVanishingState(const shared_state* s) const
{
  return original->isVanishingState(s);
}

bool cph_multiply::isAcceptingState(const shared_state* s) const
{
  return original->isAcceptingState(s);
}

bool cph_multiply::isTrapState(const shared_state* s) const
{
  return original->isTrapState(s);
}

long cph_multiply::setSourceState(const shared_state* s)
{
  src_is_vanishing = original->isVanishingState(s);
  return original->setSourceState(s);
}

void cph_multiply::getSourceState(shared_state* s) const
{
  original->getSourceState(s);
}

double cph_multiply::getOutgoingFromSource(long e, shared_state* t) const
{
  if (src_is_vanishing) return original->getOutgoingFromSource(e, t);
  return original->getOutgoingFromSource(e, t) / R;
}


// ******************************************************************
// *                                                                *
// *                       phase_choice class                       *
// *                                                                *
// ******************************************************************

/** Class for constructing phase-type choice.
    We have our own state variable that tells which one was chosen,
    plus a concatenation of state variables of all submodels.
   
    Initial state:    [0, i, i, i...]
    "middle" states:  [c, a, ..., a, s, a, ..., a]
    Accept state:     [n+1, a, a, a...]
    Trap state:       [n+2, t, t, t...]
*/
class phase_choice : public phase_hlm {
  /// The operands.
  phase_hlm** opnds;
  /// The choice probabilities.
  double* prob;
  /// Accumulated choice probabilities, for sampling.
  double* acc_prob;
  /// Number of operands.
  int num_opnds;
  /// Index for our state variable
  int state_index;
  /// Accumulated number of state vars; size is num_opnds+1.
  int* total_svs;
  /// Source state (ours)
  int source;
public:
  phase_choice(phase_hlm** the_opnds, double* p, int N);
protected:
  virtual ~phase_choice();
public:
  virtual int NumStateVars() const;
  virtual bool containsListVar() const;
  virtual void determineListVars(bool*) const;
  virtual void reindexStateVars(int &start);
  virtual void showState(OutputStream &s, const shared_state* x) const;

  virtual bool Print(OutputStream &s, int) const;
  virtual void Sample(traverse_data &x);
  virtual void getInitialState(shared_state* s) const;
  virtual void getAcceptingState(shared_state* s) const;
  virtual void getTrapState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual long setSourceState(const shared_state* s);
  virtual void getSourceState(shared_state* s) const;
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
protected:
  inline void makeTrap(shared_state* s) const {
    s->set(state_index, num_opnds+2);
    for (int i=0; i<num_opnds; i++) opnds[i]->getTrapState(s);
  }
  inline void makeAccept(shared_state* s) const {
    s->set(state_index, num_opnds+1);
    for (int i=0; i<num_opnds; i++) opnds[i]->getAcceptingState(s);
  }
  inline void uniqueifyState(shared_state* s) const {
    int num_accept = 0;
    int num_trap = 0;
    for (int i=0; i<num_opnds; i++) {
      if (opnds[i]->isAcceptingState(s))  num_accept++;
      if (opnds[i]->isTrapState(s))       num_trap++;
    }
    if (num_accept == num_opnds) {
      makeAccept(s);
      return;
    }
    if (num_trap) {
      makeTrap(s);
      return;
    }
  }
};

phase_choice::phase_choice(phase_hlm** A, double* p, int N)
: phase_hlm(A[0]->isDiscrete())
{
  opnds = A;
  prob = p;
  num_opnds = N;
  double total = 0.0;
  for (int i=0; i<N; i++) total += prob[i];
  DCASSERT(total>0);
  acc_prob = new double[N+1];
  acc_prob[0] = 0.0;
  for (int i=0; i<N; i++) {
    prob[i] /= total;
    acc_prob[i+1] = acc_prob[i] + prob[i];
  }

  state_index = 0;
  total_svs = new int[N+1];
  total_svs[0] = 1;
  for (int i=0; i<N; i++) {
    total_svs[i+1] = total_svs[i] + A[i]->NumStateVars();
  }
}

phase_choice::~phase_choice()
{
  for (int i=0; i<num_opnds; i++) Delete(opnds[i]);
  delete[] opnds;
  delete[] prob;
  delete[] acc_prob;
}

int phase_choice::NumStateVars() const
{
  return total_svs[num_opnds];
}

bool phase_choice::containsListVar() const 
{
  for (int i=0; i<num_opnds; i++)
    if (opnds[i]->containsListVar()) return true;
  return false;
}

void phase_choice::determineListVars(bool* ilv) const
{
  ilv[0] = false;
  for (int i=0; i<num_opnds; i++) 
    opnds[i]->determineListVars(ilv + total_svs[i]);
}

void phase_choice::reindexStateVars(int &start)
{
  state_index = start;
  start++;
  for (int i=0; i<num_opnds; i++)
    opnds[i]->reindexStateVars(start);
}

void phase_choice::showState(OutputStream &s, const shared_state* x) const
{
  DCASSERT(x);
  s << "[" << x->get(state_index);
  for (int i=0; i<num_opnds; i++) {
    s << ", ";
    opnds[i]->showState(s, x);
  }
  s.Put(']');
}

bool phase_choice::Print(OutputStream &s, int) const
{
  s << "choose(";
  s.Put('(');
  for (int i=0; i<num_opnds; i++) {
    if (i) s << ", ";
    opnds[i]->Print(s, 0);
    s << " : " << prob[i];
  }
  s.Put(')');
  return true;
}

void phase_choice::Sample(traverse_data &x)
{
  DCASSERT(x.stream);
  double u = x.stream->Uniform32();
  int low = 0;
  int high = num_opnds;
  while (high-low>1) {
    int mid = (low+high)/2;
    if (u < acc_prob[mid])  high = mid;
    else                    low = mid;
  }
  opnds[low]->Sample(x);
}

void phase_choice::getInitialState(shared_state* s) const
{
  s->set(state_index, 0);
  for (int i=0; i<num_opnds; i++) opnds[i]->getInitialState(s);
}

void phase_choice::getAcceptingState(shared_state* s) const
{
  makeAccept(s);
}

void phase_choice::getTrapState(shared_state* s) const
{
  makeTrap(s);
}

bool phase_choice::isVanishingState(const shared_state* s) const
{
  int ours = s->get(state_index);
  if (0==ours)          return true;    // initial state
  if (ours > num_opnds) return false;   // accept or trap
  ours--;
  CHECK_RANGE(0, ours, num_opnds);
  return opnds[ours]->isVanishingState(s);
}

bool phase_choice::isAcceptingState(const shared_state* s) const
{
  return (s->get(state_index) == 1+num_opnds);
}

bool phase_choice::isTrapState(const shared_state* s) const
{
  return (s->get(state_index) == 2+num_opnds);
}

long phase_choice::setSourceState(const shared_state* s)
{
  source = s->get(state_index);
  if (0==source) return num_opnds;
  if (source > num_opnds) {
    // accept or trap
    if (isDiscrete()) return 1;
    return 0;
  }
  int ours = source-1;
  CHECK_RANGE(0, ours, num_opnds);
  int arcs = opnds[ours]->setSourceState(s);
  return arcs;
}

void phase_choice::getSourceState(shared_state* s) const
{
  s->set(state_index, source);
  if (0==source) {
    for (int i=0; i<num_opnds; i++) opnds[i]->getInitialState(s);
    return;
  }
  if (source == 1+num_opnds) {
    for (int i=0; i<num_opnds; i++) opnds[i]->getAcceptingState(s);
    return;
  }
  if (source == 2+num_opnds) {
    for (int i=0; i<num_opnds; i++) opnds[i]->getTrapState(s);
    return;
  }
  int ours = source-1;
  CHECK_RANGE(0, ours, num_opnds);
  for (int i=0; i<ours; i++)            opnds[i]->getAcceptingState(s);
  for (int i=ours+1; i<num_opnds; i++)  opnds[i]->getAcceptingState(s);
  opnds[ours]->getSourceState(s);
}

double phase_choice::getOutgoingFromSource(long e, shared_state* t) const
{
  if (0==source) {
    t->set(state_index, e+1);
    for (int i=0; i<e; i++)           opnds[i]->getAcceptingState(t);
    for (int i=e+1; i<num_opnds; i++) opnds[i]->getAcceptingState(t);
    opnds[e]->getInitialState(t);
    uniqueifyState(t);
    return prob[e];
  }
  if (source > num_opnds) {
    getSourceState(t);
    return 1.0;
  }
  int ours = source-1;
  CHECK_RANGE(0, ours, num_opnds);
  for (int i=0; i<ours; i++)           opnds[i]->getAcceptingState(t);
  for (int i=ours+1; i<num_opnds; i++) opnds[i]->getAcceptingState(t);
  double v = opnds[ours]->getOutgoingFromSource(e, t);
  t->set(state_index, source);
  uniqueifyState(t);
  return v;
}


// ******************************************************************
// *                                                                *
// *                       phase_order  class                       *
// *                                                                *
// ******************************************************************

/** Base class for phase-type order statistics.
    State is cross product of operand states.
*/
class phase_order : public phase_cross {
  /// The one we want
  int K;
  /// For sampling
  result* samples;
  /// Are samples integers?  Otherwise, they're reals.
  bool samples_are_ints;
protected:
  /// Used by setSourceState, in different ways, in derived classes.
  long* count;
public:
  phase_order(int k, phase_hlm** the_opnds, int N, bool sai);
protected:
  virtual ~phase_order();
public:
  virtual bool Print(OutputStream &s, int) const;
  virtual void getInitialState(shared_state* s) const;
  virtual bool isVanishingState(const shared_state* s) const;
  virtual bool isAcceptingState(const shared_state* s) const;
  virtual bool isTrapState(const shared_state* s) const;
  virtual void getSourceState(shared_state* s) const;

  virtual void Sample(traverse_data &x);

protected:
  inline void uniqueifyState(shared_state* s) const {
    int num_accept = 0;
    int num_trap = 0;
    for (int i=0; i<num_opnds; i++) {
      if (opnds[i]->isAcceptingState(s))  num_accept++;
      if (opnds[i]->isTrapState(s))       num_trap++;
    }
    DCASSERT(num_accept + num_trap <= num_opnds);
    if (num_trap >= num_opnds-K+1) {
      makeTrap(s);
      return;
    }
    if (num_accept >= K) {
      makeAccept(s);
      return;
    }
  }
};

phase_order::phase_order(int k, phase_hlm** A, int N, bool sai) : phase_cross(A, N)
{
  K = k;
  count = new long[N];
  samples = new result[N];
  samples_are_ints = sai;
}

phase_order::~phase_order()
{
  delete[] samples;
  delete[] count;
}

bool phase_order::Print(OutputStream &s, int) const
{
  if (1==K)                 s << "min(";
  else if (num_opnds == K)  s << "max(";
  else                      s << "ord(" << K << ", ";
  for (int i=0; i<num_opnds; i++) {
    if (i) s << ", ";
    opnds[i]->Print(s, 0);
  }
  s.Put(')');
  return true;
}

void phase_order::getInitialState(shared_state* s) const
{
  makeInitial(s);
  uniqueifyState(s);
}

bool phase_order::isVanishingState(const shared_state* s) const
{
  for (int i=0; i<num_opnds; i++)
    if (opnds[i]->isVanishingState(s))  return true;
  return false;
}

bool phase_order::isAcceptingState(const shared_state* s) const
{
  for (int i=0; i<num_opnds; i++)
    if (!opnds[i]->isAcceptingState(s))  return false;
  return true;
}

bool phase_order::isTrapState(const shared_state* s) const
{
  for (int i=0; i<num_opnds; i++)
    if (!opnds[i]->isTrapState(s))  return false;
  return true;
}

void phase_order::getSourceState(shared_state* s) const
{
  for (int i=0; i<num_opnds; i++) opnds[i]->getSourceState(s);
}

void phase_order::Sample(traverse_data &x)
{
  DCASSERT(x.answer);
  result* ans = x.answer;
  //
  // sample all operands
  //
  for (int i=0; i<num_opnds; i++) {
    x.answer = samples+i;
    opnds[i]->Sample(x);
    if (x.answer->isNormal()) continue;
    if (x.answer->isInfinity()) continue;

    // null or other error, propogate it
    *ans = *(x.answer);
    x.answer = ans;
    return;
  } // for i

  //
  // sort the samples
  //
  if (samples_are_ints)
    sortIntegers(samples, num_opnds);
  else
    sortReals(samples, num_opnds);

  //
  // return Kth smallest
  //
  *(x.answer) = samples[K];
}


// ******************************************************************
// *                                                                *
// *                        dph_order  class                        *
// *                                                                *
// ******************************************************************

/** Discrete phase type order statistics.
*/
class dph_order : public phase_order {
public:
  dph_order(int k, phase_hlm** the_opnds, int N);
protected:
  virtual ~dph_order();
public:
  virtual long setSourceState(const shared_state* s);
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

dph_order::dph_order(int k, phase_hlm** A, int N)
 : phase_order(k, A, N, true)
{
}

dph_order::~dph_order()
{
}

long dph_order::setSourceState(const shared_state* s)
{
  bool is_vanishing = isVanishingState(s);
  long arcs = 1;
  for (int i=0; i<num_opnds; i++) {
    count[i] = opnds[i]->setSourceState(s);
    if (is_vanishing && !opnds[i]->isVanishingState(s))   count[i] = 0;
    else                                                  arcs *= count[i];
  } // for i
  return arcs;
}

double dph_order::getOutgoingFromSource(long e, shared_state* t) const
{
  double v = 1.0;
  for (int i=0; i<num_opnds; i++) {
    if (0==count[i]) {
      opnds[i]->getSourceState(t);
      continue;
    }
    long em = e % count[i];
    e /= count[i];
    v *= opnds[i]->getOutgoingFromSource(em, t);
  }
  uniqueifyState(t);
  DCASSERT(0==e);
  return v;
}

// ******************************************************************
// *                                                                *
// *                        cph_order  class                        *
// *                                                                *
// ******************************************************************

/** Continuous phase type order statistics.
*/
class cph_order : public phase_order {
public:
  cph_order(int k, phase_hlm** the_opnds, int N);
protected:
  virtual ~cph_order();
public:
  virtual long setSourceState(const shared_state* s);
  virtual double getOutgoingFromSource(long e, shared_state* t) const;
};

cph_order::cph_order(int k, phase_hlm** A, int N)
 : phase_order(k, A, N, false)
{
}

cph_order::~cph_order()
{
}

long cph_order::setSourceState(const shared_state* s)
{
  bool is_vanishing = isVanishingState(s);
  long arcs = 0;
  for (int i=0; i<num_opnds; i++) {
    count[i] = opnds[i]->setSourceState(s);
    if (is_vanishing) {
      if (!opnds[i]->isVanishingState(s)) count[i] = 0;
    }
    arcs += count[i];
  } // for i
  return arcs;
}

double cph_order::getOutgoingFromSource(long e, shared_state* t) const
{
  int i;
  double v = 0;
  for (i=0; i<num_opnds; i++) {
    if (e >= count[i]) {
      opnds[i]->getSourceState(t);
      e -= count[i];
      continue;
    }
    v = opnds[i]->getOutgoingFromSource(e, t);
    break;
  } // for i
  for (i++; i<num_opnds; i++) opnds[i]->getSourceState(t);
  uniqueifyState(t);
  return v;
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

phase_hlm* makeInfinity(bool disc)
{
  return new inf_or_zero_ph(disc, true);
}

phase_hlm* makeZero(bool disc)
{
  return new inf_or_zero_ph(disc, false);
}

phase_hlm* makeConst(long n)
{
  if (0 >n) return 0;
  if (0==n) return new inf_or_zero_ph(true, false);
  return new equilikely_dph(n, n);
}

phase_hlm* makeBernoulli(double p)
{
  return new bernoulli_dph(p);
}

phase_hlm* makeGeom(double p)
{
  return new geometric_dph(p);
}

phase_hlm* makeEquilikely(long a, long b)
{
  if (a<0)  return 0;
  if (b<0)  return 0;
  if (a>b)  return 0;
  if (b==0) return new inf_or_zero_ph(true, false);
  return new equilikely_dph(a, b);
}

phase_hlm* makeBinomial(long n, double p)
{
  if (n<0)  return 0;
  if (p<0)  return 0;
  if (p>1)  return 0;

  if (0==p) return new inf_or_zero_ph(true, false);
  if (1==p) return new equilikely_dph(n, n);

  return new binomial_dph(n, p);
}


phase_hlm* makeErlang(long n, double lambda)
{
  if (n<0) return 0;
  if (0==n) return new inf_or_zero_ph(false, false);
  return new erlang_cph(n, lambda);
}

inline bool AllDiscreteOrContinuous(phase_hlm** A, int n)
{
  DCASSERT(A);
  if (0==n) return true;
  if (0==A[0]) return false;
  for (int i=1; i<n; i++) {
    if (0==A[i]) return false;
    if (A[i]->isDiscrete() != A[0]->isDiscrete()) return false;
  }
  return true;
}

inline phase_hlm* KillList(phase_hlm** opnds, int N)
{
  for (int i=0; i<N; i++) Delete(opnds[i]);
  delete[] opnds;
  return 0;
}

phase_hlm* makeSum(phase_hlm** opnds, int N)
{
  if (0==opnds) return 0;
  if (0==N) {
    delete[] opnds;
    return 0;
  }
  if (!AllDiscreteOrContinuous(opnds, N)) return KillList(opnds, N);

  if (opnds[0]->isDiscrete())
    return new phint_addition(opnds, N);
  else
    return new phreal_addition(opnds, N);
}

phase_hlm* makeProduct(phase_hlm* X, long n)
{
  if (0==X) return 0;
  if (n<0)  return 0;
  if (0==n) return makeZero(X->isDiscrete());
  if (1==n) return X;

  if (X->isDiscrete())  return new dph_multiply(X, n);
  else                  return new cph_multiply(X, n);
}

phase_hlm* makeProduct(phase_hlm* X, double n)
{
  if (0==X)             return 0;
  if (X->isDiscrete())  return 0;
  if (n<0)              return 0;
  if (0==n)             return makeZero(false);
  if (1.0==n)           return X;
  return new cph_multiply(X, n);
}


phase_hlm* makeChoice(phase_hlm** opnds, double* probs, int N)
{
  if (0==N || 0==opnds) {
    delete[] opnds;
    delete[] probs;
    return 0;
  }
  if (0==probs)  return KillList(opnds, N);
  for (int i=0; i<N; i++) if (probs[i]<0) {
    delete[] probs;
    return KillList(opnds, N);
  }
  if (!AllDiscreteOrContinuous(opnds, N)) {
    delete[] probs;
    return KillList(opnds, N);
  }

  return new phase_choice(opnds, probs, N);
}

phase_hlm* makeOrder(int k, phase_hlm** opnds, int N)
{
  if (0==N || 0==opnds) {
    delete[] opnds;
    return 0;
  }
  if (k<1 || k>N)                         return KillList(opnds, N);
  if (!AllDiscreteOrContinuous(opnds, N)) return KillList(opnds, N);

  if (opnds[0]->isDiscrete())     return new dph_order(k, opnds, N);
  else                            return new cph_order(k, opnds, N);
}

phase_hlm* makeTTA( bool disc, long* i, double* v, int n, 
                    shared_object* a, const shared_object* t, 
                    stochastic_lldsm* mc)
{
  stateset* ssa = dynamic_cast<stateset*>(a);
  const stateset* sst = dynamic_cast<const stateset*>(t);
  if (0==i || 0==v || 0==ssa || 0==mc || (sst != t)) {
    delete[] i;
    delete[] v;
    Delete(mc);
    return 0;
  }
  return new tta_dist(disc, i, v, n, mc, ssa, sst);
}
