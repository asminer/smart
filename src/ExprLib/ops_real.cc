
#include "ops_real.h"
#include "../Streams/streams.h"
#include "exprman.h"
#include "unary.h"
#include "binary.h"
#include "assoc.h"

/**

   Implementation of operator classes, for reals.

 */

//#define DEBUG_DEEP

inline const type*
RealResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  if (em->NULTYPE == lt || em->NULTYPE == rt)  return 0;
  const type* lct = Phase2Rand(em->getLeastCommonType(lt, rt));
  if (0==lct)                         return 0;
  if (lct->getBaseType() != em->REAL) return 0;
  if (lct->isASet())                  return 0;
  return lct;
}

inline int RealAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  const type* lct = RealResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);
  if (dl<0) return -1;
  int dr = em->getPromoteDistance(rt, lct);
  if (dr<0) return -1;

  return dl+dr;
}

inline const type* AlignReals(const exprman* em, expr* &l, expr* &r)
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  DCASSERT(l);
  DCASSERT(r);
  const type* lct = RealResultType(em, l->Type(), r->Type());
  if (0==lct) {
    Delete(l);
    Delete(r);
    return 0;
  }
  l = em->promote(l, lct);   DCASSERT(em->isOrdinary(l));
  r = em->promote(r, lct);   DCASSERT(em->isOrdinary(r));
  return lct;
}

inline int RealAlignDistance(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  lct = Phase2Rand(lct);
  if (0==lct)                         return -1;
  if (lct->getBaseType() != em->REAL) return -1;
  if (lct->isASet())                  return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    if (dx<0) return -1;
    d += dx;
  }
  return d;
}

