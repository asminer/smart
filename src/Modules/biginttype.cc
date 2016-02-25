
// $Id$


#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/casting.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"
#include "biginttype.h"

#include <math.h>

// ******************************************************************
// *                                                                *
// *                      gmp  library credits                      *
// *                                                                *
// ******************************************************************

#ifdef HAVE_LIBGMP

class gmp_lib : public library {
  char* version;
public:
  gmp_lib();
  virtual ~gmp_lib();
  virtual const char* getVersionString() const;
  virtual bool hasFixedPointer() const { return false; }
  virtual void printCopyright(doc_formatter* df) const;
};

gmp_lib::gmp_lib() : library(true)
{
  version = new char[40];
  snprintf(version, 40, "GNU MP version %s (or higher)", gmp_version);
}

gmp_lib::~gmp_lib()
{
  delete[] version;
}

const char* gmp_lib::getVersionString() const
{
  return version;
}

void gmp_lib::printCopyright(doc_formatter* df) const
{
  df->begin_indent();
  df->Out() << "Copyright (C) 1991, 1999 Free Software Foundation, Inc.\n";
  df->Out() << "released under the GNU Lesser General Public License, version 2\n";
  df->Out() << "http://gmplib.org\n";
  df->end_indent();
}

gmp_lib gmp_lib_data;

#endif


// ******************************************************************
// *                         bigint methods                         *
// ******************************************************************

char* bigint::buffer = 0;
int bigint::bufsize = 0;

#ifdef HAVE_LIBGMP

bigint::bigint() : shared_object()
{
  mpz_init(value); 
}

bigint::bigint(long x) : shared_object()
{
  mpz_init_set_si(value, x); 
}

bigint::bigint(const char* c) : shared_object()
{
  mpz_init_set_str(value, c, 10);
}

bigint::bigint(const mpz_t &v) : shared_object()
{
  mpz_init_set(value, v);
}

bigint::bigint(const bigint& b) : shared_object()
{
  mpz_init_set(value, b.value);
}

bigint::~bigint()
{
  mpz_clear(value); 
}

bool bigint::Print(OutputStream &s, int width) const
{
  int digits = mpz_sizeinbase(value, 10)+2;
  if (digits > bufsize) {
    int newbufsize = (1+digits / 1024) * 1024;
    DCASSERT(newbufsize>0);
    char* newbuf = (char*) realloc(buffer, newbufsize);
    if (newbuf) {
      buffer = newbuf;
      bufsize = newbufsize;
    } else {
      s.Put("memory overflow");
      return true;
    }
  }
  mpz_get_str(buffer, 10, value);

  s.PutInteger(buffer, width);
  return true;
}

bool bigint::Equals(const shared_object *o) const
{
  const bigint* b = dynamic_cast <const bigint*> (o);
  if (0==b) return false;
  return (0 == mpz_cmp(value, b->value));
}

#else

bigint::bigint() : shared_object()
{
}

bigint::bigint(long x) : shared_object()
{
  value = x;
}

bigint::bigint(const char* c) : shared_object()
{
  value = atol(c);
}

bigint::bigint(const bigint& b) : shared_object()
{
  value = b.value;
}

bigint::~bigint()
{
}

bool bigint::Print(OutputStream &s, int width) const
{
  if (bufsize < 24) {
    char* newbuf = (char*) realloc(buffer, 24);
    if (newbuf) {
      buffer = newbuf;
      bufsize = 24;
    } else {
      s.Put("memory overflow");
      return true;
    }
  }
  snprintf(buffer, bufsize, "%ld", value);
  s.PutInteger(buffer, width);
  return true;
}

bool bigint::Equals(const shared_object *o) const
{
  const bigint* b = dynamic_cast <const bigint*> (o);
  if (0==b) return false;
  return (value == b->value);
}

#endif

// ******************************************************************
// *                                                                *
// *                       bigint_type  class                       *
// *                                                                *
// ******************************************************************

class bigint_type : public simple_type {
public:
  bigint_type();
protected:
  virtual void assign_normal(result& r, const char* s) const;
};

// ******************************************************************
// *                      bigint_type  methods                      *
// ******************************************************************

bigint_type::bigint_type() : simple_type("bigint", "Large integers", "Integers that may be larger than the machine-supportable integers; can be arbitrarily long, limited only by available memory.")
{
  setPrintable();
}

void bigint_type::assign_normal(result& r, const char* s) const
{
  r.setPtr(new bigint(s));
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                        Bigint typecasts                        *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        int2bigint class                        *
// *                                                                *
// ******************************************************************

///  Type promotion from integer to multiple precision integer.
class int2bigint : public specific_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  int2bigint();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    DCASSERT(em->INT);
    if (src != em->INT) return -1;
    return RANGE_EXPAND;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const char*, int, expr*, const type*) const;
};

int2bigint::converter
::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void int2bigint::converter::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    x.answer->setPtr(new bigint(x.answer->getInt()));
  }
}

int2bigint::int2bigint() : specific_conv(false) 
{
}

const type* int2bigint::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(em->INT == src);
  return em->BIGINT;
}

expr* int2bigint::convert(const char* fn, int ln, expr* e, const type* nt) const
{
  return new converter(fn, ln, nt, e);
}

