
#include "../Options/options.h"
#include "../ExprLib/startup.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/assoc.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/functions.h"

#include "../Formlsms/graph_llm.h"


#include "set_statesets.h"


// ******************************************************************
// *                                                                *
// *                        set_stateset methods                        *
// *                                                                *
// ******************************************************************

exprman* set_stateset::em = 0;
bool set_stateset::print_indexes;

set_stateset::set_stateset(const state_lldsm* p) : shared_object()
{
  parent = p;
}

set_stateset::~set_stateset()
{
}

const hldsm* set_stateset::getGrandparent() const
{
  return parent ? parent->GetParent() : 0;
}

bool set_stateset::parentsMatch(const expr* c, const char* op, set_stateset* A, set_stateset* B)
{
  if (0==A || 0==B) return false;

  if (A->getParent() != B->getParent()) {
    if (em->startError()) {
      em->causedBy(c);
      em->cerr() << "set_statesets in " << op << " are from different model instances";
      em->stopIO();
    }
    return false;
  }

  return true;
}

void set_stateset::storageMismatchError(const expr* c, const char* op)
{
  if (em->startError()) {
    em->causedBy(c);
    em->cerr() << "set_statesets in " << op << " use incompatible storage types";
    em->stopIO();
  }
}

// ******************************************************************
// *                                                                *
// *                      set_stateset_type  class                      *
// *                                                                *
// ******************************************************************

class set_stateset_type : public simple_type {
public:
  set_stateset_type();
};

// ******************************************************************
// *                     set_stateset_type  methods                     *
// ******************************************************************

set_stateset_type::set_stateset_type() : simple_type("set_stateset", "Set of statesets", "Type used for sets of statesets, used for CTL model checking under undertainty and other operations.")
{
  setPrintable();
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                      set_stateset expressions                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                      set_stateset_not  class                       *
// *                                                                *
// ******************************************************************

/// Negation of a set_stateset expression.
class set_stateset_not : public negop {
public:
  set_stateset_not(const char* fn, int line, expr *x);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x) const;
};

// ******************************************************************
// *                      set_stateset_not methods                      *
// ******************************************************************

set_stateset_not::set_stateset_not(const char* fn, int line, expr *x)
 : negop(fn, line, exprman::uop_not, x->Type(), x)
{
}

void set_stateset_not::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(opnd);
  opnd->Compute(x); 

  if (!x.answer->isNormal()) return;

  set_stateset* ss = smart_cast <set_stateset*> (x.answer->getPtr());
  DCASSERT(ss);
  if (ss->numRefs()>1) {
    set_stateset* ans = ss->DeepCopy();
    ans->Complement();
    x.answer->setPtr(ans);
  } else {
    // in-place
    ss->Complement();
  }
}

expr* set_stateset_not::buildAnother(expr *x) const
{
  return new set_stateset_not(Filename(), Linenumber(), x);
}

// ******************************************************************
// *                                                                *
// *                      set_stateset_diff class                       *
// *                                                                *
// ******************************************************************

/// Difference of two set_statesets.
class set_stateset_diff : public binary {
public:
  set_stateset_diff(const char* fn, int line, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x, expr* y) const;
};

// ******************************************************************
// *                     set_stateset_diff methods                      *
// ******************************************************************

set_stateset_diff::set_stateset_diff(const char* fn, int line, expr *l, expr* r)
 : binary(fn, line, exprman::bop_diff, l->Type(), l, r)
{
}

void set_stateset_diff::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
}

expr* set_stateset_diff::buildAnother(expr *x, expr* y) const
{
  return new set_stateset_diff(Filename(), Linenumber(), x, y);
}

// ******************************************************************
// *                                                                *
// *                    set_stateset_implies  class                     *
// *                                                                *
// ******************************************************************

/// Implication (ugh!) of two set_statesets.
class set_stateset_implies : public binary {
public:
  set_stateset_implies(const char* fn, int line, expr *l, expr* r);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr *x, expr* y) const;
};

