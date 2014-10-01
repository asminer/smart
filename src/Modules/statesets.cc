
// $Id$

#include "../Options/options.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"
#include "biginttype.h"
#include "../ExprLib/mod_vars.h"
#include "../ExprLib/dd_front.h"

#include "../Formlsms/check_llm.h"

// external library
#include "intset.h"

#include "statesets.h"

// ******************************************************************
// *                                                                *
// *                       expl_printer class                       *
// *                                                                *
// ******************************************************************

class expl_printer : public lldsm::state_visitor {
  OutputStream &out;
  const intset &toprint;
  bool print_indexes;
  bool comma;
public:
  expl_printer(const hldsm* mdl, OutputStream &s, const intset &p, bool pi);
  virtual bool canSkipIndex();
  virtual bool visit();
};

expl_printer
::expl_printer(const hldsm* m, OutputStream &s, const intset &p, bool pi)
 : state_visitor(m), out(s), toprint(p)
{
  print_indexes = pi;
  comma = false;
}

bool expl_printer::canSkipIndex()
{
  return (! toprint.contains(x.current_state_index) );
}

bool expl_printer::visit()
{
  if (comma)  out << ", ";
  else        comma = true;
  if (print_indexes) {
    out.Put(x.current_state_index); 
  } else {
    x.current_state->Print(out, 0);
  }
  out.can_flush();  // otherwise, huge sets will overflow the buffer
  return false;
}

// ******************************************************************
// *                                                                *
// *                        stateset methods                        *
// *                                                                *
// ******************************************************************

bool stateset::print_indexes;

stateset::stateset(const checkable_lldsm* p, intset* e) : shared_object()
{
  is_explicit = true;
  parent = p;
  expl_data = e;
  state_forest = relation_forest = 0;
  state_dd = relation_dd = 0;
}

stateset::stateset(const checkable_lldsm* p, sv_encoder* sf, shared_object* s,
                                   sv_encoder* rf, shared_object* r)
: shared_object()
{
  is_explicit = false;
  parent = p;
  expl_data = 0;
  state_forest = sf;
  state_dd = s;
  relation_forest = rf;
  relation_dd = r;
}

stateset::~stateset()
{
  delete expl_data;
  Delete(state_dd);
  Delete(state_forest);
  Delete(relation_dd);
  Delete(relation_forest);
}

void stateset::getCardinality(long &card) const
{
  if (isExplicit()) {
    card = getExplicit().cardinality();
  } else {
    DCASSERT(state_forest);
    state_forest->getCardinality(state_dd, card);
  }
}

void stateset::getCardinality(result &x) const
{
  if (isExplicit()) {
    long count = getExplicit().cardinality();
    x.setPtr(new bigint(count));
  } else {
    DCASSERT(state_forest);
    state_forest->getCardinality(state_dd, x);
  }
}

bool stateset::isEmpty() const
{
  if (isExplicit()) {
    return getExplicit().isEmpty();
  } else {
    DCASSERT(state_forest);
    bool ans;
    state_forest->isEmpty(state_dd, ans);
    return ans;
  }
}

bool stateset::Print(OutputStream &s, int width) const
{
  if (is_explicit)  return print_explicit(s);
  else              return print_symbolic(s);
}

bool stateset::print_explicit(OutputStream &s) const
{
  DCASSERT(parent);
  DCASSERT(is_explicit);
  DCASSERT(expl_data);
  expl_printer foo(parent->GetParent(), s, *expl_data, print_indexes);
  s.Put('{');
  parent->visitStates(foo);
  s.Put('}');
  return true;
}

