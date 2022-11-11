
#include "ops_int.h"
#include "../Streams/streams.h"
#include "exprman.h"
#include "unary.h"
#include "binary.h"
#include "assoc.h"

/**

   Implementation of operator classes, for integers.

 */


//#define DEBUG_DEEP


inline const type*
IntResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->INT);
  if (em->NULTYPE == lt || em->NULTYPE ==rt)  return 0;
  const type* lct = Phase2Rand(em->getLeastCommonType(lt, rt));
  if (0==lct)                         return 0;
  if (lct->getBaseType() != em->INT)  return 0;
  if (lct->isASet())                  return 0;
  return lct;
}

inline int IntAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->INT);
  const type* lct = IntResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);
  if (dl<0) return -1;
  int dr = em->getPromoteDistance(rt, lct);
  if (dr<0) return -1;

  return dl+dr;
}

inline const type* AlignIntegers(const exprman* em, expr* &l, expr* &r)
{
  DCASSERT(em);
  DCASSERT(em->INT);
  DCASSERT(l);
  DCASSERT(r);
  const type* lct = IntResultType(em, l->Type(), r->Type());
  if (0==lct) {
    Delete(l);
    Delete(r);
    return 0;
  }
  l = em->promote(l, lct);   DCASSERT(em->isOrdinary(l));
  r = em->promote(r, lct);   DCASSERT(em->isOrdinary(r));
  return lct;
}

inline int IntAlignDistance(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->INT);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  lct = Phase2Rand(lct);
  if (0==lct)                         return -1;
  if (lct->getBaseType() != em->INT)  return -1;
  if (lct->isASet())                  return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    if (dx<0) return -1;
    d += dx;
  }
  return d;
}

inline const type* AlignIntegers(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->INT);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  lct = Phase2Rand(lct);
  if (  (0==lct) || (lct->getBaseType() != em->INT) || lct->isASet() ) {
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
// *                        int_neg_op class                        *
// *                                                                *
// ******************************************************************

/** Unary negation operator for integers.
    Includes
        - int -> int,
        - ph int -> rand int,
        - rand int -> rand int,
    etc...
*/
class int_neg_op : public unary_op {
  /// Negation expression
  class expression : public negop {
  public:
    expression(const location &W, expr *x);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *x) const {
      return new expression(Where(), x);
    }
  };

public:
  int_neg_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************
// *                       int_neg_op methods                       *
// ******************************************************************

int_neg_op::expression::expression(const location &W, expr *x)
 : negop(W, exprman::uop_neg, x->Type(), x)
{
}

void int_neg_op::expression::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  if (x.answer->isNormal()) {
    x.answer->setInt( -x.answer->getInt() );
  } else if (x.answer->isInfinity()) {
    // Safer behavior for infinity
    x.answer->setInfinity(-x.answer->signInfinity());
  }
}

int_neg_op::int_neg_op() : unary_op(exprman::uop_neg)
{
}

const type* int_neg_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->INT);
  if (0==t)    return 0;
  if (t->isASet())  return 0;
  const type* bt = t->getBaseType();
  if (bt != em->INT)  return 0;
  return Phase2Rand(t);
}

unary* int_neg_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new expression(W, x);
}



// ******************************************************************
// *                                                                *
// *                      int_assoc_op methods                      *
// *                                                                *
// ******************************************************************

int_assoc_op::int_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int int_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return IntAlignDistance(em, list, N);
}

int int_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  return IntAlignDistance(em, lt, rt);
}

const type* int_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  return IntResultType(em, l, r);
}



// ******************************************************************
// *                                                                *
// *                 int_add_op::expression methods                 *
// *                                                                *
// ******************************************************************

int_add_op::expression
::expression(const location &W, const type* t, expr **x, bool* f, int n)
 : summation(W, exprman::aop_plus, t, x, f, n)
{
}

