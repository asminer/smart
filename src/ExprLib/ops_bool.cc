
#include "unary.h"
#include "binary.h"
#include "assoc.h"
#include "bogus.h"
#include "casting.h"

#include "../Utils/initializer.h"

//#define OPTIMIZE_AND_ORDER
//#define OPTIMIZE_OR_ORDER

/**

   Implementation of operator classes, for bool variables.

 */

inline const type*
BoolResultType(const type* lt, const type* rt)
{
    if (type::null == lt || type::null ==rt)            return nullptr;
    const type* lct = typeconv::getLeastCommonType(lt, rt);
    if (!lct)                                           return nullptr;
    if (!type::matches(lct->getBaseType(), "bool"))     return nullptr;
    if (lct->isASet())                                  return nullptr;
    return lct;
}

inline int BoolAlignDistance(const type* lt, const type* rt)
{
    const type* lct = BoolResultType(lt, rt);
    if (!lct) return -1;
    int dl = typeconv::getPromoteDistance(lt, lct);
    if (dl<0) return -1;
    int dr = typeconv::getPromoteDistance(rt, lct);
    if (dr<0) return -1;

    return dl+dr;
}

inline const type* AlignBooleans(const location &W, expr* &l, expr* &r)
{
    DCASSERT(l);
    DCASSERT(r);
    const type* lct = BoolResultType(l->Type(), r->Type());
    if (0==lct) {
        Delete(l);
        Delete(r);
        return nullptr;
    }
    l = typeconv::castExpr(true, W, lct, l);
    r = typeconv::castExpr(true, W, lct, r);
    DCASSERT(!bogus_expr::orNull(l));
    DCASSERT(!bogus_expr::orNull(r));
    return lct;
}

inline int BoolAlignDistance(expr** x, bool* f, int N)
{
    DCASSERT(x);

    // check flips, if any
    if (f) for (int i=0; i<N; i++) if (f[i])  return -1;

    const type* lct = expr::SafeType(x[0]);
    for (int i=1; i<N; i++) {
        lct = typeconv::getLeastCommonType(lct, expr::SafeType(x[i]));
    }
    if (!lct) return -1;
    if (!type::matches(lct->getBaseType(), "bool")) return -1;
    if (lct->isASet()) return -1;

    int d = 0;
    for (int i=0; i<N; i++) {
        int dx = typeconv::getPromoteDistance(
                    x[i] ? x[i]->Type() : type::null,
                    lct
                );
        if (dx<0) return -1;
        d += dx;
    }
    return d;
}

