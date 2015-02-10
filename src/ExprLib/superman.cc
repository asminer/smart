
// $Id$

#include "superman.h"
#include "bogus.h"
#include "engine.h"

#include "unary.h"
#include "binary.h"
#include "trinary.h"
#include "assoc.h"

#include "casting.h"

#include "../Options/options.h"

// ******************************************************************
// *                                                                *
// *                        superman methods                        *
// *                                                                *
// ******************************************************************

superman::superman(io_environ* i, option_manager* o) : exprman(i, o)
{
  // special error and default expressions
  error_expr = new bogus_expr("error");
  default_expr = new bogus_expr("default");

  // Types.
  alloc_types = 16;
  last_type = 0;
  reg_type = (type**) malloc(alloc_types * sizeof(void*));

  // Operations.
  reg_unary = new const unary_op* [uop_none];
  reg_binary = new const binary_op* [bop_none];
  reg_trinary = new const trinary_op* [top_none];
  reg_assoc = new const assoc_op* [aop_none];
  for (int i=0; i<uop_none; i++)  reg_unary[i] = 0;
  for (int i=0; i<bop_none; i++)  reg_binary[i] = 0;
  for (int i=0; i<top_none; i++)  reg_trinary[i] = 0;
  for (int i=0; i<aop_none; i++)  reg_assoc[i] = 0;

  // library registry
  num_libs = 0;
  max_libs = 16;  // can be expanded
  extlibs = (const library**) malloc(max_libs * sizeof(void*));

  // engine type registry
  ETTree = new SplayOfPointers <engtype> (16, 0);
  ETList = 0;
  num_ets = 0;
}

superman::~superman()
{
  // special expressions
  Delete(error_expr);
  Delete(default_expr);

  // type stuff
  for (int i=0; i<last_type; i++)  delete reg_type[i];
  free(reg_type);

  // operations
  for (int i=0; i<uop_none; i++) {
    while (reg_unary[i]) {
      const unary_op* ptr = reg_unary[i];
      reg_unary[i] = ptr->next;
      delete ptr;
    }
  }
  for (int i=0; i<bop_none; i++) {
    while (reg_binary[i]) {
      const binary_op* ptr = reg_binary[i];
      reg_binary[i] = ptr->next;
      delete ptr;
    }
  }
  for (int i=0; i<top_none; i++) {
    while (reg_trinary[i]) {
      const trinary_op* ptr = reg_trinary[i];
      reg_trinary[i] = ptr->next;
      delete ptr;
    }
  }
  for (int i=0; i<aop_none; i++) {
    while (reg_assoc[i]) {
      const assoc_op* ptr = reg_assoc[i];
      reg_assoc[i] = ptr->next;
      delete ptr;
    }
  }
  delete[] reg_unary;
  delete[] reg_binary;
  delete[] reg_trinary;
  delete[] reg_assoc;
  
  // library registry
  free(extlibs);
 
  // engine type registry
  delete ETTree;
  for (int i=0; i<num_ets; i++)  delete ETList[i];
  delete[] ETList;
}

void superman::finalize()
{
  if (is_finalized)  return;
  
  // Convert engine type tree into an ordered list
  num_ets = ETTree->NumElements();
  ETList = new engtype* [num_ets];
  ETTree->CopyToArray(ETList);
  delete ETTree;
  ETTree = 0;

  // Finalize each engine type
  for (int i=0; i<num_ets; i++) {
    DCASSERT(ETList[i]);
    ETList[i]->finalizeRegistry(om);
  }

  // finalize option list
  if (om) om->DoneAddingOptions();
  is_finalized = true;
}

//  
//
// Special expressions
//
//  

expr* superman::makeError() const
{
  return Share(error_expr);
}

bool superman::isError(const expr* e) const
{
  return error_expr->Equals(e);
}

expr* superman::makeDefault() const
{
  return Share(default_expr);
}

bool superman::isDefault(const expr* e) const
{
  return default_expr->Equals(e);
}

bool superman::isOrdinary(const expr* e) const
{
  if (0==e)                   return false;
  if (error_expr->Equals(e))  return false;
  return !default_expr->Equals(e);
}


//  
//  
// Types
//  
//  

