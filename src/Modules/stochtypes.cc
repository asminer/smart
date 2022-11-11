
#include "stochtypes.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/engine.h"
#include "../ExprLib/assoc.h"
#include "../ExprLib/ops_int.h"
#include "../ExprLib/intervals.h"
#include "../ExprLib/casting.h"
#include "../Formlsms/stoch_llm.h"
#include "../Formlsms/phase_hlm.h"
#include "../_RngLib/rng.h"
#include "statevects.h"

#define MATHFUNCS_DETAILED
#include "../FuncLib/mathfuncs.h"

#include <math.h>

/* TBD:

    poisson

*/

// #define PRINT_PHASE_CLASSES

#include "../include/splay.h"

struct symbol_count {
  symbol* s;
  long count;

  symbol_count(symbol* t) {
    s = t;
    count = 1;
  }
  inline int Compare(const symbol_count* x) const {
    DCASSERT(x);
    return SIGN(SafeID(s) - SafeID(x->s));
  }
};

/** Get variable dependencies that are shared by more than one expression.
      @param  i       Aggregate number
      @param  elist   List of expressions
      @param  N       number of expressions
      @param  shared  Output: list is stored here
*/
void findCommonDependencies(int i, expr** elist, int N, List <symbol> &shared)
{
  if (0==elist)   return;
  if (0==N)       return;
  SplayOfPointers <symbol_count> unique(16, 0);
  shared.Clear();
  symbol_count* tmp = 0;
  for (int n=0; n<N; n++) {
    shared.Clear();
    elist[n]->BuildSymbolList(traverse_data::GetVarDeps, i, &shared);
    for (int j=0; j<shared.Length(); j++) {
      if (0==tmp)   tmp = new symbol_count(shared.Item(j));
      else          tmp->s = shared.Item(j);
      symbol_count* find = unique.Insert(tmp);
      if (find != tmp) {
        find->count++;
      } else {
        tmp = 0;
      }
    } // for j
  } // for n

  // flatten the tree
  long len = unique.NumElements();
  symbol_count** A = new symbol_count*[len];
  unique.CopyToArray(A);

  // Copy the duplicates
  shared.Clear();
  for (int j=0; j<len; j++) {
    if (A[j]->count > 1)
      if (A[j]->s->Type()->getModifier()==PHASE)
        shared.Append(A[j]->s);
    delete A[j];
  }
  delete[] A;
}

/// Show variable dependencies.
void showCommonDependencies(const exprman* em, List <symbol> &shared, symbol* who)
{
  if (who)
    em->warn() << "Common dependencies in parameters to " << who->Name() << "():";
  else
    em->warn() << "Common dependencies:";
  for (int i=0; i<shared.Length(); i++) {
    em->newLine((i>0) ? 0 : 1);
    shared.Item(i)->Print(em->warn(), 0);
  }
}

/** Check for dependent arguments.
    Used for phase type functions like choose() and max().
*/
bool haveDependencies(const exprman* em, symbol* who, expr** pass, int np)
{
  List <symbol> foo;
  // Check for independence
  findCommonDependencies(0, pass, np, foo);
  if (foo.Length()) if (em->startWarning()) { // TBD: name this warning?
    em->causedBy(0);
    showCommonDependencies(em, foo, who);
    em->stopIO();
    return true;
  }
  return false;
}

/**
    Out of range error message.
*/
inline void OutOfRange(const exprman* em, const traverse_data &x, const type* t, const char* what, const char* range)
{
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << what;
      DCASSERT(t);
      t->print(em->cerr(), *x.answer);
      em->cerr() << " out of range" << range;
      em->stopIO();
    }
}

// ******************************************************************
// *                                                                *
// *                         Distributions                          *
// *                                                                *
// ******************************************************************

