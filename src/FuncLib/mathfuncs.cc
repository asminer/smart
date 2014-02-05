
// $Id$

#define MATHFUNCS_DETAILED
#include "mathfuncs.h"

#include "../ExprLib/exprman.h"
#include "../ExprLib/intervals.h"
#include "../SymTabs/symtabs.h"
#include "../Streams/streams.h"

#include <math.h>

// #define DEBUG_ORDER

// ******************************************************************
// *                        intdiv_si  class                        *
// ******************************************************************

class intdiv_si : public simple_internal {
public:
  intdiv_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

intdiv_si::intdiv_si() : simple_internal(em->INT, "div", 2)
{
  SetFormal(0, em->INT, "a");
  SetFormal(1, em->INT, "b");
  SetDocumentation("Integer division: computes int(a/b).");
}

void intdiv_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(2==np);
  result* ans = x.answer;
  long l_int;
  bool l_infty;
  if (pass[0])  pass[0]->Compute(x);
  else          ans->setNull();
  if (ans->isNormal()) {
    l_int = ans->getInt();
    l_infty = false;
  } else if (ans->isInfinity()) {
    l_int = ans->signInfinity();
    l_infty = true;
  } else return;  // some kind of error, propogate it

  long r_int;
  bool r_infty;
  if (pass[1])  pass[1]->Compute(x);
  else    ans->setNull();
  if (ans->isNormal()) {
    r_int = ans->getInt();
    r_infty = false;
  } else if (ans->isInfinity()) {
    r_int = ans->signInfinity();
    r_infty = true;
  } else return;  // some kind of error, propogate it

  if (0==r_int) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Illegal operation: divide by 0";
      em->stopIO();
    }
    ans->setNull();
    return;
  }

  if (r_infty) {
    if (l_infty) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Illegal operation: infty / infty";
        em->stopIO();
      }
      ans->setNull();
      return;
    }
    // a div infinity = 0.
    ans->setInt(0);
    return;
  }

  // Still here? Divisor is finite.
  if (l_infty) {
    ans->setInfinity( l_int * r_int );
    return;
  }

  // normal finite division
  ans->setInt(l_int / r_int);
}

// ******************************************************************
// *                          pow_si class                          *
// ******************************************************************

class pow_si : public simple_internal {
public:
  pow_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline void undefined(traverse_data &x, const result &r) const {
    if (em->startError()) {
      DCASSERT(em->REAL);
      DCASSERT(em->INT);
      em->causedBy(x.parent);
      em->cerr() << "pow(";
      em->REAL->print(em->cerr(), r);
      em->cerr() << ", ";
      em->INT->print(em->cerr(), *(x.answer));
      em->cerr() << ") is undefined";
      em->stopIO();
    }
    x.answer->setNull();
  }

  static double pow(double x, long n);
};

pow_si::pow_si() : simple_internal(em->REAL, "pow", 2)
{
  SetFormal(0, em->REAL, "x");
  SetFormal(1, em->INT, "n");
  SetDocumentation("Computes and returns x to the power of n.");
}

