
#include "basefuncs.h"

#include "../ExprLib/startup.h"
#include "../ExprLib/intervals.h"
#include "../ExprLib/functions.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/exprman.h"
#include "../Streams/strings.h"
#include "../ExprLib/mod_inst.h"
#include <string.h>

// ******************************************************************
// *                        delete_si  class                        *
// ******************************************************************

class delete_si : public simple_internal {
public:
  delete_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

delete_si::delete_si() : simple_internal(em->VOID, "delete", 1)
{
  SetFormal(0, em->MODEL, "x");
  SetDocumentation("Free resources used by a model.  This should be done after the model is no longer needed.");
}

void delete_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (x.stopExecution()) return;
  DCASSERT(0==x.aggregate);
  DCASSERT(np==1);
  result* answer = x.answer;
  result foo;
  x.answer = &foo;
  SafeCompute(pass[0], x);
  x.answer = answer;
  if (foo.isNormal()) {
    model_instance* mi = smart_cast<model_instance*> (foo.getPtr());
    DCASSERT(mi);
    mi->Deconstruct();
  }
}

// ******************************************************************
// *                        substr_si  class                        *
// ******************************************************************

class substr_si : public simple_internal {
  shared_string* empty_string;
public:
  substr_si();
  virtual void Compute(traverse_data &x, expr** pass, int np);
};

substr_si::substr_si() : simple_internal(em->STRING, "substr", 3)
{
  empty_string = new shared_string(strdup(""));
  SetFormal(0, em->STRING, "x");
  SetFormal(1, em->INT, "left");
  SetFormal(2, em->INT, "right");
  SetDocumentation("Get the substring between (and including) elements left and right of string x.  If left>right, returns the empty string.");
}

void substr_si::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  DCASSERT(np==3);
  result start;
  result stop;
  result* answer = x.answer;

  SafeCompute(pass[0], x);
  if (!answer->isNormal()) return;
  shared_string *ss = smart_cast <shared_string*> (answer->getPtr());
  DCASSERT(ss);
  const char* src = ss->getStr();
  long srclen = strlen(src);

  x.answer = &start;
  SafeCompute(pass[1], x);
  x.answer = answer;
  if (start.isInfinity()) {
    if (start.signInfinity()>0) {
      // +infinity: result is empty string
      answer->setPtr(Share(empty_string));
      return;
    } else {
      // -infinity is the same as 0 here
      start.setInt(0);
    }
  }
  if (!start.isNormal()) {
    *answer = start;
    return;
  }

  x.answer = &stop;
  SafeCompute(pass[2], x);
  x.answer = answer;
  if (stop.isInfinity()) {
    if (stop.signInfinity()>0) {
      // +infinity is the same as the string length
      stop.setInt(srclen);
    } else {
      // -infinity: stop is less than start, this gives empty string
      answer->setPtr(Share(empty_string));
      return;
    }
  }
  if (!stop.isNormal()) {
    *answer = stop;
    return;
  }

  // still here? stop and start are both finite, ordinary integers
  long stopi = stop.getInt();
  long starti = start.getInt();

  if (stopi < 0 || starti > srclen || starti > stopi) {
    // definitely empty string
    answer->setPtr(Share(empty_string));
    return;
  }

  if (starti < 0)       starti = 0;
  if (stopi > srclen)   stopi = srclen;
  // is it the full string?
  if ((0==starti) && (srclen==stopi)) {
    // x is the string parameter, keep it that way
    return;
  }
  // we are a proper substring, fill it
  char* sub = new char[stopi - starti+2];
  strncpy(sub, src + starti, 1+(stopi-starti));
  sub[stopi - starti + 1] = 0;
  answer->setPtr(new shared_string(sub));
}

// ******************************************************************
// *                         is_null  class                         *
// ******************************************************************

class is_null : public custom_internal {
public:
  is_null();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  int Substitute(traverse_data &x, expr** pass, int np) const;
};

is_null::is_null() : custom_internal("is_null", "bool is_null(x)")
{
  SetDocumentation("Returns true if x is null, false otherwise.");
}

void is_null::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  SafeCompute(pass[0], x);
  if (x.answer->isNull()) {
    x.answer->setBool(true);
  } else {
    x.answer->setBool(false);
  }
}

int is_null::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = em->BOOL;
        return 0;

    case traverse_data::Substitute:
        return Substitute(x, pass, np);

    case traverse_data::Typecheck:
        if (np<1)  return NotEnoughParams(np);
        if (np>1)  return TooManyParams(np);
        if (em->isError(pass[0]) || em->isDefault(pass[0]))
            return BadParam(0, np);
        return 0;

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