inline void generateUniform(double a, double b, traverse_data &x)
{
  DCASSERT(x.answer);
  if (a < b) {
    DCASSERT(x.stream);
    x.answer->setReal(a + (b-a)*x.stream->Uniform32());
    return;
  }
  if (a==b) {
    x.answer->setReal(a);
    return;
  }
  // must have a > b
  x.answer->setNull();
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       distribution class                       *
// *                                                                *
// *                                                                *
// ******************************************************************

class distribution : public simple_internal {
public:
  distribution(const type* rettype, const char* name, int np);
  inline bool hasNoRngStream(const char* fn, int ln, traverse_data &x) const {
    if (x.stream) return false;
    if (em->startInternal(fn, ln)) {
      em->causedBy(x.parent);
      em->internal() << "Missing RNG stream in call to " << Name();
      em->stopIO();
    }
    x.answer->setNull();
    return true;
  }
};

distribution::distribution(const type* rettype, const char* name, int np)
 : simple_internal(rettype, name, np)
{
}

// ******************************************************************
// *                        bernoulli  class                        *
// ******************************************************************

class bernoulli : public distribution {
public:
  bernoulli(const type* rettype, const type* parmtype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  inline void BadP(traverse_data &x) const {
    OutOfRange(em, x, em->REAL, "bernoulli probability ", "");
    x.answer->setNull();
  }
};

bernoulli::bernoulli(const type* rettype, const type* parmtype)
 : distribution(rettype, "bernoulli", 1)
{
  SetFormal(0, parmtype, "p");
  SetDocumentation("Bernoulli distribution: one with probability p, zero otherwise.");
}

int bernoulli::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return distribution::Traverse(x, pass, np);

  // TBD: check the parameter value for degenerate cases
  DCASSERT(x.answer);
  interval_object* range = new interval_object;
  range->Left().setNormal(true, 0.0);
  range->Right().setNormal(true, 1.0);
  x.answer->setPtr(range);
  return 0;
}

// ******************************************************************
// *                       bernoulli_ph class                       *
// ******************************************************************

class bernoulli_ph : public bernoulli {
public:
  bernoulli_ph(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

bernoulli_ph::bernoulli_ph(const type* rt, const type* pt)
 : bernoulli(rt, pt)
{
}

void bernoulli_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()
    && (x.answer->getReal()>=0.0)
    && (x.answer->getReal()<=1.0)) {

      shared_object* X = makeBernoulli(x.answer->getReal());
      x.answer->setPtr(X);
  } else {
      BadP(x);
  }
}

// ******************************************************************
// *                      bernoulli_rand class                      *
// ******************************************************************

class bernoulli_rand : public bernoulli {
public:
  bernoulli_rand(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

bernoulli_rand::bernoulli_rand(const type* rt, const type* pt)
 : bernoulli(rt, pt)
{
}

void bernoulli_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  SafeCompute(pass[0], x);

  bool out_of_range = false;

  if (x.answer->isNormal()) {
    if ((x.answer->getReal()>=0.0) && (x.answer->getReal()<=1.0)) {
      // p value is in legal range; sample the value
      generateBernoulli(x.answer->getReal(), x);
      return;
    }
    // illegal p value
    out_of_range = true;
  }

  if (x.answer->isInfinity() || out_of_range)  BadP(x);

  // propogate any other errors (silently).
}


// ******************************************************************
// *                        geometric  class                        *
// ******************************************************************

class geometric : public distribution {
public:
  geometric(const type* rettype, const type* parmtype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  void BadP(traverse_data &x) const {
    OutOfRange(em, x, em->REAL, "geometric probability ", "");
    x.answer->setNull();
  }
};

geometric::geometric(const type* rettype, const type* parmtype)
 : distribution(rettype, "geometric", 1)
{
  SetFormal(0, parmtype, "p");
  SetDocumentation("Geometric distribution.  I.e., a discrete distribution with Pr(X=n) = (1-p)*p^n.");
}

int geometric::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return distribution::Traverse(x, pass, np);

  // TBD: check the parameter value for degenerate cases
  DCASSERT(x.answer);
  interval_object* range = new interval_object;
  range->Left().setNormal(true, 0.0);
  range->Right().setInfinity(false, 1);
  x.answer->setPtr(range);
  return 0;
}

// ******************************************************************
// *                       geometric_ph class                       *
// ******************************************************************

class geometric_ph : public geometric {
public:
  geometric_ph(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

geometric_ph::geometric_ph(const type* rettype, const type* parmtype)
 : geometric(rettype, parmtype)
{
}

void geometric_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()
    && (x.answer->getReal()>=0.0)
    && (x.answer->getReal()<=1.0)) {

      shared_object* X = makeGeom(x.answer->getReal());
      x.answer->setPtr(X);
  } else {
      BadP(x);
  }
}

// ******************************************************************
// *                      geometric_rand class                      *
// ******************************************************************

class geometric_rand : public geometric {
public:
  geometric_rand(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

geometric_rand::geometric_rand(const type* rettype, const type* parmtype)
 : geometric(rettype, parmtype)
{
}

void geometric_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  SafeCompute(pass[0], x);
  bool out_of_range = false;

  if (x.answer->isNormal()) {
    if ((x.answer->getReal()>=0.0) && (x.answer->getReal()<=1.0)) {
      // p value is in legal range; sample the value
      generateGeometric(x.answer->getReal(), x);
      return;
    }
    // illegal p value
    out_of_range = true;
  }

  if (x.answer->isInfinity() || out_of_range) BadP(x);

  // propogate any other errors (silently).
}

// ******************************************************************
// *                        equilikely class                        *
// ******************************************************************

class equilikely : public distribution {
public:
  equilikely(const type* rettype, const type* parmtype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);

  inline bool isBadParam(const char* which, traverse_data &x) const {
    DCASSERT(x.answer);
    if (x.answer->isNormal()) if (x.answer->getInt() >= 0) return false;
    OutOfRange(em, x, em->INT, which, " for phase int");
    x.answer->setNull();
    return true;
  }

  inline void aGTb(traverse_data &x, long a, long b) const {
    DCASSERT(a>b);
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "equilikely parameters a=" << a;
      em->cerr() << ", b=" << b << " do not satisfy a<=b";
      em->stopIO();
    }
    x.answer->setNull();
  }

};

equilikely::equilikely(const type* rettype, const type* parmtype)
 : distribution(rettype, "equilikely", 2)
{
  SetFormal(0, parmtype, "a");
  SetFormal(1, parmtype, "b");
  SetDocumentation("Distribution: integers [a..b], with equal probability.  If a > b, will return null.");
}

int equilikely::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return distribution::Traverse(x, pass, np);

  DCASSERT(x.answer);
  if (0==pass[0] || 0==pass[1]) {
    x.answer->setNull();
    return 0;
  }
  pass[0]->Traverse(x);
  if (!x.answer->isNormal()) return 0;
  interval_object* r0 = smart_cast <interval_object*> (Share(x.answer->getPtr()));

  pass[1]->Traverse(x);
  if (!x.answer->isNormal()) {
    Delete(r0);
    return 0;
  }
  interval_object* r1 = smart_cast <interval_object*> (Share(x.answer->getPtr()));

  interval_object* ra = new interval_object;
  if (r0)  ra->Left() = r0->Left();
  else     ra->Left().setNull();
  if (r1)  ra->Right() = r1->Right();
  else     ra->Right().setNull();
  x.answer->setPtr(ra);
  Delete(r0);
  Delete(r1);
  return 0;
}

// ******************************************************************
// *                      equilikely_ph  class                      *
// ******************************************************************

class equilikely_ph : public equilikely {
public:
  equilikely_ph(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

equilikely_ph::equilikely_ph(const type* rettype, const type* parmtype)
 : equilikely(rettype, parmtype)
{
}

void equilikely_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (isBadParam("equilikely parameter a=", x)) return;
  long a = x.answer->getInt();
  SafeCompute(pass[1], x);
  if (isBadParam("equilikely parameter b=", x)) return;
  long b = x.answer->getInt();
  if (a>b) {
    aGTb(x, a, b);
    return;
  }
  shared_object* X = makeEquilikely(a, b);
  x.answer->setPtr(X);
}

// ******************************************************************
// *                     equilikely_rand  class                     *
// ******************************************************************

class equilikely_rand : public equilikely {
public:
  equilikely_rand(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

equilikely_rand::equilikely_rand(const type* rettype, const type* parmtype)
 : equilikely(rettype, parmtype)
{
}

void equilikely_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(x.answer);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  result* answer = x.answer;

  result a, b;
  x.answer = &a;
  SafeCompute(pass[0], x);
  x.answer = &b;
  SafeCompute(pass[1], x);
  x.answer = answer;

  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    if (a.getInt() > b.getInt()) {
      // error
      aGTb(x, a.getInt(), b.getInt());
    } else {
      // sample the value.
      generateEquilikely(a.getInt(), b.getInt(), x);
    }
    return;
  }

  // TBD: do we want to display error messages?

  answer->setNull();
}

// ******************************************************************
// *                         binomial class                         *
// ******************************************************************

class binomial : public distribution {
public:
  binomial(const type* rettype, const type* ntype, const type* ptype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);

  inline bool isBadN(traverse_data &x) const {
    DCASSERT(x.answer);
    if (x.answer->isNormal()) if (x.answer->getInt() >= 0) return false;
    OutOfRange(em, x, em->INT, "binomial parameter n=", "");
    x.answer->setNull();
    return true;
  }

  inline bool isBadP(traverse_data &x) const {
    DCASSERT(x.answer);
    if (x.answer->isNormal()) {
      double p = x.answer->getReal();
      if (p >= 0 && p <= 1) return false;
    }
    OutOfRange(em, x, em->REAL, "binomial parameter p=", "");
    x.answer->setNull();
    return true;
  }
};

binomial::binomial(const type* rettype, const type* ntype, const type* ptype)
 : distribution(rettype, "binomial", 2)
{
  SetFormal(0, ntype, "n");
  SetFormal(1, ptype, "p");
  SetDocumentation("Distribution: sum of n independent bernoulli(p) random variables, where n>=0 and p is a probability.");
}

int binomial::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return distribution::Traverse(x, pass, np);

  DCASSERT(x.answer);
  if (0==pass[0] || 0==pass[1]) {
    x.answer->setNull();
    return 0;
  }
  pass[0]->Traverse(x);
  if (!x.answer->isNormal()) return 0;
  interval_object* r0 = smart_cast <interval_object*> (Share(x.answer->getPtr()));

  interval_object* ra = new interval_object;
  ra->Left().setNormal(true, 0);
  if (r0)  ra->Right() = r0->Right();
  else     ra->Right().setNull();
  x.answer->setPtr(ra);
  Delete(r0);
  return 0;
}

// ******************************************************************
// *                       binomial_ph  class                       *
// ******************************************************************

class binomial_ph : public binomial {
public:
  binomial_ph(const type* rettype, const type* ntype, const type* ptype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

binomial_ph::binomial_ph(const type* rettype, const type* nt, const type* pt)
 : binomial(rettype, nt, pt)
{
}

void binomial_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (isBadN(x)) return;
  long n = x.answer->getInt();
  SafeCompute(pass[1], x);
  if (isBadP(x)) return;
  double p = x.answer->getReal();
  shared_object* X = makeBinomial(n, p);
  x.answer->setPtr(X);
}

// ******************************************************************
// *                      binomial_rand  class                      *
// ******************************************************************

class binomial_rand : public binomial {
public:
  binomial_rand(const type* rettype, const type* nt, const type* pt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

binomial_rand
 ::binomial_rand(const type* rettype, const type* nt, const type* pt)
 : binomial(rettype, nt, pt)
{
}

void binomial_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(x.answer);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (isBadN(x)) return;
  long n = x.answer->getInt();
  SafeCompute(pass[1], x);
  if (isBadP(x)) return;
  double p = x.answer->getReal();

  generateBinomial(n, p, x);
}

// ******************************************************************
// *                        expo_dist  class                        *
// ******************************************************************

class expo_dist : public distribution {
public:
  expo_dist(const type* rettype, const type* parmtype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  inline void BadLambda(traverse_data &x) const {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "expo with parameter ";
      DCASSERT(em->REAL);
      em->REAL->print(em->cerr(), *x.answer);
      em->cerr() << ", must be non-negative";
      em->stopIO();
    }
    x.answer->setNull();
  }
};

expo_dist::expo_dist(const type* rettype, const type* parmtype)
 : distribution(rettype, "expo", 1)
{
  SetFormal(0, parmtype, "p");
  SetDocumentation("Exponential distribution with rate lambda >= 0.  I.e., a continuous distribution with pdf f(x) = lambda * exp(-lambda * x).");
}

int expo_dist::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::ComputeExpoRate:
      DCASSERT(x.answer);
      DCASSERT(0==x.aggregate);
      DCASSERT(1==np);
      x.which = traverse_data::Compute;
      SafeCompute(pass[0], x);
      x.which = traverse_data::ComputeExpoRate;
      return 0;

    case traverse_data::BuildExpoRateDD:
      DCASSERT(x.answer);
      DCASSERT(0==x.aggregate);
      DCASSERT(1==np);
      if (pass[0]) {
        x.which = traverse_data::BuildDD;
        pass[0]->Traverse(x);
        x.which = traverse_data::BuildExpoRateDD;
      } else {
        x.answer->setNull();
      }
      return 0;

    case traverse_data::FindRange: {
      // TBD: check the parameter value for degenerate cases
      interval_object* range = new interval_object;
      range->Left().setNormal(false, 0.0);
      range->Right().setInfinity(false, 1);
      x.answer->setPtr(range);
      return 0;
    }

    default:
      return distribution::Traverse(x, pass, np);
  }
}


// ******************************************************************
// *                         expo_ph  class                         *
// ******************************************************************

class expo_ph : public expo_dist {
public:
  expo_ph(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

expo_ph::expo_ph(const type* rettype, const type* parmtype)
 : expo_dist(rettype, parmtype)
{
}

void expo_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()
    && (x.answer->getReal()>=0.0)) {

      if (x.stream) {
        generateExpo(x.answer->getReal(), x);
      } else {
        shared_object* X = makeErlang(1, x.answer->getReal());
        x.answer->setPtr(X);
      }
  }
  else {
    BadLambda(x);
  }
}

// ******************************************************************
// *                        expo_rand  class                        *
// ******************************************************************

class expo_rand : public expo_dist {
public:
  expo_rand(const type* rettype, const type* parmtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

expo_rand::expo_rand(const type* rettype, const type* parmtype)
 : expo_dist(rettype, parmtype)
{
}

void expo_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  SafeCompute(pass[0], x);

  if (x.answer->isNormal()) {
    if (x.answer->getReal()<0) {
      BadLambda(x);
      return;
    }

    generateExpo(x.answer->getReal(), x);
    return;
  }

  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity() > 0) {
      // expo(infinity) is zero
      x.answer->setReal(0.0);
      return;
    }
    BadLambda(x);
  }

  // propogate any other errors (silently).
}

// ******************************************************************
// *                          erlang class                          *
// ******************************************************************

class erlang : public distribution {
public:
  erlang(const type* rettype, const type* nt, const type* rt);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  inline bool isBadN(traverse_data &x) const {
    DCASSERT(x.answer);
    if (x.answer->isNormal()) if (x.answer->getInt() >= 0) return false;
    OutOfRange(em, x, em->INT, "erlang parameter n=", "");
    x.answer->setNull();
    return true;
  }

  inline bool isBadRate(traverse_data &x) const {
    DCASSERT(x.answer);
    if (x.answer->isNormal()) {
      if (x.answer->getReal() >= 0) return false;
    }
    OutOfRange(em, x, em->REAL, "erlang rate parameter r=", "");
    x.answer->setNull();
    return true;
  }
};

erlang::erlang(const type* rettype, const type* ntype, const type* ptype)
 : distribution(rettype, "erlang", 2)
{
  SetFormal(0, ntype, "n");
  SetFormal(1, ptype, "r");
  SetDocumentation("Distribution: sum of n independent expo(r) random variables, where n>=0 and r is a rate.");
}