void pow_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  DCASSERT(2==np);
  SafeCompute(pass[0], x);
  if (x.answer->isUnknown() || x.answer->isNull()) return;
  result first = *(x.answer);

  SafeCompute(pass[1], x);
  if (x.answer->isUnknown() || x.answer->isNull()) return;

  if (x.answer->isNormal()) {
    long n = x.answer->getInt();
    if (first.isNormal()) {
      double r = first.getReal();
      if (0.0==r) {
        if (n>0) {
          x.answer->setReal(0.0);
          return;
        }
        undefined(x, first); // 0^0, or 0^-n
        return;
      }
      x.answer->setReal( pow(first.getReal(), n) );
      return;
    }
    DCASSERT(first.isInfinity());
    if (0==n) {
      undefined(x, first);
      return;
    }
    if (n<0) {
      x.answer->setReal(0.0);
      return;
    }
    if (n%2==0) {
      x.answer->setInfinity(1);
    } else {
      x.answer->setInfinity(first.signInfinity());
    }
    return;
  } // n is normal

  DCASSERT(x.answer->isInfinity());

  if (first.isNormal()) {
    // Handle all finite to infinite power cases.
    double r = first.getReal();
    if (0.0==r) {
      undefined(x, first); // 0^infinity, 0^-infinity
      return;
    }
    if (r>=1) {
      if (x.answer->signInfinity()<0) x.answer->setReal(0);
      else                            x.answer->setInfinity(1);
      return;
    }
    if (r>0) {
      if (x.answer->signInfinity()<0) x.answer->setInfinity(1);
      else                            x.answer->setReal(0);
      return;
    }
    if (r>-1) {
      if (x.answer->signInfinity()<0) undefined(x, first);
      else                            x.answer->setReal(0);
      return;
    }
    DCASSERT(r<=-1);
    if (x.answer->signInfinity()<0) x.answer->setReal(0);
    else                            undefined(x, first);
    return;
  }

  DCASSERT(first.isInfinity());

  // deal with (+/-) infinity ^ -infinity
  if (x.answer->signInfinity()<0) {
    x.answer->setReal(0);
    return;
  }
  
  // deal with -infinity ^ infinity
  if (first.signInfinity()<0) {
    undefined(x, first);
    return;
  }

  // we should be left with infinity ^ infinity.
}

double pow_si::pow(double x, long n)
{
  if (n<0) return 1.0 / pow(x, -n);
  if (0==n) return 1.0;
  if (n%2==0) {
    double s = pow(x, n/2);
    return s*s;
  }
  // n is odd
  return x * pow(x, n-1);
}

// ******************************************************************
// *                          exp_si class                          *
// ******************************************************************

class exp_si : public simple_internal {
public:
  exp_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

exp_si::exp_si() : simple_internal(em->REAL, "exp", 1)
{
  SetFormal(0, em->REAL, "x");
  SetDocumentation("Computes and returns e to the power of x.");
}

void exp_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  SafeCompute(pass[0], x);
  if (x.answer->isUnknown() || x.answer->isNull()) return;

  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity()<0) {
      // exp(-infty) = 0
      x.answer->setReal(0);
    }
    return;
  } 
  x.answer->setReal( exp(x.answer->getReal()) );
}

// ******************************************************************
// *                          log_si class                          *
// ******************************************************************

class log_si : public simple_internal {
public:
  log_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

log_si::log_si() : simple_internal(em->REAL, "ln", 1)
{
  SetFormal(0, em->REAL, "x");
  SetDocumentation("Computes and returns the natural logarithm of x.  If x is 0, -infinity is returned.");
}

void log_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  SafeCompute(pass[0], x);
  if (x.answer->isUnknown() || x.answer->isNull()) return;

  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity()>0) return;   // log(infty) = infty;
  } else {
    DCASSERT(x.answer->isNormal());
    if (x.answer->getReal()>0) {
      x.answer->setReal(log(x.answer->getReal()));
      return;
    }
    if (x.answer->getReal()==0) {
      x.answer->setInfinity(-1);
      return;
    }
  }

  // negative log, error
  if (em->startError()) {
    em->causedBy(x.parent);
    em->cerr() << "log with negative argument: ";
    DCASSERT(em->REAL);
    em->REAL->print(em->cerr(), *x.answer);
    em->stopIO();
  }
  x.answer->setNull();
}

// ******************************************************************
// *                         sqrt_si  class                         *
// ******************************************************************

class sqrt_si : public simple_internal {
public:
  sqrt_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

sqrt_si::sqrt_si() : simple_internal(em->REAL, "sqrt", 1)
{
  SetFormal(0, em->REAL, "x");
  SetDocumentation("Computes and returns the positive square root of x.");
}

void sqrt_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  DCASSERT(1==np);
  SafeCompute(pass[0], x);
  if (x.answer->isUnknown() || x.answer->isNull()) return;

