
#include "ops_temporal.h"
#include "unary.h"
#include "sets.h"
#include "../Streams/streams.h"
#include <stdlib.h>
#include "exprman.h"
#include "assoc.h"
#include "functions.h"
#include "mod_def.h"

/// State temporal operator.
class state_temporal_op : public unary_op {
public:
  state_temporal_op(exprman::unary_opcode o);
  virtual const type* getExprType(const type* t) const;
};

state_temporal_op::state_temporal_op(exprman::unary_opcode o)
 : unary_op(o)
{
}

const type* state_temporal_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->PATHFORMULA);
  if (0==t)    return 0;
  const type* bt = t->getBaseType();
  if (bt != em->PATHFORMULA)  return 0;
  return Phase2Rand(t);
}

/// Path temporal operator.
class path_temporal_op : public unary_op {
public:
  path_temporal_op(exprman::unary_opcode o);
  virtual const type* getExprType(const type* t) const;
};

path_temporal_op::path_temporal_op(exprman::unary_opcode o)
 : unary_op(o)
{
}

const type* path_temporal_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->BOOL);
  DCASSERT(em->PATHFORMULA);
  DCASSERT(em->STATEFORMULA);
  if (0==t)    return 0;
  const type* bt = t->getBaseType();
  if (bt != em->BOOL && bt != em->PATHFORMULA && bt != em->STATEFORMULA)  return 0;
  return Phase2Rand(t);
}

/// FORALL(A) temporal operator.
class forall_op : public state_temporal_op {
  /// FORALL(A) temporal expression.
  class expression : public unary_temporal_expr {
  public:
    expression(const char* fn, int line, expr *x);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *x) const {
      return new expression(Filename(), Linenumber(), x);
    }
  };

public:
  forall_op();
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                       forall_op  methods                       *
// ******************************************************************

forall_op::forall_op()
 : state_temporal_op(exprman::uop_forall)
{
}

unary* forall_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new expression(fn, ln, x);
}

forall_op::expression::expression(const char* fn, int line, expr *x)
 : unary_temporal_expr(fn, line, exprman::uop_forall, em->STATEFORMULA, x)
{
}

void forall_op::expression::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  // TODO: To be implemented
}

/// EXISTS(E) temporal expression.
class exists_op : public state_temporal_op {
  /// EXISTS(E) temporal expression.
  class expression : public unary_temporal_expr {
  public:
    expression(const char* fn, int line, expr *x);
    virtual void Traverse(traverse_data &x);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *x) const {
      return new expression(Filename(), Linenumber(), x);
    }
  };
public:
  exists_op();
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                       exists_op  methods                       *
// ******************************************************************

exists_op::exists_op()
 : state_temporal_op(exprman::uop_exists)
{
}

unary* exists_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new expression(fn, ln, x);
}

exists_op::expression::expression(const char* fn, int line, expr *x)
 : unary_temporal_expr(fn, line, exprman::uop_exists, em->STATEFORMULA, x)
{
}

void exists_op::expression::Traverse(traverse_data &x)
{
  if (traverse_data::Temporal!=x.which) {
    unary_temporal_expr::Traverse(x);
    return;
  }

  const expr* oldp = x.parent;
  x.parent = this;
  opnd->Traverse(x);
  x.parent = oldp;
}

void exists_op::expression::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  // TODO: To be implemented
}

/// NEXT(X) temporal operator.
class next_op : public path_temporal_op {
  /// NEXT(X) temporal expression.
  class expression : public unary_temporal_expr {
  public:
    expression(const char* fn, int line, expr *x);
    virtual void Traverse(traverse_data &x);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *x) const {
      return new expression(Filename(), Linenumber(), x);
    }

    virtual void TraverseOperand(traverse_data &x) const;

    inline expr* TraverseModel(traverse_data &x) const
    {
      traverse_data xx(traverse_data::Substitute);
      result ans;
      xx.answer = &ans;
      x.model->Traverse(xx);
      return dynamic_cast<expr*>(Share(xx.answer->getPtr()));
    }
  };
public:
  next_op();
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                       exists_op  methods                       *
// ******************************************************************

next_op::next_op()
 : path_temporal_op(exprman::uop_next)
{
}

unary* next_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new expression(fn, ln, x);
}

next_op::expression::expression(const char* fn, int line, expr *x)
 : unary_temporal_expr(fn, line, exprman::uop_next, em->PATHFORMULA, x)
{
}

void next_op::expression::Traverse(traverse_data &x)
{
  if (traverse_data::Temporal!=x.which) {
    unary_temporal_expr::Traverse(x);
    return;
  }

  const unary_temporal_expr* texpr = dynamic_cast<const unary_temporal_expr*>(x.parent);
  if (0==texpr) {
    // TODO: To be implemented
  }

  traverse_data xx(traverse_data::Temporal);
  xx.model = x.model;
  result ans;
  xx.answer = &ans;
  TraverseOperand(xx);

  // Construct arguments
  int np = 2;
  // XXX: Potential memory leakage
  expr** pass = new expr*[np];
  pass[0] = TraverseModel(x);
  pass[1] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));

  function* tfunc = nullptr;
  const type* model_type = x.model->Type();
  switch(texpr->GetOpCode()) {
  case exprman::unary_opcode::uop_forall:
    // AX
    tfunc = dynamic_cast<function*>(em->findFunction(model_type, "AX"));
    break;
  case exprman::unary_opcode::uop_exists:
    // EX
    tfunc = dynamic_cast<function*>(em->findFunction(model_type, "EX"));
    break;
  default:
    break;
  }

  const expr* oldp = x.parent;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = traverse_data::Temporal;
}

void next_op::expression::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  // TODO: To be implemented
}

void next_op::expression::TraverseOperand(traverse_data &x) const
{
  if (em->BOOL==opnd->Type()) {
    // Atomic

    // Construct arguments
    int np = 2;
    // XXX: Potential memory leakage
    expr** pass = new expr*[np];
    pass[0] = TraverseModel(x);
    pass[1] = dynamic_cast<expr*>(opnd);

    const type* model_type = x.model->Type();
    function* pot = dynamic_cast<function*>(em->findFunction(model_type, "potential"));
    traverse_data xx(traverse_data::Substitute);
    xx.parent = pot;
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    pot->Traverse(xx, pass, np);
    x.answer->setPtr(Share(xx.answer->getPtr()));
  }
  else {
    opnd->Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitTemporalOps(exprman* em)
{
  if (0==em)  return;

  em->registerOperation(  new forall_op    );
  em->registerOperation(  new exists_op    );
  em->registerOperation(  new next_op      );
}
