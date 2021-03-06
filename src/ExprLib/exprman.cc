
#include "exprman.h"
#include "../Options/options.h"
#include "result.h"
#include "engine.h"

#include "intervals.h"
#include "values.h"
#include "formalism.h"
#include "casting.h"
#include "mod_def.h"
#include "mod_inst.h"

// for option initializations
#include "functions.h"
#include "converge.h"

// for the rest of the exprman functions
#include "superman.h"
#include "init_data.h"

// ******************************************************************
// *                        library  methods                        *
// ******************************************************************

library::library(bool has_cr, bool has_date)
{
  has_copyright = has_cr;
  has_release_date = has_date;
}

library::~library()
{
}

void library::printCopyright(doc_formatter* df) const
{
  DCASSERT(0);
}

void library::printReleaseDate(doc_formatter*) const
{
  DCASSERT(0);
}

// ******************************************************************
// *                     group_of_named methods                     *
// ******************************************************************

group_of_named::group_of_named(int max)
{
  alloc = max;
  curr = 0;
  DCASSERT(max>=0);
  if (max)  items = new option_const*[max];
  else      items = 0;
}

group_of_named::~group_of_named()
{
  delete[] items;
}

void group_of_named::AddItem(option_const* foo)
{
  if (0==foo) return;
  CHECK_RANGE(0, curr, alloc);
  items[curr] = foo;
  curr++;
}

void group_of_named::Finish(option* owner, const char* n, const char* docs)
{
  if (0==owner) return;
  if (0==curr)  return;
  checklist_const** boxes = new checklist_const*[curr];
  for (int i=0; i<curr; i++) {
    boxes[i] = smart_cast <checklist_const*> (items[i]);
    DCASSERT(boxes[i]);
  }
  checklist_const* foo = MakeChecklistGroup(n, docs, boxes, curr);
  DCASSERT(foo);
  CHECK_RETURN( owner->AddCheckItem(foo), option::Success );
}

// ******************************************************************
// *                       named_msg  methods                       *
// ******************************************************************

io_environ* named_msg::io = 0;
option_const* named_msg
::Initialize(option* owner, const char* n, const char* docs, bool act)
{
  name = n;
  active = act;
  if (owner) {
    checklist_const* foo = MakeChecklistConstant(n, docs, active);
    DCASSERT(foo);
    CHECK_RETURN( owner->AddCheckItem(foo), option::Success );
    return foo;
  }
  return 0;
}

// ******************************************************************
// *                        exprman  methods                        *
// ******************************************************************

exprman::exprman(io_environ* i, option_manager* o)
{
  is_finalized = false;
  io = i;
  om = o;

  // fundamental types
  VOID = 0;
  NULTYPE = 0;
  BOOL = 0;
  INT = 0;
  REAL = 0;
  MODEL = 0;

  NEXT_STATE = 0;

  EXPO = 0;
  STRING = 0;
  BIGINT = 0;
  STATESET = 0;
  STATEDIST = 0;
  STATEPROBS = 0;
  TEMPORAL = 0;
  TRACE = 0;

  NO_ENGINE = 0;
  BLOCKED_ENGINE = 0;

  option* warning = om->FindOption("Warning");
  promote_arg.Initialize(warning,
    "promote_args",
    "When arguments are automatically promoted in a function call",
    false
  );
}

exprman::~exprman()
{
}

void exprman::addOption(option* o)
{
  if (0==o) return;
  if (om)   om->AddOption(o);
  else      delete o;
}

option* exprman::findOption(const char* name) const
{
  if (om)  return om->FindOption(name);
  return 0;
}

// +-----------------------------------------------------------------+
// |                       Building  constants                       |
// +-----------------------------------------------------------------+

expr* exprman::makeLiteral(const char* file, int line, 
      const type* t, const result& c) const
{
  return new value(file, line, t, c);
}

// +-----------------------------------------------------------------+
// |                   Array and function  "calls"                   |
// +-----------------------------------------------------------------+

symbol* exprman::findFunction(const type* mt, const char* n)
{
  if (0==mt)      return 0;
  if (!mt->isAFormalism())  return 0;
  const formalism* f = smart_cast <const formalism*> (mt);
  DCASSERT(f);
  return f->findFunction(n);
}

// +-----------------------------------------------------------------+
// |                         Changing  types                         |
// +-----------------------------------------------------------------+

const type* exprman::getLeastCommonType(const type* a, const type* b) const
{
  if (0==a || 0==b)  return 0;
  if (a==b)    return a;

  bool is_set = a->isASet() || b->isASet();
  bool proc = a->hasProc() || b->hasProc();

  modifier m = MAX( a->getModifier(), b->getModifier() );

  const simple_type* ba = a->getBaseType();
  const simple_type* bb = b->getBaseType();

  const type* t = 0;
  if (isPromotable(ba, bb))  t = bb;
  if (isPromotable(bb, ba))  t = ba;
  if (t)    t = t->modifyType(m);
  if (t) if (proc)  t = t->addProc();
  if (t) if (is_set)  t = t->getSetOfThis();
  return t;
}


