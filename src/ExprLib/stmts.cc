
#include "stmts.h"
#include "../Utils/strings.h"
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
  exprstmt(const location &W, expr *e);
  virtual ~exprstmt();

  virtual bool Print(std::ostream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

exprstmt::exprstmt(const location &W, expr *e) : expr(W, STMT)
{
  x = e;
}

exprstmt::~exprstmt()
{
  Delete(x);
}

bool exprstmt::Print(std::ostream &s, int w) const
{
    s << padding(w);
    x->Print(s);
    s << ";\n";
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

  outputStream &cout = outputStream::globalOut();
  cout << "Evaluated statement ";
  x->Print(cout.stream());
  cout << ", got: ";
  DCASSERT(x->Type());
  x->Type()->print(cout.stream(), td.answer[0]);
  cout.newLine();
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
  optassign_val(const location &W, option* o, expr* v);
  virtual ~optassign_val();

  virtual bool Print(std::ostream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

optassign_val::optassign_val(const location &W, option* o, expr *v)
 : expr(W, STMT)
{
  opt = o;
  val = v;
}

optassign_val::~optassign_val()
{
  Delete(val);
}

bool optassign_val::Print(std::ostream &s, int w) const
{
    s << padding(w);
    opt->show(s);
    s << ' ';
    val->Print(s);
    s << '\n';
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

    default: {
        internal_error E(__FILE__, __LINE__, Where());
        E << "Bad option type in optassign_val class";
    }
  }

  switch (err) {
    case option::Success:
        break;

    case option::RangeError: {
        unnamed_warning W(Where());
        W << "Value ";
        DCASSERT(val->Type());
        val->Type()->print(W.stream(), foo);
        W << " out of range for option ";
        opt->show(W.stream());
        W << ", ignoring";
        break;
    }

    default: {
        internal_error E(__FILE__, __LINE__, Where());
        E << "Unexpected error code for option assignment";
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
  optassign_id(const location &W, option* o, radio_button* v);
  virtual ~optassign_id();

  virtual bool Print(std::ostream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

optassign_id::optassign_id(const location &W, option* o, radio_button* v)
 : expr(W, STMT)
{
  opt = o;
  val = v;
}

optassign_id::~optassign_id()
{
}

bool optassign_id::Print(std::ostream &s, int w) const
{
    s << padding(w);
    opt->show(s);
    s << ' ';
    val->show(s);
    s << '\n';
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

    case option::RangeError: {
        unnamed_warning W(Where());
        W << "Illegal value ";
        val->show(W.stream());
        W << " for option ";
        opt->show(W.stream());
        W << ", ignoring";
        break;
    }

    default: {
        internal_error E(__FILE__, __LINE__, Where());
        E << "Unexpected error code for option assignment";
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
  opt_checker(const location &W, option* o, bool c, checklist_enum** v, int nv);
  virtual ~opt_checker();

  virtual bool Print(std::ostream &s, int) const;
  virtual void Compute(traverse_data &x);
  virtual void Traverse(traverse_data &x);
};

opt_checker::opt_checker(const location &W, option* o, bool c,
  checklist_enum** v, int nv) : expr(W, STMT)
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

bool opt_checker::Print(std::ostream &s, int w) const
{
    s << padding(w);
    opt->show(s);
    s << (check ? "+ " : "- ");
    vals[0]->show(s);
    for (int i=1; i<numvals; i++) {
        s << ", ";
        vals[i]->show(s);
    }
    s << std::endl;
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

expr* exprman::makeExprStatement(const location &W, expr* e) const
{
  if (0==e)  {
    Delete(e);
    return 0;
  }
  if (e->Type()->matches("void"))  return Share(e);
#ifdef ALLOW_NON_VOID_EXPRSTMT
  return new exprstmt(W, e);
#else
  // print an error message here
  return makeError();
#endif
}

expr* exprman::makeOptionStatement(const location &W,
        option *o, expr *e) const
{
  if (0==o || 0==e) {
    Delete(e);
    return 0;
  }
  const type* ot = Opt2Type(this, o->Type());
  if (0==ot) {
    // we have a selection-type option, trying to plug a value.
    typechecking_error E(W);
    E << "Option ";
    o->show(E.stream());
    E << " is a selction-type option";
    Delete(e);
    return makeError();
  }
  DCASSERT(ot);
  const type* et = e->Type();
  DCASSERT(et);
  if (!isPromotable(et, ot)) {
      typechecking_error E(W);
      E << "Option ";
      o->show(E.stream());
      E << " expects type " << ot->getName();
      return makeError();
  }

  e = promote(e, ot);
  return new optassign_val(W, o, e);
}

expr* exprman::makeOptionStatement(const location &W,
        option *o, option_enum *v) const
{
    if (0==o || 0==v) {
        return 0;
    }

    // check option type
    if (option::RadioButton != o->Type()) {
        typechecking_error E(W);
        E << "Option ";
        o->show(E.stream());
        E << " is not a selection-type option";
        return makeError();
    }

    // check option constant
    if (v != o->FindConstant(v->Name())) {
        // We can only get here if the caller is foobar.
        typechecking_error E(W);
        E << "Option ";
        o->show(E.stream());
        E << " cannot be set to ";
        v->show(E.stream());
        return makeError();
    }

    radio_button* rb = smart_cast <radio_button*> (v);
    DCASSERT(rb);

    return new optassign_id(W, o, rb);
}

expr* exprman::makeOptionStatement(const location &W,
      option* o, bool check, option_enum **vlist, int nv) const
{
    if (0==o) {
        delete[] vlist;
        return 0;
    }
    if (0==vlist)  return 0;

    // check option type
    if (option::Checklist != o->Type()) {
        typechecking_error E(W);
        E << "Option ";
        o->show(E.stream());
        E << " is not a checklist-type option";
        return makeError();
    }

#ifdef DEVELOPMENT_CODE
    for (int i=0; i<nv; i++) {
        if (0==vlist[i])  continue;
        checklist_enum* foo = dynamic_cast <checklist_enum*> (vlist[i]);
        DCASSERT(foo);
    }
#endif

    return new opt_checker(W, o, check, (checklist_enum**) vlist, nv);
}