int is_null::Substitute(traverse_data &x, expr** pass, int np) const
{
  return 0;
}

// ******************************************************************
// *                         compute  class                         *
// ******************************************************************

class compute : public custom_internal {
public:
  compute();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};

compute::compute() : custom_internal("compute", "void compute(x)")
{
  SetDocumentation("Force computation of the given parameter.  Useful for model measures and expressions involving user input.");
}

void compute::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  SafeCompute(pass[0], x);
}

int compute::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = em->VOID;
        return 0;

    case traverse_data::Typecheck:
        if (np<1)  return NotEnoughParams(np);
        if (np>1)  return TooManyParams(np);
        if (em->isError(pass[0]) || em->isDefault(pass[0]))
            return BadParam(0, np);
        return 0;

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

// ******************************************************************
// *                         cond_ci  class                         *
// ******************************************************************

class cond_ci : public custom_internal {
public:
  cond_ci();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  const type* ReturnType(expr** pass, int np) const;
  int Typecheck(expr** pass, int np) const;
  int PromoteParams(expr** pass, int np) const;
  int Substitute(traverse_data &x, expr** pass, int np) const;
  inline const type* ArgType(expr* arg) const {
    if (0==arg)  return em->NULTYPE;
    if (arg->NumComponents()>1)  return 0;
    return arg->Type();
  }

  interval_object* getInterval(traverse_data &x, expr* p) const;
};

cond_ci::cond_ci() : custom_internal("cond", "cond(b, t, f)")
{
  SetDocumentation("If the boolean condition b is true, then t is returned; otherwise, f is returned.");
}

void cond_ci::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  if (0==pass[0]) {
    x.answer->setNull();
    return;
  }
  pass[0]->Compute(x);
  if (x.answer->isNormal()) {
    if (x.answer->getBool()) {
      SafeCompute(pass[1], x);
    } else {
      SafeCompute(pass[2], x);
    }
  }
}

int cond_ci::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = ReturnType(pass, np);
        if (pass[1])  x.the_model_type = pass[1]->GetModelType();
        return 0;

    case traverse_data::Typecheck:
        return Typecheck(pass, np);

    case traverse_data::Promote:
        return PromoteParams(pass, np);

    case traverse_data::Substitute:
        return Substitute(x, pass, np);

    case traverse_data::FindRange: {
        DCASSERT(x.answer);
        interval_object *a = getInterval(x, pass[1]);
        interval_object *b = getInterval(x, pass[2]);
        interval_object *ans = new interval_object;
        computeUnion(*ans, *a, *b);
        Delete(a);
        Delete(b);
        x.answer->setPtr(ans);
        return 0;
    }

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

const type* cond_ci::ReturnType(expr** pass, int np) const
{
  DCASSERT(np > 2);
  const type* t0 = ArgType(pass[0]);
  const type* t1 = ArgType(pass[1]);
  const type* t2 = ArgType(pass[2]);
  const type* ret = em->getLeastCommonType(t1, t2);
  modifier m0 = GetModifier(t0);
  modifier m12 = GetModifier(ret);
  if (m0 != m12)  m12 = RAND;
  bool proc0 = HasProc(t0);
  bool proc12 = HasProc(ret);
  if (proc0 != proc12)  proc12 = true;
  ret = GetBase(ret);
  ret = ModifyType(m12, ret);
  if (proc12)  ret = ProcifyType(ret);
  return ret;
}

int cond_ci::Typecheck(expr** pass, int np) const
{
  if (np<3)   return NotEnoughParams(np);
  if (np>3)   return TooManyParams(np);
  const type* args = ReturnType(pass, np);
  const type* test = args ? args->changeBaseType(em->BOOL) : 0;
  int d0 = em->getPromoteDistance(ArgType(pass[0]), test);
  if (d0 < 0)  return BadParam(0, np);
  int d1 = em->getPromoteDistance(ArgType(pass[1]), args);
  if (d1 < 0)  return BadParam(1, np);
  int d2 = em->getPromoteDistance(ArgType(pass[2]), args);
  if (d2 < 0)  return BadParam(2, np);
  return d0 + d1 + d2;
}