void int_add_op::expression::Compute(traverse_data &x)
{
  //
  // Two states of computation: finite addition, infinity
  //
  // finite + finite -> finite,
  // finite + infinity -> infinity,
  //
  // infinity add: just check for infinity - infinity
  //

  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* sum = x.answer;

  // Overall sum and such
  long answer = 0;
  bool normal = true;
  bool unknown = false;
  bool infinite = false;

  int i;
  // Sum until we run out of operands or hit an infinity
  for (i=0; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (sum->isNormal()) {
        // normal finite addition
        if (flip && flip[i])  answer -= sum->getInt();
        else                  answer += sum->getInt();
        continue;
      }
      if (sum->isInfinity()) {
        // infinite addition
        unknown = false;  // infinity + ? = infinity
        if (flip && flip[i])  answer = -sum->signInfinity();
        else                  answer =  sum->signInfinity();
        infinite = true;
        normal = false;
        break;
      }
      if (sum->isUnknown()) {
        unknown = true;
        normal = false;
        continue;
      }
      // must be an error; short circuit
      sum->setNull();
      return;
  } // for i

  // Check the remaining operands, if any, and throw an
  // error if we have infinity - infinity.

  for (i++; i<opnd_count; i++) {
    DCASSERT(infinite);
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (sum->isNormal() || sum->isUnknown()) continue;  // most likely case
    if (sum->isInfinity()) {
      if (flip && flip[i])  sum->setInfinity(-sum->signInfinity());
      // check operand for opposite sign for infinity
      if ( (sum->signInfinity()>0) != (answer>0) ) {
        inftyMinusInfty(operands[i]);
        sum->setNull();
        return;
      }
    } // infinity
    // must be an error; short circuit
    sum->setNull();
    return;
  } // for i

  if (normal) {
    sum->setInt( answer );
    return;
  }
  if (unknown) {
    sum->setUnknown();
    return;
  }

  sum->setInfinity(answer);
}


expr* int_add_op::expression::buildAnother(expr** x, bool* f, int n) const
{
  return new expression(Where(), Type(), x, f, n);
}


// ******************************************************************
// *                                                                *
// *                       int_add_op methods                       *
// *                                                                *
// ******************************************************************

int_add_op::int_add_op() : int_assoc_op(exprman::aop_plus)
{
}

assoc* int_add_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignIntegers(em, list, N);
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
  if (lct)  return new expression(W, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}



// ******************************************************************
// *                                                                *
// *                int_mult_op::expression  methods                *
// *                                                                *
// ******************************************************************

int_mult_op::expression
::expression(const location &W, const type* t, expr **x, int n)
 : product(W, exprman::aop_times, t, x, 0, n)
{
}

void int_mult_op::expression::Compute(traverse_data &x)
{
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

  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* prod = x.answer;

  // Overall product and such
  long answer = 1;
  bool normal = true;
  bool unknown = false;
  bool infinite = false;

  int i;
  // Initially finite; multiply until we run out of operands or change state
  for (i=0; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (prod->isNormal()) {
        if (prod->getInt()) {
          // normal finite multiply
          answer *= prod->getInt();
          continue;
        }
        // change to "zero multiply" state.
        answer = 0;
        normal = true;
        unknown = false;
        break;
      }
      if (prod->isInfinity()) {
        // change to "infinity" state.
        answer = SIGN(answer) * prod->signInfinity();
        infinite = true;
        normal = false;
        break;
      }
      if (prod->isUnknown()) {
        unknown = true;
        normal = false;
        continue;
      }
      // must be an error; short circuit
      prod->setNull();
      return;
  } // for i

  // The infinity case
  if (infinite) {
    // Keep multiplying, only worry about sign and make sure we don't hit zero
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (prod->isNormal() || prod->isInfinity()) {
        if (0==prod->getInt()) {
          zeroTimesInfty(operands[i]); // 0 * infinity, error
          prod->setNull();
          return;
        } else {
          // infinity * nonzero, fix sign
          answer *= SIGN(prod->getInt());
        }
        continue;
      }
      if (prod->isUnknown()) {
        unknown = true;  // can't be sure of sign
        continue;
      }
      // must be an error
      prod->setNull();
      return;
    } // for i
  } // if infinite

  // The zero case
  if (0 == answer) {
    // Check the remaining operands, if any, and throw an
    // error if we have infinity * 0.
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (prod->isNormal() || prod->isUnknown()) continue;

      // check for infinity
      if (prod->isInfinity()) {
        zeroTimesInfty(operands[i]);
        prod->setNull();
        return;
      }
      // some kind of error, short circuit.
      prod->setNull();
      return;
    } // for i
  }

  if (normal) {
    prod->setInt( answer );
    return;
  }
  if (unknown) {
    prod->setUnknown();
    return;
  }
  DCASSERT(infinite);
  prod->setInfinity(answer);
}

expr* int_mult_op::expression::buildAnother(expr **x, bool* f, int n) const
{
  return new expression(Where(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                      int_mult_op  methods                      *
// *                                                                *
// ******************************************************************

int_mult_op::int_mult_op() : int_assoc_op(exprman::aop_times)
{
}

int int_mult_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  // first, make sure no items are flipped
  if (flip) for (int i=0; i<N; i++) if (flip[i])  return -1;
  return IntAlignDistance(em, list, N);
}

int int_mult_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f)  return -1;
  return IntAlignDistance(em, lt, rt);
}

