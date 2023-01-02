
#include "temporal.h"

#include "../ExprLib/binary.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/assoc.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/mod_def.h"

#include "../Utils/initializer.h"

// ******************************************************************

inline bool isAtomicType(const type* t)
{
  if (0==t) {
    return false;
  }
  if (type::matches(t->getBaseType(), "bool")) {
    // can be PROC BOOL or BOOL
    return t->getModifier() == DETERM;
  }
  return false;
}

inline const formalism* getModelType(const traverse_data &x)
{
    return smart_cast <const formalism*> (x.model->Type());
}

// ******************************************************************
// *                                                                *
// *                     temporal_type  methods                     *
// *                                                                *
// ******************************************************************

temporal_type::temporal_type(bool pf, const char* name, const char* short_doc,
  const char* long_doc) : simple_type(name, short_doc, long_doc)
{
  is_path_formula = pf;
  qtype = 0;
  ptype = 0;
  ltype = this;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      temporal expressions                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                 temporal_unary class & methods                 *
// *                                                                *
// ******************************************************************

class temporal_unary : public unary {
  public:
    temporal_unary(const location &W, unary_op::opcode,
      const type* t, expr* x);
    virtual void Traverse(traverse_data &x);

  protected:
    expr* TraverseModel(traverse_data &x) const;
    void TraverseOperand(traverse_data &x) const;

  private:
    unary_op::opcode opcode;
};

// ******************************************************************

temporal_unary::temporal_unary(const location &W,
  unary_op::opcode op, const type* t, expr *x)
 : unary(W, op, t, x)
{
  opcode = op;
}

void temporal_unary::Traverse(traverse_data &x)
{
  // TBD - anything common?

  // Fall through to parent class behavior
  unary::Traverse(x);
}

expr* temporal_unary::TraverseModel(traverse_data &x) const
{
  traverse_data xx(traverse_data::Substitute);
  result ans;
  xx.answer = &ans;
  x.model->Traverse(xx);
  return dynamic_cast<expr*>(Share(xx.answer->getPtr()));
}

void temporal_unary::TraverseOperand(traverse_data &x) const
{
  if (isAtomicType(opnd->Type())) {
    // Atomic

    // Construct arguments
    int np = 2;
    // XXX: Potential memory leakage
    expr** pass = new expr*[np];
    pass[0] = TraverseModel(x);
    pass[1] = opnd;

    const formalism* model_type = getModelType(x);
    function* pot = dynamic_cast<function*>(
            model_type->findFunction("potential")
    );
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
// *                   temporal_A class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_A : public temporal_unary {
  public:
    temporal_A(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_A::temporal_A(const location &W, const type* t, expr *x)
 : temporal_unary(W, unary_op::uop_forall, t, x)
{
}

expr* temporal_A::buildAnother(expr *x) const
{
  return new temporal_A(Where(), Type(), x);
}

void temporal_A::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  temporal_unary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                   temporal_E class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_E : public temporal_unary {
  public:
    temporal_E(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_E::temporal_E(const location &W, const type* t, expr *x)
 : temporal_unary(W, unary_op::uop_exists, t, x)
{
}

expr* temporal_E::buildAnother(expr *x) const
{
  return new temporal_E(Where(), Type(), x);
}

void temporal_E::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet != x.which && traverse_data::TemporalTrace != x.which) {
    temporal_unary::Traverse(x);
    return;
  }

  const expr* oldp = x.parent;
  x.parent = this;
  opnd->Traverse(x);
  x.parent = oldp;
}

// ******************************************************************
// *                                                                *
// *                   temporal_F class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_F : public temporal_unary {
  public:
    temporal_F(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_F::temporal_F(const location &W, const type* t, expr *x)
 : temporal_unary(W, unary_op::uop_future, t, x)
{
}

expr* temporal_F::buildAnother(expr *x) const
{
  return new temporal_F(Where(), Type(), x);
}

void temporal_F::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet != x.which && traverse_data::TemporalTrace != x.which) {
    temporal_unary::Traverse(x);
    return;
  }

  const temporal_unary* texpr = dynamic_cast<const temporal_unary*>(x.parent);
  if (nullptr == texpr) {
    // TODO: To be implemented
  }

  traverse_data xx(x.which);
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
  const formalism* model_type = getModelType(x);
  switch(texpr->GetOpCode()) {
  case unary_op::opcode::uop_forall:
    // AF
    tfunc = dynamic_cast<function*>(model_type->findFunction("AF"));
    break;
  case unary_op::opcode::uop_exists:
    // EF
    tfunc = (traverse_data::TemporalStateSet == x.which)
      ? dynamic_cast<function*>(model_type->findFunction("EF"))
      : dynamic_cast<function*>(model_type->findFunction("EF_trace"));
    break;
  default:
    break;
  }

  const expr* oldp = x.parent;
  traverse_data::traversal_type oldwhich = x.which;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = oldwhich;
}

// ******************************************************************
// *                                                                *
// *                   temporal_G class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_G : public temporal_unary {
  public:
    temporal_G(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_G::temporal_G(const location &W, const type* t, expr *x)
 : temporal_unary(W, unary_op::uop_globally, t, x)
{
}

expr* temporal_G::buildAnother(expr *x) const
{
  return new temporal_G(Where(), Type(), x);
}

void temporal_G::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet != x.which && traverse_data::TemporalTrace != x.which) {
    temporal_unary::Traverse(x);
    return;
  }

  const temporal_unary* texpr = dynamic_cast<const temporal_unary*>(x.parent);
  if (nullptr == texpr) {
    // TODO: To be implemented
  }

  traverse_data xx(x.which);
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
  const formalism* model_type = getModelType(x);
  switch(texpr->GetOpCode()) {
  case unary_op::opcode::uop_forall:
    // AG
    tfunc = dynamic_cast<function*>(model_type->findFunction("AG"));
    break;
  case unary_op::opcode::uop_exists:
    // EG
    tfunc = (traverse_data::TemporalStateSet == x.which)
      ? dynamic_cast<function*>(model_type->findFunction("EG"))
      : dynamic_cast<function*>(model_type->findFunction("EG_trace"));
    break;
  default:
    break;
  }

  const expr* oldp = x.parent;
  traverse_data::traversal_type oldwhich = x.which;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = oldwhich;
}

// ******************************************************************
// *                                                                *
// *                   temporal_X class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_X : public temporal_unary {
  public:
    temporal_X(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_X::temporal_X(const location &W, const type* t, expr *x)
 : temporal_unary(W, unary_op::uop_next, t, x)
{
}

expr* temporal_X::buildAnother(expr *x) const
{
  return new temporal_X(Where(), Type(), x);
}

void temporal_X::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet!=x.which && traverse_data::TemporalTrace!=x.which) {
    temporal_unary::Traverse(x);
    return;
  }

  const temporal_unary* texpr = dynamic_cast<const temporal_unary*>(x.parent);
  if (nullptr == texpr) {
    // TODO: To be implemented
  }

  traverse_data xx(x.which);
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
  const formalism* model_type = getModelType(x);
  switch(texpr->GetOpCode()) {
  case unary_op::opcode::uop_forall:
    // AX
    tfunc = dynamic_cast<function*>(model_type->findFunction("AX"));
    break;
  case unary_op::opcode::uop_exists:
    // EX
    tfunc = (traverse_data::TemporalStateSet == x.which)
      ? dynamic_cast<function*>(model_type->findFunction("EX"))
      : dynamic_cast<function*>(model_type->findFunction("EX_trace"));
    break;
  default:
    break;
  }

  const expr* oldp = x.parent;
  traverse_data::traversal_type oldwhich = x.which;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = oldwhich;
}

// ******************************************************************
// *                                                                *
// *                   temporal_U class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_U : public binary {
  public:
    temporal_U(const location &W, const type* t, expr* l, expr* r);
    virtual bool Print(std::ostream &s, int) const;
    virtual void Traverse(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *nl, expr *nr) const;

    expr* TraverseModel(traverse_data &x) const;
    void TraverseOperand(traverse_data &x, expr* p) const;
};

