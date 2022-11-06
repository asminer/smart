
#include "../Options/optman.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"

#include "../Formlsms/graph_llm.h"


#include "statesets.h"


// ******************************************************************
// *                                                                *
// *                        stateset methods                        *
// *                                                                *
// ******************************************************************

exprman* stateset::em = 0;
bool stateset::print_indexes;

stateset::stateset(const state_lldsm* p) : shared_object()
{
  parent = p;
}

stateset::stateset(const stateset* clone) : shared_object()
{
  DCASSERT(clone);
  parent = clone->parent;
}

stateset::~stateset()
{
}

const hldsm* stateset::getGrandparent() const
{
  return parent ? parent->GetParent() : 0;
}

bool stateset::parentsMatch(const expr* c, const char* op, stateset* A, stateset* B)
{
  if (0==A || 0==B) return false;

  if (A->getParent() != B->getParent()) {
    if (em->startError()) {
      em->causedBy(c);
      em->cerr() << "Statesets in " << op << " are from different model instances";
      em->stopIO();
    }
    return false;
  }

  return true;
}

void stateset::storageMismatchError(const expr* c, const char* op)
{
  if (em->startError()) {
    em->causedBy(c);
    em->cerr() << "Statesets in " << op << " use incompatible storage types";
    em->stopIO();
  }
}

// ******************************************************************
// *                                                                *
// *                      stateset_type  class                      *
// *                                                                *
// ******************************************************************

class stateset_type : public simple_type {
public:
  stateset_type();
};

// ******************************************************************
// *                     stateset_type  methods                     *
// ******************************************************************

stateset_type::stateset_type() : simple_type("stateset", "Set of states", "Type used for sets of states, used for CTL model checking and other operations.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      stateset expressions                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                      stateset_not  class                       *
// *                                                                *
// ******************************************************************

/// Negation of a stateset expression.
class stateset_not : public negop {
public:
  stateset_not(const char* fn, int line, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                      stateset_not methods                      *
// ******************************************************************

stateset_not::stateset_not(const char* fn, int line, expr *x)
 : negop(fn, line, exprman::uop_not, x->Type(), x)
{
}

void stateset_not::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x);

  if (!x.answer->isNormal()) return;

  stateset* ss = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(ss);
  if (ss->numRefs()>1) {
    stateset* ans = ss->DeepCopy();
    ans->Complement();
    x.answer->setPtr(ans);
  } else {
    // in-place
    ss->Complement();
  }
}

expr* stateset_not::buildAnother(expr *x) const
{
  return new stateset_not(Filename(), Linenumber(), x);
}

// ******************************************************************
// *                                                                *
// *                      stateset_diff class                       *
// *                                                                *
// ******************************************************************

/// Difference of two statesets.
class stateset_diff : public binary {
public:
  stateset_diff(const char* fn, int line, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x, expr* y) const;
};

// ******************************************************************
// *                     stateset_diff methods                      *
// ******************************************************************

stateset_diff::stateset_diff(const char* fn, int line, expr *l, expr* r)
 : binary(fn, line, exprman::bop_diff, l->Type(), l, r)
{
}

void stateset_diff::Compute(traverse_data &x)
{
  //
  // L \ R   =  L * !R
  //

  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);

  SafeCompute(right, x);
  if (!x.answer->isNormal()) return;

  stateset* notR = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(notR);

  if (notR->numRefs() > 1) {
    notR = notR->DeepCopy();  // loses the original but x.answer still has it
    notR->Complement();
  } else {
    notR->Complement();
    notR = Share(notR);
  }

  //
  // We have !R.
  //
  SafeCompute(left, x); // Deletes old x.answer

  if (!x.answer->isNormal()) {
    Delete(notR);
    return;
  }

  stateset* L = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(L);

  //
  // We have L
  //

  if (stateset::parentsMatch(this, "difference", L, notR)) {
    bool ok;
    if (L->numRefs() > 1) {
      L = L->DeepCopy();
      ok = L->Intersect(this, "difference", notR);
    } else {
      ok = L->Intersect(this, "difference", notR);
      L = Share(L);
    }
    if (!ok) {
      // Intersection failed
      Delete(L);
      L = 0;
    }
  }
  Delete(notR);

  if (L) {
    x.answer->setPtr(L);
  } else {
    x.answer->setNull();
  }
}