expr* exprman::makeTypecast(const char* file, int line, 
      bool proc, bool rand, const expr *fp, expr* e) const
{
  if (0==e || isError(e) || isDefault(e))  return e;
  if (0==fp) {
    Delete(e);
    return e;
  }
  DCASSERT(! isError(fp));
  DCASSERT(! isDefault(fp));
  DCASSERT(fp->NumComponents() == e->NumComponents());
  int nc = e->NumComponents();
  if (1==nc) {
    const type* fpt = fp->Type();             DCASSERT(fpt);
    if (rand) fpt = fpt->modifyType(RAND);    DCASSERT(fpt);
    if (proc) fpt = fpt->addProc();           DCASSERT(fpt);
    return makeTypecast(file, line, fpt, e);
  }

  expr** newagg = new expr*[nc];
  bool same = true;
  bool null = false;
  int i;
  for (i=0; i<nc; i++) {
    const type* fpt = fp->Type(i);            DCASSERT(fpt);
    if (rand) fpt = fpt->modifyType(RAND);    DCASSERT(fpt);
    if (proc) fpt = fpt->addProc();           DCASSERT(fpt);
    expr* thisone = Share(e->GetComponent(i));
    newagg[i] = makeTypecast(file, line, fpt, thisone);
    if (newagg[i] != thisone) same = false;
    if (thisone) if (0==newagg[i] || isError(newagg[i])) {
      // we couldn't typecast this component
      null = true;
      break;
    }
  }
  if (!same && !null) { 
    Delete(e);
    return makeAssocOp(file, line, aop_colon, newagg, 0, nc);
  }
  delete[] newagg;
  if (same) return e;
  // must be impossible
  return makeError();
}

expr* exprman::promote(expr* e, const type* newtype) const
{
  if (0==e || isError(e) || isDefault(e))  return e;

  if (!isPromotable(e->Type(), newtype))  return makeError();

  return makeTypecast(e->Filename(), e->Linenumber(), newtype, e); 
}

expr* exprman::promote(expr* e, bool proc, bool rand, const expr* fp) const
{
  if (0==e || isError(e) || isDefault(e))  return e;
  if (0==fp) {
    Delete(e);
    return 0;
  }
  DCASSERT(! isError(fp));
  DCASSERT(! isDefault(fp));
  if (fp->NumComponents() != e->NumComponents())  
  return makeError();

  bool changetype = false;
  int nc = e->NumComponents();
  for (int i=0; i<nc; i++) {
    const type* fpt = fp->Type(i);            DCASSERT(fpt);
    if (rand) fpt = fpt->modifyType(RAND);    DCASSERT(fpt);
    if (proc) fpt = fpt->addProc();           DCASSERT(fpt);
    int d = getPromoteDistance(e->Type(i), fpt);
    if (d<0) return makeError();
    if (d>0) changetype = true;
  }
  if (changetype) {
    if (promote_arg.startWarning()) {
      promote_arg.causedBy(e);
      promote_arg.warn() << "Promoting argument ";
      e->Print(promote_arg.warn(), 0);
      promote_arg.warn() << " to type ";
      fp->PrintType(promote_arg.warn());
      promote_arg.stopIO();
    }
  }
  return makeTypecast(e->Filename(), e->Linenumber(), proc, rand, fp, e); 
}

// +-----------------------------------------------------------------+
// |                      Building  expressions                      |
// +-----------------------------------------------------------------+
 
const char* exprman::getOp(unary_opcode op)
{
  switch (op) {
    case uop_none:      return "no-op";
    case uop_not:       return "!";
    case uop_neg:       return "-";
    case uop_forall:    return "A";
    case uop_exists:    return "E";
    case uop_future:    return "F";
    case uop_globally:  return "G";
    case uop_next:      return "X";
    default:        return "unknown_op";
  }
  return "error";  // will never get here, keep compilers happy
}

const char* exprman::getOp(binary_opcode op)
{
  switch (op) {
    case bop_none:    return "no-op";
    case bop_mod:     return "%";
    case bop_diff:    return "\\";
    case bop_implies: return "->";
    case bop_equals:  return "==";
    case bop_nequal:  return "!=";
    case bop_gt:      return ">";
    case bop_ge:      return ">=";
    case bop_lt:      return "<";
    case bop_le:      return "<=";
    case bop_until:   return "U";
    case bop_and:     return "&&";
    default:          return "unknown_op";
  }
  return "error";  // will never get here, keep compilers happy
}

const char* exprman::getFirst(trinary_opcode op)
{
  switch (op) {
    case top_none:      return "no-op";
    case top_interval:  return "..";
    case top_ite:       return "?";
    default:            return "unknown";
  }
  return "error";
}