bool superman::registerType(type* t)
{
  if (0==t)             return false;
  if (0==t->getName())  return false;
  // check for duplicate names
  for (int i=0; i<last_type; i++) {
    DCASSERT(reg_type);
    DCASSERT(reg_type[i]);
    if (reg_type[i]->matches(t->getName()))  return false;
  }
  // ok, try to add us
  if (last_type >= alloc_types) {
    // too many types, enlarge the list.
    alloc_types += 16;
    type** foo = 
      (type**) realloc(reg_type, alloc_types * sizeof(void*));
    if (0==foo)  {
      alloc_types -= 16;
      return false;
    }
    reg_type = foo;
  }
  DCASSERT(reg_type);
  reg_type[last_type] = t;
  last_type++;
  return true;
}

bool superman::setFundamentalTypes()
{
  if (!VOID)        VOID        = findSimple("void");
  if (!NULTYPE)     NULTYPE     = findSimple("null");
  if (!BOOL)        BOOL        = findSimple("bool");
  if (!INT)         INT         = findSimple("int");
  if (!REAL)        REAL        = findSimple("real");
  if (!EXPO)        EXPO        = findSimple("expo");
  if (!MODEL)       MODEL       = findSimple("model");

  if (!STRING)      STRING      = findSimple("string");
  if (!BIGINT)      BIGINT      = findSimple("bigint");
  if (!STATESET)    STATESET    = findSimple("stateset");
  if (!STATEDIST)   STATEDIST   = findSimple("statedist");
  if (!STATEPROBS)  STATEPROBS  = findSimple("stateprobs");
  
  if (!NEXT_STATE)  NEXT_STATE  = findType("next state");

  return VOID && NULTYPE && BOOL && INT && REAL && EXPO && MODEL && STRING && BIGINT && STATESET && STATEDIST && NEXT_STATE;
}

const type* superman::findOWDType(const char* name) const
{
  DCASSERT(reg_type);
  for (int i=0; i<last_type; i++) {
    DCASSERT(reg_type[i]);
    if (reg_type[i]->matchesOWD(name))  return reg_type[i];
  }
  return 0;
}

simple_type* superman::findSimple(const char* name)
{
  DCASSERT(reg_type);
  for (int i=0; i<last_type; i++) {
    DCASSERT(reg_type[i]);
    if (reg_type[i]->matches(name)) {
      return dynamic_cast <simple_type*> (reg_type[i]);
    }
  }
  return 0;
}

const type* superman::findType(const char* name) const
{
  DCASSERT(reg_type);
  for (int i=0; i<last_type; i++) {
    DCASSERT(reg_type[i]);
    if (reg_type[i]->matches(name))  return reg_type[i];
  }
  return 0;
}

modifier superman::findModifier(const char* name) const
{
  if (strcmp(name, "ph") == 0)    return PHASE;
  if (strcmp(name, "rand") == 0)  return RAND;
  return NO_SUCH_MODIFIER;
}

int superman::getNumTypes() const
{
  return last_type;
}

const type* superman::getTypeNumber(int i) const
{
  CHECK_RANGE(0, i, last_type);
  return reg_type[i];
}


//  
//  
// Promotions and casting
//  
//  

void superman::registerConversion(general_conv *c)
{
  if (0==c) return;
  c->em = this;
  general_rules.Append(c);
}

void superman::registerConversion(specific_conv *c)
{
  if (0==c) return;
  c->em = this;
  if (c->isPromotion())  promotion_rules.Append(c);
  else                   casting_rules.Append(c);
}


inline const general_conv* findRule(const List <general_conv> &G,
      const type* oldt, const type* newt)
{
  DCASSERT(oldt);
  DCASSERT(newt);
  for (long i=0; i < G.Length(); i++) {
    const general_conv* g = G.ReadItem(i);
    DCASSERT(g);
    if (g->getDistance(oldt, newt) >= 0) return g;
  } // for i
  return 0;
}

inline bool findRules(const List <general_conv> &G, 
    const List <specific_conv> &S,
    const type* oldt, const type* &midt, const type* newt, 
    const general_conv* &gp, const specific_conv* &sp)
{
  DCASSERT(oldt);
  DCASSERT(newt);
  
  for (long j=0; j < S.Length(); j++) {
    sp = S.ReadItem(j);
    DCASSERT(sp);
    int d = sp->getDistance(oldt);
    if (d < 0) continue;

    midt = sp->promotesTo(oldt);
    DCASSERT(midt);
    if (midt == newt) {
      gp = 0;
      return true;
    }

    gp = findRule(G, midt, newt);
    if (gp) return true;

  } // for j

  sp = 0;
  midt = 0;
  return false;
}