// ******************************************************************

temporal_U::temporal_U(const location &W, const type* t, expr *l, expr *r)
 : binary(W, binary_op::bop_until, t, l, r)
{
}

bool temporal_U::Print(std::ostream &s, int) const
{
  s << "(";
  DCASSERT(left);
  left->Print(s, 0);
  s << ")U(";
  DCASSERT(right);
  right->Print(s, 0);
  s << ")";
  return true;
}

expr* temporal_U::buildAnother(expr *l, expr *r) const
{
  return new temporal_U(Where(), Type(), l, r);
}

void temporal_U::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet != x.which && traverse_data::TemporalTrace != x.which) {
    binary::Traverse(x);
    return;
  }

  const temporal_unary* texpr = dynamic_cast<const temporal_unary*>(x.parent);
  if (nullptr == texpr) {
    // TODO: To be implemented
  }

  // Construct arguments
  int np = 3;
  // XXX: Potential memory leakage
  expr** pass = new expr*[np];
  pass[0] = TraverseModel(x);
  {
    traverse_data xx(x.which);
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    TraverseOperand(xx, left);
    pass[1] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));
  }
  {
    traverse_data xx(x.which);
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    TraverseOperand(xx, right);
    pass[2] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));
  }

  function* tfunc = nullptr;
  const formalism* model_type = getModelType(x);
  switch(texpr->GetOpCode()) {
  case unary_op::opcode::uop_forall:
    // AU
    tfunc = dynamic_cast<function*>(model_type->findFunction("AU"));
    break;
  case unary_op::opcode::uop_exists:
    // EU
    tfunc = (traverse_data::TemporalStateSet == x.which)
      ? dynamic_cast<function*>(model_type->findFunction("EU"))
      : dynamic_cast<function*>(model_type->findFunction("EU_trace"));
    break;
  default:
    break;
  }

  const expr* oldp = x.parent;
  traverse_data::traversal_type oldwhich = x.which;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = oldwhich;
}