int erlang::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return distribution::Traverse(x, pass, np);

  // TBD: check the parameter value for degenerate cases
  DCASSERT(x.answer);
  interval_object* range = new interval_object;
  range->Left().setNormal(false, 0.0);
  range->Right().setInfinity(false, 1);
  x.answer->setPtr(range);
  return 0;
}

// ******************************************************************
// *                        erlang_ph  class                        *
// ******************************************************************

class erlang_ph : public erlang {
public:
  erlang_ph(const type* rettype, const type* ntype, const type* rtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

erlang_ph::erlang_ph(const type* rettype, const type* nt, const type* rt)
 : erlang(rettype, nt, rt)
{
}

void erlang_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);

  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (isBadN(x)) return;
  long n = x.answer->getInt();
  SafeCompute(pass[1], x);
  if (isBadRate(x)) return;
  double r = x.answer->getReal();
  shared_object* X = makeErlang(n, r);
  x.answer->setPtr(X);
}

// ******************************************************************
// *                       erlang_rand  class                       *
// ******************************************************************

class erlang_rand : public erlang {
public:
  erlang_rand(const type* rettype, const type* nt, const type* rt);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

erlang_rand::erlang_rand(const type* rettype, const type* nt, const type* rt)
 : erlang(rettype, nt, rt)
{
}

void erlang_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(x.answer);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  DCASSERT(x.answer);
  x.answer->setNull();
  SafeCompute(pass[0], x);
  if (isBadN(x)) return;
  long n = x.answer->getInt();
  SafeCompute(pass[1], x);
  if (isBadRate(x)) return;
  double r = x.answer->getReal();

  generateErlang(n, r, x);
}

// ******************************************************************
// *                         uniform  class                         *
// ******************************************************************

class uniform : public distribution {
public:
  uniform(const type* RR);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

uniform::uniform(const type* RR)
 : distribution(RR, "uniform", 2)
{
  SetFormal(0, RR, "a");
  SetFormal(1, RR, "b");
  SetDocumentation("Uniform distribution: reals in (a, b) with equal probability.  If a >= b, will return null.");
}

void uniform::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  DCASSERT(x.answer);
  if (hasNoRngStream(__FILE__, __LINE__, x)) return;

  result* answer = x.answer;

  result a, b;
  x.answer = &a;
  SafeCompute(pass[0], x);
  x.answer = &b;
  SafeCompute(pass[1], x);
  x.answer = answer;

  // Normal behavior
  if (a.isNormal() && b.isNormal()) {
    generateUniform(a.getReal(), b.getReal(), x);
    return;
  } // normal behavior

  // TBD: do we want to display error messages?

  answer->setNull();
}

int uniform::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::FindRange != x.which)
    return distribution::Traverse(x, pass, np);

  DCASSERT(x.answer);

  if (0==pass[0] || 0==pass[1]) {
    x.answer->setNull();
    return 0;
  }
  pass[0]->Traverse(x);
  if (!x.answer->isNormal()) return 0;
  interval_object* r0 = smart_cast <interval_object*> (Share(x.answer->getPtr()));

  pass[1]->Traverse(x);
  if (!x.answer->isNormal()) {
    Delete(r0);
    return 0;
  }
  interval_object* r1 = smart_cast <interval_object*> (Share(x.answer->getPtr()));

  interval_object* ra = new interval_object;
  if (r0) {
    ra->Left() = r0->Left();
    ra->Left().setInclusion(false);
  } else {
    ra->Left().setNull();
  }
  if (r1) {
    ra->Right() = r1->Right();
    ra->Right().setInclusion(false);
  } else {
    ra->Right().setNull();
  }

  x.answer->setPtr(ra);
  Delete(r0);
  Delete(r1);
  return 0;
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *                   Operations on  phase types                   *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                       phase_add_op class                       *
// *                                                                *
// ******************************************************************

/** Class for phase addition operations.
*/
class phase_add_op : public assoc_op {

  /// The actual addition expression.
  class myexpr : public summation {
  public:
    myexpr(const location &W, const type* t, expr **x, bool* f, int n);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr **x, bool* f, int n) const;
  };

public:
  phase_add_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;

  inline bool bogustype(const type* t) const {
    if (0==t)             return true;
    if (em->VOID==t)      return true;
    if (em->NULTYPE==t)   return true;
    return false;
  }

  inline const type* getType(expr** list, int N) const {
    bool all_ints = true;
    bool has_proc = false;
    for (int i=0; i<N; i++) {
      if (0==list[i])       return 0;
      const type* t = list[i]->Type();
      if (bogustype(t))     return 0;
      if (t->getBaseType() != em->INT)  all_ints = false;
      if (t->hasProc())                 has_proc = true;
    }

    const type* anstype = all_ints ? em->INT : em->REAL;
    DCASSERT(anstype);
    anstype = anstype->modifyType(PHASE);
    DCASSERT(anstype);
    if (has_proc) anstype = anstype->addProc();
    DCASSERT(anstype);
    return anstype;
  }

  static inline assoc* killArgs(expr** list, int N) {
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;
    return 0;
  }
};


// ******************************************************************
// *                  phase_add_op::myexpr methods                  *
// ******************************************************************

phase_add_op::myexpr
::myexpr(const location &W, const type* t, expr **x, bool* f, int n)
 : summation(W, exprman::aop_plus, t, x, f, n)
{
}

void phase_add_op::myexpr::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* sum = x.answer;

  bool normal = true;
  bool unknown = false;

  // allocate array of phase types
  phase_hlm** answers = new phase_hlm*[opnd_count];
  int i;
  for (i=0; i<opnd_count; i++) answers[i] = 0;

  // compute all operands first
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (sum->isNormal()) {
      answers[i] = smart_cast <phase_hlm*>(Share(sum->getPtr()));
      continue;
    }
    // something strange happened, deal with it
    normal = false;
    if (sum->isUnknown()) {
      unknown = true;
      continue;
    }
    // error
    unknown = false;
    break;
  } // for i

  if (normal) {
    phase_hlm* foo = makeSum(answers, opnd_count);
    DCASSERT(foo);
    sum->setPtr(foo);
    return;
  }
  // trash results, we won't need them
  for (i=0; i<opnd_count; i++) Delete(answers[i]);
  delete[] answers;
  if (unknown)  sum->setUnknown();
  else          sum->setNull();
}


expr* phase_add_op::myexpr::buildAnother(expr** x, bool* f, int n) const
{
  return new myexpr(Where(), Type(), x, f, n);
}

// ******************************************************************
// *                      phase_add_op methods                      *
// ******************************************************************

phase_add_op::phase_add_op() : assoc_op(exprman::aop_plus)
{
}

int phase_add_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  if (flip) for (int i=0; i<N; i++) if (flip[i]) return -1;
  const type* anstype = getType(list, N);
  if (0==anstype) return -1;
  int pd = 0;
  for (int i=0; i<N; i++) {
    int d = em->getPromoteDistance(list[i]->Type(), anstype);
    if (d<0) return -1;
    pd += d;
  }
  return pd;
}

int phase_add_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  const type* anstype = getExprType(f, lt, rt);
  if (0==anstype) return -1;
  int foo = em->getPromoteDistance(lt, anstype);
  int bar = em->getPromoteDistance(rt, anstype);
  DCASSERT(foo>=0);
  DCASSERT(bar>=0);
  return  foo + bar;
}

const type* phase_add_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f) return 0;
  if (bogustype(l) || bogustype(r)) return 0;
  const type* anstype = 0;
  if (l->getBaseType() == em->INT && r->getBaseType() == em->INT)
    anstype = em->INT;
  else
    anstype = em->REAL;
  DCASSERT(anstype);
  anstype = anstype->modifyType(PHASE);
  DCASSERT(anstype);
  if (l->hasProc() || r->hasProc()) anstype = anstype->addProc();
  DCASSERT(anstype);

  if (em->getPromoteDistance(l, anstype) < 0) return 0;
  if (em->getPromoteDistance(r, anstype) < 0) return 0;

  return anstype;
}

assoc* phase_add_op
::makeExpr(const location &W, expr** list, bool* flip, int N) const
{
  if (flip) {
    for (int i=0; i<N; i++) if (flip[i]) {
      delete[] flip;
      return killArgs(list, N);
    }
    delete[] flip;
  }
  const type* anstype = getType(list, N);
  bool ok = anstype;
  if (ok) for (int i=0; i<N; i++) {
    list[i] = em->promote(list[i], anstype);
    if (0==list[i]) ok = false;
  }
  if (!ok)  return killArgs(list, N);

  // check for parameter independence
  List <symbol> foo;
  findCommonDependencies(0, list, N, foo);
  if (foo.Length()) if (em->startWarning()) { // TBD: name this warning?
    em->causedBy(W);
    em->warn() << "operands in phase-type addition are not independent;";
    em->newLine();
    em->warn() << "phase-type model will incorrectly treat them as such.";
    em->newLine();
    showCommonDependencies(em, foo, 0);
    em->stopIO();
  }

  return new myexpr(W, anstype, list, 0, N);
}


// ******************************************************************
// *                                                                *
// *                      phase_mult_op  class                      *
// *                                                                *
// ******************************************************************

/** Class for phase multiplication operations.
    Note: there must be exactly one phase operand, the rest are const.
*/
class phase_mult_op : public int_mult_op {

  /** The actual multiply expression.
      Note that the first argument is the phase type one.
  */
  class myexpr : public product {
    expr* const_part;
  public:
    myexpr(const location &W, const type* t, expr **x, int n);
    virtual void Compute(traverse_data &x);
  protected:
    virtual ~myexpr();
    virtual expr* buildAnother(expr **x, bool* f, int n) const;
  };

public:
  phase_mult_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;

  inline bool bogustype(const type* t) const {
    if (0==t)             return true;
    if (em->VOID==t)      return true;
    if (em->NULTYPE==t)   return true;
    return false;
  }

  inline bool checkArgs(expr** list, int N, int &phi, bool &hasproc) const {
    hasproc = false;
    int num_phase = 0;
    for (int i=0; i<N; i++) {
      if (0==list[i])     return 0;
      const type* t = list[i]->Type();
      if (bogustype(t))   return 0;
      if (t->hasProc())   hasproc = true;
      if (t->getModifier() == PHASE) {
        num_phase++;
        phi = i;
      }
    } // for i
    if (1 != num_phase)   return 0;
    return 1;
  }