int superman::getPromoteDistance(const type* t1, const type* t2) const
{
  if (0==t1 || 0==t2) return -1;
  if (t1 == t2) return 0;

  const general_conv* g = findRule(general_rules, t1, t2);
  if (g) return g->getDistance(t1, t2);

  const specific_conv* s = 0;
  const type* t1mod = 0;
  if (!findRules(general_rules, promotion_rules, t1, t1mod, t2, g, s)) 
  return -1;

  DCASSERT(s);
  int d = s->getDistance(t1);
  DCASSERT(d>=0);
  if (0==g) return d;
  int dg = g->getDistance(t1mod, t2);
  DCASSERT(dg>=0);
  return d+dg;
}

bool superman::isCastable(const type* t1, const type* t2) const
{
  if (isPromotable(t1, t2)) return true;

  // try specific casts plus generic rules
  const specific_conv* s = 0;
  const general_conv* g = 0;
  const type* midt = 0;
  return findRules(general_rules, casting_rules, t1, midt, t2, g, s);
}

inline expr* convert(const char* file, int line, expr* e, 
      const type* t1mod, const type* newtype, 
      const general_conv* g, const specific_conv* s)
{
  if (0==g) 
   return s->convert(file, line, e, newtype);

  if (!g->requiresConversion(t1mod, newtype))
  return s->convert(file, line, e, newtype);

  expr* me = s->convert(file, line, e, t1mod);
  return g->convert(file, line, me, newtype);
}


expr* superman::makeTypecast(const char* file, int line, 
      const type* newtype, expr* e) const
{
  if (!isOrdinary(e))  return e;
  if (0==newtype) {
    Delete(e);
    return makeError();
  }
  const type* oldt = e->Type();
  DCASSERT(oldt);

  if (0==getPromoteDistance(oldt, newtype))  return e;

  // Try generic rules 
  const general_conv* g = findRule(general_rules, oldt, newtype);
  if (g) return g->convert(file, line, e, newtype);

  // Try promotion rules
  const specific_conv* s = 0;
  const type* midt = 0;
  if (findRules(general_rules, promotion_rules, oldt, midt, newtype, g, s)) {
    return convert(file, line, e, midt, newtype, g, s);
  }

  // Try casting rules
  if (findRules(general_rules, casting_rules, oldt, midt, newtype, g, s)) {
    return convert(file, line, e, midt, newtype, g, s);
  }

  // Slipped through the cracks?  Must be impossible.
  Delete(e);
  return makeError();
}

//  
//  
// Registering operations
//  
//  

bool superman::registerOperation(unary_op* op)
{
  if (0==op)    return false;
  op->em = this;
  unary_opcode u = op->getOpcode();
  if (uop_none == u)  return false;
  op->next = reg_unary[u];
  reg_unary[u] = op;
  return true;
}

bool superman::registerOperation(binary_op* op)
{
  if (0==op)    return false;
  op->em = this;
  binary_opcode b = op->getOpcode();
  if (bop_none == b)  return false;
  op->next = reg_binary[b];
  reg_binary[b] = op;
  return true;
}

bool superman::registerOperation(trinary_op* op)
{
  if (0==op)    return false;
  op->em = this;
  trinary_opcode t = op->getOpcode();
  if (top_none == t)  return false;
  op->next = reg_trinary[t];
  reg_trinary[t] = op;
  return true;
}

bool superman::registerOperation(assoc_op* op)
{
  if (0==op)    return false;
  op->em = this;
  assoc_opcode a = op->getOpcode();
  if (aop_none == a)  return false;
  op->next = reg_assoc[a];
  reg_assoc[a] = op;
  return true;
}

//  
//  
// Building expressions with operators
//  
//  

const type* superman::getTypeOf(unary_opcode op, const type* x) const
{
  const unary_op* list;
  if (op != uop_none) list = reg_unary[op];
  else                list = 0;
  
  const unary_op* match = 0;
  int num_matches = 0;
  for (const unary_op* ptr = list; ptr; ptr=ptr->next) {
    if (! ptr->isDefinedForType(x)) continue;
    match = ptr;
    num_matches++;
  } // for all operations with this code

  if (0==num_matches)  return 0;
  if (1==num_matches)  return match->getExprType(x);

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      noCause();
      internal() << "Cannot decide on unary operation: ";
      internal() << getOp(op) << " ";
      if (x)  internal() << x->getName();
      else    internal() << "notype";
      stopIO();
  }
  return 0;
}

