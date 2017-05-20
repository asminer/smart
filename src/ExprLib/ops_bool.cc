
#include "ops_bool.h"
#include "../Streams/streams.h"
#include "exprman.h"
#include "unary.h"
#include "binary.h"
#include "assoc.h"

//#define OPTIMIZE_AND_ORDER
//#define OPTIMIZE_OR_ORDER

/** 

   Implementation of operator classes, for bool variables.

 */

inline const type* 
BoolResultType(const exprman* em, const type* lt, const type* rt)
{
  DCASSERT(em);
  DCASSERT(em->BOOL);
  if (em->NULTYPE == lt || em->NULTYPE ==rt)  return 0;
  const type* lct = em->getLeastCommonType(lt, rt);
  if (0==lct)                         return 0;
  if (lct->getBaseType() != em->BOOL) return 0;
  if (lct->isASet())                  return 0;
  return lct;
}

inline int BoolAlignDistance(const exprman* em, const type* lt, const type* rt)
{
  const type* lct = BoolResultType(em, lt, rt);
  if (0==lct)        return -1;

  int dl = em->getPromoteDistance(lt, lct);
  if (dl<0) return -1;
  int dr = em->getPromoteDistance(rt, lct);
  if (dr<0) return -1;

  return dl+dr;
}

inline const type* AlignBooleans(const exprman* em, expr* &l, expr* &r)
{
  DCASSERT(l);
  DCASSERT(r);
  const type* lct = BoolResultType(em, l->Type(), r->Type());
  if (0==lct) {
    Delete(l);
    Delete(r);
    return 0;
  }
  l = em->promote(l, lct);   DCASSERT(em->isOrdinary(l));
  r = em->promote(r, lct);   DCASSERT(em->isOrdinary(r));
  return lct;
}

inline int BoolAlignDistance(const exprman* em, expr** x, bool* f, int N)
{
  DCASSERT(em);
  DCASSERT(em->BOOL);
  DCASSERT(x);

  // check flips, if any
  if (f) for (int i=0; i<N; i++) if (f[i])  return -1;

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (0==lct)                         return -1;
  if (lct->getBaseType() != em->BOOL) return -1;
  if (lct->isASet())                  return -1;

  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(x[i]), lct);
    if (dx<0) return -1;
    d += dx;
  }
  return d;
}