int cond_ci::PromoteParams(expr** pass, int np) const
{
  const type* args = ReturnType(pass, np);

  // const type* test = args ? args->changeBaseType(em->BOOL) : 0;
  // pass[0] = em->promote(pass[0], test);  // Don't think we need to!
  pass[1] = em->promote(pass[1], args);
  pass[2] = em->promote(pass[2], args);

  const model_def* mt1 = pass[1] ? pass[1]->GetModelType() : 0;
  const model_def* mt2 = pass[2] ? pass[2]->GetModelType() : 0;

  if (mt1 == mt2)   return Promote_Success;
  else              return Promote_MTMismatch;
}

int cond_ci::Substitute(traverse_data &x, expr** pass, int np) const
{
  DCASSERT(x.answer);
  if (0==pass[0]) {
    Delete(pass[1]);
    Delete(pass[2]);
    delete[] pass;
    x.answer->setPtr(0);
    return 1;
  }
  if (GetModifier(pass[0]->Type()) != DETERM)  return 0;
  if (pass[0]->BuildExprList(traverse_data::GetSymbols, 0, 0))    return 0;
  // pass[0] is a "constant", compute it
  if (pass[0]->Type()->hasProc()) {
    pass[0]->PreCompute();
  }
  traverse_data y(traverse_data::Compute);
  result foo;
  y.answer = &foo;
  pass[0]->Compute(y);
  if (! foo.isNormal())  return 0;
  if (foo.getBool())     x.answer->setPtr(Share(pass[1]));
  else                   x.answer->setPtr(Share(pass[2]));
  Delete(pass[0]);
  Delete(pass[1]);
  Delete(pass[2]);
  delete[] pass;
  return 1;
}

interval_object* cond_ci::getInterval(traverse_data &x, expr* p) const
{
  DCASSERT(x.answer);
  interval_object* a = 0;
  if (p) {
    p->Traverse(x);
    if (x.answer->isNormal())
      a = Share(smart_cast <interval_object*> (x.answer->getPtr()));
  }
  if (0==a) {
    a = new interval_object;
    a->Left().setNull();
    a->Right().setNull();
  }
  return a;
}

// ******************************************************************
// *                         case_ci  class                         *
// ******************************************************************

class case_ci : public custom_internal {
public:
  case_ci();
  virtual void Compute(traverse_data &x, expr** pass, int np);
  virtual int Traverse(traverse_data &x, expr** pass, int np);
  const type* ReturnType(expr** pass, int np) const;
  int Typecheck(expr** pass, int np) const;
  int PromoteParams(expr** pass, int np) const;
  int Substitute(traverse_data &x, expr** pass, int np) const;
};

case_ci::case_ci() : custom_internal("case", "case(int c, type d, ..., int:type c_i:v_i, ...)")
{
  SetDocumentation("Returns the first v_i such that c_i equals c, if one exists; otherwise, returns d.");
}

void case_ci::Compute(traverse_data &x, expr** pass, int np)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  result* answer = x.answer;
  result key;
  x.answer = &key;
  SafeCompute(pass[0], x);
  x.answer = answer;

  const type* t = em->INT;
  DCASSERT(t);
  for (int i=2; i<np; i++) {
    SafeCompute(pass[i], x);

    if (t->equals(*answer, key)) {
      x.aggregate = 1;
      SafeCompute(pass[i], x);
      x.aggregate = 0;
      return;
    }
  } // for i
  // not found in list, use the default
  SafeCompute(pass[1], x);
}

int case_ci::Traverse(traverse_data &x, expr** pass, int np)
{
  switch (x.which) {
    case traverse_data::GetType:
        x.the_type = ReturnType(pass, np);
        if (pass[1])  x.the_model_type = pass[1]->GetModelType();
        return 0;

    case traverse_data::Typecheck:
        return Typecheck(pass, np);

    case traverse_data::Promote:
        return PromoteParams(pass, np);

    case traverse_data::Substitute:
        return Substitute(x, pass, np);

    default:
        return custom_internal::Traverse(x, pass, np);
  }
}

const type* case_ci::ReturnType(expr** pass, int np) const
{
  if (np < 2)  return 0;
  // Get "selector" type, must be some kind of INT...
  const type* t = em->SafeType(pass[0]);

  bool rand = DETERM != GetModifier(t);
  bool proc = HasProc(t);

  // Get default return type, ANY
  const type* rettype = em->SafeType(pass[1]);

  // The remaining arguments are INT : ANY,
  // as long as the entire collection of "ANY" has
  // a common type to be promoted to.
  for (int i=2; i<np; i++) {
    if (0==pass[i])  continue; // we can deal with null.
    DCASSERT(pass[i]->NumComponents() == 2);
    t = em->SafeType(pass[i], 0);
    DCASSERT((em->NULTYPE==t) || (em->INT == t));
    t = em->SafeType(pass[i], 1);
    rettype = em->getLeastCommonType(rettype, t);
  }
  rand |= DETERM != GetModifier(rettype);
  proc |= HasProc(rettype);
  rettype = GetBase(rettype);
  if (rand)  rettype = ModifyType(RAND, rettype);
  if (proc)  rettype = ProcifyType(rettype);
  return rettype;
}