const type* superman
::getTypeOf(const type* lt, binary_opcode op, const type* rt) const
{
  const binary_op* list;
  if (op != bop_none) list = reg_binary[op];
  else                list = 0;
  
  const binary_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const binary_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(lt, rt);
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches)  return 0;
  if (1==num_matches)  return match->getExprType(lt, rt);

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      noCause();
      internal() << "Cannot decide on binary operation: ";
      if (lt) internal() << lt->getName();
      else    internal() << "notype";
      internal() << " " << getOp(op) << " ";
      if (rt) internal() << rt->getName();
      else    internal() << "notype";
      stopIO();
  }
  return 0;
}

const type* superman::getTypeOf(trinary_opcode op, const type* lt,
      const type* mt, const type* rt) const
{
  const trinary_op* list;
  if (op != top_none) list = reg_trinary[op];
  else                list = 0;
  
  const trinary_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const trinary_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(lt, mt, rt);
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches)  return 0;
  if (1==num_matches)  return match->getExprType(lt, mt, rt);

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      noCause();
      internal() << "Cannot decide on trinary operation: ";
      if (lt) internal() << lt->getName();
      else    internal() << "notype";
      internal() << " " << getFirst(op) << " ";
      if (mt) internal() << mt->getName();
      else    internal() << "notype";
      internal() << " " << getSecond(op) << " ";
      if (rt) internal() << rt->getName();
      else    internal() << "notype";
      stopIO();
  }
  return 0;
}

const type* superman
::getTypeOf(const type* lt, bool flip, assoc_opcode op, const type* rt) const
{
  const assoc_op* list;
  if (op != aop_none) list = reg_assoc[op];
  else                list = 0;
  
  const assoc_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const assoc_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(flip, lt, rt);
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches)  return 0;
  if (1==num_matches)  return match->getExprType(flip, lt, rt);

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      noCause();
      internal() << "Cannot decide on associative operation: ";
      if (lt) internal() << lt->getName();
      else    internal() << "notype";
      internal() << " " << getOp(flip, op) << " ";
      if (rt) internal() << rt->getName();
      else    internal() << "notype";
      stopIO();
  }
  return 0;
}



expr* superman::makeUnaryOp(const char* file, int line, 
      unary_opcode op, expr* opnd) const
{
  if (!isOrdinary(opnd))  return opnd;  // null, error, or default
  
  const unary_op* list;
  if (op != uop_none) 
  list = reg_unary[op];
  else
  list = 0;
  
  const unary_op* match = 0;
  int num_matches = 0;
  for (const unary_op* ptr = list; ptr; ptr=ptr->next) {
    if (! ptr->isDefinedForType(opnd->Type())) continue;
    match = ptr;
    num_matches++;
  } // for all operations with this code

  if (0==num_matches) {
    if (startError()) {
      causedBy(file, line);
      cerr() << "Undefined unary operation: ";
      cerr() << getOp(op) << " ";
      opnd->PrintType(cerr());
      stopIO();
    }
    Delete(opnd);
    return makeError();
  }

  if (1==num_matches) {
    DCASSERT(match);
    expr* answer = match->makeExpr(file, line, opnd);
    if (0==answer && startInternal(__FILE__, __LINE__)) {
      causedBy(file, line);
      internal() << "Couldn't build unary expression for " << getOp(op);
      stopIO();
    }
    return answer;
  }

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      causedBy(file, line);
      internal() << "Cannot decide on unary operation: ";
      internal() << getOp(op) << " ";
      opnd->PrintType(internal());
      stopIO();
  }
  return 0;
}