  if (x.answer->isInfinity()) {
    if (x.answer->signInfinity()>0) return;  // sqrt(infty) = infty
  } else {
    DCASSERT(x.answer->isNormal());
    if (x.answer->getReal()>=0) {
      x.answer->setReal(sqrt(x.answer->getReal()));
      return;
    }
  }
  
  // negative square root, error (we don't have complex)
  if (em->startError()) {
    em->causedBy(x.parent);
    em->cerr() << "Square root with negative argument: ";
    DCASSERT(em->REAL);
    em->REAL->print(em->cerr(), *x.answer);
    em->stopIO();
  }
  x.answer->setNull();
}

// ******************************************************************
// *                         max_si  methods                        *
// ******************************************************************

/*
This class is given in the header file!
*/

max_si::max_si(const type* args) : simple_internal(args, "max", 1)
{
  SetFormal(0, args, "x");
  SetRepeat(0);
  SetDocumentation("Returns the largest passed argument.");
}

int max_si::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  DCASSERT(x.answer);
  DCASSERT(pass[0]);
  pass[0]->Traverse(x);
  if (!x.answer->isNormal())  return 0;
  const interval_object* x0 = smart_cast<interval_object*>(x.answer->getPtr());
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
    computeMaximum(*answer, *answer, *xi);
  } // for i
  x.answer->setPtr(answer);
  return 0;
}


// ******************************************************************
// *                         imax_si  class                         *
// ******************************************************************

class imax_si : public max_si {
public:
  imax_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

imax_si::imax_si() : max_si(em->INT)
{
  // nothing to do
}

void imax_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  bool has_unknown = 0;
  int has_infinity = 0;
  
  long max;
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    max = x.answer->getInt();
  } else if (x.answer->isInfinity()) {
    has_infinity = x.answer->signInfinity();
    max = 0;  // keep compiler happy
  } else if (x.answer->isUnknown()) {
    has_unknown = 1;
    max = 0;
  } else {
    // other error or null
    return;
  }
  for (int i=1; i<np; i++) {
    SafeCompute(pass[i], x);
    if (x.answer->isNormal()) {
      if (0==has_infinity) {
        max = MAX(max, x.answer->getInt());
      } else if (has_infinity < 0) {
        max = x.answer->getInt();
        has_infinity = 0;
      }
      continue;
    }
    if (x.answer->isInfinity()) {
      has_infinity = MAX(has_infinity, x.answer->signInfinity());
      continue;
    }
    if (x.answer->isUnknown()) {
      has_unknown = 1;
      continue;
    }
    // other error or null
    return;
  }

  if (0==has_unknown && 0==has_infinity) {
    x.answer->setInt(max);
    return;
  }
  if (has_infinity > 0) {
    x.answer->setInfinity(1);
    return;
  }
  if (has_unknown) {
    x.answer->setUnknown();
    return;
  }
  DCASSERT(has_infinity < 0);
  x.answer->setInfinity(-1);
}


// ******************************************************************
// *                         rmax_si  class                         *
// ******************************************************************

class rmax_si : public max_si {
public:
  rmax_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

rmax_si::rmax_si() : max_si(em->REAL)
{
  // Nothing to do
}

void rmax_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  bool has_unknown = 0;
  int has_infinity = 0;
  
  double max;
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    max = x.answer->getReal();
  } else if (x.answer->isInfinity()) {
    has_infinity = x.answer->signInfinity();
    max = 0;  // keep compiler happy
  } else if (x.answer->isUnknown()) {
    has_unknown = 1;
    max = 0;
  } else {
    // other error or null
    return;
  }
  for (int i=1; i<np; i++) {
    SafeCompute(pass[i], x);
    if (x.answer->isNormal()) {
      if (0==has_infinity) {
        max = MAX(max, x.answer->getReal());
      } else if (has_infinity < 0) {
        max = x.answer->getInt();
        has_infinity = 0;
      }
      continue;
    }
    if (x.answer->isInfinity()) {
      has_infinity = MAX(has_infinity, x.answer->signInfinity());
      continue;
    }
    if (x.answer->isUnknown()) {
      has_unknown = 1;
      continue;
    }
    // other error or null
    return;
  }

  if (0==has_unknown && 0==has_infinity) {
    x.answer->setReal(max);
    return;
  }
  if (has_infinity > 0) {
    x.answer->setInfinity(1);
    return;
  }
  if (has_unknown) {
    x.answer->setUnknown();
    return;
  }
  DCASSERT(has_infinity < 0);
  x.answer->setInfinity(-1);
}