// ******************************************************************
// *                   set_stateset_implies  methods                    *
// ******************************************************************

set_stateset_implies::set_stateset_implies(const char* fn, int line, expr *l, expr* r)
 : binary(fn, line, exprman::bop_implies, l->Type(), l, r)
{
}

void set_stateset_implies::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
}

expr* set_stateset_implies::buildAnother(expr *x, expr* y) const
{
  return new set_stateset_implies(Filename(), Linenumber(), x, y);
}

// ******************************************************************
// *                                                                *
// *                     set_stateset_union  class                      *
// *                                                                *
// ******************************************************************

/// Union of set_stateset expressions.
class set_stateset_union : public summation {
public:
  set_stateset_union(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                     set_stateset_union methods                     *
// ******************************************************************

set_stateset_union
::set_stateset_union(const char* fn, int line, const type* t, expr **x, int n)
 : summation(fn, line, exprman::aop_or, t, x, 0, n)
{
}

void set_stateset_union::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  SafeCompute(operands[0], x);
  if (!x.answer->isNormal()) return;
  set_stateset* total = smart_cast <set_stateset*> (x.answer->getPtr());
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
    set_stateset* curr = smart_cast <set_stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    bool ok = false;
    if (set_stateset::parentsMatch(this, "union", total, curr)) {
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

expr* set_stateset_union::buildAnother(expr **x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new set_stateset_union(Filename(), Linenumber(), Type(), x, n);
}

// ******************************************************************
// *                                                                *
// *                   set_stateset_intersect  class                    *
// *                                                                *
// ******************************************************************

/// Intersection of set_stateset expressions.
class set_stateset_intersect : public product {
public:
  set_stateset_intersect(const char* fn, int line, const type* t, expr **x, int n);
  virtual void Compute(traverse_data &x);
protected:
  virtual expr* buildAnother(expr **x, bool* f, int n) const;
};

// ******************************************************************
// *                   set_stateset_intersect methods                   *
// ******************************************************************

set_stateset_intersect
::set_stateset_intersect(const char* fn, int line, const type* t, expr **x, int n)
 : product(fn, line, exprman::aop_and, t, x, 0, n)
{
}

void set_stateset_intersect::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  SafeCompute(operands[0], x);
  if (!x.answer->isNormal()) return;
  set_stateset* total = smart_cast <set_stateset*> (x.answer->getPtr());
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
    set_stateset* curr = smart_cast <set_stateset*> (x.answer->getPtr());
    DCASSERT(curr);

    bool ok = false;
    if (set_stateset::parentsMatch(this, "intersection", total, curr)) {
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

expr* set_stateset_intersect::buildAnother(expr **x, bool* f, int n) const
{
  DCASSERT(0==f);
  return new set_stateset_intersect(Filename(), Linenumber(), Type(), x, n);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                      set_stateset  operations                      *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                     set_stateset_not_op  class                     *
// *                                                                *
// ******************************************************************

class set_stateset_not_op : public unary_op {
public:
  set_stateset_not_op();
  virtual const type* getExprType(const type* t) const;
  virtual unary* makeExpr(const char* fn, int ln, expr* x) const;
};

// ******************************************************************
// *                    set_stateset_not_op  methods                    *
// ******************************************************************

set_stateset_not_op::set_stateset_not_op() : unary_op(exprman::uop_not)
{
}

const type* set_stateset_not_op::getExprType(const type* t) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  if (0==t)    return 0;
  if (t->isASet())  return 0; 
  if (t != em->SETSTATESET)  return 0;
  return t;
}

unary* set_stateset_not_op::makeExpr(const char* fn, int ln, expr* x) const
{
  DCASSERT(x);
  if (!isDefinedForType(x->Type())) {
    Delete(x);
    return 0;
  }
  return new set_stateset_not(fn, ln, x);
}

// ******************************************************************
// *                                                                *
// *                     set_stateset_binary  class                     *
// *                                                                *
// ******************************************************************

/// Abstract base class for binary operations on set_statesets
class set_stateset_binary : public binary_op {
public:
  set_stateset_binary(exprman::binary_opcode opc);
  virtual int getPromoteDistance(const type* lt, const type* rt) const;
  virtual const type* getExprType(const type* lt, const type* rt) const;
};

// ******************************************************************
// *                    set_stateset_binary  methods                    *
// ******************************************************************

set_stateset_binary::set_stateset_binary(exprman::binary_opcode opc) : binary_op(opc)
{
}

int set_stateset_binary::getPromoteDistance(const type* lt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  int ld = em->getPromoteDistance(lt, em->SETSTATESET);
  if (ld < 0) return ld;
  int rd = em->getPromoteDistance(rt, em->SETSTATESET);
  if (rd < 0) return rd;
  return ld + rd;
}

const type* set_stateset_binary::getExprType(const type* l, const type* r) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  if (em->NULTYPE==l) return 0;
  if (em->NULTYPE==r) return 0;
  return em->SETSTATESET;
}


// ******************************************************************
// *                                                                *
// *                     set_stateset_diff_op class                     *
// *                                                                *
// ******************************************************************

class set_stateset_diff_op : public set_stateset_binary {
public:
  set_stateset_diff_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                    set_stateset_diff_op methods                    *
// ******************************************************************

set_stateset_diff_op::set_stateset_diff_op() : set_stateset_binary(exprman::bop_diff)
{
}

binary* set_stateset_diff_op
::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  if (0==l || 0==r) {
    Delete(l);
    Delete(r);
    return 0;
  }
  return new set_stateset_diff(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                   set_stateset_implies_op  class                   *
// *                                                                *
// ******************************************************************

class set_stateset_implies_op : public set_stateset_binary {
public:
  set_stateset_implies_op();
  virtual binary* makeExpr(const char* fn, int ln, expr* l, expr* r) const;
};

// ******************************************************************
// *                  set_stateset_implies_op  methods                  *
// ******************************************************************

set_stateset_implies_op::set_stateset_implies_op()
: set_stateset_binary(exprman::bop_implies)
{
}

binary* set_stateset_implies_op
::makeExpr(const char* fn, int ln, expr* l, expr* r) const
{
  if (0==l || 0==r) {
    Delete(l);
    Delete(r);
    return 0;
  }
  return new set_stateset_implies(fn, ln, l, r);
}

// ******************************************************************
// *                                                                *
// *                    set_stateset_assoc_op  class                    *
// *                                                                *
// ******************************************************************

class set_stateset_assoc_op : public assoc_op {
public:
  set_stateset_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                   set_stateset_assoc_op  methods                   *
// ******************************************************************

set_stateset_assoc_op::set_stateset_assoc_op(exprman::assoc_opcode op) : assoc_op(op)
{
}

int set_stateset_assoc_op::getPromoteDistance(expr** list, bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  int d = 0;
  for (int i=0; i<N; i++) {
    int dx = em->getPromoteDistance(em->SafeType(list[i]), em->SETSTATESET);
    if (dx < 0) return dx;
    d += dx;
  }
  return d;
}

int set_stateset_assoc_op
::getPromoteDistance(bool f, const type* lt, const type* rt) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  int ld = em->getPromoteDistance(lt, em->SETSTATESET);
  if (ld < 0) return ld;
  int rd = em->getPromoteDistance(rt, em->SETSTATESET);
  if (rd < 0) return rd;
  return ld + rd;
}

const type* set_stateset_assoc_op
::getExprType(bool f, const type* l, const type* r) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  if (em->NULTYPE==l) return 0;
  if (em->NULTYPE==r) return 0;
  return em->SETSTATESET;
}



// ******************************************************************
// *                                                                *
// *                    set_stateset_union_op  class                    *
// *                                                                *
// ******************************************************************

class set_stateset_union_op : public set_stateset_assoc_op {
public:
  set_stateset_union_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                   set_stateset_union_op  methods                   *
// ******************************************************************

set_stateset_union_op::set_stateset_union_op() : set_stateset_assoc_op(exprman::aop_or)
{
}

assoc* set_stateset_union_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  if (getPromoteDistance(list, flip, N) < 0) {
    delete[] flip;
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;  
    return 0;
  }
  return new set_stateset_union(fn, ln, em->SETSTATESET, list, N);
}


// ******************************************************************
// *                                                                *
// *                  set_stateset_intersect_op  class                  *
// *                                                                *
// ******************************************************************

class set_stateset_intersect_op : public set_stateset_assoc_op {
public:
  set_stateset_intersect_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                 set_stateset_intersect_op  methods                 *
// ******************************************************************

set_stateset_intersect_op::set_stateset_intersect_op()
 : set_stateset_assoc_op(exprman::aop_and)
{
}

assoc* set_stateset_intersect_op::makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const
{
  DCASSERT(em);
  DCASSERT(em->SETSTATESET);
  if (getPromoteDistance(list, flip, N) < 0) {
    delete[] flip;
    for (int i=0; i<N; i++) Delete(list[i]);
    delete[] list;  
    return 0;
  }
  return new set_stateset_intersect(fn, ln, em->SETSTATESET, list, N);
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                           Functions                            *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         empty_ssi class                         *
// ******************************************************************

// class empty_ssi : public simple_internal {
// public:
//   empty_ssi();
//   virtual void Compute(traverse_data &x, expr** pass, int np);
// };

// empty_ssi::empty_ssi() : simple_internal(em->BOOL, "empty", 1)
// {
//   DCASSERT(em->SETSTATESET);
//   SetFormal(0, em->SETSTATESET, "P");
//   SetDocumentation("Returns true if and only if the set P is empty.");
// }

// void empty_ssi::Compute(traverse_data &x, expr** pass, int np)
// {
//   DCASSERT(x.answer);
//   DCASSERT(1==np);
//   DCASSERT(0==x.aggregate);

//   SafeCompute(pass[0], x);
//   if (!x.answer->isNormal()) return;

//   set_stateset* ss = smart_cast <set_stateset*> (x.answer->getPtr());
//   DCASSERT(ss);
//   x.answer->setBool(ss->isEmpty());
// }


// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_set_statesets : public initializer {
  public:
    init_set_statesets();
    virtual bool execute();
};
init_set_statesets the_set_stateset_initializer;

init_set_statesets::init_set_statesets() : initializer("init_set_statesets")
{
  usesResource("em");
  usesResource("st");
  usesResource("biginttype");
  buildsResource("set_statesettype");
  buildsResource("types");
}

bool init_set_statesets::execute()
{
  if (0==em)  return false;

  set_stateset::em = em;
  
  // Library registry
  // em->registerLibrary(  &intset_lib_data );

  // Type registry
  simple_type* t_set_stateset = new set_stateset_type;
  em->registerType(t_set_stateset);
  em->setFundamentalTypes();

  // Operators
  em->registerOperation(  new set_stateset_not_op         );
  em->registerOperation(  new set_stateset_diff_op        );
  em->registerOperation(  new set_stateset_implies_op     );
  em->registerOperation(  new set_stateset_union_op       );
  em->registerOperation(  new set_stateset_intersect_op   );

  // Options
  set_stateset::print_indexes = true;
  em->addOption(
    MakeBoolOption("set_statesetPrintIndexes", 
      "If true, when a set_stateset is printed, state indexes are displayed; otherwise, states are displayed.",
      set_stateset::print_indexes
    )
  );

  if (0==st) return false;

  // Functions
  //st->AddSymbol(  new card_ssi   );
  // st->AddSymbol(  new empty_ssi  );
  return true;
}