int case_ci::Typecheck(expr** pass, int np) const
{
  if (np < 2)  return NotEnoughParams(np);
  const type* t;

  // Get "selector" type, must be some kind of INT...
  if (pass[0]) {
    if (pass[0]->NumComponents() != 1)  return BadParam(0, np);
    t = pass[0]->Type();
    if (GetBase(t) != em->INT)  return BadParam(0, np);
  } else {
    // ... or null
    t = em->NULTYPE;
  }

  bool rand = DETERM != GetModifier(t);
  bool proc = HasProc(t);

  // Get default return type, ANY
  const type* rettype;
  if (pass[1]) {
    if (pass[1]->NumComponents() != 1)  return BadParam(1, np);
    rettype = pass[1]->Type();
  } else {
    rettype = em->NULTYPE;
  }

  // The remaining arguments are INT : ANY,
  // as long as the entire collection of "ANY" has
  // a common type to be promoted to.
  for (int i=2; i<np; i++) {
    if (0==pass[i])  continue; // we can deal with null.
    if (pass[i]->NumComponents() != 2)  return BadParam(i, np);
    t = em->SafeType(pass[i], 0);
    if ((t != em->NULTYPE) && (t != em->INT))  return BadParam(i, np);
    t = em->SafeType(pass[i], 1);
    rettype = em->getLeastCommonType(rettype, t);
    if (0 == rettype)  return BadParam(i, np);
  }
  rand |= DETERM != GetModifier(rettype);
  proc |= HasProc(rettype);
  rettype = GetBase(rettype);
  if (rand)  rettype = ModifyType(RAND, rettype);
  if (proc)  rettype = ProcifyType(rettype);

  // Ok, this is a valid function call, let's determine the total score
  t = em->SafeType(pass[0]);
  int score = em->getPromoteDistance(t, ApplyPM(rettype, em->INT));
  DCASSERT(score >= 0);

  int pd = em->getPromoteDistance(em->SafeType(pass[1]), rettype);
  DCASSERT(pd >= 0);
  score += pd;

  for (int i=2; i<np; i++) {
    pd = em->getPromoteDistance(em->SafeType(pass[i], 0), em->INT);
    DCASSERT(pd>=0);
    score += pd;
    pd = em->getPromoteDistance(em->SafeType(pass[i], 1), rettype);
    DCASSERT(pd>=0);
    score += pd;
  }
  return score;
}

int case_ci::PromoteParams(expr** pass, int np) const
{
  const type* rettype = ReturnType(pass, np);

  pass[0] = em->promote(pass[0], ApplyPM(rettype, em->INT));
  pass[1] = em->promote(pass[1], rettype);

  const model_def* mt = pass[1] ? pass[1]->GetModelType() : 0;

  // A bit ugly, but by far the easiest way
  typelist* t = new typelist(2);
  t->SetItem(0, em->INT);
  t->SetItem(1, rettype);
  symbol* foo = MakeFormalParam(t, 0);
  for (int i=2; i<np; i++) {
    pass[i] = em->promote(pass[i], false, false, foo);
    expr* val = pass[i]->GetComponent(1);
    const model_def* val_mt = val ? val->GetModelType() : 0;
    if (mt != val_mt)  return Promote_MTMismatch;
  }
  Delete(foo);
  return Promote_Success;
}

int case_ci::Substitute(traverse_data &x, expr** pass, int np) const
{
  // TBD: fancy-pants substitute
  return 0;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// *                                                                *
// ******************************************************************

class init_basefuncs : public initializer {
  public:
    init_basefuncs();
    virtual bool execute();
};
init_basefuncs the_basefunc_initializer;

init_basefuncs::init_basefuncs() : initializer("init_basefuncs")
{
  usesResource("em");
  usesResource("st");
  usesResource("types");
}

bool init_basefuncs::execute()
{
  if (0==st || 0==em)  return false;

  st->AddSymbol(  new delete_si   );
  st->AddSymbol(  new substr_si   );
  st->AddSymbol(  new is_null     );
  st->AddSymbol(  new compute     );
  st->AddSymbol(  new cond_ci     );
  st->AddSymbol(  new case_ci     );

  return true;
}