// ******************************************************************
// *                         min_si  methods                        *
// ******************************************************************

/*
This class is given in the header file!
*/

min_si::min_si(const type* args) : simple_internal(args, "min", 1)
{
  SetFormal(0, args, "x");
  SetRepeat(0);
  SetDocumentation("Returns the smallest passed argument.");
}

int min_si::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  DCASSERT(x.answer);
  DCASSERT(pass[0]);
  pass[0]->Traverse(x);
  if (!x.answer->isNormal())  return 0;
  const interval_object* x0 = smart_cast<interval_object*>(x.answer->getPtr());
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
    computeMinimum(*answer, *answer, *xi);
  } // for i
  x.answer->setPtr(answer);
  return 0;
}


// ******************************************************************
// *                         imin_si  class                         *
// ******************************************************************

class imin_si : public min_si {
public:
  imin_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

imin_si::imin_si() : min_si(em->INT)
{
  // Nothing to do
}

void imin_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  bool has_unknown = 0;
  int has_infinity = 0;
  
  long min;
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    min = x.answer->getInt();
  } else if (x.answer->isInfinity()) {
    has_infinity = x.answer->signInfinity();
    min = 0;  // keep compiler happy
  } else if (x.answer->isUnknown()) {
    has_unknown = 1;
    min = 0;
  } else {
    // other error or null
    return;
  }
  for (int i=1; i<np; i++) {
    SafeCompute(pass[i], x);
    if (x.answer->isNormal()) {
      if (0==has_infinity) {
        min = MIN(min, x.answer->getInt());
      } else if (has_infinity > 0) {
        min = x.answer->getInt();
        has_infinity = 0;
      }
      continue;
    }
    if (x.answer->isInfinity()) {
      has_infinity = MIN(has_infinity, x.answer->signInfinity());
      continue;
    }
    if (x.answer->isUnknown()) {
      has_unknown = 1;
      continue;
    }
    // other error or null
    return;
  }

  if (0==has_unknown && 0==has_infinity) {
    x.answer->setInt(min);
    return;
  }
  if (has_infinity < 0) {
    x.answer->setInfinity(-1);
    return;
  }
  if (has_unknown) {
    x.answer->setUnknown();
    return;
  }
  DCASSERT(has_infinity > 0);
  x.answer->setInfinity(1);
}

// ******************************************************************
// *                         rmin_si  class                         *
// ******************************************************************

class rmin_si : public min_si {
public:
  rmin_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

rmin_si::rmin_si() : min_si(em->REAL)
{
  // Nothing to do
}

void rmin_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  bool has_unknown = 0;
  int has_infinity = 0;
  
  double min;
  SafeCompute(pass[0], x);
  if (x.answer->isNormal()) {
    min = x.answer->getReal();
  } else if (x.answer->isInfinity()) {
    has_infinity = x.answer->signInfinity();
    min = 0;  // keep compiler happy
  } else if (x.answer->isUnknown()) {
    has_unknown = 1;
    min = 0;
  } else {
    // other error or null
    return;
  }
  for (int i=1; i<np; i++) {
    SafeCompute(pass[i], x);
    if (x.answer->isNormal()) {
      if (0==has_infinity) {
        min = MIN(min, x.answer->getReal());
      } else if (has_infinity > 0) {
        min = x.answer->getReal();
        has_infinity = 0;
      }
      continue;
    }
    if (x.answer->isInfinity()) {
      has_infinity = MIN(has_infinity, x.answer->signInfinity());
      continue;
    }
    if (x.answer->isUnknown()) {
      has_unknown = 1;
      continue;
    }
    // other error or null
    return;
  }