// ******************************************************************
// *                                                                *
// *                        bigint2int class                        *
// *                                                                *
// ******************************************************************

///  Type casting from bigint to int; failure (doesn't fit) produces null.
class bigint2int : public specific_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  bigint2int();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    DCASSERT(em->BIGINT);
    return (src == em->BIGINT) ? RANGE_EXPAND : -1;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const char*, int, expr*, const type*) const;
};

bigint2int::converter
::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void bigint2int::converter::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    const bigint* a = smart_cast <const bigint*> (x.answer->getPtr());
    DCASSERT(a);
#ifdef HAVE_LIBGMP
    if (mpz_fits_slong_p(a->value)) {
      long z = mpz_get_si(a->value);
      x.answer->setInt(z);
    } else {
      x.answer->setNull();
    }
#else
    x.answer->setInt(a->value);
#endif
  }
}

bigint2int::bigint2int() : specific_conv(true) 
{
}

const type* bigint2int::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(em->BIGINT == src);
  return em->INT;
}

expr* bigint2int::convert(const char* fn, int ln, expr* e, const type* nt) const
{
  return new converter(fn, ln, nt, e);
}

// ******************************************************************
// *                                                                *
// *                       bigint2real  class                       *
// *                                                                *
// ******************************************************************

///  Type casting from bigint to real; failure (doesn't fit) produces null.
class bigint2real : public specific_conv {

    class converter : public typecast {
    public:
      converter(const char* fn, int line, const type* nt, expr* x);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr* x) const {
        return new converter(Filename(), Linenumber(), Type(), x);
      }
    };

public:
  bigint2real();
  virtual int getDistance(const type* src) const {
    DCASSERT(src);
    DCASSERT(em->BIGINT);
    return (src == em->BIGINT) ? SIMPLE_CONV : -1;
  }
  virtual const type* promotesTo(const type* src) const;
  virtual expr* convert(const char*, int, expr*, const type*) const;
};

bigint2real::converter
::converter(const char* fn, int ln, const type* nt, expr* x)
 : typecast(fn, ln, nt, x) 
{ 
}

void bigint2real::converter::Compute(traverse_data &x) 
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    const bigint* a = smart_cast <const bigint*> (x.answer->getPtr());
    DCASSERT(a);
#ifdef HAVE_LIBGMP
    double d = mpz_get_d(a->value);
#else
    double d = a->value;
#endif
    if ((d >= HUGE_VAL) || (d <= -HUGE_VAL)) {
      x.answer->setNull();
    } else {
      x.answer->setReal(d);
    }
  }
}

bigint2real::bigint2real() : specific_conv(true) 
{
}

const type* bigint2real::promotesTo(const type* src) const
{
  DCASSERT(src);
  DCASSERT(em->BIGINT == src);
  return em->REAL;
}