bool stateset::print_symbolic(OutputStream &s) const
{
  DCASSERT(parent);
  DCASSERT(!is_explicit);
  DCASSERT(state_forest);
  DCASSERT(state_dd);
  shared_state* st = new shared_state(parent->GetParent());
  const int* mt = state_forest->firstMinterm(state_dd);
  s.Put('{');
  bool comma = false;
  while (mt) {
    if (comma)  s << ", ";
    else        comma = true;
    state_forest->minterm2state(mt, st);
    parent->GetParent()->showState(s, st);
    mt = state_forest->nextMinterm(state_dd);
  }
  s.Put('}');
  Delete(st);
  return true;
}

bool stateset::Equals(const shared_object *o) const
{
  const stateset* b = dynamic_cast <const stateset*> (o);
  if (0==b) return false;
  if (parent != b->parent) return false;  // TBD: may want to allow this
  if (is_explicit != b->is_explicit) return false; // TBD: and this
  
  DCASSERT(is_explicit);
  if (0==expl_data && 0==b->expl_data) return true;
  if (0==expl_data || 0==b->expl_data) return false;
  
  // TBD: intset comparison here
  return true;
}

// ******************************************************************
// *                                                                *
// *                     intset library credits                     *
// *                                                                *
// ******************************************************************

class intset_lib : public library {
public:
  intset_lib();
  virtual const char* getVersionString() const;
  virtual bool hasFixedPointer() const { return true; }
};

intset_lib::intset_lib() : library(false)
{
}

const char* intset_lib::getVersionString() const
{
  return intset::getVersion();
}