expr* stateset_diff::buildAnother(expr *x, expr* y) const
{
  return new stateset_diff(Filename(), Linenumber(), x, y);
}

// ******************************************************************
// *                                                                *
// *                    stateset_implies  class                     *
// *                                                                *
// ******************************************************************

/// Implication (ugh!) of two statesets.
class stateset_implies : public binary {
public:
  stateset_implies(const char* fn, int line, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x, expr* y) const;
};

// ******************************************************************
// *                   stateset_implies  methods                    *
// ******************************************************************

stateset_implies::stateset_implies(const char* fn, int line, expr *l, expr* r)
 : binary(fn, line, exprman::bop_implies, l->Type(), l, r)
{
}

void stateset_implies::Compute(traverse_data &x)
{
  //
  // L -> R   =  !L + R
  //
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);

  SafeCompute(left, x);

  if (!x.answer->isNormal()) return;

  stateset* notL = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(notL);

  if (notL->numRefs() > 1) {
    notL->Complement();
    notL = Share(notL);
  } else {
    notL = notL->DeepCopy();  // loses the original but x.answer still has it
    notL->Complement();
  }

  //
  // We have !L.
  //
  SafeCompute(right, x);      // Deletes old x.answer
  if (!x.answer->isNormal()) {
    return;
  }

  stateset* R = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(R);

  //
  // We have R
  //

  stateset* foo = 0;

  if (stateset::parentsMatch(this, "implication", notL, R)) {
    bool ok;
    if (R->numRefs() > 1) {
      R = R->DeepCopy();
      ok = R->Union(this, "implication", notL);
    } else {
      ok = R->Union(this, "implication", notL);
      R = Share(R);
    }
    if (!ok) {
      // Union failed
      Delete(foo);
      foo = 0;
    }
  }
  Delete(notL);

  if (foo) {
    x.answer->setPtr(foo);
  } else {
    x.answer->setNull();
  }
}

expr* stateset_implies::buildAnother(expr *x, expr* y) const
{
  return new stateset_implies(Filename(), Linenumber(), x, y);
}

// ******************************************************************
// *                                                                *
// *                     stateset_union  class                      *
// *                                                                *
// ******************************************************************

/// Union of stateset expressions.
class stateset_union : public summation {
public:
  stateset_union(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                     stateset_union methods                     *
// ******************************************************************

stateset_union
::stateset_union(const char* fn, int line, const type* t, expr **x, int n)
 : summation(fn, line, exprman::aop_or, t, x, 0, n)
{
}

void stateset_union::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  SafeCompute(operands[0], x);
  if (!x.answer->isNormal()) return;
  stateset* total = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(total);
  if (total->numRefs() > 1) {
    total = total->DeepCopy();
  } else {
    total = Share(total);
  }
  DCASSERT(total);

  for (int i=1; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      Delete(total);
      return;
    }
    stateset* curr = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    bool ok = false;
    if (stateset::parentsMatch(this, "union", total, curr)) {
      ok = total->Union(this, curr);
    }
    if (!ok) {
      Delete(total);
      x.answer->setNull();
      return;
    }
  } // for i

  x.answer->setPtr(total);
}

