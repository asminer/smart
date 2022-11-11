
#include "symbols.h"
#include "exprman.h"
#include "../Utils/strings.h"
#include "../Streams/textfmt.h"
#include "values.h"
#include "dd_front.h"
#include "functions.h"
#include "../Options/options.h"

// ******************************************************************
// *                                                                *
// *                         symbol methods                         *
// *                                                                *
// ******************************************************************

symbol::symbol(const location &W, const type* t, char* n)
 : expr(W, t)
{
  name = (n) ? (new shared_string(n)) : 0;
  substitute_value = true;
  next = 0;
  waitlist = 0;
}

symbol::symbol(const location &W, typelist* t, char* n)
 : expr(W, t)
{
  name = (n) ? (new shared_string(n)) : 0;
  substitute_value = true;
  next = 0;
  waitlist = 0;
}

symbol::symbol(const symbol* wrapper) : expr(wrapper)
{
  if (wrapper) {
    name = Share( smart_cast <shared_string*> (wrapper->SharedName()) );
  }
  substitute_value = true;
  next = 0;
  waitlist = 0;
}

symbol::~symbol()
{
  Delete(name);
  delete waitlist;
}

const char* symbol::Name() const
{
  if (name)  return name->getStr();
  return 0;
}

shared_object* symbol::SharedName() const
{
  return name;
}

void symbol::Rename(shared_object* newname)
{
  if (name != newname) {
    Delete(name);
    name = Share( smart_cast <shared_string*> (newname) );
  }
}

bool symbol::Print(OutputStream &s, int width) const
{
  if (0==name) return false;
  s.Put(name->getStr(), width);
  return true;
}

void symbol::Traverse(traverse_data &x)
{
  DCASSERT(0==x.aggregate);
  switch (x.which) {
    case traverse_data::Substitute:
        DCASSERT(x.answer);
        if (!substitute_value) {
          x.answer->setPtr(Share(this));
        } else {
          traverse_data xx(traverse_data::Compute);
          result ans;
          xx.answer = &ans;
          Compute(xx);
          x.answer->setPtr(new value(Where(), Type(), ans));
        }
        return;

    case traverse_data::BuildDD: {
        DCASSERT(x.answer);
        DCASSERT(x.ddlib);
        shared_object* dd = x.ddlib->makeEdge(0);

        try {
          x.ddlib->buildSymbolicSV(this, false, 0, dd);
          x.answer->setPtr(dd);
        }
        catch (sv_encoder::error e) {
          if (em->startError()) {
            em->causedBy(this);
            em->cerr() << "Error while building state variable ";
            em->cerr() << Name() << ": ";
            em->cerr() << sv_encoder::getNameOfError(e);
            em->stopIO();
          }
          Delete(dd);
          x.answer->setNull();
        }
        return;
    }

    case traverse_data::GetVarDeps:
    case traverse_data::GetSymbols:
        if (x.elist)  x.elist->Append(this);
        if (x.slist)  x.slist->Append(this);
        DCASSERT(x.answer);
        x.answer->setInt(x.answer->getInt()+1);
        return;

    case traverse_data::PreCompute:
    case traverse_data::GetMeasures:
        return;

    default:
        expr::Traverse(x);
  }
}

void symbol::PrintDocs(doc_formatter* df, const char* keyword) const
{
  if (0==df)  return;
  if (0==name)  return;
  df->begin_heading();
  PrintType(df->Out());
  df->Out() << " " << name->getStr();
  df->end_heading();
  df->begin_indent();
  df->Out() << "Defined " << Where();
  df->end_indent();
}

void symbol::addToWaitList(symbol* w)
{
  DCASSERT(w);
  if (isComputed() || !OK()) return;
  if (waitlist_debug.startReport()) {
    waitlist_debug.report() << "Adding symbol ";
    if (w->Name()) waitlist_debug.report() << w->Name() << " ";
    waitlist_debug.report() << "to waiting list";
    if (Name()) waitlist_debug.report() << " of symbol " << Name();
    waitlist_debug.report() << "\n";
    waitlist_debug.stopIO();
  }
#ifdef DEVELOPMENT_CODE
  if (w->couldNotify(this)) {
    if (em->startInternal(__FILE__, __LINE__)) {
      em->causedBy(w);
      em->internal() << "Circular dependency in symbol waiting lists";
      em->newLine();
      em->internal() << "when adding symbol ";
      if (w->Name()) em->internal() << w->Name();
      em->stopIO();
    }
  }
#endif
  if (0==waitlist) waitlist = new List <symbol>;
  waitlist->Append(w);
  w->setBlocked();
}