expr* bigint2real
::convert(const char* fn, int ln, expr* e, const type* nt) const
{
  return new converter(fn, ln, nt, e);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Bigint expressions                       *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                       bigint_neg  class                        *
// *                                                                *
// ******************************************************************

/// Negation of a bigint expression.
class bigint_neg : public negop {
public:
  bigint_neg(const char* fn, int line, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                       bigint_neg methods                       *
// ******************************************************************

bigint_neg::bigint_neg(const char* fn, int line, expr *x)
 : negop(fn, line, exprman::uop_neg, x->Type(), x)
{
}

void bigint_neg::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);
  if (x.answer->isNormal()) {
    const bigint* a = smart_cast <bigint*>(x.answer->getPtr());
    DCASSERT(a);
    bigint* z = new bigint;
    z->neg(*a);
    x.answer->setPtr(z);
  } else if (x.answer->isInfinity()) {
    x.answer->setInfinity(-x.answer->signInfinity());
  }
}

expr* bigint_neg::buildAnother(expr *x) const
{
  return new bigint_neg(Filename(), Linenumber(), x);
}

// ******************************************************************
// *                                                                *
// *                       bigint_add  class                        *
// *                                                                *
// ******************************************************************

/// Addition of bigint expressions.
class bigint_add : public summation {
public:
  bigint_add(const char* fn, int line, const type* t, expr **x, bool* f, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                       bigint_add methods                       *
// ******************************************************************

bigint_add::
bigint_add(const char* fn, int line, const type* t, expr** x, bool* f, int n) 
 : summation(fn, line, exprman::aop_plus, t, x, f, n) 
{ 
}
  
void bigint_add::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* sum = x.answer;
  bigint* bi_sum = new bigint(0L);
  sum->setPtr(bi_sum);

  bool unknown = false;
  int i;
  result foo;
  x.answer = &foo;
  // Sum until we run out of operands or hit an infinity
  for (i=0; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
        // ordinary finite integer addition
        if (sum->getPtr()) {
          const bigint* a = smart_cast <bigint*> (foo.getPtr());
          DCASSERT(a);
          if (flip && flip[i]) {
            bi_sum->sub(*bi_sum, *a);
          } else {
            bi_sum->add(*bi_sum, *a);
          }
        } // if sum->getPtr()
        continue;
      }
      if (foo.isInfinity()) {
        // infinite addition
        unknown = false;  // infinity + ? = infinity
        (*sum) = foo;
        break;  
      }
      if (foo.isUnknown()) {
        unknown = true;
        sum->deletePtr(); // definitely won't need running sum
        continue;
      }
      if (foo.isNull()) {
        sum->setNull();
        x.answer = sum;
        return;  // null...short circuit
      }
      // must be an error
      sum->setNull();
      x.answer = sum;
      return;  // error...short circuit
  } // for i


  // sum so far is +/- infinity, or we are out of operands.
  // Check the remaining operands, if any, and throw an
  // error if we have infinity - infinity.
  
  for (i++; i<opnd_count; i++) {
    DCASSERT(sum->isInfinity());
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (foo.isNormal()) { // most likely case
      continue;
    }
    if (foo.isUnknown()) continue;
    if (foo.isInfinity()) {
      if (flip && flip[i])  foo.setInfinity(-foo.signInfinity());
      // check operand for opposite sign for infinity
      if ( (sum->signInfinity()>0) != (foo.signInfinity()>0) ) {
        inftyMinusInfty(operands[i]);
        sum->setNull();
        x.answer = sum;
        return;
      }
    } // infinity
    if (foo.isNull()) {
      sum->setNull();
      x.answer = sum;
      return;  // null...short circuit
    }
    // must be an error
    sum->setNull();
    x.answer = sum;
    return;  // error...short circuit
  } // for i
  if (unknown) sum->setUnknown();
  x.answer = sum;
}

expr* bigint_add::buildAnother(expr** x, bool* f, int n) const
{
  return new bigint_add(Filename(), Linenumber(), Type(), x, f, n);
}


// ******************************************************************
// *                                                                *
// *                       bigint_mult  class                       *
// *                                                                *
// ******************************************************************

/// Multiplication of bigint expressions.
class bigint_mult : public product {
public:
  bigint_mult(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                      bigint_mult  methods                      *
// ******************************************************************

bigint_mult
::bigint_mult(const char* fn, int line, const type* t, expr **x, int n) 
 : product(fn, line, exprman::aop_times, t, x, 0, n)
{ 
}
  
void bigint_mult::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* prod = x.answer;
  bigint* bi_prod = new bigint(1);
  prod->setPtr(bi_prod);

  bool unknown = false;
  int i;

  //
  // Three states of computation: finite multiply, zero, infinity
  //  
  // finite * zero -> zero,
  // finite * infinity -> infinity,
  // zero * infinity -> error,
  // 
  // infinity multiply: only keep track of sign, check for errors
  //
  // zero multiply: check only for errors 
  //
  result foo;
  x.answer = &foo;

  // Multiply until we run out of operands or change state
  for (i=0; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
        const bigint* a = smart_cast <bigint*> (foo.getPtr());
        DCASSERT(a);
        if (0==a->cmp_si(0)) {
          prod->setPtr(new bigint(0L));
          unknown = false;
          break;  // change state
        } 
        // normal finite multiply
        bi_prod->mul(*bi_prod, *a);
        continue;
      }
      if (foo.isInfinity()) {
        // fix sign and change state
        int prodsign = SIGN(bi_prod->cmp_si(0));
        prod->setInfinity(foo.signInfinity() * prodsign);
        break;
      }
      // Still here?  
      prod->deletePtr();
      if (foo.isUnknown()) {
        unknown = true;
        continue;
      }
      if (foo.isNull()) {
        prod->setNull();
        x.answer = prod;
        return; // short circuit
      }
      // must be an error
      prod->setNull();
      x.answer = prod;
      return; // short circuit
  } // for i

  // The infinity case
  if (prod->isInfinity()) {
    // Keep multiplying, only worry about sign and make sure we don't hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
        const bigint* a = smart_cast <bigint*> (foo.getPtr());
        DCASSERT(a);
        if (0==a->cmp_si(0)) {
          zeroTimesInfty(operands[i]); // 0 * infinity, error
          prod->setNull();
          x.answer = prod;
          return;
        }
        continue;  // finite nonzero times infinity is still infinity
      }
      if (foo.isInfinity()) {
        // fix sign
        prod->setInfinity(prod->signInfinity() * foo.signInfinity());
        continue;
      }
      if (foo.isUnknown()) {
        unknown = true;  // can't be sure of sign
        continue;
      }
      if (foo.isNull()) {
        prod->setNull();
        x.answer = prod;
        return; // short circuit
      }
      // must be an error
      prod->setNull();
      x.answer = prod;
      return;
    } // for i
  } 

  // The zero case
  if (prod->getPtr()) {
    // Check the remaining operands, if any, and throw an
    // error if we have infinity * 0.
    for (i++; i<opnd_count; i++) {
      DCASSERT(0==bi_prod->cmp_si(0));
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) continue;
      if (foo.isUnknown()) continue;

      // check for infinity
      if (foo.isInfinity()) {
        zeroTimesInfty(operands[i]);
        prod->setNull();
        x.answer = prod;
        return;
      }
      if (foo.isNull()) {
        prod->setNull();
        x.answer = prod;
        return;  // null...short circuit
      }
      prod->setNull();
      x.answer = prod;
      return;  // error...short circuit
    } // for i
  }

  // Only thing to worry about:
  if (unknown) {
    prod->setUnknown();
  }
  x.answer = prod;
}