expr* superman::makeBinaryOp(const char* fn, int ln, 
      expr* lt, binary_opcode op, expr* rt) const
{
  if (0==lt || 0==rt) {
    Delete(lt);
    Delete(rt);
    return 0;
  }
  if (isError(lt) || isError(rt)) {
    Delete(lt);
    Delete(rt);
    return makeError();
  }
  DCASSERT(isOrdinary(lt));
  DCASSERT(isOrdinary(rt));

  const binary_op* list;
  if (op != bop_none) 
  list = reg_binary[op];
  else
  list = 0;
  
  const binary_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const binary_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(lt->Type(), rt->Type());
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches) {
    if (startError()) {
      causedBy(fn, ln);
      cerr() << "Undefined binary operation: ";
      lt->PrintType(cerr());
      cerr() << " " << getOp(op) << " ";
      rt->PrintType(cerr());
      stopIO();
    }
    Delete(lt);
    Delete(rt);
    return makeError();
  }

  if (1==num_matches) {
    DCASSERT(match);
    expr* answer = match->makeExpr(fn, ln, lt, rt);
    if (0==answer && startInternal(__FILE__, __LINE__)) {
      causedBy(fn, ln);
      internal() << "Couldn't build binary expression for " << getOp(op);
      stopIO();
    }
    return answer;
  }

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
    causedBy(fn, ln);
    internal() << "Cannot decide on binary operation: ";
    lt->PrintType(internal());
    internal() << " " << getOp(op) << " ";
    rt->PrintType(internal());
    stopIO();
  }
  return 0;
}

expr* superman::makeTrinaryOp(const char* fn, int ln, trinary_opcode op, 
        expr* l, expr* m, expr* r) const
{
  if (0==l || 0==m || 0==r) {
    Delete(l);
    Delete(m);
    Delete(r);
    return 0;
  }
  if (isError(l) || isError(m) || isError(r)) {
    Delete(l);
    Delete(m);
    Delete(r);
    return makeError();
  }
  DCASSERT(isOrdinary(l));
  DCASSERT(isOrdinary(m));
  DCASSERT(isOrdinary(r));

  const trinary_op* list;
  if (op != top_none) 
  list = reg_trinary[op];
  else
  list = 0;
  
  const trinary_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const trinary_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(l->Type(), m->Type(), r->Type());
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches) {
    if (startError()) {
      causedBy(fn, ln);
      cerr() << "Undefined trinary operation: ";
      l->PrintType(cerr());
      cerr() << " " << getFirst(op) << " ";
      m->PrintType(cerr());
      cerr() << " " << getSecond(op) << " ";
      r->PrintType(cerr());
      stopIO();
    }
    Delete(l);
    Delete(m);
    Delete(r);
    return makeError();
  }

  if (1==num_matches) {
    DCASSERT(match);
    expr* answer = match->makeExpr(fn, ln, l, m, r);
    if (0==answer && startInternal(__FILE__, __LINE__)) {
      causedBy(fn, ln);
      internal() << "Couldn't build trinary expression for ";
      internal() << getFirst(op) << " ";
      internal() << getSecond(op);
      stopIO();
    }
    return answer;
  }

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
    causedBy(fn, ln);
    internal() << "Cannot decide on trinary operation: ";
    l->PrintType(internal());
    cerr() << " " << getFirst(op) << " ";
    m->PrintType(cerr());
    cerr() << " " << getSecond(op) << " ";
    r->PrintType(cerr());
    stopIO();
  }
  return 0;
}