expr* stateset_union::buildAnother(expr **x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new stateset_union(Filename(), Linenumber(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                   stateset_intersect  class                    *
// *                                                                *
// ******************************************************************

/// Intersection of stateset expressions.
class stateset_intersect : public product {
public:
  stateset_intersect(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                   stateset_intersect methods                   *
// ******************************************************************

stateset_intersect
::stateset_intersect(const char* fn, int line, const type* t, expr **x, int n)
 : product(fn, line, exprman::aop_and, t, x, 0, n)
{
}

void stateset_intersect::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  SafeCompute(operands[0], x);
  if (!x.answer->isNormal()) return;
  stateset* total = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(total);
  if (total->numRefs() > 1) {
    total = total->DeepCopy();
  } else {
    total = Share(total);
  }
  DCASSERT(total);

  for (int i=1; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      Delete(total);
      return;
    }
    stateset* curr = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    bool ok = false;
    if (stateset::parentsMatch(this, "intersection", total, curr)) {
      ok = total->Intersect(this, curr);
    }
    if (!ok) {
      Delete(total);
      x.answer->setNull();
      return;
    }
  } // for i

  x.answer->setPtr(total);
}

expr* stateset_intersect::buildAnother(expr **x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new stateset_intersect(Filename(), Linenumber(), Type(), x, n);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                      stateset  operations                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                     stateset_not_op  class                     *
// *                                                                *
// ******************************************************************

class stateset_not_op : public unary_op {
public:
  stateset_not_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                    stateset_not_op  methods                    *
// ******************************************************************

stateset_not_op::stateset_not_op() : unary_op(exprman::uop_not)
{
}

const type* stateset_not_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  if (0==t)    return 0;
  if (t->isASet())  return 0;
  if (t != em->STATESET)  return 0;
  return t;
}

unary* stateset_not_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new stateset_not(fn, ln, x);
}

// ******************************************************************
// *                                                                *
// *                     stateset_binary  class                     *
// *                                                                *
// ******************************************************************

/// Abstract base class for binary operations on statesets
class stateset_binary : public binary_op {
public:
  stateset_binary(exprman::binary_opcode opc);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* lt, const type* rt) const;
};

// ******************************************************************
// *                    stateset_binary  methods                    *
// ******************************************************************

stateset_binary::stateset_binary(exprman::binary_opcode opc) : binary_op(opc)
{
}

int stateset_binary::getPromoteDistance(const type* lt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  int ld = em->getPromoteDistance(lt, em->STATESET);
  if (ld < 0) return ld;
  int rd = em->getPromoteDistance(rt, em->STATESET);
  if (rd < 0) return rd;
  return ld + rd;
}

const type* stateset_binary::getExprType(const type* l, const type* r) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  if (em->NULTYPE==l) return 0;
  if (em->NULTYPE==r) return 0;
  return em->STATESET;
}


// ******************************************************************
// *                                                                *
// *                     stateset_diff_op class                     *
// *                                                                *
// ******************************************************************

class stateset_diff_op : public stateset_binary {
public:
  stateset_diff_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                    stateset_diff_op methods                    *
// ******************************************************************

stateset_diff_op::stateset_diff_op() : stateset_binary(exprman::bop_diff)
{
}

binary* stateset_diff_op
::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  if (0==l || 0==r) {
    Delete(l);
    Delete(r);
    return 0;
  }
  return new stateset_diff(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                   stateset_implies_op  class                   *
// *                                                                *
// ******************************************************************

class stateset_implies_op : public stateset_binary {
public:
  stateset_implies_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                  stateset_implies_op  methods                  *
// ******************************************************************

stateset_implies_op::stateset_implies_op()
: stateset_binary(exprman::bop_implies)
{
}

binary* stateset_implies_op
::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  if (0==l || 0==r) {
    Delete(l);
    Delete(r);
    return 0;
  }
  return new stateset_implies(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                    stateset_assoc_op  class                    *
// *                                                                *
// ******************************************************************

class stateset_assoc_op : public assoc_op {
public:
  stateset_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                   stateset_assoc_op  methods                   *
// ******************************************************************

stateset_assoc_op::stateset_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int stateset_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(list[i]), em->STATESET);
    if (dx < 0) return dx;
    d += dx;
  }
  return d;
}

int stateset_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  int ld = em->getPromoteDistance(lt, em->STATESET);
  if (ld < 0) return ld;
  int rd = em->getPromoteDistance(rt, em->STATESET);
  if (rd < 0) return rd;
  return ld + rd;
}

const type* stateset_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  if (em->NULTYPE==l) return 0;
  if (em->NULTYPE==r) return 0;
  return em->STATESET;
}



// ******************************************************************
// *                                                                *
// *                    stateset_union_op  class                    *
// *                                                                *
// ******************************************************************

class stateset_union_op : public stateset_assoc_op {
public:
  stateset_union_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                   stateset_union_op  methods                   *
// ******************************************************************

stateset_union_op::stateset_union_op() : stateset_assoc_op(exprman::aop_or)
{
}

assoc* stateset_union_op::makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  if (getPromoteDistance(list, flip, N) < 0) {
    delete[] flip;
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;
    return 0;
  }
  return new stateset_union(fn, ln, em->STATESET, list, N);
}


// ******************************************************************
// *                                                                *
// *                  stateset_intersect_op  class                  *
// *                                                                *
// ******************************************************************

class stateset_intersect_op : public stateset_assoc_op {
public:
  stateset_intersect_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const;
};

// ******************************************************************
// *                 stateset_intersect_op  methods                 *
// ******************************************************************

stateset_intersect_op::stateset_intersect_op()
 : stateset_assoc_op(exprman::aop_and)
{
}

assoc* stateset_intersect_op::makeExpr(const char* fn, int ln, expr** list,
        bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->STATESET);
  if (getPromoteDistance(list, flip, N) < 0) {
    delete[] flip;
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;
    return 0;
  }
  return new stateset_intersect(fn, ln, em->STATESET, list, N);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         card_si  class                         *
// ******************************************************************

class card_si : public simple_internal {
public:
  card_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

card_si::card_si() : simple_internal(em->BIGINT, "card", 1)
{
  DCASSERT(em->BIGINT);
  DCASSERT(em->STATESET);
  SetFormal(0, em->STATESET, "P");
  SetDocumentation("Return the number of elements in the set P.");
}

void card_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(1==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateset* ss = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(ss);
  ss->getCardinality( *(x.answer) );
}

// ******************************************************************
// *                         empty_si class                         *
// ******************************************************************

class empty_si : public simple_internal {
public:
  empty_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

empty_si::empty_si() : simple_internal(em->BOOL, "empty", 1)
{
  DCASSERT(em->STATESET);
  SetFormal(0, em->STATESET, "P");
  SetDocumentation("Returns true if and only if the set P is empty.");
}

void empty_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(1==np);
  DCASSERT(0==x.aggregate);

  SafeCompute(pass[0], x);
  if (!x.answer->isNormal()) return;

  stateset* ss = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(ss);
  x.answer->setBool(ss->isEmpty());
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_statesets : public initializer {
  public:
    init_statesets();
    virtual bool execute();
};
init_statesets the_stateset_initializer;

init_statesets::init_statesets() : initializer("init_statesets")
{
  usesResource("em");
  usesResource("st");
  usesResource("biginttype");
  buildsResource("statesettype");
  buildsResource("types");
}

bool init_statesets::execute()
{
  if (0==em)  return false;

  stateset::em = em;

  // Library registry
  // em->registerLibrary(  &intset_lib_data );

  // Type registry
  simple_type* t_stateset = new stateset_type;
  em->registerType(t_stateset);
  em->setFundamentalTypes();

  // Operators
  em->registerOperation(  new stateset_not_op         );
  em->registerOperation(  new stateset_diff_op        );
  em->registerOperation(  new stateset_implies_op     );
  em->registerOperation(  new stateset_union_op       );
  em->registerOperation(  new stateset_intersect_op   );

  // Options
  stateset::print_indexes = true;
  if (em->OptMan())
      em->OptMan()->addBoolOption("StatesetPrintIndexes",
        "If true, when a stateset is printed, state indexes are displayed; otherwise, states are displayed.",
        stateset::print_indexes
      );

  if (0==st) return false;

  // Functions
  st->AddSymbol(  new card_si   );
  st->AddSymbol(  new empty_si  );
  return true;
}