inline const type* AlignReals(const exprman* em, expr** x, int N)
{
  DCASSERT(em);
  DCASSERT(em->REAL);
  DCASSERT(x);

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  lct = Phase2Rand(lct);
  if (  (0==lct) || (lct->getBaseType() != em->REAL) || lct->isASet() ) {
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
// *                      real_neg_expr  class                      *
// *                                                                *
// ******************************************************************

/// Negation of a real expression.
class real_neg_expr : public negop {
public:
  real_neg_expr(const location &W, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                     real_neg_expr  methods                     *
// ******************************************************************

real_neg_expr::real_neg_expr(const location &W, expr *x)
 : negop(W, exprman::uop_neg, x->Type(), x)
{
}

void real_neg_expr::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  if (x.answer->isNormal()) {
    x.answer->setReal( -x.answer->getReal() );
  } else if (x.answer->isInfinity()) {
    x.answer->setInfinity(-x.answer->signInfinity());
  }
}

expr* real_neg_expr::buildAnother(expr *x) const
{
  return new real_neg_expr(Where(), x);
}

// ******************************************************************
// *                                                                *
// *                       real_neg_op  class                       *
// *                                                                *
// ******************************************************************

class real_neg_op : public unary_op {
public:
  real_neg_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************
// *                      real_neg_op  methods                      *
// ******************************************************************

real_neg_op::real_neg_op() : unary_op(exprman::uop_neg)
{
}

const type* real_neg_op::getExprType(const type* t) const
{
  if (0==t)    return 0;
  if (t->isASet())  return 0;
  const type* bt = t->getBaseType();
  if (bt != em->REAL)  return 0;
  return Phase2Rand(t);
}

unary* real_neg_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new real_neg_expr(W, x);
}

// ******************************************************************
// *                                                                *
// *                         real_add class                         *
// *                                                                *
// ******************************************************************

/// Addition of real expressions.
class real_add : public summation {
public:
  real_add(const location &W, const type* t, expr **x, bool* f, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                        real_add methods                        *
// ******************************************************************

real_add
::real_add(const location &W, const type* t, expr** x, bool* f, int n)
 : summation(W, exprman::aop_plus, t, x, f, n)
{
}

void real_add::Compute(traverse_data &x)
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
  double answer = 0;
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
        if (flip && flip[i])  answer -= sum->getReal();
        else                  answer += sum->getReal();
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
    sum->setReal( answer );
    return;
  }
  if (unknown) {
    sum->setUnknown();
    return;
  }

  sum->setInfinity(SIGN(answer));
}

expr* real_add::buildAnother(expr **x, bool* f, int n) const
{
  return new real_add(Where(), Type(), x, f, n);
}

// ******************************************************************
// *                                                                *
// *                      real_assoc_op  class                      *
// *                                                                *
// ******************************************************************

class real_assoc_op : public assoc_op {
public:
  real_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                     real_assoc_op  methods                     *
// ******************************************************************

real_assoc_op::real_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int real_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return RealAlignDistance(em, list, N);
}

int real_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  return RealAlignDistance(em, lt, rt);
}

const type* real_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  return RealResultType(em, l, r);
}


// ******************************************************************
// *                                                                *
// *                       real_add_op  class                       *
// *                                                                *
// ******************************************************************

class real_add_op : public real_assoc_op {
public:
  real_add_op();
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                      real_add_op  methods                      *
// ******************************************************************

real_add_op::real_add_op() : real_assoc_op(exprman::aop_plus)
{
}

assoc* real_add_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignReals(em, list, N);
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
  if (lct)  return new real_add(W, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}



// ******************************************************************
// *                                                                *
// *                        real_mult  class                        *
// *                                                                *
// ******************************************************************

/// Multiplication of real expressions.
class real_mult : public product {
public:
  real_mult(const location &W, const type* t, expr **x, bool* f, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                       real_mult  methods                       *
// ******************************************************************

real_mult
::real_mult(const location &W, const type* t, expr **x, bool* f, int n)
 : product(W, exprman::aop_times, t, x, f, n)
{
}

void real_mult::Compute(traverse_data &x)
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
        if (prod->getReal()) {
          // normal finite multiply / divide
          if (flip && flip[i])  answer /= prod->getReal();
          else                  answer *= prod->getReal();
          continue;
        }
        // we have a zero term
        if (flip && flip[i]) {
          // divide by zero, bail out
          divideByZero(operands[i]);
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
        if (flip && flip[i]) {
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
        if (prod->getReal()) {
          // infinity * a or infinity / a, just fix the sign.
          answer *= SIGN(prod->getReal());
          continue;
         }
        // infinity * 0 or infinity / 0, error
        inftyTimesZero(flip && flip[i], operands[i]);
        prod->setNull();
        return;
      } // if prod->isNormal()
      if (prod->isInfinity()) {
        if (!flip || !flip[i]) {
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
        if (prod->getReal())  continue;
        if (flip && flip[i]) {
          divideByZero(operands[i]);
          prod->setNull();
          return;  // short circuit.
        }
        continue;
      } // if prod->isNormal
      if (prod->isInfinity()) {
        if (flip && flip[i])  continue;  // 0 / infinity = 0.
        zeroTimesInfty(operands[i]);
        prod->setNull();
        return;
      } // if prod->isInfinity()
      prod->setNull();
      return;  // error...short circuit
    } // for i
  }

  if (normal) {
    prod->setReal( answer );
    return;
  }
  if (unknown) {
    prod->setUnknown();
    return;
  }
  DCASSERT(infinite);
  prod->setInfinity(SIGN(answer));
}

expr* real_mult::buildAnother(expr **x, bool* f, int n) const
{
  return new real_mult(Where(), Type(), x, f, n);
}

// ******************************************************************
// *                                                                *
// *                       real_mult_op class                       *
// *                                                                *
// ******************************************************************

class real_mult_op : public real_assoc_op {
public:
  real_mult_op();
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                      real_mult_op methods                      *
// ******************************************************************

real_mult_op::real_mult_op() : real_assoc_op(exprman::aop_times)
{
}

assoc* real_mult_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignReals(em, list, N);
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
  if (lct)  return new real_mult(W, lct, list, flip, N);
  // there was an error
  delete[] list;
  delete[] flip;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                        real_equal class                        *
// *                                                                *
// ******************************************************************

/** Check equality of two real expressions.
    To do still: check precision option, etc.
 */
class real_equal : public eqop {
public:
  real_equal(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                       real_equal methods                       *
// ******************************************************************

real_equal::real_equal(const location &W, const type* t, expr *l, expr *r)
 : eqop(W, t, l, r)
{
}

void real_equal::Compute(traverse_data &x)
{
  result l,r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() == r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_equal::buildAnother(expr *l, expr *r) const
{
  return new real_equal(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      real_binary_op class                      *
// *                                                                *
// ******************************************************************

class real_binary_op : public binary_op {
public:
  real_binary_op(exprman::binary_opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                     real_binary_op methods                     *
// ******************************************************************

real_binary_op::real_binary_op(exprman::binary_opcode op) : binary_op(op)
{
}

int real_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return RealAlignDistance(em, lt, rt);
}

const type* real_binary_op::getExprType(const type* l, const type* r) const
{
  return RealResultType(em, l, r);
}

// ******************************************************************
// *                                                                *
// *                       real_comp_op class                       *
// *                                                                *
// ******************************************************************

class real_comp_op : public real_binary_op {
public:
  real_comp_op(exprman::binary_opcode op);
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                      real_comp_op methods                      *
// ******************************************************************

real_comp_op
::real_comp_op(exprman::binary_opcode op) : real_binary_op(op)
{
}

const type* real_comp_op::getExprType(const type* l, const type* r) const
{
  const type* t = RealResultType(em, l, r);
  if (t)  t = t->changeBaseType(em->BOOL);
  return t;
}

// ******************************************************************
// *                                                                *
// *                      real_equal_op  class                      *
// *                                                                *
// ******************************************************************

class real_equal_op : public real_comp_op {
public:
  real_equal_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                     real_equal_op  methods                     *
// ******************************************************************

real_equal_op::real_equal_op() : real_comp_op(exprman::bop_equals)
{
}

binary* real_equal_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_equal(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         real_neq class                         *
// *                                                                *
// ******************************************************************

/// Check inequality of two real expressions.
class real_neq : public neqop {
public:
  real_neq(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        real_neq methods                        *
// ******************************************************************

real_neq::real_neq(const location &W, const type* t, expr *l, expr *r)
 : neqop(W, t, l, r)
{
}

void real_neq::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() != r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_neq::buildAnother(expr *l, expr *r) const
{
  return new real_neq(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                       real_neq_op  class                       *
// *                                                                *
// ******************************************************************

class real_neq_op : public real_comp_op {
public:
  real_neq_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                      real_neq_op  methods                      *
// ******************************************************************

real_neq_op::real_neq_op() : real_comp_op(exprman::bop_nequal)
{
}

binary* real_neq_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_neq(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         real_gt  class                         *
// *                                                                *
// ******************************************************************

/// Check if one real expression is greater than another.
class real_gt : public gtop {
public:
  real_gt(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        real_gt  methods                        *
// ******************************************************************

real_gt::real_gt(const location &W, const type* t, expr *l, expr *r)
 : gtop(W, t, l, r)
{
}

void real_gt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() > r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_gt::buildAnother(expr *l, expr *r) const
{
  return new real_gt(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        real_gt_op class                        *
// *                                                                *
// ******************************************************************

class real_gt_op : public real_comp_op {
public:
  real_gt_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       real_gt_op methods                       *
// ******************************************************************

real_gt_op::real_gt_op() : real_comp_op(exprman::bop_gt)
{
}

binary* real_gt_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_gt(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         real_ge  class                         *
// *                                                                *
// ******************************************************************

/// Check if one real expression is greater than or equal another.
class real_ge : public geop {
public:
  real_ge(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        real_ge  methods                        *
// ******************************************************************

real_ge::real_ge(const location &W, const type* t, expr *l, expr *r)
 : geop(W, t, l, r)
{
}

void real_ge::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() >= r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_ge::buildAnother(expr *l, expr *r) const
{
  return new real_ge(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        real_ge_op class                        *
// *                                                                *
// ******************************************************************

class real_ge_op : public real_comp_op {
public:
  real_ge_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       real_ge_op methods                       *
// ******************************************************************

real_ge_op::real_ge_op() : real_comp_op(exprman::bop_ge)
{
}

binary* real_ge_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_ge(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         real_lt  class                         *
// *                                                                *
// ******************************************************************

/// Check if one real expression is less than another.
class real_lt : public ltop {
public:
  real_lt(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        real_lt  methods                        *
// ******************************************************************

real_lt::real_lt(const location &W, const type* t, expr *l, expr *r)
 : ltop(W, t, l, r)
{
}

void real_lt::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() < r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_lt::buildAnother(expr *l, expr *r) const
{
  return new real_lt(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        real_lt_op class                        *
// *                                                                *
// ******************************************************************

class real_lt_op : public real_comp_op {
public:
  real_lt_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       real_lt_op methods                       *
// ******************************************************************

real_lt_op::real_lt_op() : real_comp_op(exprman::bop_lt)
{
}

binary* real_lt_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_lt(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         real_le  class                         *
// *                                                                *
// ******************************************************************

/// Check if one real expression is less than or equal another.
class real_le : public leop {
public:
  real_le(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        real_le  methods                        *
// ******************************************************************

real_le::real_le(const location &W, const type* t, expr *l, expr *r)
 : leop(W, t, l, r)
{
}

void real_le::Compute(traverse_data &x)
{
  result l, r;
  LRCompute(l, r, x);

  if (l.isNormal() && r.isNormal()) {
    // normal comparison
    x.answer->setBool(l.getReal() <= r.getReal());
  } else {
    Special(l, r, x);
  }
}

expr* real_le::buildAnother(expr *l, expr *r) const
{
  return new real_le(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                        real_le_op class                        *
// *                                                                *
// ******************************************************************

class real_le_op : public real_comp_op {
public:
  real_le_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                       real_le_op methods                       *
// ******************************************************************

real_le_op::real_le_op() : real_comp_op(exprman::bop_le)
{
}

binary* real_le_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignReals(em, l, r);
  if (0==lct)  return 0;
  lct = lct->changeBaseType(em->BOOL);
  DCASSERT(lct);
  return new real_le(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitRealOps(exprman* em)
{
  if (0==em)  return;
  em->registerOperation(  new real_neg_op   );
  em->registerOperation(  new real_add_op   );
  em->registerOperation(  new real_mult_op  );
  em->registerOperation(  new real_equal_op );
  em->registerOperation(  new real_neq_op   );
  em->registerOperation(  new real_gt_op    );
  em->registerOperation(  new real_ge_op    );
  em->registerOperation(  new real_lt_op    );
  em->registerOperation(  new real_le_op    );
}