  if (0==has_unknown && 0==has_infinity) {
    x.answer->setReal(min);
    return;
  }
  if (has_infinity < 0) {
    x.answer->setInfinity(-1);
    return;
  }
  if (has_unknown) {
    x.answer->setUnknown();
    return;
  }
  DCASSERT(has_infinity > 0);
  x.answer->setInfinity(1);
}

// ******************************************************************
// *                        order_si  methods                       *
// ******************************************************************

/*
This class is given in the header file!
*/

order_si::order_si(const type* args) : simple_internal(args, "ord", 2)
{
  SetFormal(0, em->INT, "k");
  SetFormal(1, args, "x");
  SetRepeat(1);
  SetDocumentation("Returns the kth smallest of the passed arguments, or null if k is out of range (1..last).");
}

int order_si::Traverse(traverse_data &x, expr** pass, int np)
{
  if (x.which != traverse_data::FindRange)
        return simple_internal::Traverse(x, pass, np);

  DCASSERT(x.answer);
  DCASSERT(pass[0]);
  pass[0]->Traverse(x);
  if (!x.answer->isNormal())  return 0;

  // determine, and save, the possible values for k
  const interval_object* Kr = smart_cast <interval_object*>(x.answer->getPtr());
  long kmin = 1;
  long kmax = np-1;
  if (Kr->Left().isNormal()) {
    long kleft = long(Kr->Left().getValue());
    if (!Kr->Left().contains()) kleft++;
    kmin = MAX(kmin, kleft); 
  }
  if (Kr->Right().isNormal()) {
    long kright = long(Kr->Right().getValue());
    if (!Kr->Right().contains()) kright--;
    kmax = MIN(kmax, kright);
  }
  if (kmin > kmax) {
    // This happens when kleft is larger than it should be
    x.answer->setNull();
    return 0;
  }

  // build our answer interval, for later
  interval_object* range = new interval_object;

  // determine, and save, the ranges for the repeating arguments
  bool left_null = false;
  bool right_null = false;
  bool left_unknown = false;
  bool right_unknown = false;
  interval_object** Ar = new interval_object* [np];
  Ar[0] = 0;
  for (int i=1; i<np; i++) {
    pass[i]->Traverse(x);
    Ar[i] = smart_cast <interval_object*>(Share(x.answer->getPtr()));
    if (Ar[i]->Left().isNull())     left_null = true;
    if (Ar[i]->Right().isNull())    right_null = true;
    if (Ar[i]->Left().isUnknown())  left_unknown = true;
    if (Ar[i]->Right().isUnknown()) right_unknown = true;
  }

#ifdef DEBUG_ORDER
    em->cout() << "Got argument ranges:\n";
    for (int i=1; i<np; i++) {
      em->cout() << "    ";
      Ar[i]->Print(em->cout(), 0);
      em->cout() << "\n";
    }
#endif


  //
  // Determine left point, including special cases
  //

  if (left_null) {
    range->Left().setNull();
  } else if (left_unknown) {
    range->Left().setUnknown();
  } else {
    // Insertion-like sort of the left points
    // so we can determine the kth smallest
    // (this should NOT be critical code, no need for heapsort)
    for (int i=1; i+1<np; i++) {
      for (int j=i+1; j<np; j++) {
        sortLeft(Ar[i], Ar[j]);
      }
    } // for i

#ifdef DEBUG_ORDER
    em->cout() << "Sorted by left points:\n";
    for (int i=1; i<np; i++) {
      em->cout() << "    ";
      Ar[i]->Print(em->cout(), 0);
      em->cout() << "\n";
    }
#endif

    // Find kth smallest, for smallest possible k
    // and set this as our left endpoint
    range->Left() = Ar[kmin]->Left();
  }

  //
  // Determine right point, including special cases
  //

  if (right_null) {
    range->Right().setNull();
  } else if (right_unknown) {
    range->Right().setUnknown();
  } else {
    // Insertion-like sort of the right points
    // so we can determine the kth smallest
    for (int i=1; i+1<np; i++) {
      for (int j=i+1; j<np; j++) {
        sortRight(Ar[i], Ar[j]);
      }
    } // for i

#ifdef DEBUG_ORDER
    em->cout() << "Sorted by right points:\n";
    for (int i=1; i<np; i++) {
      em->cout() << "    ";
      Ar[i]->Print(em->cout(), 0);
      em->cout() << "\n";
    }
#endif

    // Find kth smallest, for smallest possible k
    // and set this as our right endpoint
    range->Right() = Ar[kmax]->Right();
  }

  // cleanup
  for (int i=1; i<np; i++) {
    Delete(Ar[i]);
  }
  delete[] Ar;

  x.answer->setPtr(range);
  return 0;
}


