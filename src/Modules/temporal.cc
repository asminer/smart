
#include "temporal.h"

#include "../ExprLib/exprman.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/assoc.h"

// ******************************************************************

inline bool isAtomicType(const exprman* em, const type* t)
{
  if (0==t) {
    return false;
  }
  if (t->getBaseType() == em->BOOL) {
    // can be PROC BOOL or BOOL
    return t->getModifier() == DETERM;
  }
  return false;
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
    temporal_unary(const char* fn, int line, exprman::unary_opcode, 
      const type* t, expr* x);
    virtual bool Print(OutputStream &s, int) const;
    virtual void Traverse(traverse_data &x);
  private:
    exprman::unary_opcode opcode;
};

// ******************************************************************

temporal_unary::temporal_unary(const char* fn, int line, 
  exprman::unary_opcode op, const type* t, expr *x)
 : unary(fn, line, t, x)
{
  opcode = op;
}

bool temporal_unary::Print(OutputStream &s, int) const
{
  s << em->getOp(opcode);
  DCASSERT(opnd);
  opnd->Print(s, 0);
  return true;
}

void temporal_unary::Traverse(traverse_data &x)
{
  // TBD - anything common?

  // Fall through to parent class behavior
  unary::Traverse(x);
}


// ******************************************************************
// *                                                                *
// *                   temporal_A class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_A : public temporal_unary {
  public:
    temporal_A(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_A::temporal_A(const char* fn, int line, const type* t, expr *x)
 : temporal_unary(fn, line, exprman::uop_forall, t, x)
{
}

expr* temporal_A::buildAnother(expr *x) const
{
  return new temporal_A(Filename(), Linenumber(), Type(), x);
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
    temporal_E(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_E::temporal_E(const char* fn, int line, const type* t, expr *x)
 : temporal_unary(fn, line, exprman::uop_exists, t, x)
{
}

expr* temporal_E::buildAnother(expr *x) const
{
  return new temporal_E(Filename(), Linenumber(), Type(), x);
}

void temporal_E::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  temporal_unary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                   temporal_F class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_F : public temporal_unary {
  public:
    temporal_F(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_F::temporal_F(const char* fn, int line, const type* t, expr *x)
 : temporal_unary(fn, line, exprman::uop_future, t, x)
{
}

expr* temporal_F::buildAnother(expr *x) const
{
  return new temporal_F(Filename(), Linenumber(), Type(), x);
}

void temporal_F::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  temporal_unary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                   temporal_G class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_G : public temporal_unary {
  public:
    temporal_G(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_G::temporal_G(const char* fn, int line, const type* t, expr *x)
 : temporal_unary(fn, line, exprman::uop_globally, t, x)
{
}

expr* temporal_G::buildAnother(expr *x) const
{
  return new temporal_G(Filename(), Linenumber(), Type(), x);
}

void temporal_G::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  temporal_unary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                   temporal_X class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_X : public temporal_unary {
  public:
    temporal_X(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_X::temporal_X(const char* fn, int line, const type* t, expr *x)
 : temporal_unary(fn, line, exprman::uop_next, t, x)
{
}

expr* temporal_X::buildAnother(expr *x) const
{
  return new temporal_X(Filename(), Linenumber(), Type(), x);
}

void temporal_X::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  temporal_unary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                   temporal_U class & methods                   *
// *                                                                *
// ******************************************************************

class temporal_U : public binary {
  public:
    temporal_U(const char* fn, int line, const type* t, expr* l, expr* r);
    virtual void Traverse(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr *nl, expr *nr) const;
};

// ******************************************************************

temporal_U::temporal_U(const char* fn, int line, const type* t, expr *l, expr *r)
 : binary(fn, line, exprman::bop_until, t, l, r)
{
}

expr* temporal_U::buildAnother(expr *l, expr *r) const
{
  return new temporal_U(Filename(), Linenumber(), Type(), l, r);
}

void temporal_U::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  binary::Traverse(x);
}

// ******************************************************************
// *                                                                *
// *                  temporal_neg class & methods                  *
// *                                                                *
// ******************************************************************

class temporal_neg : public negop {
  public:
    temporal_neg(const char* fn, int line, const type* t, expr* x);
  protected:
    virtual expr* buildAnother(expr *x) const;
    virtual void Traverse(traverse_data &x);
};

// ******************************************************************

temporal_neg::temporal_neg(const char* fn, int line, const type* t, expr *x)
 : negop(fn, line, exprman::uop_neg, t, x)
{
}

expr* temporal_neg::buildAnother(expr *x) const
{
  return new temporal_neg(Filename(), Linenumber(), Type(), x);
}

void temporal_neg::Traverse(traverse_data &x)
{
  // TBD - our CTL/LTL traversals here

  // Fall through to parent class behavior
  opnd->Traverse(x);
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
    temporal_quantifier_op(exprman::unary_opcode op);
    virtual const type* getExprType(const type* t) const;
    virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************

temporal_quantifier_op::temporal_quantifier_op(exprman::unary_opcode op)
 : unary_op(op)
{
  if ((op != exprman::uop_forall) && (op != exprman::uop_exists)) {
    em->startInternal(__FILE__, __LINE__);
    em->noCause();
    em->cerr() << "Bad operator " << em->getOp(op) << " in temporal_quantifier_op";
    em->stopIO();
  }
}

const type* temporal_quantifier_op::getExprType(const type* t) const
{
  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->quantify();
}

unary* temporal_quantifier_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  switch (getOpcode()) {
    case exprman::uop_forall:
        return new temporal_A(fn, ln, t, x);

    case exprman::uop_exists:
        return new temporal_E(fn, ln, t, x);

    default:
        em->startInternal(__FILE__, __LINE__);
        em->noCause();
        em->cerr() << "Bad operator " << em->getOp(getOpcode()) << " in temporal_quantifier_op";
        em->stopIO();
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
    temporal_unarypath_op(exprman::unary_opcode op);
    virtual const type* getExprType(const type* t) const;
    virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************

temporal_unarypath_op::temporal_unarypath_op(exprman::unary_opcode op)
 : unary_op(op)
{
  if ((op != exprman::uop_future) && (op != exprman::uop_globally) && (op != exprman::uop_next)) {
    em->startInternal(__FILE__, __LINE__);
    em->noCause();
    em->cerr() << "Bad operator " << em->getOp(op) << " in temporal_unarypath_op";
    em->stopIO();
  }
}

const type* temporal_unarypath_op::getExprType(const type* t) const
{
  if (isAtomicType(em, t)) {
    return temporal_types::t_single_pathop;
  }

  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->pathify();
}

unary* temporal_unarypath_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  switch (getOpcode()) {
    case exprman::uop_future:
        return new temporal_F(fn, ln, t, x);

    case exprman::uop_globally:
        return new temporal_G(fn, ln, t, x);

    case exprman::uop_next:
        return new temporal_X(fn, ln, t, x);

    default:
        em->startInternal(__FILE__, __LINE__);
        em->noCause();
        em->cerr() << "Bad operator " << em->getOp(getOpcode()) << " in temporal_unarypath_op";
        em->stopIO();
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
    temporal_binarypath_op(exprman::binary_opcode op);
    virtual int getPromoteDistance(const type* lt, const type* rt) const;
    virtual const type* getExprType(const type* lt, const type* rt) const;
    virtual binary* makeExpr(const char* fn, int ln, expr* left, expr* right) const;
  private:
    inline bool isValidOperandType(const type* t) const {
      if (isAtomicType(em, t))  return true;
      return dynamic_cast <const temporal_type*> (t);
    }
};

// ******************************************************************

temporal_binarypath_op::temporal_binarypath_op(exprman::binary_opcode op)
 : binary_op(op)
{
  // TBD - should we add release, weak until?
  if (op != exprman::bop_until) {
    em->startInternal(__FILE__, __LINE__);
    em->noCause();
    em->cerr() << "Bad operator " << em->getOp(op) << " in temporal_binarypath_op";
    em->stopIO();
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
  if (isAtomicType(em, lt)) {
    ltt = static_cast <const temporal_type*> (temporal_types::t_single_pathop);
  } else {
    const temporal_type* tt = dynamic_cast <const temporal_type*> (lt);
    if (tt) ltt = tt->pathify();
  }

  const temporal_type* rtt = 0;
  if (isAtomicType(em, rt)) {
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

binary* temporal_binarypath_op::makeExpr(const char* fn, int ln, expr* left, 
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
    case exprman::bop_until:
        return new temporal_U(fn, ln, t, left, right);

    // Release would go here

    // Weak until would go here

    default:
        em->startInternal(__FILE__, __LINE__);
        em->noCause();
        em->cerr() << "Bad operator " << em->getOp(getOpcode()) << " in temporal_binarypath_op";
        em->stopIO();
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
    virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************

temporal_neg_op::temporal_neg_op() : unary_op(exprman::uop_neg)
{
}

const type* temporal_neg_op::getExprType(const type* t) const
{
  const temporal_type* tt = dynamic_cast <const temporal_type*> (t);
  if (0==tt) return 0;

  return tt->logicify();
}


unary* temporal_neg_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  const type* t = getExprType(x->Type());
  if (0==t) {
    Delete(x);
    return 0;
  }
  return new temporal_neg(fn, ln, t, x);
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
    virtual bool execute();
};
init_temporal the_temporal_initializer;


init_temporal::init_temporal() : initializer("init_temporal")
{
  usesResource("em");
  usesResource("st");
  buildsResource("temporal");
  buildsResource("types");
}

bool init_temporal::execute()
{
  if (0==em)  return false;

  using namespace temporal_types;

  // ******************************************************************
  // Types first!
  // ******************************************************************

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

  em->registerType(t_single_pathop);
  em->registerType(t_ctl_pathform);
  em->registerType(t_ctl_stateform);
  em->registerType(t_ltl_pathform);
  em->registerType(t_ltl_topform);
  em->registerType(t_ctlstar_pathform);
  em->registerType(t_ctlstar_stateform);

  // ******************************************************************
  // Operations  
  // ******************************************************************

  em->registerOperation(  new temporal_quantifier_op(exprman::uop_forall)   );
  em->registerOperation(  new temporal_quantifier_op(exprman::uop_exists)   );

  em->registerOperation(  new temporal_unarypath_op(exprman::uop_future)    );
  em->registerOperation(  new temporal_unarypath_op(exprman::uop_globally)  );
  em->registerOperation(  new temporal_unarypath_op(exprman::uop_next)      );

  em->registerOperation(  new temporal_binarypath_op(exprman::bop_until)    );

  // Logic

  em->registerOperation(  new temporal_neg_op   );
  // TBD: implies

  // TBD: and
  // TBD: or


  return true;
}