expr* bigint_mult::buildAnother(expr **x, bool* f, int n) const 
{
  DCASSERT(0==f);
  return new bigint_mult(Filename(), Linenumber(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                      bigint_multdiv class                      *
// *                                                                *
// ******************************************************************

/** Multiplication and division of bigint expressions.
    The return type is REAL.
 */
class bigint_multdiv : public product {
public:
  bigint_multdiv(const char* fn, int line, const type* t, 
      expr **x, bool* f, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                     bigint_multdiv methods                     *
// ******************************************************************

bigint_multdiv::bigint_multdiv(const char* fn, int line, const type* t, 
  expr** x, bool* f, int n) : product(fn, line, exprman::aop_times, t, x, f, n)
{ 
  DCASSERT(f);
}
  
void bigint_multdiv::Compute(traverse_data &x)
{
  DCASSERT(flip);
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  bigint* numer = new bigint(1);
  bigint* denom = new bigint(1);
  bool unknown = false;
  bool zero = false;
  int i;

  //
  // Three states of computation: finite multiply, zero, infinity
  //  
  // finite * zero -> zero,
  // finite * infinity -> infinity,
  // zero * infinity -> error,
  // 
  // infinity multiply: only keep track of sign, check for errors
  //
  // zero multiply: check only for errors 
  //
  result* prod = x.answer;
  result foo;
  x.answer = &foo;

  // Multiply until we run out of operands or change state
  for (i=0; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
        const bigint* a = smart_cast <bigint*> (foo.getPtr());
        DCASSERT(a);
        if (0==a->cmp_si(0)) {
          Delete(numer);
          Delete(denom);
          numer = 0;
          denom = 0;
          if (flip[i]) {
            divideByZero(operands[i]);
            prod->setNull();
            x.answer = prod;
            return;  // short circuit.
          }
          unknown = false;
          zero = true;
          break;  // change state
        } // if 0
        // normal finite multiply or divide
        if (flip[i])  denom->mul(*denom, *a);
        else          numer->mul(*numer, *a);
        continue;
      } // if foo.isNormal()

      if (foo.isInfinity()) {
        // fix sign and change state
        int numsign = SIGN(numer->cmp_si(0));
        int densign = SIGN(denom->cmp_si(0));
        prod->setInfinity(foo.signInfinity() * numsign * densign);
        Delete(numer);
        Delete(denom);
        numer = 0;
        denom = 0;
        break;
      } // if foo.isInfinity()

      // Still here?  
      Delete(numer);
      Delete(denom);
      numer = 0;
      denom = 0;
      if (foo.isUnknown()) {
        unknown = true;
        continue;
      }
      if (foo.isNull())  {
        prod->setNull();
        x.answer = prod;
        return; // short circuit
      }

      // must be an error
      prod->setNull();
      x.answer = prod;
      return; // short circuit
  } // for i


  // The infinity case
  if (prod->isInfinity()) {
    // Keep multiplying, only worry about sign and make sure we don't
    // (1) multiply by zero
    // (2) divide by zero
    // (3) divide by infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isNormal()) {
        const bigint* a = smart_cast <bigint*> (foo.getPtr());
        DCASSERT(a);
        int asign = a->cmp_si(0);
        if (0==asign) {
          // infinity * 0 or infinity / 0, error
          inftyTimesZero(flip[i], operands[i]);
          prod->setNull();
          x.answer = prod;
          return;
        } else {
          // infinity * a or infinity / a, just fix the sign.
          prod->setInfinity(prod->signInfinity() * asign);
        }
        continue;
      } // if foo.isNormal()
      if (foo.isInfinity()) {
        if (flip[i]) {
          // infinity / infinity, error
          inftyDivInfty(operands[i]);
          prod->setNull();
          x.answer = prod;
          return;  // short circuit.
        } 
      } // if foo.isInfinity()
      if (foo.isUnknown()) {
        unknown = true;  // can't be sure of sign
        continue;
      }
      if (foo.isNull()) {
        prod->setNull();
        x.answer = prod;
        return; // short circuit
      }
      // must be an error
      prod->setNull();
      x.answer = prod;
      return;
    } // for i
  } // if prod->isInfinity()

  // The zero case
  if (zero) {
    // Check the remaining operands, and make sure we don't
    // (1) divide by 0
    // (2) multiply by infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (foo.isUnknown()) continue;
      if (foo.isNormal()) {
        const bigint* a = smart_cast <bigint*> (foo.getPtr());
        DCASSERT(a);
        if (0==a->cmp_si(0)) {
          if (flip[i]) {
            divideByZero(operands[i]);
            prod->setNull();
            x.answer = prod;
            return;  // short circuit.
          }
        }
        continue;
      } // if foo.isNormal
      if (foo.isInfinity()) {
        if (flip[i])  continue;  // 0 / infinity = 0.
        zeroTimesInfty(operands[i]);
        prod->setNull();
        x.answer = prod;
        return;
      } // if foo.isInfinity()
      if (foo.isNull()) {
        prod->setNull();
        x.answer = prod;
        return;  // null...short circuit
      }
      prod->setNull();
      x.answer = prod;
      return;  // error...short circuit
    } // for i
    // ok, answer is zero
    prod->setReal(0.0);
    x.answer = prod;
    return;
  }

  x.answer = prod;

  // Only thing to worry about:
  if (unknown) {
    prod->setUnknown();
    return;
  }

  if (numer) {
    // answer is numer / denom
    DCASSERT(denom);
    DCASSERT(prod->isNormal());
#ifdef HAVE_LIBGMP
    mpq_t q;
    mpq_init(q);
    mpq_set_num(q, numer->value);
    mpq_set_den(q, denom->value);
    mpq_canonicalize(q);
    prod->setReal(mpq_get_d(q));
    mpq_clear(q);
#else
    double d = numer->value;
    d /= denom->value;
    prod->setReal(d);
#endif
    Delete(numer);
    Delete(denom);

    // TBD: check that prod->rvalue is not too large
  }
}

expr* bigint_multdiv::buildAnother(expr **x, bool* f, int n) const 
{
  return new bigint_multdiv(Filename(), Linenumber(), Type(), x, f, n);
}

// ******************************************************************
// *                                                                *
// *                        bigint_mod class                        *
// *                                                                *
// ******************************************************************

/// Modulo arithmetic for bigint expressions.
class bigint_mod : public modulo {
public:
  bigint_mod(const char* fn, int line, const type* t, expr* l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr* l, expr* r) const;
};

// ******************************************************************
// *                       bigint_mod methods                       *
// ******************************************************************

bigint_mod
::bigint_mod(const char* fn, int line, const type* t, expr *l, expr *r)
 : modulo(fn, line, t, l, r) 
{ 
}
  
void bigint_mod::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    const bigint* br = smart_cast <bigint*> (r.getPtr());
    DCASSERT(br);
    if (br->cmp_si(0)) {
      // normal modulo
      const bigint* bl = smart_cast <bigint*> (l.getPtr());
      DCASSERT(bl);
      bigint* c = new bigint;
      c->div_r(*bl, *br);
      x.answer->setPtr(c);
    } else {
      // mod 0 error
      if (em->startError()) {
        em->causedBy(this);
        em->cerr() << "Illegal operation: modulo 0";
        em->stopIO();
      }
      x.answer->setNull();
    }
    return;
  } 
  if (l.isNull() || r.isNull()) {
    x.answer->setNull();
    return;
  }
  if (l.isUnknown() || r.isUnknown()) {
    x.answer->setUnknown();
    return;
  }
  if (l.isInfinity() && r.isInfinity()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Illegal operation: infty % infty";
      em->stopIO();
    }
    x.answer->setNull();
    return;
  }
  if (l.isNormal() && r.isInfinity()) {
    // a mod +-infty = a
    x.answer->setPtr(Share(l.getPtr()));  // should we check signs?
    return;
  }
  if (l.isInfinity() && r.isNormal()) {
    // +- infty mod b is undefined
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Illegal operation: infty mod " << r.getInt();
      em->stopIO();
    }
    x.answer->setNull();
    return;
  }
  // still here? must be an error.
  x.answer->setNull();
}