// ******************************************************************
// *                        irorder_si class                        *
// ******************************************************************

class irorder_si : public order_si {
  bool is_for_integers;
private:
  static result* scratch;
  static int scratch_size;
public:
  irorder_si(const type* args);
  virtual void Compute(traverse_data &x, expr** pass, int np);

  inline static void cleanup() {
    delete[] scratch;
    scratch = 0;
    scratch_size = 0;
  }
  inline static void expand(int sz) {
    if (sz > scratch_size) {
      scratch_size = sz + (64-sz%64);
      delete[] scratch;
      scratch = new result[scratch_size];
    }
  }
};

result* irorder_si::scratch = 0;
int irorder_si::scratch_size = 0;

irorder_si::irorder_si(const type* args) : order_si(args)
{
  is_for_integers = (args == em->INT);
}

void irorder_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(x.parent);
  DCASSERT(0==x.aggregate);
  DCASSERT(np>1);

  expand(np);

  //
  // Determine k
  // 
  pass[0]->Compute(x);
  if (!x.answer->isNormal()) {
    x.answer->setNull();
    return;
  }
  long k = x.answer->getInt();

  if (k<1 || k>=np) {
    x.answer->setNull();
    return;
  }

  //
  // Compute all operands
  //
  result* ans = x.answer;
  for (int i=1; i<np; i++) {
    x.answer = scratch+i;
    pass[i]->Compute(x);
    if (x.answer->isNormal()) continue;
    if (x.answer->isInfinity()) continue;

    // null or other error, propogate it
    *ans = *(x.answer);
    x.answer = ans;
    return;
  }

  //
  // Sort the samples
  //
  if (is_for_integers) {
    sortIntegers(scratch+1, np-1);
  } else {
    sortReals(scratch+1, np-1);
  }

  //
  // return Kth smallest
  //
  *(x.answer) = scratch[k];
}

// ******************************************************************
// *                                                                *
// *                           front  end                           *
// *                                                                *
// ******************************************************************


void AddMathFunctions(symbol_table* st, const exprman* em)
{
  if (0==st || 0==em)  return;

  static intdiv_si  the_intdiv;           st->AddSymbol(  &the_intdiv );
  static pow_si     the_pow;              st->AddSymbol(  &the_pow    );
  static exp_si     the_exp;              st->AddSymbol(  &the_exp    );
  static log_si     the_log;              st->AddSymbol(  &the_log    );
  static sqrt_si    the_sqrt;             st->AddSymbol(  &the_sqrt   );
  static imax_si    the_imax;             st->AddSymbol(  &the_imax   );
  static rmax_si    the_rmax;             st->AddSymbol(  &the_rmax   );
  static imin_si    the_imin;             st->AddSymbol(  &the_imin   );
  static rmin_si    the_rmin;             st->AddSymbol(  &the_rmin   );
  static irorder_si the_iorder(em->INT);  st->AddSymbol(  &the_iorder );
  static irorder_si the_rorder(em->REAL); st->AddSymbol(  &the_rorder );
}