  static inline assoc* killArgs(expr** list, int N) {
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;
    return 0;
  }
};

// ******************************************************************
// *                 phase_mult_op::myexpr  methods                 *
// ******************************************************************

phase_mult_op::myexpr
::myexpr(const location &W, const type* t, expr **x, int n)
 : product(W, exprman::aop_times, t, x, 0, n)
{
  if (n==2) {
    const_part = Share(x[1]);
  } else {
    expr** opnds = new expr*[n-1];
    for (int i=1; i<n; i++) opnds[i-1] = Share(x[i]);
    const type* noph = opnds[1]->Type();
    DCASSERT(noph);
    const_part = new expression(W, noph, opnds, n-1);
  }
}

phase_mult_op::myexpr::~myexpr()
{
  Delete(const_part);
}


void phase_mult_op::myexpr::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  /*
  em->cout() << "phase-mult-op traverse\n";
  em->cout() << "constant part: ";
  const_part->Print(em->cout(), 0);
  em->cout() << ", type ";
  const_part->PrintType(em->cout());
  em->cout() << "\nphase part: ";
  operands[0]->Print(em->cout(), 0);
  em->cout() << ", type ";
  operands[0]->PrintType(em->cout());
  em->cout() << "\n";
  em->cout().flush();
  DCASSERT(0);
  */
  // Grab phase operand
  operands[0]->Compute(x);
  if (! x.answer->isNormal()) return;
  phase_hlm* X = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));

  // Grab constant operand
  bool discrete = (operands[0]->Type()->getBaseType() == em->INT);
  SafeCompute(const_part, x);

  phase_hlm* newph = 0;

  // Normal product
  if (x.answer->isNormal()) {
    if (discrete) newph = makeProduct(X, x.answer->getInt());
    else          newph = makeProduct(X, x.answer->getReal());
    if (0==newph) if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Couldn't build phase-type product, bad constant?";
      em->stopIO();
    }
    x.answer->setPtr(newph);
    return;
  }

  if (x.answer->isInfinity()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Cannot build phase-type product with infinity";
      em->stopIO();
    }
    x.answer->setNull();
  }

  // whatever error is left, propogate it
}

expr* phase_mult_op::myexpr::buildAnother(expr** x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new myexpr(Where(), Type(), x, n);
}

// ******************************************************************
// *                     phase_mult_op  methods                     *
// ******************************************************************

phase_mult_op::phase_mult_op() : int_mult_op()
{
}

int phase_mult_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  if (N<2) return -1;
  if (flip) for (int i=0; i<N; i++) if (flip[i]) return -1;
  int phi = -1;
  bool has_proc;
  if (!checkArgs(list, N, phi, has_proc)) return -1;
  CHECK_RANGE(0, phi, N);
  const type* ph = list[phi]->Type();
  DCASSERT(ph);
  const type* con = ph->getBaseType();
  DCASSERT(con);
  if (has_proc) {
    ph = ph->addProc();
    con = con->addProc();
    DCASSERT(ph);
    DCASSERT(con);
  }
  int pd = em->getPromoteDistance(list[phi]->Type(), ph);
  DCASSERT(pd>=0);
  for (int i=0; i<phi; i++) {
    int d = em->getPromoteDistance(list[i]->Type(), con);
    if (d<0) return -1;
    pd += d;
  }
  for (int i=phi+1; i<N; i++) {
    int d = em->getPromoteDistance(list[i]->Type(), con);
    if (d<0) return -1;
    pd += d;
  }
  return pd;
}

int phase_mult_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f) return -1;
  if (bogustype(lt) || bogustype(rt)) return -1;
  bool has_proc = lt->hasProc() || rt->hasProc();
  if (PHASE == lt->getModifier() && DETERM == rt->getModifier()) {
    SWAP(lt, rt);
  }
  if (DETERM == lt->getModifier() && PHASE == rt->getModifier()) {
    const type* base = rt->getBaseType();
    DCASSERT(base);
    if (has_proc) base = base->addProc();
    DCASSERT(base);
    int foo = em->getPromoteDistance(lt, base);
    if (foo<0) return -1;
    const type* ph = rt;
    if (has_proc) if (false == ph->hasProc()) ph = ph->addProc();
    int bar = em->getPromoteDistance(rt, ph);
    DCASSERT(bar>=0);
    return foo+bar;
  }
  return -1;
}

const type* phase_mult_op
::getExprType(bool f, const type* lt, const type* rt) const
{
  if (f) return 0;
  if (bogustype(lt) || bogustype(rt)) return 0;
  bool has_proc = lt->hasProc() || rt->hasProc();
  if (PHASE == lt->getModifier() && DETERM == rt->getModifier()) {
    SWAP(lt, rt);
  }
  if (DETERM == lt->getModifier() && PHASE == rt->getModifier()) {
    const type* base = rt->getBaseType();
    DCASSERT(base);
    if (has_proc) base = base->addProc();
    DCASSERT(base);
    int foo = em->getPromoteDistance(lt, base);
    if (foo<0) return 0;
    const type* ph = rt;
    if (has_proc) if (false == ph->hasProc()) ph = ph->addProc();
    DCASSERT(em->getPromoteDistance(rt, ph) >= 0);
    return ph;
  }
  return 0;
}

assoc* phase_mult_op
::makeExpr(const location &W, expr** list, bool* flip, int N) const
{
  if (flip) {
    for (int i=0; i<N; i++) if (flip[i]) {
      delete[] flip;
      return killArgs(list, N);
    }
    delete[] flip;
  }
  if (N<2) return killArgs(list, N);
  int phi = -1;
  bool has_proc;
  if (!checkArgs(list, N, phi, has_proc)) return killArgs(list, N);
  CHECK_RANGE(0, phi, N);
  const type* ph = list[phi]->Type();
  DCASSERT(ph);
  const type* con = ph->getBaseType();
  DCASSERT(con);
  if (has_proc) {
    ph = ph->addProc();
    con = con->addProc();
    DCASSERT(ph);
    DCASSERT(con);
  }
  SWAP(list[0], list[phi]);
  list[0] = em->promote(list[0], ph);
  if (0==list[0]) return killArgs(list, N);
  for (int i=1; i<N; i++) {
    list[i] = em->promote(list[i], con);
    if (0==list[i]) return killArgs(list, N);
  }
  return new myexpr(W, ph, list, N);
}



// ******************************************************************
// *                                                                *
// *                                                                *
// *      Functions  to build distributions from distributions      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       cph2dph_unif class                       *
// ******************************************************************

/**
  Uniformize a continuous phase, to produce a discrete phase.
*/
class cph2dph_unif : public simple_internal {
public:
  cph2dph_unif(const type* intype, const type* outtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

cph2dph_unif::cph2dph_unif(const type* intype, const type* outtype)
 : simple_internal(outtype, "uniformize", 2)
{
  SetFormal(0, intype, "x");
  SetFormal(1, em->REAL, "q");
  SetDocumentation("Uniformize a continuous phase type random variable x, with uniformization constant q>0, to obtain a discrete phase type random variable.  Does not currently work within simulations.");
}

void cph2dph_unif::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(2==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  // Get the passed distribution
  pass[0]->Compute(x);
  if (!x.answer->isNormal()) return;
  phase_hlm* opnd = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));

  // Get q
  pass[1]->Compute(x);
  if (x.answer->isNormal()) {
    double q = x.answer->getReal();
    if (q<=0) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Invalid uniformization constant: " << q << " (must be positive)";
        em->stopIO();
      } // error
      x.answer->setNull();
      return;
    }
    phase_hlm* ans = makeUniformized(opnd, q);
    DCASSERT(ans);
    x.answer->setPtr(ans);
  } else {
    Delete(opnd);
  }
}

int cph2dph_unif::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  //
  // Best we can do at this point
  //
  DCASSERT(x.answer);
  interval_object* range = new interval_object;
  range->Left().setNormal(true, 0.0);
  range->Right().setInfinity(false, 1);
  x.answer->setPtr(range);
  return 0;
}


// ******************************************************************
// *                      cph2dph_embed  class                      *
// ******************************************************************

/**
  Get the discrete phase embedded in a continuous phase.
*/
class cph2dph_embed : public simple_internal {
public:
  cph2dph_embed(const type* intype, const type* outtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

cph2dph_embed::cph2dph_embed(const type* intype, const type* outtype)
 : simple_internal(outtype, "embedding", 1)
{
  SetFormal(0, intype, "x");
  SetDocumentation("Take the embedded discrete phase type random variable from a continuous phase type random variable x.  Does not currently work within simulations.");
}

void cph2dph_embed::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  // Get the passed distribution
  pass[0]->Compute(x);
  if (!x.answer->isNormal()) return;
  phase_hlm* opnd = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));

  phase_hlm* ans = makeEmbedded(opnd);
  DCASSERT(ans);
  x.answer->setPtr(ans);
}

int cph2dph_embed::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  //
  // Best we can do at this point
  //
  DCASSERT(x.answer);
  interval_object* range = new interval_object;
  range->Left().setNormal(true, 0.0);
  range->Right().setInfinity(false, 1);
  x.answer->setPtr(range);
  return 0;
}


// ******************************************************************
// *                          max_ph class                          *
// ******************************************************************

/**
  Max of phase types.
  Base class is defined in FuncLib/mathfuncs.h
*/
class max_ph : public max_si {
public:
  max_ph(const type* args);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

max_ph::max_ph(const type* argt) : max_si(argt)
{
}

void max_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  // Get the passed distributions
  phase_hlm** opnds = new phase_hlm*[np];
  for (int i=0; i<np; i++) {
    pass[i]->Compute(x);
    opnds[i] = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));
  }
  // Build appropriate phase-type model, and return it
  phase_hlm* ans = makeOrder(np, opnds, np);
  DCASSERT(ans);
  x.answer->setPtr(ans);
}