const char* exprman::getSecond(trinary_opcode op)
{
  switch (op) {
    case top_none:      return "no-op";
    case top_interval:  return "..";
    case top_ite:       return ":";
    default:            return "unknown";
  }
  return "error";
}

const char* exprman::getOp(bool flip, assoc_opcode op)
{
  switch (op) {
    case aop_none:  return "no-op";
    case aop_and:   return flip ?  0  : "&";
    case aop_or:    return flip ?  0  : "|";
    case aop_plus:  return flip ? "-" : "+";
    case aop_times: return flip ? "/" : "*";
    case aop_colon: return flip ?  0  : ":";
    case aop_semi:  return flip ?  0  : ";";
    case aop_union: return flip ?  0  : ",";
    default:        return "unknown";
  }
  return "error";  // will never get here, keep compilers happy
}

const char* exprman::documentOp(unary_opcode op)
{
  switch (op) {
    case uop_not: return "logical negation";
    case uop_neg: return "numerical negation";
    default:      return 0;
  }
  return 0;  // will never get here, keep compilers happy
}

const char* exprman::documentOp(binary_opcode op)
{
  switch (op) {
    case bop_mod:     return "modulo";
    case bop_diff:    return "set difference";
    case bop_implies: return "implication";
    case bop_equals:  return "equality comparison";
    case bop_nequal:  return "inequality comparison";
    case bop_gt:      return "greater than";
    case bop_ge:      return "greater or equal";
    case bop_lt:      return "less than";
    case bop_le:      return "less or equal";
    default:          return 0;
  }
  return 0;  // will never get here, keep compilers happy
}

const char* exprman::documentOp(trinary_opcode op)
{
  switch (op) {
    case top_interval:  return "intervals (within {})";
    case top_ite:       return "if-then-else";
    default:            return 0;
  }
  return 0;
}

const char* exprman::documentOp(bool flip, assoc_opcode op)
{
  switch (op) {
    case aop_and:   return flip ?  0  : "logical and";
    case aop_or:    return flip ?  0  : "logical or";
    case aop_plus:  return flip ? "difference" : "addition";
    case aop_times: return flip ? "division" : "multiplication";
    case aop_colon: return flip ?  0  : "aggregation";
    case aop_semi:  return flip ?  0  : "statement aggregation";
    case aop_union: return flip ?  0  : "set union (inside {})";
    default:        return 0;
  }
  return 0;  // will never get here, keep compilers happy
}


// +-----------------------------------------------------------------+
// |                         Building models                         |
// +-----------------------------------------------------------------+

model_def* exprman::makeModel(const char* fn, int ln, const type* t, 
        char* name, symbol** formals, int np) const
{
  const formalism* f = smart_cast <const formalism*> (t);
  if (f) return f->makeNewModel(fn, ln, name, formals, np);

  // error
  free(name);
#ifdef TURN_OFF
  for (int i=0; i<np; i++)  Delete(formals[i]);
#endif
  delete[] formals; 
  return 0;
}

// +-----------------------------------------------------------------+
// |                Building  model and measure calls                |
// +-----------------------------------------------------------------+

model_def* exprman::isAModelDef(symbol* f)
{
  return dynamic_cast<model_def*> (f);
}

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

exprman* The_Man = 0;
bool builtManager = 0;

exprman* Initialize_Expressions(io_environ* io, option_manager* om)
{
  if (builtManager)  return The_Man;  
  builtManager = 1;

  named_msg::io = io;
  The_Man = new superman(io, om);
  InitTypes(The_Man);
  InitEngines(The_Man);

  expr::em = The_Man;
  if (The_Man)
    expr::STMT = The_Man->findType("void");
  else
    expr::STMT = 0;

  //
  // Option initialization
  //
  option* debug = om ? om->FindOption("Debug") : 0;
  expr::expr_debug.Initialize(debug, 
      "exprs", 
      "When set, low-level expression and statement messages are displayed.",
      false
  );

#ifdef EXPR_DEBUG
  expr::expr_debug.active = true;
#endif

  expr::waitlist_debug.Initialize(debug,
      "waitlist",
      "When set, diagnostic messages are displayed regarding symbol waiting lists.", 
      false
  );

  expr::model_debug.Initialize(debug,
      "models",
      "When set, diagnostic messages are displayed regarding model construction.", 
      false
  );

  // Other options to initialize
  InitTypeOptions(The_Man);
  InitConvergeOptions(The_Man);
  InitFunctions(The_Man);
  InitModelDefs(The_Man);
  InitLLM(The_Man);
  InitIntervals(The_Man);

  return The_Man;
}

exprman* getExpressionManager()
{
  return The_Man;
}

void destroyExpressionManager(exprman* em)
{
  DCASSERT(em == The_Man);
  delete The_Man;
  The_Man = 0;
}
