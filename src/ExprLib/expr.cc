
/*
  Implementation of the base classes defined in exprlib.h
*/

#include "expr.h"
#include "symbols.h"
#include "../Options/options.h"
#include "../Utils/init_opts.h"
#include "../include/list.h"
#include "type.h"
#include "result.h"
// #include "exprman.h"

// ******************************************************************
// *                     traverse_data  methods                     *
// ******************************************************************

bool traverse_data::Print(std::ostream &s) const
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

const type* expr::STMT = nullptr;
unsigned expr::global_IDnum = 0;
debugging_msg expr::expr_debug;
debugging_msg expr::waitlist_debug;
debugging_msg expr::model_debug;

expr::expr(const location &W, const type* t) : shared_object()
{
  Init(W, t, 0, 0);

#ifdef DEBUG_LOC
  if (em) {
    em->cout() << "Built expr " << IDnum << " " << where << "\n";
    em->cout().flush();
  }
#endif
}

expr::expr(const location &W, typelist* t) : shared_object()
{
  Init(W, 0, t, 0);

#ifdef DEBUG_LOC
  if (em) {
    em->cout() << "Built expr " << IDnum << " " << where << "\n";
    em->cout().flush();
  }
#endif
}

expr::expr(const expr* x) : shared_object()
{
  if (x) {
    Init(x->Where(), x->simple, Share(x->aggtype), x->model_type);
#ifdef DEBUG_LOC
    if (em) {
        em->cout() << "Built expr " << IDnum << " from " << x->IDnum
               << " kept " << where << "\n";
        em->cout().flush();
    }
#endif
  } else {
    setNull();
#ifdef DEBUG_LOC
    if (em) {
        em->cout() << "Built expr " << IDnum << " from null\n";
        em->cout().flush();
    }
#endif
  }

}

expr::~expr()
{
  Delete(aggtype);
}

void expr::Init(const location &W, const type* st, typelist* at, const model_def* mt)
{
  IDnum = ++global_IDnum;
  if (!IDnum) {
    internal_error E(__FILE__, __LINE__, W);
    E << "Too many expressions, global ID overflow";
  }
  where = W;

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
    if (!mt) return;
    DCASSERT(!aggtype);
    if (simple) {
        DCASSERT(simple->isAFormalism());
    } else {
        simple = type::find("model");
    }
    DCASSERT(!model_type);
    model_type = mt;
}

expr* expr::GetComponent(int i)
{
  if (0==i) return this;
  return 0;
}

void expr::PrintType(std::ostream &s) const
{
  if (aggtype) {
    aggtype->Print(s);
    return;
  }
  if (simple) {
    s << *simple;
    return;
  }
  DCASSERT(0);
  s << "Unknown type";
}

void expr::Compute(traverse_data &x)
{
    internal_error E(__FILE__, __LINE__);
    E << "Trying to compute uncomputable expression: ";
    Print(E.stream());
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
    internal_error E(__FILE__, __LINE__);
    E << "Trying to rename an unnamed expression: ";
    Print(E.stream());
    E << " to: ";
    n->Print(E.stream());
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
    return 0==aggtype->Compare(sym->aggtype);
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

std::ostream& operator<< (std::ostream &s, const expr* e)
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
    internal_error E(__FILE__, __LINE__);
    E << "No traversal handler ";
    x.Print(E.stream());
    E << " for expression: ";
    Print(E.stream());
}

long expr::getDelta() const
{
  return 0;
}

long expr::getLower() const
{
  return 0;
}

long expr::getUpper() const
{
  return -1;
}


// ******************************************************************
// *                       expr_error methods                       *
// ******************************************************************

expr_error::expr_error(const expr* cause, result* _ans) : error_msg("ERROR")
{
    ans = _ans;
    if (cause) {
        Out << ' ' << cause->Where();
    }
    Out << ':';
    newLine();
}

expr_error::~expr_error()
{
    if (ans) {
        ans->setNull();
    }
}

// ******************************************************************
// *                   typechecking_error methods                   *
// ******************************************************************

typechecking_error::typechecking_error(const location &W)
    : error_msg("ERROR")
{
    if (W) {
        Out << ' ' << W;
    }
    Out << ':';
    newLine();
}

typechecking_error::typechecking_error(const expr* x)
    : error_msg("ERROR")
{
    if (x && x->Where()) {
        Out << ' ' << x->Where();
    }
    Out << ':';
    newLine();
}

// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class expr_initializer : public initializer {
    public:
        expr_initializer();
    protected:
        virtual void execute();
};
static expr_initializer the_expr_initializer;

expr_initializer::expr_initializer() : initializer(__FILE__, 3)
{
    builds_resource("exprs");
    needs_resource("types");
    needs_resource("Debug");
}

void expr_initializer::execute()
{
    expr::STMT = type::find("void");
    initialize_msg(expr::expr_debug,
        "exprs",
        "When set, low-level expression and statement messages are displayed.",
        get_object("Debug")
    );

    initialize_msg(expr::waitlist_debug,
        "waitlist",
        "When set, diagnostic messages are displayed regarding symbol waiting lists.",
        get_object("Debug")
    );

    initialize_msg(expr::model_debug,
        "models",
        "When set, diagnostic messages are displayed regarding model construction.",
        get_object("Debug")
    );
}