int max_ph::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Promote != x.which)
    return max_si::Traverse(x, pass, np);

  // Promotion: first, do the usual
  formals.promote(em, pass, np, Type());

  // Now, check for independence
  if (haveDependencies(em, this, pass, np))
    return Promote_Dependent;
  else
    return Promote_Success;
}



// ******************************************************************
// *                          min_ph class                          *
// ******************************************************************

/**
  Min of phase types.
  Base class is defined in FuncLib/mathfuncs.h
*/
class min_ph : public min_si {
public:
  min_ph(const type* args);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

min_ph::min_ph(const type* argt) : min_si(argt)
{
}

void min_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  // Get the passed distributions
  phase_hlm** opnds = new phase_hlm*[np];
  for (int i=0; i<np; i++) {
    pass[i]->Compute(x);
    opnds[i] = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));
  }
  // Build appropriate phase-type model, and return it
  phase_hlm* ans = makeOrder(1, opnds, np);
  DCASSERT(ans);
  x.answer->setPtr(ans);
}

int min_ph::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Promote != x.which)
    return min_si::Traverse(x, pass, np);

  // Promotion: first, do the usual
  formals.promote(em, pass, np, Type());

  // Now, check for independence
  if (haveDependencies(em, this, pass, np))
    return Promote_Dependent;
  else
    return Promote_Success;
}



// ******************************************************************
// *                         order_ph class                         *
// ******************************************************************

/**
  Order statistics for phase types.
  Base class is defined in FuncLib/mathfuncs.h
*/
class order_ph : public order_si {
public:
  order_ph(const type* args);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

order_ph::order_ph(const type* argt) : order_si(argt)
{
}

void order_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);

  // Get the order index, K
  pass[0]->Compute(x);
  if (!x.answer->isNormal()) {
    x.answer->setNull();
    return;
  }
  int K = x.answer->getInt();
  if (K<1 || K>np-1) {
    // out of range, should we print an error?
    x.answer->setNull();
    return;
  }

  // Get the passed distributions
  phase_hlm** opnds = new phase_hlm*[np-1];
  for (int i=1; i<np; i++) {
    pass[i]->Compute(x);
    opnds[i-1] = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));
  }
  // Build appropriate phase-type model, and return it
  phase_hlm* ans = makeOrder(K, opnds, np-1);
  DCASSERT(ans);
  x.answer->setPtr(ans);
}

int order_ph::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Promote != x.which)
    return order_si::Traverse(x, pass, np);

  // Promotion: first, do the usual
  formals.promote(em, pass, np, Type());

  // Now, check for independence
  if (haveDependencies(em, this, pass+1, np-1))
    return Promote_Dependent;
  else
    return Promote_Success;
}



// ******************************************************************
// *                        choose_si  class                        *
// ******************************************************************

class choose_si : public simple_internal {
public:
  choose_si(const type* argtype);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

choose_si::choose_si(const type* argt)
 : simple_internal(argt, "choose", 1)
{
  typelist* t = new typelist(2);
  t->SetItem(0, argt);
  t->SetItem(1, em->REAL);
  SetFormal(0, t, "x:p");
  SetDocumentation("Choose random variable x with probability p.  Probability arguments (weights) are normalized so that they sum to one.");
  SetRepeat(0);
}

int choose_si::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  DCASSERT(x.answer);
  DCASSERT(pass[0]);
  pass[0]->Traverse(x);
  if (!x.answer->isNormal())  return 0;
  interval_object* x0 = smart_cast<interval_object*>(x.answer->getPtr());
  DCASSERT(x0);
  interval_object* answer = new interval_object(*x0);
  for (int i=1; i<np; i++) {
    DCASSERT(pass[i]);
    pass[i]->Traverse(x);
    if (!x.answer->isNormal()) {
      Delete(answer);
      return 0;
    }
    interval_object* xi;
    xi = smart_cast<interval_object*>(x.answer->getPtr());
    DCASSERT(xi);
    computeUnion(*answer, *answer, *xi);
  } // for i
  x.answer->setPtr(answer);
  return 0;
}

// ******************************************************************
// *                        choose_ph  class                        *
// ******************************************************************

class choose_ph : public choose_si {
public:
  choose_ph(const type* argtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

choose_ph::choose_ph(const type* argt) : choose_si(argt)
{
}

void choose_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  x.answer->setNull();
  double* probarray = new double[np];
  double total = 0;
  x.aggregate = 1;
  for (int i=0; i<np; i++) {
    SafeCompute(pass[i], x);
    if (x.answer->isNormal()) {
      probarray[i] = x.answer->getReal();
      total += probarray[i];
      if (probarray[i] >= 0) continue;
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Negative probability for parameter " << i;
        em->stopIO();
      } // error
      return;
    }
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Bad probability for parameter " << i;
      em->stopIO();
    } // error
    return;
  }
  if (0==total) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Weights sum to zero in call to choose.";
      em->stopIO();
    } // error
    return;
  }
  // weights are ok, now get distributions
  x.aggregate = 0;
  phase_hlm** opnds = new phase_hlm*[np];
  for (int i=0; i<np; i++) {
    pass[i]->Compute(x);
    opnds[i] = smart_cast <phase_hlm*>(Share(x.answer->getPtr()));
  }
  phase_hlm* ans = makeChoice(opnds, probarray, np);
  DCASSERT(ans);
  x.answer->setPtr(ans);
}

int choose_ph::Traverse(traverse_data &x, expr** pass, int np)
{
  if (traverse_data::Promote != x.which)
    return choose_si::Traverse(x, pass, np);

  // Promotion: first, do the usual
  formals.promote(em, pass, np, Type());

  // Now, check for independence
  if (haveDependencies(em, this, pass, np))
    return Promote_Dependent;
  else
    return Promote_Success;
}


// ******************************************************************
// *                       choose_rand  class                       *
// ******************************************************************

class choose_rand : public choose_si {
public:
  choose_rand(const type* argtype);
  virtual void Compute(traverse_data &x, expr** pass, int np);
private:
  static double* probarray;
  static int probarray_size;

  inline void expandArray(int np) {
    if (np <= probarray_size) return;
    if (0==probarray_size)  probarray_size = 64;
    while (probarray_size < np) probarray_size += 64;
    double* npa = (double*) realloc(probarray, sizeof(double)*probarray_size);
    if (0==npa) {
      free(probarray);
      probarray_size = 0;
      // no luck this time, but fix ourself for next
    }
    probarray = npa;
  }
};

double* choose_rand::probarray = 0;
int choose_rand::probarray_size = 0;

choose_rand::choose_rand(const type* argt) : choose_si(argt)
{
}

void choose_rand::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  expandArray(np);
  if (0==probarray) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Not enough memory for probability array";
      em->stopIO();
    }
    x.answer->setNull();
    return;
  }

  double total = 0;
  for (int i=0; i<np; i++) {
    x.aggregate = 1;
    SafeCompute(pass[i], x);
    x.aggregate = 0;
    if (!x.answer->isNormal()) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Bad probability for parameter " << i;
        em->stopIO();
      } // error
      x.answer->setNull();
      return;
    }
    if (x.answer->getReal()<0) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Negative probability for parameter " << i;
        em->stopIO();
      } // error
      x.answer->setNull();
      return;
    }
    total += (probarray[i] = x.answer->getReal());
  }
  if (0==total) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Weights sum to zero in call to choose.";
      em->stopIO();
    } // error
    x.answer->setNull();
    return;
  }
  // normalize
  for (int i=0; i<np; i++) probarray[i] /= total;
  // accumulate
  for (int i=1; i<np; i++) probarray[i] += probarray[i-1];

  DCASSERT(x.stream);
  double u = x.stream->Uniform32();

  // find appropriate item
  int i;
  for (i=0; i<np; i++) {
    if (u < probarray[i]) break;
  }
  CHECK_RANGE(0, i, np);

  SafeCompute(pass[i], x);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Analysis  and such                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       prob_finite  class                       *
// ******************************************************************

class prob_finite : public simple_internal {
  engtype* doAvg;
public:
  prob_finite(const type* xtype, engtype* w);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

prob_finite::prob_finite(const type* xtype, engtype* w)
: simple_internal(em->REAL, "prob_finite", 1)
{
  doAvg = w;
  SetFormal(0, xtype, "x");
  SetDocumentation("Determines the probability that x < infinity.");
}

void prob_finite::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  shared_object* foo = x.answer->getPtr();
  phase_hlm* X = smart_cast <phase_hlm*> (foo);
  DCASSERT(X);
  DCASSERT(doAvg);

  result dummy;
  doAvg->runEngine(X, dummy);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    x.answer->setNull();
    return;
  }

  if (proc->knownAcceptProb()) {
    x.answer->setReal(proc->getAcceptProb());
  } else {
    x.answer->setNull();
  }
}


// ******************************************************************
// *                          avg_ph class                          *
// ******************************************************************

class avg_ph : public simple_internal {
  engtype* doAvg;
public:
  avg_ph(const type* xtype, engtype* w);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

avg_ph::avg_ph(const type* xtype, engtype* w)
: simple_internal(em->REAL, "avg", 1)
{
  doAvg = w;
  SetFormal(0, xtype, "x");
  SetDocumentation("Determines the expected value of x.");
}


void avg_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  shared_object* foo = x.answer->getPtr();
  phase_hlm* X = smart_cast <phase_hlm*> (foo);
  DCASSERT(X);
  DCASSERT(doAvg);

  result dummy;
  doAvg->runEngine(X, dummy);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    x.answer->setNull();
    return;
  }

  if (proc->knownMTTA()) {
    if (proc->finiteMTTA()) {
      x.answer->setReal(proc->getMTTA());
    } else {
      x.answer->setInfinity(1);
    }
  } else {
    x.answer->setNull();
  }
}


// ******************************************************************
// *                         avg_rand class                         *
// ******************************************************************

class avg_rand : public func_engine {
public:
  avg_rand(const type* xtype, engtype* w);
protected:
  virtual void BuildParams(traverse_data &x, expr** pass, int np);
};