expr* superman::makeAssocOp(const char* fn, int ln, assoc_opcode op, 
        expr** opnds, bool* flip, int N) const
{
  bool has_null = false;
  bool has_error = false;
  for (int i=0; i<N; i++) {
    if (0==opnds[i]) {
        has_null = true;
        continue;
    }
    if (isError(opnds[i])) {
        has_error = true;
        continue;
    }
    DCASSERT(isOrdinary(opnds[i]));
  }
  if ( (op != aop_colon && has_null) || has_error ) {
    for (int i=0; i<N; i++) Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (op != aop_colon && has_null)  return 0;
    return makeError();
  }

  const assoc_op* list;
  if (op != aop_none) 
  list = reg_assoc[op];
  else
  list = 0;
  
  const assoc_op* match = 0;
  int best_match = -1;
  int num_matches = 0;
  for (const assoc_op* ptr = list; ptr; ptr=ptr->next) {
    int d = ptr->getPromoteDistance(opnds, flip, N);
    if (d<0)  continue;
    if (best_match >= 0) {
      if (d > best_match)  continue;
      if (d == best_match) {
        num_matches++;
        continue;
      }
    }
    num_matches = 1;
    best_match = d;
    match = ptr;
  } // for all operations with this code

  if (0==num_matches) {
    if (startError()) {
      causedBy(fn, ln);
      cerr() << "Undefined associative operation: ";
      if (opnds[0])  opnds[0]->PrintType(cerr());
      else    cerr() << NULTYPE->getName();
      for (int i=1; i<N; i++) {
        bool f = flip ? flip[i] : 0;
        cerr() << " " << getOp(f, op) << " ";
        if (opnds[i]) opnds[i]->PrintType(cerr());
        else          cerr() << NULTYPE->getName();
      }
      stopIO();
    }
    for (int i=0; i<N; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    return makeError();
  }

  if (1==num_matches) {
    DCASSERT(match);
    expr* answer = match->makeExpr(fn, ln, opnds, flip, N);
    if (0==answer && startInternal(__FILE__, __LINE__)) {
      causedBy(fn, ln);
      internal() << "Couldn't build associative expression for ";
      internal() << getOp(0, op);
      stopIO();
    }
    return answer;
  }

  // too many matches, this should not happen!
  if (startInternal(__FILE__, __LINE__)) {
      causedBy(fn, ln);
      internal() << "Cannot decide on associative operation: ";
      if (opnds[0])   opnds[0]->PrintType(internal());
      else            internal() << NULTYPE->getName();
      for (int i=1; i<N; i++) {
        bool f = flip ? flip[i] : 0;
        internal() << " " << getOp(f, op) << " ";
        if (opnds[i])   opnds[i]->PrintType(internal());
        else            internal() << NULTYPE->getName();
      }
      stopIO();
  }
  return 0;
}

//  
//  
// Solution  engines
//  
//  

bool superman::registerEngineType(engtype* et)
{
  if (is_finalized)  return false;
  DCASSERT(ETTree);
  et->setIndex(ETTree->NumElements());
  engtype* foo = ETTree->Insert(et);
  if (foo != et) {
    // duplicate!
    delete et;
    return false;
  }
  return true;
}

engtype* superman::findEngineType(const char* n) const
{
  if (0==n) return 0;
  if (ETTree) {
    // not finalized
    return ETTree->Find(n);
  }
  // finalized
  int low = 0;
  int high = num_ets;
  while (low < high) {
    int mid = (low + high)/2;
    CHECK_RANGE(0, mid, num_ets);
    int cmp = ETList[mid]->Compare(n);
    if (0==cmp) return ETList[mid];
    if (cmp<0)  low  = mid+1;
    else        high = mid;
  }
  return 0;
}


int superman::getNumEngineTypes() const
{
  return num_ets;
}

const engtype* superman::getEngineTypeNumber(int i) const
{
  DCASSERT(ETList);
  CHECK_RANGE(0, i, num_ets);
  return ETList[i];
}

//  
//  
// Supporting  libraries
//  
//  

char superman::registerLibrary(const library* lib)
{
  if (0==lib)         return 1;
  if (isFinalized())  return 2;
  // first, check for duplicates
  const char* libv = lib->getVersionString();
  if (0==libv)  return 3;
  if (lib->hasFixedPointer()) {
    // We can use a fast check... 
    for (int i=0; i<num_libs; i++)
      if (extlibs[i]->getVersionString() == libv)
    return 4;
  } else {
    // darn, gotta use strcmp
    for (int i=0; i<num_libs; i++)
      if (0==strcmp(libv, extlibs[i]->getVersionString()))
    return 4;
  }
  // still here? register the library
  if (num_libs >= max_libs) {
    max_libs += 16;
    extlibs = (const library**) realloc(extlibs, max_libs * sizeof(void*));
  }
  extlibs[num_libs] = lib;
  num_libs++;
  return 0;
}

void superman::printLibraryVersions(OutputStream &s) const
{
  for (int i=0; i<num_libs; i++) {
    const char* v = extlibs[i]->getVersionString();
    DCASSERT(v);
    s << "\t" << v << "\n";
  }
}

void superman::printLibraryCopyrights(doc_formatter* df) const
{
  for (int i=0; i<num_libs; i++) {
    DCASSERT(extlibs[i]);
    if (!extlibs[i]->hasCopyright()) continue;
    df->Out() << "\n";
    df->begin_heading();
    const char* v = extlibs[i]->getVersionString();
    DCASSERT(v);
    df->Out() << v;
    df->end_heading();
    extlibs[i]->printCopyright(df);
  }
}