const type* int_mult_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f)  return 0;
  return IntResultType(em, l, r);
}

assoc* int_mult_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignIntegers(em, list, N);
  if (flip) for (int i=0; i<N; i++) if (flip[i])  lct = 0;
  delete[] flip;
  if (lct)  return new expression(W, lct, list, N);
  // there was an error
  delete[] list;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                       int_multdiv  class                       *
// *                                                                *
// ******************************************************************

/** Multiplication and division of integer expressions.
    Note that the return type is REAL (or RAND_REAL, or...)
 */
class int_multdiv : public product {
public:
  int_multdiv(const location &W, const type* t, expr **x, bool* f, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                      int_multdiv  methods                      *
// ******************************************************************

int_multdiv
 ::int_multdiv(const location &W, const type* t, expr **x, bool *f, int n)
 : product(W, exprman::aop_times, t, x, f, n)
{
  DCASSERT(f);  // otherwise, use int_mult!
}

void int_multdiv::Compute(traverse_data &x)
{
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
  DCASSERT(flip);
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(operands[0]);
  result* prod = x.answer;

  // Overall product and such
  double answer = 1.0;
  bool normal = true;
  bool unknown = false;
  bool infinite = false;

  int i;
  // Initially finite; multiply until we run out of operands or change state
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (prod->isNormal()) {
        if (prod->getInt()) {
          // normal finite multiply / divide
          if (flip[i])  answer /= prod->getInt();
          else          answer *= prod->getInt();
          continue;
        }
        // we have a zero term
        if (flip[i]) {
          divideByZero(operands[i]); // divide by zero, bail out
          prod->setNull();
          return;  // short circuit.
        }
        // multiply by zero.
        answer = 0;
        unknown = false;
        normal = true;
        break;  // change state
    } // if prod->isNormal()
    if (prod->isInfinity()) {
        if (flip[i]) {
          // divide by infinity.
          answer = 0;
          unknown = false;
          normal = true;
        } else {
          // multiply by infinity.  Check sign.
          answer = SIGN(answer) * prod->signInfinity();
          infinite = true;
          normal = false;
           }
        break;
    }
    if (prod->isUnknown()) {
        unknown = true;
        normal = false;
        continue;
    }
    // must be an error, short circuit
    prod->setNull();
    return;
  } // for i


  // The infinity case
  if (infinite) {
    // Keep multiplying, only worry about sign and make sure we don't
    // (1) multiply by zero
    // (2) divide by zero
    // (3) divide by infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (prod->isNormal()) {
        if (prod->getInt()) {
          // infinity * a or infinity / a, just fix the sign.
          answer *= SIGN(prod->getInt());
          continue;
         }
        // infinity * 0 or infinity / 0, error
        inftyTimesZero(flip[i], operands[i]);
        prod->setNull();
        return;
      } // if prod->isNormal()
      if (prod->isInfinity()) {
        if (!flip[i]) {
          // infinity * infinity, check sign
          answer *= prod->signInfinity();
          continue;
        }
        // infinity / infinity, error
        inftyDivInfty(operands[i]);
        prod->setNull();
        return;  // short circuit.
      } // if foo.isInfinity()
      if (prod->isUnknown()) {
        unknown = true;  // can't be sure of sign
        continue;
      }
      // must be an error
      prod->setNull();
      return;
    } // for i
  } // if infinite

  // The zero case
  if (0.0 == answer) {
    // Check the remaining operands, and make sure we don't
    // (1) divide by 0
    // (2) multiply by infinity
    for (i++; i<opnd_count; i++) {
      DCASSERT(operands[i]);
      operands[i]->Compute(x);
      if (prod->isUnknown()) continue;
      if (prod->isNormal()) {
        if (prod->getInt())  continue;
        if (flip[i]) {
          divideByZero(operands[i]);
          prod->setNull();
          return;  // short circuit.
        }
        continue;
      } // if prod->isNormal
      if (prod->isInfinity()) {
        if (flip[i])  continue;  // 0 / infinity = 0.
        zeroTimesInfty(operands[i]);
        prod->setNull();
        return;
      } // if prod->isInfinity()
      prod->setNull();
      return;  // error...short circuit
    } // for i
  }

  if (normal) {
    prod->setReal(answer);
    return;
  }
  if (unknown) {
    prod->setUnknown();
    return;
  }
  DCASSERT(infinite);
  prod->setInfinity(SIGN(answer));
}