void symbol::notifyFrom(const symbol* p)
{
  if (0==p) return;
  if (waitlist_debug.startReport()) {
    waitlist_debug.report() << "Notifying symbol ";
    if (Name()) waitlist_debug.report() << Name() << " ";
    if (p->Name()) waitlist_debug.report() << "from symbol " << p->Name();
    waitlist_debug.report() << "\n";
    waitlist_debug.stopIO();
  }
}

bool symbol::couldNotify(const symbol* s) const
{
  if (0==waitlist) return false;
  for (int i=0; i<waitlist->Length(); i++) {
    const symbol* item = waitlist->ReadItem(i);
    if (item==s) return true;
    if (item->couldNotify(s)) return true;
  }
  return false;
}

void symbol::notifyList()
{
  if (0==waitlist) return;
  for (int i=0; i<waitlist->Length(); i++)
    waitlist->Item(i)->notifyFrom(this);
  delete waitlist;
  waitlist = 0;
}

// ******************************************************************
// *                                                                *
// *                       help_topic methods                       *
// *                                                                *
// ******************************************************************

help_topic::help_topic(const char* n, const char* s)
 : symbol(location::NOWHERE(), (typelist*) 0, strdup(n))
{
  summary = s;
}

help_topic::help_topic()
 : symbol(location::NOWHERE(), (typelist*) 0, 0)
{
  summary = 0;
}

help_topic::~help_topic()
{
}

void help_topic::setName(char* n)
{
  Rename(new shared_string(n));
}

void help_topic::PrintHeader(OutputStream &s) const
{
  s << "Help topic: ";
  s.Put(Name());
}

// ******************************************************************
// *                                                                *
// *                       help_group methods                       *
// *                                                                *
// ******************************************************************

help_group::help_group(const char* n, const char* s, const char* d)
 : help_topic(n, s)
{
  DCASSERT(d);
  docs = d;
}

help_group::~help_group()
{
}

void help_group::PrintDocs(doc_formatter* df, const char* keyword) const
{
  df->begin_heading();
  PrintHeader(df->Out());
  df->end_heading();
  df->begin_indent();
  df->Out() << docs << "\n";
  if (funcs.Length()) {
    df->Out() << "\nRelevant functions:\n";
    df->begin_indent();
    for (int i=0; i<funcs.Length(); i++) {
      const function* f = funcs.ReadItem(i);
      f->PrintHeader(df->Out(), true);
      df->Out() << "\n";
    }
    df->end_indent();
  }
  if (options.Length()) {
    df->Out() << "\nRelevant options:\n";
    df->begin_indent();
    for (int i=0; i<options.Length(); i++) {
      const option* o = options.ReadItem(i);
      o->PrintDocs(df, keyword);
    }
    df->end_indent();
  }
  df->end_indent();
}

// ******************************************************************
// *                                                                *
// *                        constfunc  class                        *
// *                                                                *
// ******************************************************************


/** Constant functions (with no parameters).
    These are used often as building blocks for more complex items.
    So, some functionality here is provided for derived classes.
 */
class constfunc : public symbol {
protected:
  /// The return expression for the function.
  expr* return_expr;
  /// The cached value
  result cache;
  /// Dependency list.
  List <symbol> *deplist;
public:
  constfunc(const location &W, const type* t, char *n, expr* rhs, List <symbol> *dl);
  constfunc(const symbol* wrap, expr* rhs, List <symbol> *dl);
  virtual ~constfunc();
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
protected:
  inline void Initialize(expr* rhs, List <symbol> *dl) {
      return_expr = rhs;
      SetSubstitution(false);
      cache.setNull();
      setDefined();
      deplist = dl;
      if (deplist) for (int i=0; i<deplist->Length(); i++) {
        deplist->Item(i)->addToWaitList(this);
      }
  }
};