expr* bigint_mod::buildAnother(expr *l, expr *r) const
{
  return new bigint_mod(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                       bigint_equal class                       *
// *                                                                *
// ******************************************************************

/// Check equality of two bigint expressions.
class bigint_equal : public eqop {
public:
  bigint_equal(const char* fn, int line, const type* nt, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                      bigint_equal methods                      *
// ******************************************************************

bigint_equal
::bigint_equal(const char* fn, int line, const type* nt, expr *l, expr *r)
 : eqop(fn, line, nt, l, r) 
{ 
}

void bigint_equal::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) == 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_equal::buildAnother(expr *l, expr *r) const 
{
  return new bigint_equal(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        bigint_neq class                        *
// *                                                                *
// ******************************************************************

/// Check inequality of two bigint expressions.
class bigint_neq : public neqop {
public:
  bigint_neq(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       bigint_neq methods                       *
// ******************************************************************

bigint_neq
::bigint_neq(const char* fn, int line, const type* t, expr *l, expr *r)
 : neqop(fn, line, t, l, r) 
{ 
}
  
void bigint_neq::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) != 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_neq::buildAnother(expr *l, expr *r) const
{
  return new bigint_neq(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        bigint_gt  class                        *
// *                                                                *
// ******************************************************************

/// Check if one bigint expression is greater than another.
class bigint_gt : public gtop {
public:
  bigint_gt(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       bigint_gt  methods                       *
// ******************************************************************

bigint_gt::bigint_gt(const char* fn, int line, const type* t, expr *l, expr *r)
 : gtop(fn, line, t, l, r) 
{ 
}

void bigint_gt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) > 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_gt::buildAnother(expr *l, expr *r) const
{
  return new bigint_gt(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        bigint_ge  class                        *
// *                                                                *
// ******************************************************************

/// Check if one bigint expression is greater or equal another.
class bigint_ge : public geop {
public:
  bigint_ge(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       bigint_ge  methods                       *
// ******************************************************************

bigint_ge::bigint_ge(const char* fn, int line, const type* t, expr *l, expr *r)
 : geop(fn, line, t, l, r) 
{ 
}

void bigint_ge::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) >= 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_ge::buildAnother(expr *l, expr *r) const
{
  return new bigint_ge(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        bigint_lt  class                        *
// *                                                                *
// ******************************************************************

/// Check if one bigint expression is less than another.
class bigint_lt : public ltop {
public:
  bigint_lt(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       bigint_lt  methods                       *
// ******************************************************************

bigint_lt::bigint_lt(const char* fn, int line, const type* t, expr *l, expr *r)
 : ltop(fn, line, t, l, r) 
{ 
}

void bigint_lt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) < 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_lt::buildAnother(expr *l, expr *r) const
{
  return new bigint_lt(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        bigint_le  class                        *
// *                                                                *
// ******************************************************************

/// Check if one bigint expression is less or equal another.
class bigint_le : public leop {
public:
  bigint_le(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       bigint_le  methods                       *
// ******************************************************************

bigint_le::bigint_le(const char* fn, int line, const type* t, expr *l, expr *r)
 : leop(fn, line, t, l, r) 
{ 
}

void bigint_le::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);
  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    const bigint* bl = smart_cast <bigint*> (l.getPtr());  DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());   DCASSERT(br);
    x.answer->setBool(bl->cmp(*br) <= 0);
  } else {
    Special(l, r, x);
  }
}

expr* bigint_le::buildAnother(expr *l, expr *r) const
{
  return new bigint_le(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                       Bigint  operations                       *
// *                                                                *
// *                                                                *
// ******************************************************************

inline const type* 
BigintResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  if (em->NULTYPE == lt || em->NULTYPE ==rt)  return 0;
  const type* lct = em->getLeastCommonType(lt, rt);
  if (0==lct)        return 0;
  if (lct->getBaseType() != em->BIGINT)  return 0;
  if (lct->isASet())      return 0;
  return lct;
}

inline int 
BigintAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  const type* lct = BigintResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);   DCASSERT(dl>=0);
  int dr = em->getPromoteDistance(rt, lct);   DCASSERT(dr>=0);

  return dl+dr;
}

inline const type* AlignBigints(const exprman* em, expr* &l, expr* &r)
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  DCASSERT(l);
  DCASSERT(r);
  const type* lct = BigintResultType(em, l->Type(), r->Type());
  if (0==lct) {
    Delete(l);
    Delete(r);
    return 0;
  }
  l = em->promote(l, lct);   DCASSERT(em->isOrdinary(l));
  r = em->promote(r, lct);   DCASSERT(em->isOrdinary(r));
  return lct;
}

inline int BigintAlignDistance(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (0==lct)        return -1;
  if (lct->getBaseType() != em->BIGINT)  return -1;
  if (lct->isASet())      return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    DCASSERT(dx>=0);
    d += dx;
  }
  return d;
}

inline const type* AlignBigints(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (  (0==lct) || (lct->getBaseType() != em->BIGINT) || lct->isASet() ) {
    for (int i=0; i<N; i++)  Delete(x[i]);
    return 0;
  }
  for (int i=0; i<N; i++) {
    x[i] = em->promote(x[i], lct);
    DCASSERT(em->isOrdinary(x[i]));
  }
  return lct;
}


// ******************************************************************
// *                                                                *
// *                     bigint_assoc_op  class                     *
// *                                                                *
// ******************************************************************

class bigint_assoc_op : public assoc_op {
public:
  bigint_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                    bigint_assoc_op  methods                    *
// ******************************************************************

bigint_assoc_op::bigint_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int bigint_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return BigintAlignDistance(em, list, N);
}

int bigint_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  return BigintAlignDistance(em, lt, rt);
}

const type* bigint_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  return BigintResultType(em, l, r);
}


// ******************************************************************
// *                                                                *
// *                     bigint_binary_op class                     *
// *                                                                *
// ******************************************************************

class bigint_binary_op : public binary_op {
public:
  bigint_binary_op(exprman::binary_opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                    bigint_binary_op methods                    *
// ******************************************************************

bigint_binary_op::bigint_binary_op(exprman::binary_opcode op) : binary_op(op)
{
}

int bigint_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return BigintAlignDistance(em, lt, rt);
}

const type* bigint_binary_op::getExprType(const type* l, const type* r) const
{
  return BigintResultType(em, l, r);
}

// ******************************************************************
// *                                                                *
// *                      bigint_comp_op class                      *
// *                                                                *
// ******************************************************************

class bigint_comp_op : public bigint_binary_op {
public:
  bigint_comp_op(exprman::binary_opcode op);
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                     bigint_comp_op methods                     *
// ******************************************************************

bigint_comp_op::bigint_comp_op(exprman::binary_opcode op) : bigint_binary_op(op)
{
}

const type* bigint_comp_op::getExprType(const type* l, const type* r) const
{
  const type* t = BigintResultType(em, l, r);
  if (t)  t = t->changeBaseType(em->BOOL);
  return t;
}

// ******************************************************************
// *                                                                *
// *                      bigint_neg_op  class                      *
// *                                                                *
// ******************************************************************

class bigint_neg_op : public unary_op {
public:
  bigint_neg_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                     bigint_neg_op  methods                     *
// ******************************************************************

bigint_neg_op::bigint_neg_op() : unary_op(exprman::uop_neg)
{
}

const type* bigint_neg_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->BIGINT);
  if (0==t)    return 0;
  if (t->isASet())  return 0; 
  const type* bt = t->getBaseType();
  if (bt != em->BIGINT)  return 0;
  return t;
}

unary* bigint_neg_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new bigint_neg(fn, ln, x);
}

// ******************************************************************
// *                                                                *
// *                      bigint_add_op  class                      *
// *                                                                *
// ******************************************************************

class bigint_add_op : public bigint_assoc_op {
public:
  bigint_add_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                     bigint_add_op  methods                     *
// ******************************************************************

bigint_add_op::bigint_add_op() : bigint_assoc_op(exprman::aop_plus)
{
}

assoc* bigint_add_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  const type* lct = AlignBigints(em, list, N);
  // see if flips are redundant
  if (flip) {
    bool all_false = true;
    for (int i=0; i<N; i++) {
      if (! flip[i])  continue;
      all_false = false;
      break;
    }
    if (all_false) {
      delete[] flip;
      flip = 0;
    }
  }
  if (lct)  return new bigint_add(fn, ln, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}

// ******************************************************************
// *                                                                *
// *                      bigint_mult_op class                      *
// *                                                                *
// ******************************************************************

class bigint_mult_op : public bigint_assoc_op {
public:
  bigint_mult_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                     bigint_mult_op methods                     *
// ******************************************************************

bigint_mult_op::bigint_mult_op() : bigint_assoc_op(exprman::aop_times)
{
}

int bigint_mult_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  // first, make sure no items are flipped
  if (flip) for (int i=0; i<N; i++) if (flip[i])  return -1;
  return BigintAlignDistance(em, list, N);
}

int bigint_mult_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f)  return -1;
  return BigintAlignDistance(em, lt, rt);
}

const type* bigint_mult_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f)  return 0;
  return BigintResultType(em, l, r);
}

assoc* bigint_mult_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  const type* lct = AlignBigints(em, list, N);
  if (flip) for (int i=0; i<N; i++) if (flip[i])  lct = 0;
  delete[] flip;
  if (lct)  return new bigint_mult(fn, ln, lct, list, N);
  // there was an error
  delete[] list;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                    bigint_multdiv_op  class                    *
// *                                                                *
// ******************************************************************

class bigint_multdiv_op : public bigint_assoc_op {
public:
  bigint_multdiv_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                   bigint_multdiv_op  methods                   *
// ******************************************************************

bigint_multdiv_op::bigint_multdiv_op() : bigint_assoc_op(exprman::aop_times)
{
}

int bigint_multdiv_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  // first, make there is at least one flipped
  // (otherwise we are handled by bigint_mult_op)
  if (0==flip)    return -1;
  bool unflipped = true;
  for (int i=0; i<N; i++) if (flip[i]) {
    unflipped = false;
    break;
  }
  if (unflipped)  return -1;
  return BigintAlignDistance(em, list, N);
}

int bigint_multdiv_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (!f)  return -1;
  return BigintAlignDistance(em, lt, rt);
}

const type* bigint_multdiv_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (!f)  return 0;
  const type* lct = BigintResultType(em, l, r);
  if (lct)  lct = lct->changeBaseType(em->REAL);
  return lct;
}

assoc* bigint_multdiv_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  const type* lct = AlignBigints(em, list, N);
  if (0==flip) {
    lct = 0;
  } else {
    bool unflipped = true;
    for (int i=0; i<N; i++) if (flip[i]) {
      unflipped = false;
      break;
    }
    if (unflipped)  lct = 0;
  }
  if (lct)  lct = lct->changeBaseType(em->REAL);
  if (lct)  return new bigint_multdiv(fn, ln, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}

// ******************************************************************
// *                                                                *
// *                      bigint_mod_op  class                      *
// *                                                                *
// ******************************************************************

class bigint_mod_op : public bigint_binary_op {
public:
  bigint_mod_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                     bigint_mod_op  methods                     *
// ******************************************************************

bigint_mod_op::bigint_mod_op() : bigint_binary_op(exprman::bop_mod)
{
}

binary* bigint_mod_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  return new bigint_mod(fn, ln, lct, l, r);
}


// ******************************************************************
// *                                                                *
// *                     bigint_equal_op  class                     *
// *                                                                *
// ******************************************************************

class bigint_equal_op : public bigint_comp_op {
public:
  bigint_equal_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                    bigint_equal_op  methods                    *
// ******************************************************************

bigint_equal_op::bigint_equal_op() : bigint_comp_op(exprman::bop_equals)
{
}

binary* bigint_equal_op
 ::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_equal(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                      bigint_neq_op  class                      *
// *                                                                *
// ******************************************************************

class bigint_neq_op : public bigint_comp_op {
public:
  bigint_neq_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                     bigint_neq_op  methods                     *
// ******************************************************************

bigint_neq_op::bigint_neq_op() : bigint_comp_op(exprman::bop_nequal)
{
}

binary* bigint_neq_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_neq(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                       bigint_gt_op class                       *
// *                                                                *
// ******************************************************************

class bigint_gt_op : public bigint_comp_op {
public:
  bigint_gt_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bigint_gt_op methods                      *
// ******************************************************************

bigint_gt_op::bigint_gt_op() : bigint_comp_op(exprman::bop_gt)
{
}

binary* bigint_gt_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_gt(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                       bigint_ge_op class                       *
// *                                                                *
// ******************************************************************

class bigint_ge_op : public bigint_comp_op {
public:
  bigint_ge_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bigint_ge_op methods                      *
// ******************************************************************

bigint_ge_op::bigint_ge_op() : bigint_comp_op(exprman::bop_ge)
{
}

binary* bigint_ge_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_ge(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                       bigint_lt_op class                       *
// *                                                                *
// ******************************************************************

class bigint_lt_op : public bigint_comp_op {
public:
  bigint_lt_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bigint_lt_op methods                      *
// ******************************************************************

bigint_lt_op::bigint_lt_op() : bigint_comp_op(exprman::bop_lt)
{
}

binary* bigint_lt_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_lt(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                       bigint_le_op class                       *
// *                                                                *
// ******************************************************************

class bigint_le_op : public bigint_comp_op {
public:
  bigint_le_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bigint_le_op methods                      *
// ******************************************************************

bigint_le_op::bigint_le_op() : bigint_comp_op(exprman::bop_le)
{
}

binary* bigint_le_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBigints(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new bigint_le(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       bigintdiv_si class                       *
// ******************************************************************

class bigintdiv_si : public simple_internal {
public:
  bigintdiv_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

bigintdiv_si::bigintdiv_si() : simple_internal(em->BIGINT, "div", 2)
{
  SetFormal(0, em->BIGINT, "a");
  SetFormal(1, em->BIGINT, "b");
  SetDocumentation("Integer division: computes bigint(a/b).");
}

void bigintdiv_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(2==np);
  DCASSERT(0==x.aggregate);

  result* answer = x.answer;

  result l;
  x.answer = &l;
  SafeCompute(pass[0], x);

  result r;
  x.answer = &r;
  SafeCompute(pass[1], x);

  x.answer = answer;

  if (r.isNormal()) {  // Handle the divide by zero case first
    const bigint* br = smart_cast <bigint*> (r.getPtr());
    DCASSERT(br);
    if (0==br->cmp_si(0)) {
      if (em->startError()) {
        em->causedBy(x.parent);
        em->cerr() << "Undefined operation (divide by 0)";
        em->stopIO();
      }
      answer->setNull();
      return;
    }
  }

  if (l.isNormal() && r.isNormal()) {
    bigint* c = new bigint;
    answer->setPtr(c);
    const bigint* bl = smart_cast <bigint*> (l.getPtr());
    DCASSERT(bl);
    const bigint* br = smart_cast <bigint*> (r.getPtr());
    DCASSERT(br);
    
    c->div_q(*bl, *br);
    return;
  }

  if (l.isInfinity() && r.isInfinity()) {
    if (em->startError()) {
      em->causedBy(x.parent);
      em->cerr() << "Undefined operation (infty / infty)";
      em->stopIO();
    }
    answer->setNull();
    return;
  }

  if (l.isInfinity()) {
    // infinity / finite, check sign only
    const bigint* br = smart_cast <bigint*> (r.getPtr());
    DCASSERT(br);
    answer->setInfinity(l.signInfinity() * SIGN(br->cmp_si(0)));
    return;
  }
  if (r.isInfinity()) {
    // finite / infinity = 0  
    answer->setPtr(new bigint(0L));
    return;
  }

  if (l.isUnknown() || r.isUnknown()) {
    answer->setUnknown();
    return;
  }
  answer->setNull();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_bigints : public initializer {
  public:
    init_bigints();
    virtual bool execute();
};
init_bigints the_bigint_initializer;

init_bigints::init_bigints() : initializer("init_bigints")
{
  usesResource("em");
  usesResource("st");
  buildsResource("biginttype");
  buildsResource("types");
}

bool init_bigints::execute()
{
  if (0==em)  return false;
  
  // Library registry
#ifdef HAVE_LIBGMP
  em->registerLibrary(  &gmp_lib_data  );
#endif

  // Type registry
  simple_type* t_bigint = new bigint_type;
  em->registerType(t_bigint);

  em->setFundamentalTypes();

  // Type changes
  em->registerConversion(  new int2bigint  );
  em->registerConversion(  new bigint2int  );
  em->registerConversion(  new bigint2real );

  // Operators
  em->registerOperation(  new bigint_neg_op     );
  em->registerOperation(  new bigint_add_op     );
  em->registerOperation(  new bigint_mult_op    );
  em->registerOperation(  new bigint_multdiv_op );
  em->registerOperation(  new bigint_mod_op     );
  em->registerOperation(  new bigint_equal_op   );
  em->registerOperation(  new bigint_neq_op     );
  em->registerOperation(  new bigint_gt_op      );
  em->registerOperation(  new bigint_ge_op      );
  em->registerOperation(  new bigint_lt_op      );
  em->registerOperation(  new bigint_le_op      );

  if (0==st) return true;

  // Functions
  st->AddSymbol(  new bigintdiv_si  );

  return true;
}