intset_lib intset_lib_data;

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

  stateset* ss = smart_cast <stateset*> (Share(x.answer->getPtr()));
  DCASSERT(ss);
  x.answer->setPtr(Complement(em, this, ss));
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
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);

  SafeCompute(right, x);

  if (!x.answer->isNormal()) return;

  stateset* rt = smart_cast <stateset*> (Share(x.answer->getPtr()));
  DCASSERT(rt);

  SafeCompute(left, x);
  if (!x.answer->isNormal()) {
    Delete(rt);
    return;
  }

  stateset* lt = smart_cast <stateset*> (Share(x.answer->getPtr()));
  DCASSERT(lt);

  if (lt->getParent() != rt->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Statesets in difference are from different model instances";
      em->stopIO();
    }
    Delete(rt);
    Delete(lt);
    x.answer->setNull();
    return;
  }

  if (lt->isExplicit() != rt->isExplicit()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Statesets in difference use different storage types";
      em->stopIO();
    }
    Delete(rt);
    Delete(lt);
    x.answer->setNull();
    return;
  }

  if (lt->isExplicit()) {
    // EXPLICIT
    stateset* foo = new stateset(lt->getParent(), new intset);
    foo->changeExplicit() = lt->getExplicit() - rt->getExplicit();
    x.answer->setPtr(foo);
  } else {
    // SYMBOLIC
    shared_object* newdd = lt->getStateForest()->makeEdge(0);
    stateset* foo = new stateset(
      lt->getParent(), Share(lt->getStateForest()), newdd,
      Share(lt->getRelationForest()), Share(lt->getRelationDD())
    );
    lt->getStateForest()->buildBinary(
        lt->getStateDD(), exprman::bop_diff, rt->getStateDD(), newdd
    );
    x.answer->setPtr(foo);
  }
  Delete(lt);
  Delete(rt);
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
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);

  SafeCompute(right, x);

  if (!x.answer->isNormal()) return;

  stateset* rt = smart_cast <stateset*> (Share(x.answer->getPtr()));
  DCASSERT(rt);

  SafeCompute(left, x);
  if (!x.answer->isNormal()) {
    Delete(rt);
    return;
  }

  stateset* lt = smart_cast <stateset*> (Share(x.answer->getPtr()));
  DCASSERT(lt);

  if (lt->getParent() != rt->getParent()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Statesets in implication are from different model instances";
      em->stopIO();
    }
    Delete(rt);
    Delete(lt);
    x.answer->setNull();
    return;
  }

  if (lt->isExplicit() != rt->isExplicit()) {
    if (em->startError()) {
      em->causedBy(this);
      em->cerr() << "Statesets in implication use different storage types";
      em->stopIO();
    }
    Delete(rt);
    Delete(lt);
    x.answer->setNull();
    return;
  }

  // L -> R   =  !L + R
  //

  x.answer->setPtr(
    Union(em, this, Complement(em, this, lt), rt)
  );
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
  stateset* first = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(first);
  const checkable_lldsm* parent = first->getParent();
  bool is_explicit = first->isExplicit();
  stateset* answer = 0;
  if (is_explicit) {
    answer = new stateset(parent, new intset(first->getExplicit()));
  } else {
    answer = new stateset(
      parent, Share(first->getStateForest()), 
      first->getStateForest()->makeEdge(first->getStateDD()),
      Share(first->getRelationForest()), Share(first->getRelationDD())
    );
  }
  
  for (int i=1; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      Delete(answer);
      return;
    }
    stateset* curr = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    if (curr->getParent() != parent) {
      if (em->startError()) {
        em->causedBy(this);
        em->cerr() << "Statesets in union are from different model instances";
        em->stopIO();
      }
      Delete(answer);
      x.answer->setNull();
      return;
    } // if parent

    if (curr->isExplicit() != is_explicit) {
      if (em->startError()) {
        em->causedBy(this);
        em->cerr() << "Statesets in union use different storage types";
        em->stopIO();
      }
      Delete(answer);
      x.answer->setNull();
      return;
    } // if explicit

    // ok, we can actually perform the union now!
    if (is_explicit) {
      answer->changeExplicit() += curr->getExplicit();
    } else {
      answer->getStateForest()->buildAssoc(
        answer->getStateDD(), false, exprman::aop_or,
        curr->getStateDD(), answer->changeStateDD()
      );
    }
  } // for i

  // success, cleanup
  x.answer->setPtr(answer);
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
  stateset* first = smart_cast <stateset*> (x.answer->getPtr());
  DCASSERT(first);
  const checkable_lldsm* parent = first->getParent();
  bool is_explicit = first->isExplicit();
  stateset* answer = 0;
  if (is_explicit) {
    answer = new stateset(parent, new intset(first->getExplicit()));
  } else {
    answer = new stateset(
      parent, Share(first->getStateForest()), 
      first->getStateForest()->makeEdge(first->getStateDD()),
      Share(first->getRelationForest()), Share(first->getRelationDD())
    );
  }
  
  for (int i=1; i<opnd_count; i++) {
    SafeCompute(operands[i], x);
    if (!x.answer->isNormal()) {
      Delete(answer);
      return;
    }
    stateset* curr = smart_cast <stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    if (curr->getParent() != parent) {
      if (em->startError()) {
        em->causedBy(this);
        em->cerr() << "Statesets in intersection are from different model instances";
        em->stopIO();
      }
      Delete(answer);
      x.answer->setNull();
      return;
    } // if parent

    if (curr->isExplicit() != is_explicit) {
      if (em->startError()) {
        em->causedBy(this);
        em->cerr() << "Statesets in intersection use different storage types";
        em->stopIO();
      }
      Delete(answer);
      x.answer->setNull();
      return;
    } // if explicit

    // ok, we can actually perform the intersection now!
    if (is_explicit) {
      answer->changeExplicit() *= curr->getExplicit();
    } else {
      answer->getStateForest()->buildAssoc(
        answer->getStateDD(), false, exprman::aop_and,
        curr->getStateDD(), answer->changeStateDD()
      );
    }
  } // for i

  // success, cleanup
  x.answer->setPtr(answer);
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
// *                           Front  end                           *
// *                                                                *
// *                                                                *
// ******************************************************************

