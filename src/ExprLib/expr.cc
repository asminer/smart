
/*
  Implementation of the base classes defined in exprlib.h
*/

#include "expr.h"
#include "symbols.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../include/list.h"
#include "type.h"
#include "result.h"
#include "exprman.h"

// ******************************************************************
// *                     traverse_data  methods                     *
// ******************************************************************

bool traverse_data::Print(OutputStream &s) const
{
  switch (which) {
    case None:
        s << "None";
        return true;

    case Compute:
        s << "Compute";
        return true;

    case ComputeExpoRate:
        s << "ComputeExpoRate";
        return true;

    case BuildDD:
        s << "BuildDD";
        return true;

    case BuildExpoRateDD:
        s << "BuildExpoRateDD";
        return true;

    case FindRange:
        s << "FindRange";
        return true;

    case PreCompute:
        s << "PreCompute";
        return true;

    case ClearCache:
        s << "ClearCache";
        return true;

    case Guess:
        s << "Guess";
        return true;

    case Update:
        s << "Update";
        return true;

    case Block:
        s << "Block";
        return true;

    case Affix:
        s << "Affix";
        return true;

    case Substitute:
        s << "Substitute";
        return true;

    case GetSymbols:
        s << "GetSymbols";
        return true;

    case GetVarDeps:
        s << "GetVarDeps";
        return true;

    case GetMeasures:
        s << "GetMeasures";
        return true;

    case GetProducts:
        s << "GetProducts";
        return true;

    case GetType:
        s << "GetType";
        return true;

    case Typecheck:
        s << "Typecheck";
        return true;

    case Promote:
        s << "Promote";
        return true;

    default:
        s << "Unknown traversal type";
  }
  return true;
}

// ******************************************************************
// *                          expr methods                          *
// ******************************************************************

const type* expr::STMT = 0;
named_msg expr::expr_debug;
named_msg expr::waitlist_debug;
named_msg expr::model_debug;
exprman* expr::em = 0;
int expr::global_IDnum = 0;

expr::expr(const char* fn, int line, const type* t) : shared_object()
{
  Init(fn, line, t, 0, 0);
}

expr::expr(const char* fn, int line, typelist* t) : shared_object()
{
  Init(fn, line, 0, t, 0);
}

expr::expr(const expr* x) : shared_object()
{
  if (x) {
    Init(x->Filename(), x->Linenumber(), x->simple, 
          Share(x->aggtype), x->model_type);
  } else {
    setNull();
  }
}

expr::~expr()
{
  Delete(aggtype);
}

void expr::Init(const char* fn, int ln, const type* st, typelist* at, const model_def* mt)
{
  global_IDnum++;
  if (global_IDnum < 0) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(fn, ln);
      em->internal() << "Too many expressions, global ID overflow";
      em->stopIO();
    }
  }
  IDnum = global_IDnum;
  filename = fn;
  linenumber = ln;
  simple = st;
  aggtype = at;
  model_type = mt;
  setOK();
}

void expr::SetType(const type* t)
{
  DCASSERT(0 == simple);
  DCASSERT(0 == aggtype);
  simple = t;
}

void expr::SetType(typelist* t)
{
  DCASSERT(0 == simple);
  DCASSERT(0 == aggtype);
  aggtype = t;
}

void expr::SetType(const expr* t)
{
  DCASSERT(0 == simple);
  DCASSERT(0 == aggtype);
  simple = t->simple;
  aggtype = Share(t->aggtype);
}

void expr::SetModelType(const model_def* mt)
{
  if (0==mt) return;
  DCASSERT(0 == aggtype);
  if (0 == simple) simple = em->MODEL;
  DCASSERT(em->MODEL == simple || simple->isAFormalism());
  DCASSERT(0 == model_type);
  model_type = mt;
}

expr* expr::GetComponent(int i)
{
  if (0==i) return this;
  return 0;
}

void expr::PrintType(OutputStream &s) const
{
  if (aggtype) {
    aggtype->Print(s, 0);
    return;
  }
  if (simple) {
    s.Put(simple->getName());
    return;
  }
  DCASSERT(0);
  s.Put("Unknown type");
}

void expr::Compute(traverse_data &x)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Trying to compute uncomputable expression: ";
    Print(em->internal(), 0);
    em->stopIO();
  }
}

const char* expr::Name() const
{
  return 0;
}

shared_object* expr::SharedName() const
{
  return 0;
}

void expr::Rename(shared_object* n)
{
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "Trying to rename an unnamed expression: ";
    Print(em->internal(), 0);
    em->internal() << " to: ";
    n->Print(em->internal(), 0);
    em->stopIO();
  }
}

bool expr::Matches(const expr* sym) const
{
  if (0==sym)        return 0;
  const char* myname = Name();
  const char* sname = sym->Name();
  if ((0==myname) != (0==sname))  return 0;
  if (myname) 
  if (strcmp(myname, sname))  return 0;
  if (aggtype)
    return aggtype->Equals(sym->aggtype);
  else
    return simple == sym->simple;
}

void expr::ClearCache()
{
  traverse_data foo(traverse_data::ClearCache);
  result bar;
  foo.answer = &bar;
  Traverse(foo);
}

void expr::PreCompute()
{
  traverse_data foo(traverse_data::PreCompute);
  result bar;
  foo.answer = &bar;
  Traverse(foo);
}

void expr::Affix()
{
  traverse_data foo(traverse_data::Affix);
  result bar;
  foo.answer = &bar;
  Traverse(foo);
}

expr* expr::Substitute(int i)
{
  traverse_data foo(traverse_data::Substitute);
  foo.aggregate = i;
  result answer;
  foo.answer = &answer;
  Traverse(foo);
  if (answer.isNull()) return 0; // can happen?
  DCASSERT(answer.getPtr());
  expr* sub = smart_cast <expr*> (answer.getPtr());
  return Share(sub);
}

expr* expr::Measurify(model_def* parent)
{
  traverse_data foo(traverse_data::Substitute);
  foo.model = parent;
  result answer;
  foo.answer = &answer;
  Traverse(foo);
  if (answer.isNull()) return 0; // can happen?
  DCASSERT(answer.getPtr());
  expr* sub = smart_cast <expr*> (answer.getPtr());
  return Share(sub);
}

int expr
::BuildExprList(traverse_data::traversal_type w, int i, List <expr> *list)
{
  traverse_data x(w);
  result ans(0L);
  x.answer = &ans;
  x.elist = list;
  Traverse(x);
  return ans.getInt();
}

int expr
::BuildSymbolList(traverse_data::traversal_type w, int i, List <symbol> *list)
{
  traverse_data x(w);
  result ans(0L);
  x.answer = &ans;
  x.slist = list;
  Traverse(x);
  return ans.getInt();
}

OutputStream& operator<< (OutputStream &s, const expr* e)
{
  if (e) e->Print(s, 0);
  return s;
}

void expr::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::None:
    case traverse_data::Block:
        return;

    case traverse_data::GetProducts:
        if (x.elist) {
          x.elist->Append(this);
        }
        x.answer->setInt(x.answer->getInt()+1);
        return;

    default:
        break;
  }
  // bail out
  DCASSERT(em);
  if (em->startInternal(__FILE__, __LINE__)) {
    em->noCause();
    em->internal() << "No traversal handler ";
    x.Print(em->internal());
    em->internal() << " for expression: ";
    Print(em->internal(), 0);
    em->stopIO();
  }
}

// Nice, conservative default.
bool expr::Equals(const shared_object* o) const
{
  if (o==this) return true;
  return false;
}

long expr::getDelta() const
{
  return 0;
}

long expr::getLower() const
{
  return 0;
}