inline const type* AlignBooleans(const location &W, expr** x, bool* f, int N)
{
    DCASSERT(x);

    // check flips, if any
    if (f) for (int i=0; i<N; i++) if (f[i])  return nullptr;

    const type* lct = expr::SafeType(x[0]);
    for (int i=1; i<N; i++) {
        lct = typeconv::getLeastCommonType(lct, expr::SafeType(x[i]));
    }
    if (  (0==lct) || (!type::matches(lct->getBaseType(), "bool"))
                   || lct->isASet() )
    {
        for (int i=0; i<N; i++)  Delete(x[i]);
        return nullptr;
    }
    for (int i=0; i<N; i++) {
        x[i] = typeconv::castExpr(true, W, lct, x[i]);
        DCASSERT(!bogus_expr::orNull(x[i]));
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
  bool_not_expr(const location &W, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                     bool_not_expr  methods                     *
// ******************************************************************

bool_not_expr::bool_not_expr(const location &W, expr *x)
 : negop(W, unary_op::uop_not, x->Type(), x)
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
  return new bool_not_expr(Where(), x);
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
  virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************
// *                      bool_not_op  methods                      *
// ******************************************************************

bool_not_op::bool_not_op() : unary_op(unary_op::uop_not)
{
}

const type* bool_not_op::getExprType(const type* t) const
{
  if (0==t)    return 0;
  if (t->isASet())  return 0;
  const type* bt = t->getBaseType();
  if (!type::matches(bt, "bool"))  return 0;
  return t;
}

unary* bool_not_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new bool_not_expr(W, x);
}

// ******************************************************************
// *                                                                *
// *                         bool_or  class                         *
// *                                                                *
// ******************************************************************

/// Or of boolean expressions.
class bool_or : public summation {
public:
  bool_or(const location &W, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                        bool_or  methods                        *
// ******************************************************************

bool_or::bool_or(const location &W, const type* t, expr **x, int n)
 : summation(W, assoc_op::aop_or, t, x, 0, n)
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
  return new bool_or(Where(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                      bool_assoc_op  class                      *
// *                                                                *
// ******************************************************************

class bool_assoc_op : public assoc_op {
public:
  bool_assoc_op(assoc_op::opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                     bool_assoc_op  methods                     *
// ******************************************************************

bool_assoc_op::bool_assoc_op(assoc_op::opcode op) : assoc_op(op)
{
}

int bool_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  return BoolAlignDistance(list, flip, N);
}

int bool_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  if (f) return -1;
  return BoolAlignDistance(lt, rt);
}

const type* bool_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  if (f) return 0;
  return BoolResultType(l, r);
}


// ******************************************************************
// *                                                                *
// *                        bool_or_op class                        *
// *                                                                *
// ******************************************************************

class bool_or_op : public bool_assoc_op {
public:
  bool_or_op();
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                       bool_or_op methods                       *
// ******************************************************************

bool_or_op::bool_or_op() : bool_assoc_op(assoc_op::aop_or)
{
}

assoc* bool_or_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignBooleans(W, list, flip, N);
  delete[] flip;
  if (lct)  return new bool_or(W, lct, list, N);
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
  bool_and(const location &W, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                        bool_and methods                        *
// ******************************************************************

bool_and::bool_and(const location &W, const type* t, expr **x, int n)
 : product(W, assoc_op::aop_and, t, x, 0, n)
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
  return new bool_and(Where(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                       bool_and_op  class                       *
// *                                                                *
// ******************************************************************

class bool_and_op : public bool_assoc_op {
public:
  bool_and_op();
  virtual assoc* makeExpr(const location &W, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                      bool_and_op  methods                      *
// ******************************************************************

bool_and_op::bool_and_op() : bool_assoc_op(assoc_op::aop_and)
{
}

assoc* bool_and_op::makeExpr(const location &W, expr** list,
        bool* flip, int N) const
{
  const type* lct = AlignBooleans(W, list, flip, N);
  delete[] flip;
  if (lct)  return new bool_and(W, lct, list, N);
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
  bool_implies(const location &W, const type* t, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr* l, expr* r) const;
};

// ******************************************************************
// *                      bool_implies methods                      *
// ******************************************************************

bool_implies::bool_implies(const location &W, const type* t, expr *l, expr* r)
 : binary(W, binary_op::bop_implies, t, l, r)
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
  return new bool_implies(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      bool_binary_op class                      *
// *                                                                *
// ******************************************************************

class bool_binary_op : public binary_op {
public:
  bool_binary_op(binary_op::opcode op);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* l, const type* r) const;
};

// ******************************************************************
// *                     bool_binary_op methods                     *
// ******************************************************************

bool_binary_op::bool_binary_op(binary_op::opcode op) : binary_op(op)
{
}

int bool_binary_op::getPromoteDistance(const type* lt, const type* rt) const
{
  return BoolAlignDistance(lt, rt);
}

const type* bool_binary_op::getExprType(const type* l, const type* r) const
{
  return BoolResultType(l, r);
}

// ******************************************************************
// *                                                                *
// *                     bool_implies_op  class                     *
// *                                                                *
// ******************************************************************

class bool_implies_op : public bool_binary_op {
public:
  bool_implies_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                    bool_implies_op  methods                    *
// ******************************************************************

bool_implies_op::bool_implies_op() : bool_binary_op(binary_op::bop_implies)
{
}

binary* bool_implies_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(W, l, r);
  if (0==lct)  return 0;
  return new bool_implies(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                        bool_equal class                        *
// *                                                                *
// ******************************************************************

/// Check equality of two boolean expressions.
class bool_equal : public eqop {
public:
  bool_equal(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr* r) const;
};

// ******************************************************************
// *                       bool_equal methods                       *
// ******************************************************************

bool_equal::bool_equal(const location &W, const type* t, expr *l, expr *r)
 : eqop(W, t, l, r)
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
  return new bool_equal(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                      bool_equal_op  class                      *
// *                                                                *
// ******************************************************************

class bool_equal_op : public bool_binary_op {
public:
  bool_equal_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                     bool_equal_op  methods                     *
// ******************************************************************

bool_equal_op::bool_equal_op() : bool_binary_op(binary_op::bop_equals)
{
}

binary* bool_equal_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(W, l, r);
  if (0==lct)  return 0;
  return new bool_equal(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         bool_neq class                         *
// *                                                                *
// ******************************************************************

/// Check inequality of two boolean expressions.
class bool_neq : public neqop {
public:
  bool_neq(const location &W, const type* t, expr *l, expr *r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *l, expr *r) const;
};

// ******************************************************************
// *                        bool_neq methods                        *
// ******************************************************************

bool_neq::bool_neq(const location &W, const type* t, expr *l, expr *r)
 : neqop(W, t, l, r)
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
  return new bool_neq(Where(), Type(), l, r);
}

// ******************************************************************
// *                                                                *
// *                       bool_neq_op  class                       *
// *                                                                *
// ******************************************************************

class bool_neq_op : public bool_binary_op {
public:
  bool_neq_op();
  virtual binary* makeExpr(const location &W, expr* l, expr* r) const;
};

// ******************************************************************
// *                      bool_neq_op  methods                      *
// ******************************************************************

bool_neq_op::bool_neq_op() : bool_binary_op(binary_op::bop_nequal)
{
}

binary* bool_neq_op::makeExpr(const location &W, expr* l, expr* r) const
{
  const type* lct = AlignBooleans(W, l, r);
  if (0==lct)  return 0;
  return new bool_neq(W, lct, l, r);
}

// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class ops_bool_init : public initializer {
    public:
        ops_bool_init();
    protected:
        virtual void execute();
};
static ops_bool_init the_ops_bool_initializer;

ops_bool_init::ops_bool_init() : initializer(__FILE__, 1)
{
    builds_resource("ops_bool");
}

void ops_bool_init::execute()
{
    // The constructors will register these operations
    new bool_not_op;
    new bool_or_op;
    new bool_and_op;
    new bool_implies_op;
    new bool_equal_op;
    new bool_neq_op;
}