avg_rand::avg_rand(const type* xtype, engtype* w)
: func_engine(em->REAL, "avg", 1, w)
{
  SetFormal(0, xtype, "x");
  SetDocumentation("Determines the expected value of random variable x.  Unless the variance is not needed, var(x) should be computed before avg(x).");
}

void avg_rand::BuildParams(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  setParam(0).setPtr(Share(pass[0]));
}

// ******************************************************************
// *                          var_ph class                          *
// ******************************************************************

class var_ph : public simple_internal {
  engtype* doVar;
public:
  var_ph(const type* xtype, engtype* w);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

var_ph::var_ph(const type* xtype, engtype* w)
: simple_internal(em->REAL, "var", 1)
{
  doVar = w;
  SetFormal(0, xtype, "x");
  SetDocumentation("Determines the variance of x.  Note that most algorithms to do this must also compute the expected value of x, so for efficiency, var(x) should be computed before avg(x).");
}


void var_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  shared_object* foo = x.answer->getPtr();
  phase_hlm* X = smart_cast <phase_hlm*> (foo);
  DCASSERT(X);
  DCASSERT(doVar);

  result dummy;
  doVar->runEngine(X, dummy);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    x.answer->setNull();
    return;
  }

  if (proc->knownVTTA()) {
    if (proc->finiteVTTA()) {
      x.answer->setReal(proc->getVTTA());
    } else {
      x.answer->setInfinity(1);
    }
  } else {
    x.answer->setNull();
  }
}


// ******************************************************************
// *                       print_range  class                       *
// ******************************************************************

class print_range : public simple_internal {
public:
  print_range(const type* RANDREAL);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

print_range::print_range(const type* RANDREAL)
 : simple_internal(em->VOID, "print_range", 1)
{
  SetFormal(0, RANDREAL, "X");
  SetDocumentation("Prints the range of possible values for random variable X.");
  Hide();  // This is not supposed to be an end-user function
}

void print_range::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.which = traverse_data::FindRange;
  em->cout() << "Random variable ";
  if (pass[0]) {
    pass[0]->Print(em->cout(), 0);
    pass[0]->Traverse(x);
  }
  else {
    em->cout() << "null";
    x.answer->setNull();
  }
  em->cout() << " has range: ";
  shared_object* so = x.answer->getPtr();
  if (0==so)  em->cout() << "null";
  else        so->Print(em->cout(), 0);
  em->cout() << "\n";
  em->cout().flush();
  x.which = traverse_data::Compute;
  x.answer->setNull();
}

// ******************************************************************
// *                         print_ph class                         *
// ******************************************************************

class print_ph : public simple_internal {
  engtype* ProcGen;
public:
  print_ph(const type* PHTYPE);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

print_ph::print_ph(const type* PHTYPE)
 : simple_internal(em->VOID, "print_ph", 1)
{
  SetFormal(0, PHTYPE, "X");
  SetDocumentation("Prints information about phase-type distribution X, primarily for debugging purposes.");
  Hide();   // This is not supposed to be an end-user function
  ProcGen = 0;
}

void print_ph::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[0], x);
  shared_object* foo = (x.answer->isNormal()) ? x.answer->getPtr() : 0;
  if (0==foo) {
    em->cout() << "null phase type\n";
    em->cout().flush();
    return;
  }

  em->cout() << "Information for phase type ";
  if (0==foo) {
    em->cout() << "null\n";
    em->cout().flush();
    return;
  }
  foo->Print(em->cout(), 0);
  em->cout() << "\n";

  phase_hlm* X = dynamic_cast <phase_hlm*> (foo);
  if (0==X) return;

  if (X->isDiscrete()) em->cout() << "Discrete phase type";
  else                 em->cout() << "Continuous phase type";

  em->cout() << ", state has dimension " << X->NumStateVars() << "\n";

  if (0==ProcGen) {
    ProcGen = em->findEngineType("ProcessGeneration");
  }
  if (0==ProcGen) {
    em->cout() << "\tCouldn't build process: no engine type!\n";
    em->cout().flush();
    return;
  }

  result fls;
  fls.setBool(false);
  ProcGen->runEngine(X, fls);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    em->cout() << "\tCouldn't build process\n";
    em->cout().flush();
    return;
  }
  em->cout() << "Reachable states:\n";
  proc->showStates(false);
  em->cout() << proc->getNumStates() << " states total\n";

  long goal = proc->getAcceptingState();
  if (goal < 0) em->cout() << "No accepting state\n";
  else          em->cout() << "Accepting state index: " << goal << "\n";

  long trap = proc->getTrapState();
  if (trap < 0) em->cout() << "No trap state\n";
  else          em->cout() << "Trap state index: " << trap << "\n";

  em->cout() << "Initial distribution:\n    ";
  statedist* pi0 = proc->getInitialDistribution();
  if (pi0) {
    pi0->Print(em->cout(), 0);
    em->cout() << "\n";
  } else {
    em->cout() << "(null)\n";
  }

#ifdef PRINT_PHASE_CLASSES
  long nc = proc->getNumClasses(true);
  em->cout() << nc << " recurrent classes\n";
#endif

  proc->showProc(false);
  em->cout() << proc->getNumArcs() << " edges total\n";

  em->cout() << "End of information for phase type\n\n";
  em->cout().flush();
}


// ******************************************************************
// *                       print_ddist  class                       *
// ******************************************************************

class print_ddist : public simple_internal {
public:
  print_ddist(const type* DPH);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

print_ddist::print_ddist(const type* DPH)
 : simple_internal(em->VOID, "print_dist", 3)
{
  SetFormal(0, DPH, "X");
  SetFormal(1, em->REAL, "epsilon");
  SetFormal(2, em->INT, "max_size", 1000L);
  SetDocumentation("Prints the discrete distribution of X (to precision epsilon>0, limited by max_size), to the current output stream.");
}

void print_ddist::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(3==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) return;
  double epsilon = x.answer->getReal();
  if (epsilon <= 0) {
    OutOfRange(em, x, em->REAL, "print_dist epsilon value ", "");
    return;
  }

  SafeCompute(pass[2], x);
  long maxsize = 1000;
  if (x.answer->isNormal()) {
    maxsize = x.answer->getInt();
    if (maxsize <= 0) {
      OutOfRange(em, x, em->INT, "print_dist max_size value ", "");
      return;
    }
  }

  x.answer->setNull();
  SafeCompute(pass[0], x);
  shared_object* foo = (x.answer->isNormal()) ? x.answer->getPtr() : 0;
  if (0==foo) return;

  em->cout() << "#  PDF of ";
  foo->Print(em->cout(), 0);
  em->cout() << " determined to precision epsilon= " << epsilon << "\n";
  em->cout().flush();

  phase_hlm* X = dynamic_cast <phase_hlm*> (foo);
  if (0==X) return;
  if (!X->isDiscrete()) return;

  engtype* ProcGen = em->findEngineType("ProcessGeneration");
  if (0==ProcGen) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't build process: no engine type!\n";
      em->stopIO();
    }
    return;
  }

  result fls;
  fls.setBool(false);
  ProcGen->runEngine(X, fls);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Expected stochastic process, didn't get one\n";
      em->stopIO();
    }
    return;
  }

  discrete_pdf dist;

  em->cout() << "#\n";
  em->cout().flush();

  bool ok = proc->computeDiscreteTTA(epsilon, maxsize, dist);
  x.answer->setBool(ok);

  if (!ok) {
    em->cout() << "# error, bailing out\n";
    em->cout().flush();
    return;
  }

  // print the distribution


  //
  // Print distribution
  //
  em->cout() << "# N        Prob(reach acceptance at time N)\n";
  em->cout().flush();
  for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
    em->cout() << "  ";
    em->cout().Put(i, -9);
    em->cout() << dist.f(i) << "\n";
    em->cout().flush();
  }
  if (dist.f_infinity()) {
    em->cout() << "#infinity ";
    em->cout() << dist.f_infinity() << "\n";
  }

  //
  // Compute mean and variance from PDF
  //
  em->cout() << "#\n#  Stats determined directly from PDF:\n";
  if (dist.f_infinity()) {
    em->cout() << "#  E[X]  : infinity\n";
    em->cout() << "#  E[X^2]: infinity\n";
    em->cout() << "#  Var[X]: infinity\n";
  } else {
    double mean = 0.0;
    double mom2 = 0.0;
    double var  = 0.0;
    for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
      double mpt = i * dist.f(i);
      mean += mpt;
      mom2 += i * mpt;
    }
    em->cout() << "#  E[X]  : " << mean << "\n";
    em->cout() << "#  E[X^2]: " << mom2 << "\n";
    // hopefully more stable computation of variance
    for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
      double vpt = (i-mean);
      var += vpt * vpt * dist.f(i);
    }
    em->cout() << "#  Var[X]: " << var << "\n";
  }
  em->cout().flush();
}

// ******************************************************************
// *                       print_cdist  class                       *
// ******************************************************************