// ******************************************************************
// *                                                                *
// *                       constfunc  methods                       *
// *                                                                *
// ******************************************************************

constfunc::constfunc(const location &W, const type* t, char* n,
  expr* rhs, List <symbol> *dl) : symbol(W, t, n)
{
  Initialize(rhs, dl);
}

constfunc::constfunc(const symbol* wrap, expr* rhs, List <symbol> *dl)
  : symbol(wrap)
{
  Initialize(rhs, dl);
}

constfunc::~constfunc()
{
  cache.deletePtr();
  Delete(return_expr);
  delete deplist;
}

void constfunc::Compute(traverse_data &x)
{
  DCASSERT(x.answer);
  DCASSERT(0==x.aggregate);
  if (WillSubstitute()) {
    *(x.answer) = cache;
    return;
  }
  if (deplist) {
    for (int i=0; i<deplist->Length(); i++) {
      symbol* item = deplist->Item(i);
      DCASSERT(item);
      if (item->isComputed()) continue;
      item->Compute(x);
    } // for i
    delete deplist;
    deplist = 0;
  } // if deplist
  SafeCompute(return_expr, x);
  // neat trick!!!
  if (em->MODEL == Type() && x.answer->getPtr()) {
    symbol* mi = smart_cast <symbol*> (x.answer->getPtr());
    DCASSERT(mi);
    mi->Rename(SharedName());
  }
  cache = *(x.answer);
  if (Type() != em->VOID) {
    SetSubstitution(true);
  }
}

void constfunc::Traverse(traverse_data &x)
{
  switch (x.which) {
    case traverse_data::ClearCache:
        SetSubstitution(false);
        return;

    case traverse_data::FindRange:
        DCASSERT(x.answer);
        if (return_expr)  return_expr->Traverse(x);
        else              x.answer->setNull();
        return;

    case traverse_data::GetSymbols:
    case traverse_data::Substitute:
        symbol::Traverse(x);
        return;

    case traverse_data::GetVarDeps:
        symbol::Traverse(x);
        if (0==Type()) return;
        if (Type()->getModifier() != PHASE) return;
        if (return_expr)  return_expr->Traverse(x);
        return;

    default:
        if (return_expr)  return_expr->Traverse(x);
  }
}

// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

symbol* exprman::makeConstant(const location &W, const type* t,
      char* name, expr* rhs, List <symbol> *deps) const
{
  if (0==t || isError(rhs)) {
    free(name);
    return 0;
  }

  const type* rhstype = SafeType(rhs);

  if (!isPromotable(rhstype, t)) {
    if (startError()) {
      causedBy(W);
      cerr() << "Return type for identifier ";
      if (name)   cerr() << name;
      else        cerr() << "(no name)";
      cerr() << " should be ";
      cerr() << t->getName();
      stopIO();
    }
    free(name);
    return 0;
  }
  rhs = promote(rhs, t);
  symbol* s = new constfunc(W, t, name, rhs, deps);
  if (rhs) s->SetModelType(rhs->GetModelType());
  if (s->OK())  return s;
  Delete(s);
  return 0;
}

symbol* exprman::makeConstant(const symbol* w,
      expr* rhs, List <symbol> *deps) const
{
  if (isError(rhs)) return 0;
  if (0==w)         return 0;

  const type* rhstype = SafeType(rhs);
  const type* t = SafeType(w);

  if (!isPromotable(rhstype, t)) {
    if (startError()) {
      causedBy(w);
      cerr() << "Return type for identifier ";
      if (w->Name())  cerr() << w->Name();
      else            cerr() << "(no name)";
      cerr() << " should be ";
      cerr() << t->getName();
      stopIO();
    }
    return 0;
  }
  rhs = promote(rhs, t);
  symbol* s = new constfunc(w, rhs, deps);
  if (rhs) s->SetModelType(rhs->GetModelType());
  if (s->OK())  return s;
  Delete(s);
  return 0;
}