expr* temporal_U::TraverseModel(traverse_data &x) const
{
  traverse_data xx(traverse_data::Substitute);
  result ans;
  xx.answer = &ans;
  x.model->Traverse(xx);
  return dynamic_cast<expr*>(Share(xx.answer->getPtr()));
}

void temporal_U::TraverseOperand(traverse_data &x, expr* p) const
{
  if (isAtomicType(p->Type())) {
    // Atomic

    // Construct arguments
    int np = 2;
    // XXX: Potential memory leakage
    expr** pass = new expr*[np];
    pass[0] = TraverseModel(x);
    pass[1] = p;

    const formalism* model_type = getModelType(x);
    function* pot = dynamic_cast<function*>(model_type->findFunction("potential"));
    traverse_data xx(traverse_data::Substitute);
    xx.parent = pot;
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    pot->Traverse(xx, pass, np);
    x.answer->setPtr(Share(xx.answer->getPtr()));
  }
  else {
    p->Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                  temporal_neg class & methods                  *
// *                                                                *
// ******************************************************************

class temporal_neg : public negop {
  public:
    temporal_neg(const location &W, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_neg::temporal_neg(const location &W, const type* t, expr *x)
 : negop(W, unary_op::uop_neg, t, x)
{
}

expr* temporal_neg::buildAnother(expr *x) const
{
  return new temporal_neg(Where(), Type(), x);
}

void temporal_neg::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  opnd->Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                  temporal_and class & methods                  *
// *                                                                *
// ******************************************************************

class temporal_and : public binary {
  public:
    temporal_and(const location &W, const type* t, expr* l, expr* r);
    virtual void Traverse(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *nl, expr *nr) const;

    expr* TraverseModel(traverse_data &x) const;
    void TraverseOperand(traverse_data &x, expr* p) const;
};

// ******************************************************************

temporal_and::temporal_and(const location &W, const type* t,
  expr* l, expr *r) : binary(W, binary_op::bop_and, t, l, r)
{
}

expr* temporal_and::buildAnother(expr *l, expr *r) const
{
  return new temporal_and(Where(), Type(), l, r);
}

void temporal_and::Traverse(traverse_data &x)
{
  if (traverse_data::TemporalStateSet != x.which && traverse_data::TemporalTrace != x.which) {
    binary::Traverse(x);
    return;
  }

  if (traverse_data::TemporalStateSet == x.which) {
    if (!isAtomicType(left->Type())) {
      left->Traverse(x);
    }
    if (!isAtomicType(right->Type())) {
      right->Traverse(x);
    }
    return;
  }

  // Construct arguments
  int np = 3;
  // XXX: Potential memory leakage
  expr** pass = new expr*[np];
  pass[0] = TraverseModel(x);
  {
    traverse_data xx(x.which);
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    TraverseOperand(xx, left);
    pass[1] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));
  }
  {
    traverse_data xx(x.which);
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    TraverseOperand(xx, right);
    pass[2] = dynamic_cast<expr*>(Share(xx.answer->getPtr()));
  }

  const formalism* model_type = getModelType(x);
  function* tfunc = dynamic_cast<function*>(model_type->findFunction("And_trace"));

  const expr* oldp = x.parent;
  traverse_data::traversal_type oldwhich = x.which;
  x.parent = tfunc;
  x.which = traverse_data::Substitute;
  tfunc->Traverse(x, pass, np);
  x.parent = oldp;
  x.which = oldwhich;
}

expr* temporal_and::TraverseModel(traverse_data &x) const
{
  traverse_data xx(traverse_data::Substitute);
  result ans;
  xx.answer = &ans;
  x.model->Traverse(xx);
  return dynamic_cast<expr*>(Share(xx.answer->getPtr()));
}

void temporal_and::TraverseOperand(traverse_data &x, expr* p) const
{
  if (isAtomicType(p->Type())) {
    // Atomic

    // Construct arguments
    int np = 2;
    // XXX: Potential memory leakage
    expr** pass = new expr*[np];
    pass[0] = TraverseModel(x);
    pass[1] = p;

    const formalism* model_type = getModelType(x);
    function* pot = dynamic_cast<function*>(model_type->findFunction("potential"));
    traverse_data xx(traverse_data::Substitute);
    xx.parent = pot;
    xx.model = x.model;
    result ans;
    xx.answer = &ans;
    pot->Traverse(xx, pass, np);
    x.answer->setPtr(Share(xx.answer->getPtr()));
  }
  else {
    p->Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                temporal_implies class & methods                *
// *                                                                *
// ******************************************************************

class temporal_implies : public binary {
  public:
    temporal_implies(const location &W, const type* t, expr* l, expr* r);
    virtual void Traverse(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *nl, expr *nr) const;
};

// ******************************************************************

temporal_implies::temporal_implies(const location &W, const type* t,
  expr* l, expr *r) : binary(W, binary_op::bop_implies, t, l, r)
{
}

expr* temporal_implies::buildAnother(expr *l, expr *r) const
{
  return new temporal_implies(Where(), Type(), l, r);
}

void temporal_implies::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  binary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      temporal  operations                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// Used by the expression manager to build expressions

// ******************************************************************
// *                                                                *
// *             temporal_quantifier_op class & methods             *
// *                                                                *
// ******************************************************************

class temporal_quantifier_op : public unary_op {
  public:
    temporal_quantifier_op(unary_op::opcode op);
    virtual const type* getExprType(const type* t) const;
    virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************

temporal_quantifier_op::temporal_quantifier_op(unary_op::opcode op)
 : unary_op(op)
{
  if ((op != unary_op::uop_forall) && (op != unary_op::uop_exists)) {
    internal_error E(__FILE__, __LINE__);
    E << "Bad operator " << getOp(op) << " in temporal_quantifier_op";
  }
}

const type* temporal_quantifier_op::getExprType(const type* t) const
{
  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->quantify();
}

unary* temporal_quantifier_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  switch (getOpcode()) {
    case unary_op::uop_forall:
        return new temporal_A(W, t, x);

    case unary_op::uop_exists:
        return new temporal_E(W, t, x);

    default:
    {
        internal_error E(__FILE__, __LINE__, W);
        E << "Bad operator " << getOp(getOpcode()) << " in temporal_quantifier_op";
    }
  }
  // shouldn't get here
  return 0;
}




// ******************************************************************
// *                                                                *
// *             temporal_unarypath_op  class & methods             *
// *                                                                *
// ******************************************************************

class temporal_unarypath_op : public unary_op {
  public:
    temporal_unarypath_op(unary_op::opcode op);
    virtual const type* getExprType(const type* t) const;
    virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************

temporal_unarypath_op::temporal_unarypath_op(unary_op::opcode op)
 : unary_op(op)
{
  if ((op != unary_op::uop_future) && (op != unary_op::uop_globally) && (op != unary_op::uop_next)) {
    internal_error E(__FILE__, __LINE__);
    E << "Bad operator " << getOp(op) << " in temporal_unarypath_op";
  }
}

const type* temporal_unarypath_op::getExprType(const type* t) const
{
  if (isAtomicType(t)) {
    return temporal_types::t_single_pathop;
  }

  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->pathify();
}

unary* temporal_unarypath_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  switch (getOpcode()) {
    case unary_op::uop_future:
        return new temporal_F(W, t, x);

    case unary_op::uop_globally:
        return new temporal_G(W, t, x);

    case unary_op::uop_next:
        return new temporal_X(W, t, x);

    default:
    {
        internal_error E(__FILE__, __LINE__, W);
        E << "Bad operator " << getOp(getOpcode()) << " in temporal_unarypath_op";
    }
  }
  // shouldn't get here
  return 0;
}



// ******************************************************************
// *                                                                *
// *             temporal_binarypath_op class & methods             *
// *                                                                *
// ******************************************************************

class temporal_binarypath_op : public binary_op {
  public:
    temporal_binarypath_op(binary_op::opcode op);
    virtual int getPromoteDistance(const type* lt, const type* rt) const;
    virtual const type* getExprType(const type* lt, const type* rt) const;
    virtual binary* makeExpr(const location &W, expr* left, expr* right) const;
  private:
    inline bool isValidOperandType(const type* t) const {
      if (isAtomicType(t))  return true;
      return dynamic_cast <const temporal_type*> (t);
    }
};

// ******************************************************************

temporal_binarypath_op::temporal_binarypath_op(binary_op::opcode op)
 : binary_op(op)
{
  // TBD - should we add release, weak until?
  if (op != binary_op::bop_until) {
    internal_error E(__FILE__, __LINE__);
    E << "Bad operator " << getOp(op) << " in temporal_binarypath_op";
  }
}

int temporal_binarypath_op::getPromoteDistance(const type* lt, const type* rt) const
{
  // We don't do promotions, so...

  if (!isValidOperandType(lt)) return -1;
  if (!isValidOperandType(rt)) return -1;
  return 0;
}

const type* temporal_binarypath_op::getExprType(const type* lt, const type* rt) const
{
  //
  // Pathify left and right types,
  //

  const temporal_type* ltt = 0;
  if (isAtomicType(lt)) {
    ltt = static_cast <const temporal_type*> (temporal_types::t_single_pathop);
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (lt);
    if (tt) ltt = tt->pathify();
  }

  const temporal_type* rtt = 0;
  if (isAtomicType(rt)) {
    rtt = static_cast <const temporal_type*> (temporal_types::t_single_pathop);
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (rt);
    if (tt) rtt = tt->pathify();
  }

  //
  // Determine the mixture of ltt and rtt
  //

  //
  // Something wasn't happy - operation doesn't make sense
  //

  if ((0==ltt) || (0==rtt)) return 0;

  DCASSERT(ltt->isPathFormula());
  DCASSERT(rtt->isPathFormula());

  //
  // If they're equal - return that
  //
  if (ltt == rtt) {
    return ltt;
  }

  //
  // If one is a single pathop, then return the other one
  //
  if (ltt == temporal_types::t_single_pathop) {
    return rtt;
  }
  if (rtt == temporal_types::t_single_pathop) {
    return ltt;
  }

  //
  // Remaining cases - mixture of ctl, ltl, or ctlstar path formulas.
  // Result for sure is a ctlstar path formula.
  //
  return temporal_types::t_ctlstar_pathform;
}

binary* temporal_binarypath_op::makeExpr(const location &W, expr* left,
  expr* right) const
{
  DCASSERT(left);
  DCASSERT(right);

  const type* t = getExprType(left->Type(), right->Type());
  if (0==t) {
    Delete(left);
    Delete(right);
    return 0;
  }

  switch (getOpcode()) {
    case binary_op::bop_until:
        return new temporal_U(W, t, left, right);

    // Release would go here

    // Weak until would go here

    default:
    {
        internal_error E(__FILE__, __LINE__, W);
        E << "Bad operator " << getOp(getOpcode()) << " in temporal_binarypath_op";
    }
  }
  // shouldn't get here
  return 0;
}



// ******************************************************************
// *                                                                *
// *                temporal_neg_op  class & methods                *
// *                                                                *
// ******************************************************************

class temporal_neg_op : public unary_op {
  public:
    temporal_neg_op();
    virtual const type* getExprType(const type* t) const;
    virtual unary* makeExpr(const location &W, expr* x) const;
};

// ******************************************************************

temporal_neg_op::temporal_neg_op() : unary_op(unary_op::uop_neg)
{
}

const type* temporal_neg_op::getExprType(const type* t) const
{
  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->logicify();
}


unary* temporal_neg_op::makeExpr(const location &W, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  return new temporal_neg(W, t, x);
}

// ******************************************************************
// *                                                                *
// *                temporal_and_op  class & methods                *
// *                                                                *
// ******************************************************************

class temporal_and_op : public binary_op {
  public:
    temporal_and_op();
    virtual int getPromoteDistance(const type* lt, const type* rt) const;
    virtual const type* getExprType(const type* lt, const type* rt) const;
    virtual binary* makeExpr(const location &W, expr* left, expr* right) const;
};

// ******************************************************************

temporal_and_op::temporal_and_op() : binary_op(binary_op::bop_and)
{
}

int temporal_and_op::getPromoteDistance(const type* lt, const type* rt) const
{
  if (getExprType(lt, rt)) return 0;
  return -1;
}

const type* temporal_and_op::getExprType(const type* lt, const type* rt) const
{
  //
  // Logicify left and right types,
  //

  const temporal_type* ltt = 0;
  bool left_atomic = false;
  if (isAtomicType(lt)) {
    left_atomic = true;
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (lt);
    if (tt) ltt = tt->logicify();
  }

  const temporal_type* rtt = 0;
  bool right_atomic = false;
  if (isAtomicType(rt)) {
    right_atomic = true;
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (rt);
    if (tt) rtt = tt->logicify();
  }

  //
  // Determine what happens when we mix left and right
  //

  if (left_atomic && right_atomic) {
    // There's already an operator for this, it's called boolean logic
    return 0;
  }

  if (left_atomic) {
    if (0==rtt) return 0;
    if (rtt->isStateFormula()) return rtt;
    if (rtt == temporal_types::t_ltl_pathform) {
      return rtt;
    } else {
      return temporal_types::t_ctlstar_pathform;
    }
  }

  if (right_atomic) {
    if (0==ltt) return 0;
    if (ltt->isStateFormula()) return ltt;
    if (ltt == temporal_types::t_ltl_pathform) {
      return ltt;
    } else {
      return temporal_types::t_ctlstar_pathform;
    }
  }

  // Done with atomic cases

  if (0==ltt) return 0;
  if (0==rtt) return 0;

  // We have two temporal logic formulas, make sure they're both path or state

  if (ltt->isPathFormula() != rtt->isPathFormula()) return 0;

  if (ltt == rtt) return ltt;

  //
  // Remaining cases - mixture of ctl, ltl, or ctlstar formulas.
  // Result for sure is a ctlstar formula.
  //

  if (ltt->isPathFormula())   return temporal_types::t_ctlstar_pathform;
  else                        return temporal_types::t_ctlstar_stateform;
}

binary* temporal_and_op::makeExpr(const location &W, expr* left,
  expr* right) const
{
  DCASSERT(left);
  DCASSERT(right);

  const type* t = getExprType(left->Type(), right->Type());
  if (0==t) {
    Delete(left);
    Delete(right);
    return 0;
  }

  return new temporal_and(W, t, left, right);
}

// ******************************************************************
// *                                                                *
// *              temporal_implies_op  class & methods              *
// *                                                                *
// ******************************************************************

class temporal_implies_op : public binary_op {
  public:
    temporal_implies_op();
    virtual int getPromoteDistance(const type* lt, const type* rt) const;
    virtual const type* getExprType(const type* lt, const type* rt) const;
    virtual binary* makeExpr(const location &W, expr* left, expr* right) const;
};

// ******************************************************************

temporal_implies_op::temporal_implies_op() : binary_op(binary_op::bop_implies)
{
}

int temporal_implies_op::getPromoteDistance(const type* lt, const type* rt) const
{
  if (getExprType(lt, rt)) return 0;
  return -1;
}

const type* temporal_implies_op::getExprType(const type* lt, const type* rt) const
{
  //
  // Logicify left and right types,
  //

  const temporal_type* ltt = 0;
  bool left_atomic = false;
  if (isAtomicType(lt)) {
    left_atomic = true;
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (lt);
    if (tt) ltt = tt->logicify();
  }

  const temporal_type* rtt = 0;
  bool right_atomic = false;
  if (isAtomicType(rt)) {
    right_atomic = true;
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (rt);
    if (tt) rtt = tt->logicify();
  }

  //
  // Determine what happens when we mix left and right
  //

  if (left_atomic && right_atomic) {
    // There's already an operator for this, it's called boolean logic
    return 0;
  }

  if (left_atomic) {
    if (0==rtt) return 0;
    if (rtt->isStateFormula()) return rtt;
    if (rtt == temporal_types::t_ltl_pathform) {
      return rtt;
    } else {
      return temporal_types::t_ctlstar_pathform;
    }
  }

  if (right_atomic) {
    if (0==ltt) return 0;
    if (ltt->isStateFormula()) return ltt;
    if (ltt == temporal_types::t_ltl_pathform) {
      return ltt;
    } else {
      return temporal_types::t_ctlstar_pathform;
    }
  }

  // Done with atomic cases

  if (0==ltt) return 0;
  if (0==rtt) return 0;

  // We have two temporal logic formulas, make sure they're both path or state

  if (ltt->isPathFormula() != rtt->isPathFormula()) return 0;

  if (ltt == rtt) return ltt;

  //
  // Remaining cases - mixture of ctl, ltl, or ctlstar formulas.
  // Result for sure is a ctlstar formula.
  //

  if (ltt->isPathFormula())   return temporal_types::t_ctlstar_pathform;
  else                        return temporal_types::t_ctlstar_stateform;
}

binary* temporal_implies_op::makeExpr(const location &W, expr* left,
  expr* right) const
{
  DCASSERT(left);
  DCASSERT(right);

  const type* t = getExprType(left->Type(), right->Type());
  if (0==t) {
    Delete(left);
    Delete(right);
    return 0;
  }

  return new temporal_implies(W, t, left, right);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_temporal : public initializer {
    public:
        init_temporal();
    protected:
        virtual void execute();
};
static init_temporal the_temporal_initializer;


init_temporal::init_temporal() : initializer(__FILE__, 1, 0)
{
  builds_resource(0, "temporal");
}

void init_temporal::execute()
{
  using namespace temporal_types;

  // ******************************************************************
  // Types first!
  // ******************************************************************

  t_temporal = new temporal_type(
    false, // abstract type that every temporal type can be converted into
    "temporal_formula",
    "Temporal formula",
    "Temporal formula"
  );

  t_single_pathop = new temporal_type(
    true, // path formula
    "tl_simple_path",
    "Temporal logic simple path formula",
    "Temporal logic special type, for formulas containing only a single path operator F, G, U, or X"
  );

  t_ctl_pathform = new temporal_type(
    true, // path formula
    "ctl_path_formula",
    "CTL path formula",
    "CTL path formula; subformulas may be CTL state formulas"
  );

  t_ctl_stateform = new temporal_type(
    false, // state formula
    "ctl_state_formula",
    "CTL state formula",
    "CTL state formula; could be a \"top level\" CTL formula"
  );

  t_ltl_pathform = new temporal_type(
    true, // path formula
    "ltl_path_formula",
    "LTL formula",
    "LTL formula with no path quantifier"
  );

  t_ltl_topform = new temporal_type(
    false, // state formula
    "ltl_top",
    "Quantified LTL formula",
    "LTL formula plus path quantifier; could be a \"top level\" LTL formula"
  );

  t_ctlstar_pathform = new temporal_type(
    true, // path formula
    "ctlstar_path_formula",
    "CTL* path formula",
    "CTL* path formula"
  );

  t_ctlstar_stateform = new temporal_type(
    false, // state formula
    "ctlstar_state_formula",
    "CTL* state formula",
    "CTL* state formula; could be a \"top level\" CTL* formula"
  );

  // ------------------------------------------------------------

  t_single_pathop->setQPtypes(t_ctl_stateform, t_ltl_pathform);
  t_ctl_pathform->setQPtypes(t_ctl_stateform, t_ctlstar_pathform);
  t_ctl_stateform->setQPtypes(0, t_ctl_pathform);
  t_ltl_pathform->setQPtypes(t_ltl_topform, t_ltl_pathform);
  t_ltl_topform->setQPtypes(0, t_ctlstar_pathform);
  t_ctlstar_pathform->setQPtypes(t_ctlstar_stateform, t_ctlstar_pathform);
  t_ctlstar_stateform->setQPtypes(0, t_ctlstar_pathform);

  // ------------------------------------------------------------

  t_single_pathop->setLogic(t_ltl_pathform);
  t_ctl_pathform->setLogic(t_ctlstar_pathform);

  // ------------------------------------------------------------

  type::registerNew(t_temporal);
  type::registerNew(t_single_pathop);
  type::registerNew(t_ctl_pathform);
  type::registerNew(t_ctl_stateform);
  type::registerNew(t_ltl_pathform);
  type::registerNew(t_ltl_topform);
  type::registerNew(t_ctlstar_pathform);
  type::registerNew(t_ctlstar_stateform);

  // ******************************************************************
  // Operations (automatically register)
  // ******************************************************************

  new temporal_quantifier_op(unary_op::uop_forall);
  new temporal_quantifier_op(unary_op::uop_exists);

  new temporal_unarypath_op(unary_op::uop_future);
  new temporal_unarypath_op(unary_op::uop_globally);
  new temporal_unarypath_op(unary_op::uop_next);

  new temporal_binarypath_op(binary_op::bop_until);

  // Logic

  new temporal_neg_op;
  new temporal_and_op;
  new temporal_implies_op;

  // TBD: and
  // TBD: or

}
