
#include "exprman.h"
#include "../Options/options.h"
#include "../Options/optman.h"
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

  promote_arg.initialize(om, "promote_args", "When arguments are automatically promoted in a function call");
  promote_arg.Deactivate();
}

exprman::~exprman()
{
}

option* exprman::findOption(const char* name) const
{
  if (om)  return om->FindOption(name);
  return 0;
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


expr* exprman::makeTypecast(const location& where,
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
    return makeTypecast(where, fpt, e);
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
    newagg[i] = makeTypecast(where, fpt, thisone);
    if (newagg[i] != thisone) same = false;
    if (thisone) if (0==newagg[i] || isError(newagg[i])) {
      // we couldn't typecast this component
      null = true;
      break;
    }
  }
  if (!same && !null) {
    expr* foo = makeAssocOp(where, aop_colon, newagg, 0, nc);
    Delete(e);  // need to delete e AFTER we use where ^ here
    return foo;
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

  return makeTypecast(e->Where(), newtype, e);
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
      promote_arg.causedBy(e->Where());
      promote_arg.warn() << "Promoting argument ";
      e->Print(promote_arg.warn(), 0);
      promote_arg.warn() << " to type ";
      fp->PrintType(promote_arg.warn());
      promote_arg.stopIO();
    }
  }
  return makeTypecast(e->Where(), proc, rand, fp, e);
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

model_def* exprman::makeModel(const location& W, const type* t,
        char* name, symbol** formals, int np) const
{
  const formalism* f = smart_cast <const formalism*> (t);
  if (f) return f->makeNewModel(W, name, formals, np);

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

  abstract_msg::initStatic(io);
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
  expr::expr_debug.initialize(om, "exprs",
      "When set, low-level expression and statement messages are displayed."
  );
#ifdef EXPR_DEBUG
  expr::expr_debug.Activate();
#endif

  expr::waitlist_debug.initialize(om, "waitlist",
      "When set, diagnostic messages are displayed regarding symbol waiting lists."
  );

  expr::model_debug.initialize(om, "models",
      "When set, diagnostic messages are displayed regarding model construction."
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