stateset* Complement(exprman* em, const expr* c, stateset* ss)
{
  if (0==ss) return 0;
  if (ss->isExplicit()) {
    if (1==ss->numRefs()) {
      ss->changeExplicit().complement();
      return ss;
    }
    // make a copy and invert it
    intset* foo = new intset;
    *foo = !(ss->getExplicit());
    stateset* ans = new stateset(ss->getParent(), foo);
    Delete(ss);
    return ans;
  } else {
    // symbolic complement: do "reachable - ss".
    result reachable;
    DCASSERT(ss->getParent());
    ss->getParent()->getReachable(reachable);
    stateset* rss = smart_cast <stateset*> (reachable.getPtr());
    DCASSERT(rss);
    shared_object* x = ss->getStateForest()->makeEdge(0);
    DCASSERT(x);
    ss->getStateForest()->buildBinary(
      rss->getStateDD(), exprman::bop_diff, ss->getStateDD(), x
    );
    stateset* ans = new stateset(
        ss->getParent(), Share(ss->getStateForest()), x,
        Share(ss->getRelationForest()), Share(ss->getRelationDD())
    );
    Delete(ss);
    return ans;
  }
}

stateset* Union(exprman* em, const expr* c, stateset* x, stateset* y)
{
  if (0==x || 0==y) return 0;
  
  if (x->getParent() != y->getParent()) {
    if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(c);
        em->internal() << "Statesets in union are from different model instances";
        em->stopIO();
    }
    return 0;
  } // if parent

  if (x->isExplicit() != y->isExplicit()) {
    if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(c);
        em->internal() << "Statesets in union use different storage types";
        em->stopIO();
    }
    return 0;
  } // if explicit

  if (!x->isExplicit()) {
    DCASSERT(x->getStateForest() == y->getStateForest());
    shared_object* z = x->getStateForest()->makeEdge(0);
    DCASSERT(z);
    x->getStateForest()->buildAssoc(
      x->getStateDD(), false, exprman::aop_or, y->getStateDD(), z
    );
    return new stateset(
        x->getParent(), Share(x->getStateForest()), z,
        Share(x->getRelationForest()), Share(x->getRelationDD())
    );
  }

  intset* expl_data = new intset;
  *expl_data = x->getExplicit() + y->getExplicit();

  return new stateset(x->getParent(), expl_data);
}

stateset* Intersection(exprman* em, const expr* c, stateset* x, stateset* y)
{
  if (0==x || 0==y) return 0;
  
  if (x->getParent() != y->getParent()) {
    if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(c);
        em->internal() << "Statesets in intersection are from different model instances";
        em->stopIO();
    }
    return 0;
  } // if parent

  if (x->isExplicit() != y->isExplicit()) {
    if (em->startInternal(__FILE__, __LINE__)) {
        em->causedBy(c);
        em->internal() << "Statesets in intersection use different storage types";
        em->stopIO();
    }
    return 0;
  } // if explicit

  if (!x->isExplicit()) {
    DCASSERT(x->getStateForest() == y->getStateForest());
    shared_object* z = x->getStateForest()->makeEdge(0);
    DCASSERT(z);
    x->getStateForest()->buildAssoc(
      x->getStateDD(), false, exprman::aop_and, y->getStateDD(), z
    );
    return new stateset(
        x->getParent(), Share(x->getStateForest()), z,
        Share(x->getRelationForest()), Share(x->getRelationDD())
    );
  }

  intset* expl_data = new intset;
  *expl_data = x->getExplicit() * y->getExplicit();

  return new stateset(x->getParent(), expl_data);
}

void InitStatesets(exprman* em, symbol_table* st)
{
  if (0==em)  return;
  
  // Library registry
  em->registerLibrary(  &intset_lib_data );

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
  em->addOption(
    MakeBoolOption("StatesetPrintIndexes", 
  "If true, when a stateset is printed, state indexes are displayed; otherwise, states are displayed.",
  stateset::print_indexes
    )
  );

  if (0==st) return;

  // Functions
  st->AddSymbol(  new card_si   );
  st->AddSymbol(  new empty_si  );
}