inline const type* AlignBooleans(const exprman* em, expr** x, bool* f, int N)
{
  DCASSERT(em);
  DCASSERT(em->BOOL);
  DCASSERT(x);

  // check flips, if any
  if (f) for (int i=0; i<N; i++) if (f[i])  return 0;

  const type* lct = em->SafeType(x[0]);
  for (int i=1; i<N; i++) {
    lct = em->getLeastCommonType(lct, em->SafeType(x[i]));
  }
  if (  (0==lct) || (lct->getBaseType() != em->BOOL) || lct->isASet() ) {
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
// *                      bool_not_expr  class                      *
// *                                                                *
// ******************************************************************

/// Negation of a boolean expression.
class bool_not_expr : public negop {
public:
  bool_not_expr(const char* fn, int line, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                     bool_not_expr  methods                     *
// ******************************************************************

bool_not_expr::bool_not_expr(const char* fn, int line, expr *x)
 : negop(fn, line, exprman::uop_not, x->Type(), x) 
{ 
}

void bool_not_expr::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x); 

  if (!x.answer->isNormal()) return;

  x.answer->setBool( !x.answer->getBool() );
}

expr* bool_not_expr::buildAnother(expr *x) const 
{
  return new bool_not_expr(Filename(), Linenumber(), x);
}

// ******************************************************************
// *                                                                *
// *                       bool_not_op  class                       *
// *                                                                *
// ******************************************************************

class bool_not_op : public unary_op {
public:
  bool_not_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                      bool_not_op  methods                      *
// ******************************************************************

bool_not_op::bool_not_op() : unary_op(exprman::uop_not)
{
}

const type* bool_not_op::getExprType(const type* t) const
{
  if (0==t)    return 0;
  if (t->isASet())  return 0; 
  const type* bt = t->getBaseType();
  DCASSERT(em);
  DCASSERT(em->BOOL);
  if (bt != em->BOOL)  return 0;
  return t;
}

unary* bool_not_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new bool_not_expr(fn, ln, x);
}

// ******************************************************************
// *                                                                *
// *                         bool_or  class                         *
// *                                                                *
// ******************************************************************

/// Or of boolean expressions.
class bool_or : public summation {
public:
  bool_or(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                        bool_or  methods                        *
// ******************************************************************

bool_or::bool_or(const char* fn, int line, const type* t, expr **x, int n)
 : summation(fn, line, exprman::aop_or, t, x, 0, n) 
{ 
}

void bool_or::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (x.answer->isNormal()) {
      if (x.answer->getBool()) {
#ifdef OPTIMIZE_OR_ORDER
        if (i) SWAP(operands[0], operands[i]);
#endif
        return; // short circuit
      }
      continue; // still false
    }
    if (x.answer->isUnknown()) {
      unknown = true;
      continue;
    }
    // error or null, short circuit
    return;
  } // for i
  if (unknown) x.answer->setUnknown();
}

expr* bool_or::buildAnother(expr **x, bool* f, int n) const
{
  return new bool_or(Filename(), Linenumber(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                      bool_assoc_op  class                      *
// *                                                                *
// ******************************************************************

class bool_assoc_op : public assoc_op {
public:
  bool_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                     bool_assoc_op  methods                     *
// ******************************************************************

bool_assoc_op::bool_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int bool_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return BoolAlignDistance(em, list, flip, N);
}

int bool_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f) return -1;
  return BoolAlignDistance(em, lt, rt);
}

const type* bool_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f) return 0;
  return BoolResultType(em, l, r);
}


// ******************************************************************
// *                                                                *
// *                        bool_or_op class                        *
// *                                                                *
// ******************************************************************

class bool_or_op : public bool_assoc_op {
public:
  bool_or_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                       bool_or_op methods                       *
// ******************************************************************

bool_or_op::bool_or_op() : bool_assoc_op(exprman::aop_or)
{
}

assoc* bool_or_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  const type* lct = AlignBooleans(em, list, flip, N);
  delete[] flip;
  if (lct)  return new bool_or(fn, ln, lct, list, N);
  // there was an error
  delete[] list;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                         bool_and class                         *
// *                                                                *
// ******************************************************************

/// And of boolean expressions.
class bool_and : public product {
public:
  bool_and(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                        bool_and methods                        *
// ******************************************************************

bool_and::bool_and(const char* fn, int line, const type* t, expr **x, int n) 
 : product(fn, line, exprman::aop_and, t, x, 0, n) 
{ 
}

void bool_and::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  int i;
  bool unknown = false;
  for (i=0; i<opnd_count; i++) {
    DCASSERT(operands[i]);
    operands[i]->Compute(x);
    if (x.answer->isNormal()) {
      if (!x.answer->getBool()) {
#ifdef OPTIMIZE_OR_ORDER
        if (i) SWAP(operands[0], operands[i]);
#endif
        return; // short circuit
      }
      continue; // still true
    }
    if (x.answer->isUnknown()) {
      unknown = true;
      continue;
    }
    // error or null, short circuit
    return;
  } // for i
  if (unknown) x.answer->setUnknown();
}

expr* bool_and::buildAnother(expr **x, bool* f, int n) const
{
  return new bool_and(Filename(), Linenumber(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                       bool_and_op  class                       *
// *                                                                *
// ******************************************************************

class bool_and_op : public bool_assoc_op {
public:
  bool_and_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                      bool_and_op  methods                      *
// ******************************************************************

bool_and_op::bool_and_op() : bool_assoc_op(exprman::aop_and)
{
}

assoc* bool_and_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  const type* lct = AlignBooleans(em, list, flip, N);
  delete[] flip;
  if (lct)  return new bool_and(fn, ln, lct, list, N);
  // there was an error
  delete[] list;
  return 0;
}


// ******************************************************************
// *                                                                *
// *                       bool_implies class                       *
// *                                                                *
// ******************************************************************

/// Implication.
class bool_implies : public binary {
public:
  bool_implies(const char* fn, int line, const type* t, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr* l, expr* r) const;
};

// ******************************************************************
// *                      bool_implies methods                      *
// ******************************************************************

bool_implies::bool_implies(const char* fn, int line, const type* t, expr *l, expr* r)
 : binary(fn, line, exprman::bop_implies, t, l, r) 
{ 
}

void bool_implies::Compute(traverse_data &x)
{
  result lv, rv;
  LRCompute(lv, rv, x);

  if (lv.isNormal() && rv.isNormal()) {
    x.answer->setBool( (!lv.getBool()) || rv.getBool() );
    return;
  }
  if (lv.isNull() || rv.isNull()) {
    x.answer->setNull();
    return;
  }
  DCASSERT(lv.isNormal() || lv.isUnknown());  
  DCASSERT(rv.isNormal() || rv.isUnknown());  
  // left and right are either both unknown, or
  // at most one is known; see if we have enough
  // information to determine the result
  if (lv.isNormal() && !lv.getBool()) {
    x.answer->setBool( true );
    return;
  }
  if (rv.isNormal() && rv.getBool()) {
    x.answer->setBool( true );
    return;
  }
  // not enough info
  x.answer->setUnknown();
}

expr* bool_implies::buildAnother(expr* l, expr* r) const
{
  return new bool_implies(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      bool_binary_op class                      *
// *                                                                *
// ******************************************************************

class bool_binary_op : public binary_op {
public:
  bool_binary_op(exprman::binary_opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                     bool_binary_op methods                     *
// ******************************************************************

bool_binary_op::bool_binary_op(exprman::binary_opcode op) : binary_op(op)
{
}

int bool_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return BoolAlignDistance(em, lt, rt);
}

const type* bool_binary_op::getExprType(const type* l, const type* r) const
{
  return BoolResultType(em, l, r);
}

// ******************************************************************
// *                                                                *
// *                     bool_implies_op  class                     *
// *                                                                *
// ******************************************************************

class bool_implies_op : public bool_binary_op {
public:
  bool_implies_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                    bool_implies_op  methods                    *
// ******************************************************************

bool_implies_op::bool_implies_op() : bool_binary_op(exprman::bop_implies)
{
}

binary* bool_implies_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(em, l, r);
  if (0==lct)  return 0;
  return new bool_implies(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                        bool_equal class                        *
// *                                                                *
// ******************************************************************

/// Check equality of two boolean expressions.
class bool_equal : public eqop {
public:
  bool_equal(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr* r) const;
};

// ******************************************************************
// *                       bool_equal methods                       *
// ******************************************************************

bool_equal::bool_equal(const char* fn, int line, const type* t, expr *l, expr *r) 
 : eqop(fn, line, t, l, r) 
{ 
}
  
void bool_equal::Compute(traverse_data &x)
{
  result lv, rv;
  LRCompute(lv, rv, x);

  if (lv.isNormal() && rv.isNormal()) {
    x.answer->setBool( lv.getBool() == rv.getBool() );
  } else {
    Special(lv, rv, x);
  }
}

expr* bool_equal::buildAnother(expr *l, expr* r) const
{
  return new bool_equal(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      bool_equal_op  class                      *
// *                                                                *
// ******************************************************************

class bool_equal_op : public bool_binary_op {
public:
  bool_equal_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                     bool_equal_op  methods                     *
// ******************************************************************

bool_equal_op::bool_equal_op() : bool_binary_op(exprman::bop_equals)
{
}

binary* bool_equal_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(em, l, r);
  if (0==lct)  return 0;
  return new bool_equal(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         bool_neq class                         *
// *                                                                *
// ******************************************************************

/// Check inequality of two boolean expressions.
class bool_neq : public neqop {
public:
  bool_neq(const char* fn, int line, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        bool_neq methods                        *
// ******************************************************************

bool_neq::bool_neq(const char* fn, int line, const type* t, expr *l, expr *r)
 : neqop(fn, line, t, l, r) 
{ 
}
  
void bool_neq::Compute(traverse_data &x)
{
  result lv, rv;
  LRCompute(lv, rv, x);

  if (lv.isNormal() && rv.isNormal()) {
    x.answer->setBool( lv.getBool() != rv.getBool() );
  } else {
    Special(lv, rv, x);
  }
}

expr* bool_neq::buildAnother(expr *l, expr *r) const
{
  return new bool_neq(Filename(), Linenumber(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                       bool_neq_op  class                       *
// *                                                                *
// ******************************************************************

class bool_neq_op : public bool_binary_op {
public:
  bool_neq_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bool_neq_op  methods                      *
// ******************************************************************

bool_neq_op::bool_neq_op() : bool_binary_op(exprman::bop_nequal)
{
}

binary* bool_neq_op::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(em, l, r);
  if (0==lct)  return 0;
  return new bool_neq(fn, ln, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitBooleanOps(exprman* em)
{
  if (0==em)  return;
  em->registerOperation(  new bool_not_op     );
  em->registerOperation(  new bool_or_op      );
  em->registerOperation(  new bool_and_op     );
  em->registerOperation(  new bool_implies_op );
  em->registerOperation(  new bool_equal_op   );
  em->registerOperation(  new bool_neq_op     );
}