class print_cdist : public simple_internal {
public:
  print_cdist(const type* CPH);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

print_cdist::print_cdist(const type* CPH)
 : simple_internal(em->VOID, "print_dist", 4)
{
  SetFormal(0, CPH, "X");
  SetFormal(1, em->REAL, "dt");
  SetFormal(2, em->REAL, "epsilon");
  SetFormal(3, em->INT, "max_size", 1000L);
  SetDocumentation("Prints the continuous distribution of X at time points dt, 2*dt, 3*dt, ..., (up to desired precision epsilon>0, limited by max_size), to the current output stream.");
}

void print_cdist::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(4==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  SafeCompute(pass[1], x);
  if (!x.answer->isNormal()) return;
  double dt = x.answer->getReal();
  if (dt <= 0) {
    OutOfRange(em, x, em->REAL, "print_dist dt value ", " (should be positive)");
    return;
  }

  SafeCompute(pass[2], x);
  if (!x.answer->isNormal()) return;
  double epsilon = x.answer->getReal();
  if (epsilon <= 0) {
    OutOfRange(em, x, em->REAL, "print dist epsilon value ", "");
    return;
  }

  SafeCompute(pass[3], x);
  long maxsize = 1000;
  if (x.answer->isNormal()) {
    maxsize = x.answer->getInt();
    if (maxsize <= 0) {
      OutOfRange(em, x, em->INT, "print_dist max_size value ", "");
      return;
    }
  }

  x.answer->setNull();
  SafeCompute(pass[0], x);
  shared_object* foo = (x.answer->isNormal()) ? x.answer->getPtr() : 0;
  if (0==foo) return;

  em->cout() << "#  PDF of ";
  foo->Print(em->cout(), 0);
  em->cout() << ", determined using\n";
  em->cout() << "#\t dt= " << dt << "\n";
  em->cout() << "#\t epsilon= " << epsilon << "\n";
  em->cout().flush();

  phase_hlm* X = dynamic_cast <phase_hlm*> (foo);
  if (0==X) return;
  if (X->isDiscrete()) return;

  engtype* ProcGen = em->findEngineType("ProcessGeneration");
  if (0==ProcGen) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Couldn't build process: no engine type!\n";
      em->stopIO();
    }
    return;
  }

  result fls;
  fls.setBool(false);
  ProcGen->runEngine(X, fls);

  stochastic_lldsm* proc = smart_cast <stochastic_lldsm*> (X->GetProcess());
  if (0==proc) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Expected stochastic process, didn't get one\n";
      em->stopIO();
    }
    return;
  }

  discrete_pdf dist;

  bool ok = proc->computeContinuousTTA(dt, epsilon, maxsize, dist);
  x.answer->setBool(ok);

  if (!ok) return;

  // print the distribution


  //
  // Print distribution
  //
  em->cout() << "# t        PDF at t\n";
  em->cout().flush();
  for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
    em->cout() << "  ";
    em->cout().Put(i*dt, -9);
    em->cout() << dist.f(i) << "\n";
    em->cout().flush();
  }
  if (dist.f_infinity()) {
    em->cout() << "#infinity ";
    em->cout() << dist.f_infinity() << "\n";
  }

  //
  // Compute mean and variance from PDF
  //
  em->cout() << "#\n#  Stats determined directly from PDF:\n";
  em->cout() << "#  (quantization error may be significant)\n";
  if (dist.f_infinity()) {
    em->cout() << "#  E[X]  : infinity\n";
    em->cout() << "#  E[X^2]: infinity\n";
    em->cout() << "#  Var[X]: infinity\n";
  } else {
    double mean = 0.0;
    double mom2 = 0.0;
    double var  = 0.0;
    for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
      double t = i * dt;
      double mpt = t * dist.f(i) * dt;
      mean += mpt;
      mom2 += t * mpt;
    }
    em->cout() << "#  E[X]  : " << mean << "\n";
    em->cout() << "#  E[X^2]: " << mom2 << "\n";
    // hopefully more stable computation of variance
    for (long i=dist.left_trunc(); i<=dist.right_trunc(); i++) {
      double t = i * dt;
      double vpt = (t-mean);
      var += vpt * vpt * dist.f(i) * dt;
    }
    em->cout() << "#  Var[X]: " << var << "\n";
  }
  em->cout().flush();
}


// ******************************************************************
// *                        print_deps class                        *
// ******************************************************************

class print_deps : public simple_internal {
public:
  print_deps(const type* PHTYPE);
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

print_deps::print_deps(const type* PHTYPE)
 : simple_internal(em->VOID, "print_deps", 1)
{
  SetFormal(0, PHTYPE, "X");
  SetDocumentation("Prints dependency information about phase-type distribution X, primarily for debugging purposes.");
  Hide();   // This is not supposed to be an end-user function
}

void print_deps::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(1==np);
  DCASSERT(x.answer);
  x.answer->setNull();

  if (0==pass[0]) {
    em->cout() << "null distribution\n";
    em->cout().flush();
    return;
  }
  em->cout() << "Phase type distribution ";
  pass[0]->Print(em->cout(), 0);

  List <symbol> dlist;
  int numdeps = pass[0]->BuildSymbolList(traverse_data::GetVarDeps, 0, &dlist);
  if (numdeps) {
    em->cout() << " has " << numdeps << " variable dependencies:\n";
  } else {
    em->cout() << " has no variable dependencies\n";
  }
  for (int i=0; i<numdeps; i++) {
    em->cout() << "\t";
    const symbol* item = dlist.ReadItem(i);
    item->Print(em->cout(), 0);
    if (item->Type()->getModifier() != PHASE) em->cout() << " (ignored)";
    em->cout() << "\n";
  }
  em->cout().flush();
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                        type  promotions                        *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                     expo_promotions  class                     *
// *                                                                *
// ******************************************************************

/** Type promotions for expo.
  expo -> ph real
  expo -> rand real
  expo -> proc ph real
  expo -> proc rand real
  proc expo -> proc ph real
  proc expo -> proc rand real
*/
class expo_promotions : public general_conv {
public:
  expo_promotions();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return false;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new typecast(W, t, e);
  }
};

expo_promotions::expo_promotions() : general_conv()
{
}

int expo_promotions::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->getBaseType() != em->EXPO) return -1;
  if (dest->getBaseType() != em->REAL) return -1;
  if (dest->getModifier() != PHASE && dest->getModifier() != RAND) return -1;
  if (src->hasProc() && !dest->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;

  int d = 0;
  if (dest->getModifier() == PHASE) d = MAKE_PHASE;
  else                              d = MAKE_RAND;
  if (src->hasProc() != dest->hasProc()) d += MAKE_PROC;
  return d;
}



// ******************************************************************
// *                                                                *
// *                        int2phint  class                        *
// *                                                                *
// ******************************************************************

/** Type promotions from int to phase int:
  int -> ph int
  int -> proc ph int
  proc int -> proc ph int
*/
class int2phint : public general_conv {

    class converter : public typecast {
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  int2phint();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return true;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new converter(W, t, e);
  }
};

int2phint::converter
 ::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  silent = true;
}

void int2phint::converter::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  SafeCompute(opnd, x);
  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity()>0) {
      shared_object* X = makeInfinity(true);
      x.answer->setPtr(X);
      return;
    }
    // must be -infinity
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "integer -" << type::getInfinityString();
      em->cerr() << " cannot be converted to type phase int";
      em->stopIO();
    }
    x.answer->setNull();
    return;
  }

  if (x.answer->isNormal()) {
    long a = x.answer->getInt();
    if (a<0) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "integer " << a;
        em->cerr() << " cannot be converted to type phase int";
        em->stopIO();
      }
      x.answer->setNull();
    } else {
      shared_object* X = makeConst(a);
      x.answer->setPtr(X);
    }
    return;
  }
  x.answer->setNull();
}

void int2phint::converter::Traverse(traverse_data &x)
{
  if (x.which != traverse_data::FindRange) {
    typecast::Traverse(x);
    return;
  }
  x.which = traverse_data::Compute;
  SafeCompute(opnd, x);
  interval_object *range = new interval_object(*x.answer, Type(0));
  x.answer->setPtr(range);
  x.which = traverse_data::FindRange;
}

int2phint::int2phint() : general_conv()
{
}

int int2phint::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->getBaseType() != em->INT) return -1;
  if (dest->getBaseType() != em->INT) return -1;
  if (src->getModifier() != DETERM) return -1;
  if (dest->getModifier() != PHASE) return -1;
  if (src->hasProc() && !dest->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;

  int d = MAKE_PHASE;
  if (src->hasProc() != dest->hasProc()) d += MAKE_PROC;
  return d;
}



// ******************************************************************
// *                                                                *
// *                       real2phreal  class                       *
// *                                                                *
// ******************************************************************

/** Type promotions from real to phase real:
  real -> ph real
  real -> proc ph real
  proc real -> proc ph real
*/
class real2phreal : public general_conv {

    class converter : public typecast {
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  real2phreal();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return true;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new converter(W, t, e);
  }
};

real2phreal::converter
 ::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  silent = true;
}

void real2phreal::converter::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  SafeCompute(opnd, x);
  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity()>0) {
      shared_object* X = makeInfinity(false);
      x.answer->setPtr(X);
      return;
    }
    // must be -infinity
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "real -" << type::getInfinityString();
      em->cerr() << " cannot be converted to type phase real";
      em->stopIO();
    }
    x.answer->setNull();
    return;
  }

  if (x.answer->isNormal()) {
    if (0.0==x.answer->getReal()) {
      shared_object* X = makeZero(false);
      x.answer->setPtr(X);
      return;
    } else {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "real " << x.answer->getReal();
        em->cerr() << " cannot be converted to type phase real";
        em->stopIO();
      }
    }
  }
  x.answer->setNull();
}

void real2phreal::converter::Traverse(traverse_data &x)
{
  if (x.which != traverse_data::FindRange) {
    typecast::Traverse(x);
    return;
  }
  x.which = traverse_data::Compute;
  SafeCompute(opnd, x);
  interval_object *range = new interval_object(*x.answer, Type(0));
  x.answer->setPtr(range);
  x.which = traverse_data::FindRange;
}

real2phreal::real2phreal() : general_conv()
{
}

int real2phreal::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->getBaseType() != em->REAL) return -1;
  if (dest->getBaseType() != em->REAL) return -1;
  if (src->getModifier() != DETERM) return -1;
  if (dest->getModifier() != PHASE) return -1;
  if (src->hasProc() && !dest->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;

  int d = MAKE_PHASE;
  if (src->hasProc() != dest->hasProc()) d += MAKE_PROC;
  return d;
}


// ******************************************************************
// *                                                                *
// *                         ph2rand  class                         *
// *                                                                *
// ******************************************************************

/** Type promotions from ph x to rand x:
      ph x --> rand x
      ph x --> proc rand x
      proc ph x --> proc rand x
*/
class ph2rand : public general_conv {

    class converter : public typecast {
      phase_hlm* cached;
      bool precomputed;
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  ph2rand();
  virtual int getDistance(const type* src, const type* dest) const;
  virtual bool requiresConversion(const type* src, const type* dest) const {
    return true;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new converter(W, t, e);
  }
};

ph2rand::converter
 ::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  precomputed = false;
  cached = 0;
  silent = true;
}