expr* int_multdiv::buildAnother(expr **x, bool* f, int n) const
{
  return new int_multdiv(Where(), Type(), x, f, n);
}

// ******************************************************************
// *                                                                *
// *                      int_multdiv_op class                      *
// *                                                                *
// ******************************************************************

class int_multdiv_op : public int_assoc_op {
public:
  int_multdiv_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                     int_multdiv_op methods                     *
// ******************************************************************

int_multdiv_op::int_multdiv_op() : int_assoc_op(exprman::aop_times)
{
}

int int_multdiv_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  // first, make there is at least one flipped
  // (otherwise we are handled by int_mult_op)
  if (0==flip)    return -1;
  bool unflipped = true;
  for (int i=0; i<N; i++) if (flip[i]) {
    unflipped = false;
    break;
  }
  if (unflipped)  return -1;
  return IntAlignDistance(em, list, N);
}

int int_multdiv_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (!f)  return -1;
  return IntAlignDistance(em, lt, rt);
}

const type* int_multdiv_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (!f)  return 0;
  const type* lct = IntResultType(em, l, r);
  if (lct)  lct = lct->changeBaseType(em->REAL);
  return lct;
}

assoc* int_multdiv_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignIntegers(em, list, N);
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
  if (lct)  return new int_multdiv(W, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                         int_mod  class                         *
// *                                                                *
// ******************************************************************

/// Modulo arithmetic for integer expressions.
class int_mod : public modulo {
public:
  int_mod(const location &W, const type* t, expr* l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr* l, expr* r) const;
};

// ******************************************************************
// *                        int_mod  methods                        *
// ******************************************************************

int_mod::int_mod(const location &W, const type* t, expr *l, expr *r)
 : modulo(W, t, l, r)
{
}

void int_mod::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    if (r.getInt()) {
      // normal modulo
      x.answer->setInt( l.getInt() % r.getInt() );
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
    x.answer->setInt( l.getInt() );  // should we check signs?
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

expr* int_mod::buildAnother(expr *l, expr *r) const
{
  return new int_mod(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      int_binary_op  class                      *
// *                                                                *
// ******************************************************************

class int_binary_op : public binary_op {
public:
  int_binary_op(exprman::binary_opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                     int_binary_op  methods                     *
// ******************************************************************

int_binary_op::int_binary_op(exprman::binary_opcode op) : binary_op(op)
{
}

int int_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return IntAlignDistance(em, lt, rt);
}

const type* int_binary_op::getExprType(const type* l, const type* r) const
{
  return IntResultType(em, l, r);
}

// ******************************************************************
// *                                                                *
// *                       int_comp_op  class                       *
// *                                                                *
// ******************************************************************

class int_comp_op : public int_binary_op {
public:
  int_comp_op(exprman::binary_opcode op);
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                      int_comp_op  methods                      *
// ******************************************************************

int_comp_op::int_comp_op(exprman::binary_opcode op) : int_binary_op(op)
{
}

const type* int_comp_op::getExprType(const type* l, const type* r) const
{
  const type* t = IntResultType(em, l, r);
  if (t)  t = t->changeBaseType(em->BOOL);
  return t;
}

// ******************************************************************
// *                                                                *
// *                        int_mod_op class                        *
// *                                                                *
// ******************************************************************

class int_mod_op : public int_binary_op {
public:
  int_mod_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_mod_op methods                       *
// ******************************************************************

int_mod_op::int_mod_op() : int_binary_op(exprman::bop_mod)
{
}

binary* int_mod_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  return new int_mod(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_equal  class                        *
// *                                                                *
// ******************************************************************

/// Check equality of two integer expressions.
class int_equal : public eqop {
public:
  int_equal(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       int_equal  methods                       *
// ******************************************************************

int_equal::int_equal(const location &W, const type* t, expr *l, expr *r)
 : eqop(W, t, l, r)
{
}

void int_equal::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() == r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_equal::buildAnother(expr *l, expr *r) const
{
  return new int_equal(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                       int_equal_op class                       *
// *                                                                *
// ******************************************************************

class int_equal_op : public int_comp_op {
public:
  int_equal_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                      int_equal_op methods                      *
// ******************************************************************

int_equal_op::int_equal_op() : int_comp_op(exprman::bop_equals)
{
}

binary* int_equal_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_equal(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         int_neq  class                         *
// *                                                                *
// ******************************************************************

/** Check inequality of two integer expressions.
 */
class int_neq : public neqop {
public:
  int_neq(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        int_neq  methods                        *
// ******************************************************************

int_neq::int_neq(const location &W, const type* t, expr *l, expr *r)
 : neqop(W, t, l, r)
{
}

void int_neq::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() != r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_neq::buildAnother(expr *l, expr *r) const
{
  return new int_neq(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_neq_op class                        *
// *                                                                *
// ******************************************************************

class int_neq_op : public int_comp_op {
public:
  int_neq_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_neq_op methods                       *
// ******************************************************************

int_neq_op::int_neq_op() : int_comp_op(exprman::bop_nequal)
{
}

binary* int_neq_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_neq(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                          int_gt class                          *
// *                                                                *
// ******************************************************************

/// Check if one integer expression is greater than another.
class int_gt : public gtop {
public:
  int_gt(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                         int_gt methods                         *
// ******************************************************************

int_gt::int_gt(const location &W, const type* t, expr *l, expr *r)
 : gtop(W, t, l, r)
{
}

void int_gt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() > r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_gt::buildAnother(expr *l, expr *r) const
{
  return new int_gt(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_gt_op  class                        *
// *                                                                *
// ******************************************************************

class int_gt_op : public int_comp_op {
public:
  int_gt_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_gt_op  methods                       *
// ******************************************************************

int_gt_op::int_gt_op() : int_comp_op(exprman::bop_gt)
{
}

binary* int_gt_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_gt(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                          int_ge class                          *
// *                                                                *
// ******************************************************************

/// Check if one integer expression is greater than or equal another.
class int_ge : public geop {
public:
  int_ge(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                         int_ge methods                         *
// ******************************************************************

int_ge::int_ge(const location &W, const type* t, expr *l, expr *r)
 : geop(W, t, l, r)
{
}

void int_ge::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() >= r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_ge::buildAnother(expr *l, expr *r) const
{
  return new int_ge(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_ge_op  class                        *
// *                                                                *
// ******************************************************************

class int_ge_op : public int_comp_op {
public:
  int_ge_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_ge_op  methods                       *
// ******************************************************************

int_ge_op::int_ge_op() : int_comp_op(exprman::bop_ge)
{
}

binary* int_ge_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_ge(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                          int_lt class                          *
// *                                                                *
// ******************************************************************

/// Check if one integer expression is less than another.
class int_lt : public ltop {
public:
  int_lt(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                         int_lt methods                         *
// ******************************************************************

int_lt::int_lt(const location &W, const type* t, expr *l, expr *r)
 : ltop(W, t, l, r)
{
}

void int_lt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() < r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_lt::buildAnother(expr *l, expr *r) const
{
  return new int_lt(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_lt_op  class                        *
// *                                                                *
// ******************************************************************

class int_lt_op : public int_comp_op {
public:
  int_lt_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_lt_op  methods                       *
// ******************************************************************

int_lt_op::int_lt_op() : int_comp_op(exprman::bop_lt)
{
}

binary* int_lt_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_lt(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                          int_le class                          *
// *                                                                *
// ******************************************************************

/// Check if one integer expression is less than or equal another.
class int_le : public leop {
public:
  int_le(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                         int_le methods                         *
// ******************************************************************

int_le::int_le(const location &W, const type* t, expr *l, expr *r)
 : leop(W, t, l, r)
{
}

void int_le::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool( l.getInt() <= r.getInt() );
  } else {
    Special(l, r, x);
  }
}

expr* int_le::buildAnother(expr *l, expr *r) const
{
  return new int_le(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        int_le_op  class                        *
// *                                                                *
// ******************************************************************

class int_le_op : public int_comp_op {
public:
  int_le_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       int_le_op  methods                       *
// ******************************************************************

int_le_op::int_le_op() : int_comp_op(exprman::bop_le)
{
}

binary* int_le_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignIntegers(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new int_le(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitIntegerOps(exprman* em)
{
  if (0==em)  return;
  em->registerOperation( new int_neg_op     );
  em->registerOperation( new int_add_op     );
  em->registerOperation( new int_mult_op    );
  em->registerOperation( new int_multdiv_op );
  em->registerOperation( new int_mod_op     );
  em->registerOperation( new int_equal_op   );
  em->registerOperation( new int_neq_op     );
  em->registerOperation( new int_gt_op      );
  em->registerOperation( new int_ge_op      );
  em->registerOperation( new int_lt_op      );
  em->registerOperation( new int_le_op      );
}


