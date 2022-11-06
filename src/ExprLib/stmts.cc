
#include "stmts.h"
#include "../Utils/strings.h"
#include "../Streams/streams.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Options/radio_opt.h"
#include "../Options/checklist.h"
#include "expr.h"
#include "exprman.h"
#include "result.h"

#include <string.h>

#define ALLOW_NON_VOID_EXPRSTMT

inline const type* Opt2Type(const exprman* em, option::type ot)
{
  switch (ot) {
    case option::Boolean: return em->BOOL;
    case option::Integer: return em->INT;
    case option::Real:    return em->REAL;
    case option::String:  return em->STRING;
    default:              return 0;
  }
  return 0;
}

// ******************************************************************
// *                                                                *
// *                         exprstmt class                         *
// *                                                                *
// ******************************************************************

#ifdef ALLOW_NON_VOID_EXPRSTMT

/** An expression statement.
    Used for function calls, like "print".
 */
class exprstmt : public expr {
  expr *x;
public:
  exprstmt(const char* fn, int line, expr *e);
  virtual ~exprstmt();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

exprstmt::exprstmt(const char* fn, int line, expr *e) : expr(fn, line, STMT)
{
  x = e;
}

exprstmt::~exprstmt()
{
  Delete(x);
}

bool exprstmt::Print(OutputStream &s, int w) const
{
  s.Pad(' ', w);
  x->Print(s, 0);
  s.Put(";\n");
  return true;
}

void exprstmt::Compute(traverse_data &td)
{
  DCASSERT(em);
  DCASSERT(td.which == traverse_data::Compute);
  DCASSERT(td.answer);
  if (td.stopExecution())  return;
  result foo;
  result* old = td.answer;
  td.answer = &foo;
  SafeCompute(x, td);
  if (em->hasIO()) {
    em->cout() << "Evaluated statement ";
    x->Print(em->cout(), 0);
    em->cout() << ", got: ";
    DCASSERT(x->Type());
    x->Type()->print(em->cout(), td.answer[0]);
    em->cout() << "\n";
    em->cout().flush();
  }
  td.answer = old;
}

void exprstmt::Traverse(traverse_data &td)
{
  switch (td.which) {
    case traverse_data::Affix:
        return;

    default:
         if (x)  x->Traverse(td);
  }
}

#endif


// ******************************************************************
// *                                                                *
// *                      optassign_val  class                      *
// *                                                                *
// ******************************************************************

/** An option assignment from an expression.
 */
class optassign_val : public expr {
  option* opt;
  expr* val;
public:
  optassign_val(const char* fn, int line, option* o, expr* v);
  virtual ~optassign_val();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

optassign_val::optassign_val(const char* fn, int line, option* o, expr *v)
 : expr(fn, line, STMT)
{
  opt = o;
  val = v;
}

optassign_val::~optassign_val()
{
  Delete(val);
}

bool optassign_val::Print(OutputStream &s, int w) const
{
  s.Pad(' ', w);
  opt->show(s);
  s.Put(' ');
  val->Print(s, 0);
  s.Put("\n");
  return true;
}

void optassign_val::Compute(traverse_data &td)
{
  DCASSERT(em);
  DCASSERT(td.which == traverse_data::Compute);
  DCASSERT(td.answer);
  if (td.stopExecution())  return;
  result foo;
  result* old = td.answer;
  td.answer = &foo;
  SafeCompute(val, td);
  td.answer = old;

  option::error err = option::Success;

  switch (opt->Type()) {
    case option::Boolean:
        err = opt->SetValue(foo.getBool());
        break;

    case option::Integer:
        err = opt->SetValue(foo.getInt());
        break;

    case option::Real:
        err = opt->SetValue(foo.getReal());
        break;

    case option::String: {
        shared_string* s = smart_cast <shared_string*> (foo.getPtr());
        err = opt->SetValue(s);
        break;
    }

    default:
        if (em->startInternal(__FILE__, __LINE__)) {
          em->causedBy(this);
          em->internal() << "Bad option type in optassign_val class";
          em->stopIO();
        }
  }

  switch (err) {
    case option::Success:
        break;

    case option::RangeError:
        if (em->startWarning()) {
          em->causedBy(this);
          em->warn() << "Value ";
          DCASSERT(val->Type());
          val->Type()->print(em->warn(), foo);
          em->warn() << " out of range for option ";
            opt->show(em->warn());
            em->warn() << ", ignoring";
          em->stopIO();
        }
        break;

    default:
        if (em->startInternal(__FILE__, __LINE__)) {
          em->causedBy(this);
          em->internal() << "Unexpected error code for option assignment";
          em->stopIO();
        }
  }
}

void optassign_val::Traverse(traverse_data &td)
{
  val->Traverse(td);
}


// ******************************************************************
// *                                                                *
// *                       optassign_id class                       *
// *                                                                *
// ******************************************************************

/** An option assignment from an option constant (identifier).
 */
class optassign_id : public expr {
  option* opt;
  radio_button* val;
public:
  optassign_id(const char* fn, int line, option* o, radio_button* v);
  virtual ~optassign_id();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

optassign_id::optassign_id(const char* fn, int line, option* o, radio_button* v)
 : expr(fn, line, STMT)
{
  opt = o;
  val = v;
}

optassign_id::~optassign_id()
{
}

bool optassign_id::Print(OutputStream &s, int w) const
{
  s.Pad(' ', w);
  opt->show(s);
  s.Put(' ');
  val->show(s);
  s.Put("\n");
  return true;
}

void optassign_id::Compute(traverse_data &td)
{
  DCASSERT(em);
  DCASSERT(td.which == traverse_data::Compute);
  DCASSERT(td.answer);
  if (td.stopExecution())  return;
  option::error err = opt->SetValue(val);
  switch (err) {
    case option::Success:
        break;

    case option::RangeError:
        if (em->startWarning()) {
          em->causedBy(this);
          em->warn() << "Illegal value ";
          val->show(em->warn());
          em->warn() << " for option ";
          opt->show(em->warn());
          em->warn() << ", ignoring";
          em->stopIO();
        }
        break;

    default:
        if (em->startInternal(__FILE__, __LINE__)) {
          em->causedBy(this);
          em->internal() << "Unexpected error code for option assignment";
          em->stopIO();
        }
  }
}

void optassign_id::Traverse(traverse_data &td)
{
}


// ******************************************************************
// *                                                                *
// *                       opt_checker  class                       *
// *                                                                *
// ******************************************************************

/** Checkbox options.
 */
class opt_checker : public expr {
  option* opt;
  bool check;
  checklist_enum** vals;
  int numvals;
public:
  opt_checker(const char* fn, int line, option* o, bool c, checklist_enum** v, int nv);
  virtual ~opt_checker();

  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

opt_checker::opt_checker(const char* fn, int line, option* o, bool c,
  checklist_enum** v, int nv) : expr(fn, line, STMT)
{
  opt = o;
  check = c;
  vals = v;
  numvals = nv;
}

opt_checker::~opt_checker()
{
  delete[] vals;
}

bool opt_checker::Print(OutputStream &s, int w) const
{
  s.Pad(' ', w);
  opt->show(s);
  if (check)   s.Put('+');
  else    s.Put('-');
  s.Put(' ');
  vals[0]->show(s);
  for (int i=1; i<numvals; i++) {
    s.Put(", ");
    vals[i]->show(s);
  }
  s.Put("\n");
  return true;
}

void opt_checker::Compute(traverse_data &td)
{
  DCASSERT(td.which == traverse_data::Compute);
  DCASSERT(td.answer);
  if (td.stopExecution())  return;

  if (check)  for (int i=0; i<numvals; i++)  vals[i]->CheckMe();
  else        for (int i=0; i<numvals; i++)  vals[i]->UncheckMe();
}

void opt_checker::Traverse(traverse_data &td)
{
}



// ******************************************************************
// *                                                                *
// *                        exprman  methods                        *
// *                                                                *
// ******************************************************************

expr* exprman::makeExprStatement(const char* file, int line, expr* e) const
{
  if (0==e)  {
    Delete(e);
    return 0;
  }
  if (e->Type()->matches("void"))  return Share(e);
#ifdef ALLOW_NON_VOID_EXPRSTMT
  return new exprstmt(file, line, e);
#else
  // print an error message here
  return makeError();
#endif
}

expr* exprman::makeOptionStatement(const char* file, int line,
        option *o, expr *e) const
{
  if (0==o || 0==e) {
    Delete(e);
    return 0;
  }
  const type* ot = Opt2Type(this, o->Type());
  if (0==ot) {
    // we have a selection-type option, trying to plug a value.
    if (startError()) {
      causedBy(file, line);
      cerr() << "Option ";
      o->show(cerr());
      cerr() << " is a selction-type option";
      stopIO();
    }
    Delete(e);
    return makeError();
  }
  DCASSERT(ot);
  const type* et = e->Type();
  DCASSERT(et);
  if (!isPromotable(et, ot)) {
    if (startError()) {
      causedBy(file, line);
      cerr() << "Option ";
      o->show(cerr());
      cerr() << " expects type " << ot->getName();
      stopIO();
    }
    return makeError();
  }

  e = promote(e, ot);
  return new optassign_val(file, line, o, e);
}

expr* exprman::makeOptionStatement(const char* file, int line,
        option *o, option_enum *v) const
{
  if (0==o || 0==v) {
    return 0;
  }

  // check option type
  if (option::RadioButton != o->Type()) {
    if (startError()) {
      causedBy(file, line);
      cerr() << "Option ";
      o->show(cerr());
      cerr() << " is not a selection-type option";
      stopIO();
    }
    return makeError();
  }

  // check option constant
  if (v != o->FindConstant(v->Name())) {
    // We can only get here if the caller is foobar.
    if (startError()) {
      causedBy(file, line);
      cerr() << "Option ";
      o->show(cerr());
      cerr() << " cannot be set to ";
      v->show(cerr());
      stopIO();
    }
    return makeError();
  }
  radio_button* rb = smart_cast <radio_button*> (v);
  DCASSERT(rb);

  return new optassign_id(file, line, o, rb);
}

expr* exprman::makeOptionStatement(const char* file, int line,
      option* o, bool check, option_enum **vlist, int nv) const
{
  if (0==o) {
    delete[] vlist;
    return 0;
  }
  if (0==vlist)  return 0;

  // check option type
  if (option::Checklist != o->Type()) {
    if (startError()) {
      causedBy(file, line);
      cerr() << "Option ";
      o->show(cerr());
      cerr() << " is not a checklist-type option";
      stopIO();
    }
    return makeError();
  }

#ifdef DEVELOPMENT_CODE
  for (int i=0; i<nv; i++) {
    if (0==vlist[i])  continue;
    checklist_enum* foo = dynamic_cast <checklist_enum*> (vlist[i]);
    DCASSERT(foo);
  }
#endif

  return new opt_checker(file, line, o, check, (checklist_enum**) vlist, nv);
}