void ph2rand::converter::Compute(traverse_data &x)
{
  if (!precomputed) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(this);
      em->internal() << "Expression not precomputed: ";
      Print(em->internal(), 0);
      em->stopIO();
    }
    exit(1);
  }
  DCASSERT(x.answer);
  if (cached) cached->Sample(x);
  else        x.answer->setNull();
}

void ph2rand::converter::Traverse(traverse_data &x)
{
  if (x.which != traverse_data::PreCompute) {
    typecast::Traverse(x);
    return;
  }
  result answer;
  result* save = x.answer;
  x.answer = &answer;
  x.which = traverse_data::Compute;
  SafeCompute(opnd, x);
  x.which = traverse_data::PreCompute;
  x.answer = save;
  cached = smart_cast <phase_hlm*> ( Share(answer.getPtr()) );
  precomputed = true;
}

ph2rand::ph2rand() : general_conv()
{
}

int ph2rand::getDistance(const type* src, const type* dest) const
{
  DCASSERT(src != dest);
  DCASSERT(em);
  if (src->getModifier() != PHASE) return -1;
  if (dest->getModifier() != RAND) return -1;
  if (src->hasProc() && !dest->hasProc()) return -1;
  if (src->isASet()) return -1;
  if (dest->isASet()) return -1;
  if (src->getBaseType() != dest->getBaseType()) return -1;
  int d = MAKE_RAND;
  if (dest->hasProc() != src->hasProc()) d += MAKE_PROC;
  return d;
}


// ******************************************************************
// *                                                                *
// *                      phint2randreal class                      *
// *                                                                *
// ******************************************************************

/** The name says it all: type promotions from ph int to rand real.
*/
class phint2randreal : public specific_conv {

    class converter : public typecast {
      phase_hlm* cached;
      bool precomputed;
    public:
      converter(const location &W, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
      virtual void Traverse(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Where(), Type(), x);
      }
    };

public:
  phint2randreal();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    DCASSERT(em->INT);
    if (src->getBaseType() != em->INT)  return -1;
    if (src->getModifier() != PHASE)    return -1;
    return SIMPLE_CONV + MAKE_RAND;
  }
  virtual const type* promotesTo(const type* src) const {
    DCASSERT(src);
    const type* dest = em->REAL;
    DCASSERT(dest);
    dest = dest->modifyType(RAND);
    DCASSERT(dest);
    if (src->hasProc()) dest = dest->addProc();
    DCASSERT(dest);
    return dest;
  }
  virtual expr* convert(const location &W, expr* e, const type* t) const {
    return new converter(W, t, e);
  }
};

phint2randreal::converter
 ::converter(const location &W, const type* nt, expr* x)
 : typecast(W, nt, x)
{
  precomputed = false;
  cached = 0;
}

void phint2randreal::converter::Compute(traverse_data &x)
{
  if (!precomputed) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(this);
      em->internal() << "Expression not precomputed: ";
      Print(em->internal(), 0);
      em->stopIO();
    }
    exit(1);
  }
  DCASSERT(x.answer);
  if (cached) {
    cached->Sample(x);
    if (x.answer->isNormal()) x.answer->setReal(x.answer->getInt());
  } else {
    x.answer->setNull();
  }
}

void phint2randreal::converter::Traverse(traverse_data &x)
{
  if (x.which != traverse_data::PreCompute) {
    typecast::Traverse(x);
    return;
  }
  result answer;
  result* save = x.answer;
  x.answer = &answer;
  x.which = traverse_data::Compute;
  SafeCompute(opnd, x);
  x.which = traverse_data::PreCompute;
  x.answer = save;
  cached = smart_cast <phase_hlm*> ( Share(answer.getPtr()) );
  precomputed = true;
}


phint2randreal::phint2randreal() : specific_conv(false)
{
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_stochtypes : public initializer {
  public:
    init_stochtypes();
    virtual bool execute();
};
init_stochtypes the_stochtype_initializer;


init_stochtypes::init_stochtypes() : initializer("init_stochtypes")
{
  usesResource("em");
  usesResource("st");
  buildsResource("stochtypes");
  buildsResource("types");
}

bool init_stochtypes::execute()
{
  if (0==em)        return false;
  if (0==st)        return false;
  if (0==em->BOOL)  return false;
  if (0==em->INT)   return false;
  if (0==em->REAL)  return false;

  simple_type* t_expo  = new simple_type("expo", "Exponential distribution", "Special type for the exponential distribution.");
  type* t_proc_expo  = newProcType("proc expo", t_expo);

  type* t_ph_int  = newModifiedType("ph int", PHASE, em->INT);
  type* t_proc_ph_int = newProcType("proc ph int", t_ph_int);

  type* t_ph_real  = newModifiedType("ph real", PHASE, em->REAL);
  type* t_proc_ph_real= newProcType("proc ph real", t_ph_real);

  type* t_rand_bool  = newModifiedType("rand bool", RAND, em->BOOL);
  type* t_proc_rand_bool= newProcType("proc rand bool", t_rand_bool);

  type* t_rand_int  = newModifiedType("rand int", RAND, em->INT);
  type* t_proc_rand_int = newProcType("proc rand int", t_rand_int);

  type* t_rand_real  = newModifiedType("rand real", RAND, em->REAL);
  type* t_proc_rand_real= newProcType("proc rand real", t_rand_real);


  // register types
  em->registerType( t_expo            );
  em->registerType( t_proc_expo       );

  em->registerType( t_ph_int          );
  em->registerType( t_proc_ph_int     );
  em->registerType( t_ph_real         );
  em->registerType( t_proc_ph_real    );

  em->registerType( t_rand_bool       );
  em->registerType( t_proc_rand_bool  );
  em->registerType( t_rand_int        );
  em->registerType( t_proc_rand_int   );
  em->registerType( t_rand_real       );
  em->registerType( t_proc_rand_real  );

  em->setFundamentalTypes();

  // Operations
  em->registerOperation( new phase_add_op   );
  em->registerOperation( new phase_mult_op  );

  // expo multiplication...

  // Type changes
  em->registerConversion( new expo_promotions );
  em->registerConversion( new int2phint       );
  em->registerConversion( new real2phreal     );
  em->registerConversion( new ph2rand         );
  em->registerConversion( new phint2randreal  );

  // Engines
  engtype* AvgPh = MakeEngineType(em,
      "AvgPh",
      "Algorithm to use for computing the expected value of a phase expression.",
      engtype::Model
  );
  engtype* VarPh = MakeEngineType(em,
      "VarPh",
      "Algorithm to use for computing the variance of a phase expression.",
      engtype::Model
  );
  engtype* AvgRandReal = MakeEngineType(em,
      "AvgRandReal",
      "Algorithm to use for computing the expected value of a rand real expression, or the probability of a rand bool expression.",
      engtype::FunctionCall
  );

  // Functions
  st->AddSymbol( new prob_finite(t_ph_int, AvgPh)             );
  st->AddSymbol( new prob_finite(t_ph_real, AvgPh)            );

  st->AddSymbol( new avg_ph(t_ph_int, AvgPh)                  );
  st->AddSymbol( new avg_ph(t_ph_real, AvgPh)                 );
  st->AddSymbol( new avg_rand(t_rand_real, AvgRandReal)       );

  st->AddSymbol( new var_ph(t_ph_int, VarPh)                  );
  st->AddSymbol( new var_ph(t_ph_real, VarPh)                 );

  st->AddSymbol( new bernoulli_ph(t_ph_int, em->REAL)         );
  st->AddSymbol( new bernoulli_rand(t_rand_int, t_rand_real)  );

  st->AddSymbol( new geometric_ph(t_ph_int, em->REAL)         );
  st->AddSymbol( new geometric_rand(t_rand_int, t_rand_real)  );

  st->AddSymbol( new equilikely_ph(t_ph_int, em->INT)         );
  st->AddSymbol( new equilikely_rand(t_rand_int, t_rand_int)  );

  st->AddSymbol( new binomial_ph(t_ph_int, em->INT, em->REAL) );
  st->AddSymbol( new binomial_rand(t_rand_int, t_rand_int, t_rand_real) );

  st->AddSymbol( new expo_ph(t_expo, em->REAL)                );
  st->AddSymbol( new erlang_ph(t_ph_real, em->INT, em->REAL)  );

  st->AddSymbol( new expo_rand(t_rand_real, t_rand_real)      );
  st->AddSymbol( new erlang_rand(t_rand_real, t_rand_int, t_rand_real)  );
  st->AddSymbol( new uniform(t_rand_real)                     );

  st->AddSymbol( new choose_ph(t_ph_int)                      );
  st->AddSymbol( new choose_ph(t_ph_real)                     );
  st->AddSymbol( new choose_rand(t_rand_int)                  );
  st->AddSymbol( new choose_rand(t_rand_real)                 );

  st->AddSymbol( new cph2dph_unif(t_ph_real, t_ph_int)        );
  st->AddSymbol( new cph2dph_embed(t_ph_real, t_ph_int)       );
  st->AddSymbol( new max_ph(t_ph_int)                         );
  st->AddSymbol( new max_ph(t_ph_real)                        );
  st->AddSymbol( new min_ph(t_ph_int)                         );
  st->AddSymbol( new min_ph(t_ph_real)                        );
  st->AddSymbol( new order_ph(t_ph_int)                       );
  st->AddSymbol( new order_ph(t_ph_real)                      );

  st->AddSymbol( new print_range(t_rand_real)                 );
  st->AddSymbol( new print_ph(t_ph_int)                       );
  st->AddSymbol( new print_ph(t_ph_real)                      );
  st->AddSymbol( new print_deps(t_ph_int)                     );
  st->AddSymbol( new print_deps(t_ph_real)                    );
  st->AddSymbol( new print_ddist(t_ph_int)                    );
  st->AddSymbol( new print_cdist(t_ph_real)                   );

  return true;
}

