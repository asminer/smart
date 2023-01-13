
#include "compile.h"
#include "lexer.h"
#include "../Options/options.h"
#include "../Options/optman.h"
#include "../Utils/strings.h"
#include "../Utils/init_opts.h"

#include "../ExprLib/symb_tab.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/values.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/trinary.h"
#include "../ExprLib/assoc.h"
#include "../ExprLib/bogus.h"
#include "../ExprLib/casting.h"
#include "../ExprLib/mod_def.h"

#include "../include/heap.h"
#include "parse_icp.h"
#include <string.h>
#include <stdlib.h>

// Put this one last.
#include "ParseICP/icpyacc.hh"

// #define PARSER_DEBUG
// #define COMPILE_DEBUG

/*
    Hopefully, this time around, the compiler support functions
    can be simplified a bit.

    Conventions:

    We use circular linked-lists (with good old-fashioned nodes)
    to allow maximum memory re-use and hopefully speed.
    The list pointer is to the last element in the list
    (or to 0 for an empty list),
    and the last element's next pointer is to the front of the list.
    This allows us to quickly add to the tail of the list :^)
    These are indicated by the type parser_list*


    Other conventions here...

*/

// should be built by bison.
int yyparse();

/* =====================================================================

  Global variables.

   ===================================================================== */

parse_module* pm;
debugging_msg parser_debug;
debugging_msg compiler_debug;

// Expression for the integer constant 1.
expr* ONE = 0;

/// Type of model under construction, if any.
const formalism* ModelType = 0;

/// Model under construction
model_def* model_under_construction = 0;

/// Internal symbols for model under construction.
symbol_table* ModelInternal = 0;

/// External (visible) symbols for model under construction.
HeapOfPointers <symbol> ModelExternal;

/// List of statements in the model.
parser_list* list_of_statements = 0;

/// List of measures, to solve, in order, by name.
List <char> MeasureNames;


// Compiler stats:

long list_depth;

inline int Compare(const symbol* a, const symbol* b)
{
  DCASSERT(a);
  DCASSERT(b);
  return strcmp(a->Name(), b->Name());
}

/* =====================================================================

  I/O and such.

   ===================================================================== */

parse_error::parse_error(bool err) : error_msg(err ? "ERROR" : "WARNING")
{
    Out << ' ' << Where();
    newLine();
}

void yyerror(const char *msg)
{
    parse_error E;
    E << msg;
}

void Reducing(const char* msg)
{
  DCASSERT(pm);
  if (parser_debug.start()) {
    parser_debug << "reducing rule:\n\t\t";
    parser_debug << msg;
    parser_debug.stop();
  }
}

inline expr* ShowNewStatement(const char* what, expr* f)
{
  // Make noise as appropriate
  if (compiler_debug.start()) {
    compiler_debug << "built ";
    if (what) compiler_debug << what;
    else      compiler_debug << "statement: ";
    if (f)  f->Print(compiler_debug.stream(), 4);
    else    compiler_debug << "    null\n";
    compiler_debug.stop();
  }
  return f;
}

template <class EXPR>
inline EXPR* ShowWhatWeBuilt(const char* what, EXPR* f)
{
  // Make noise as appropriate
  if (compiler_debug.start()) {
    compiler_debug << "built ";
    if (what) compiler_debug << what;
    else      compiler_debug << "expression: ";
    if (f)  f->Print(compiler_debug.stream());
    else    compiler_debug << "null";
    compiler_debug << "\t type: ";
    if (f)  f->PrintType(compiler_debug.stream());
    else    compiler_debug << "nulltype";
    compiler_debug.stop();
  }
  return f;
}


/* =====================================================================

  Lists for associative arithmetic

   ===================================================================== */

/// Used for lists of sums or products
class expr_term : public shared_object {
public:
  int op;
  expr* term;
public:
  expr_term(int o, expr* t);
  virtual ~expr_term();

  virtual bool Print(std::ostream &s, int width) const;
};

expr_term::expr_term(int o, expr* t) : shared_object()
{
  op = o;
  term = t;
}

expr_term::~expr_term()
{
  Delete(term);
}

bool expr_term::Print(std::ostream &s, int) const
{
  s << "(" << TokenName(op) << ", ";
  if (term)   term->Print(s);
  else        s << "null";
  s << ")";
  return true;
}

/* =====================================================================

  Centralized list manager.

   ===================================================================== */

struct parser_list {
  shared_object* data;
  parser_list* next;
};

parser_list* FreeList = 0;

parser_list* MakeListNode(shared_object* d)
{
  parser_list* ptr;
  if (FreeList) {
    ptr = FreeList;
    FreeList = ptr->next;
  } else {
    ptr = new parser_list;
    list_depth++;
  }
  ptr->data = d;
  ptr->next = 0;
  return ptr;
}

void RecycleNode(parser_list* ptr)
{
  if (0==ptr) return;
  ptr->next = FreeList;
  FreeList = ptr;
}

void RecycleCircular(parser_list* ptr)
{
  if (0==ptr)  return;
  parser_list* front = ptr->next;
  ptr->next = FreeList;
  FreeList = front;
}

void DeleteCircular(parser_list* ptr)
{
  if (0==ptr)  return;
  parser_list* front = ptr->next;
  ptr->next = 0;
  while (front) {
    parser_list* next = front->next;
    Delete(front->data);
    front->data = 0;
    RecycleNode(front);
    front = next;
  }
}

inline parser_list* AppendCircular(parser_list* list, shared_object* s)
{
  parser_list* ptr = MakeListNode(s);
  if (list) {
    ptr->next = list->next;
    list->next = ptr;
  } else {
    ptr->next = ptr;
  }
  return ptr;
}

inline parser_list* PrependCircular(parser_list* list, shared_object* s)
{
  parser_list* ptr = MakeListNode(s);
  if (list) {
    ptr->next = list->next;
    list->next = ptr;
    return list;
  } else {
    ptr->next = ptr;
    return ptr;
  }
}

parser_list* RemoveCircular(parser_list* thisone)
{
  if (0==thisone)  return 0;
  if (thisone->next == thisone)  {
    RecycleNode(thisone);
    return 0;
  }
  parser_list* next = thisone->next;
  thisone->data = next->data;
  thisone->next = next->next;
  RecycleNode(next);
  return thisone;
}

int CircularLength(parser_list* list)
{
  if (0==list) return 0;
  int length = 1;
  for (parser_list* ptr = list->next; ptr != list; ptr=ptr->next)
  length++;
  return length;
}

template <class DATA>
void CopyCircular(parser_list* ptr, DATA** array, int N)
{
  if (0==ptr) return;
  for (int i=0; i<N; i++) {
    ptr = ptr->next;
    array[i] = smart_cast <DATA*> (ptr->data);
  }
}

expr* MakeStatementBlock(parser_list* stmts)
{
  if (0==stmts)  return 0;
  int length = CircularLength(stmts);
  if (1==length) {
    expr* foo = smart_cast <expr*> (stmts->data);
    RecycleCircular(stmts);
    return foo;
  }
  // build block statement
  expr** opnds = new expr*[length];
  CopyCircular(stmts, opnds, length);
  RecycleCircular(stmts);
  return ShowNewStatement("model block\n",
    assoc_op::makeExpr(Where(), assoc_op::aop_semi,
    opnds, 0, length)
  );
}

// ******************************************************************
// *                                                                *
// *                     List-related functions                     *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
void AppendStatement(expr* s)
{
  if (0==pm) return;
  if (bogus_expr::orNull(s))  return;

  list_of_statements = AppendCircular(list_of_statements, s);
}

// --------------------------------------------------------------
parser_list* AppendExpression(int behv, parser_list* list, expr* item)
{
  switch (behv) {
    case 0:  // add regardless.
        return AppendCircular(list, item);

    case 1:  // add, unless item is null or error
        if (bogus_expr::orNull(item))
            return list;
        return AppendCircular(list, item);

    case 2:  // collapse on null or error.
        if (0==list)
          return AppendCircular(0, item);  // correct regardless
        if (bogus_expr::orNull(item) ) {
          // collapse the list
          DeleteCircular(list);
          return AppendCircular(0, item);
        }
        if (bogus_expr::orNull((expr*) list->data) ) {
          // list is already collapsed
          Delete(item);
          return list;
        }
        return AppendCircular(list, item);
  } // switch
  return 0;
}


// --------------------------------------------------------------
parser_list* AppendTerm(parser_list* list, int op, expr* t)
{
  // TBD: what about null or error expressions?
  expr_term* foo = new expr_term(op, t);
  return AppendCircular(list, foo);
}

// --------------------------------------------------------------
parser_list* AppendName(parser_list* list, char* ident)
{
  shared_string* i = new shared_string(ident);
  return AppendCircular(list, i);
}


// ******************************************************************
// *                                                                *
// *                  Statement-related  functions                  *
// *                                                                *
// ******************************************************************


// --------------------------------------------------------------
expr* MakeConstraint(expr *x)
{
  if (bogus_expr::orNull(x))  return 0;
  DCASSERT(x->Type());
  if (! x->Type()->matches("bool")) {
    parse_error E;
    E << "Expected boolean constraint, got ";
    x->PrintType(E.stream());
    E << ", ignoring";
    return nullptr;
  }
  const type* ICP_TYPE = type::find("dcp");
  if (!ICP_TYPE->isAFormalism()) return 0;
  const formalism* f = smart_cast <const formalism*> (ICP_TYPE);
  DCASSERT(f);
  symbol* best = f->findSymbol("constraint");
  if (0==best)  return 0;
  DCASSERT(best->Next() == 0);
  expr** pass = new expr* [2];
  pass[0] = Share((expr*) model_under_construction);
  pass[1] = x;
  return ShowWhatWeBuilt(0,
    expr::makeFunctionCall(Where(), best, pass, 2)
  );
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, expr* v)
{
  return ShowNewStatement(
    "option statement:\n",
    expr::makeOptionStatement(Where(), o, v)
  );
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, char* n)
{
  expr* foo;
  option_enum* oc = o ? o->FindConstant(n) : 0;
  if (0==oc) {
    if (o) {
      parse_error E;
      E << "Illegal value " << n << " for option " << o->Name();
      E << ", ignoring";
    }
    foo = nullptr;
  } else {
    foo = expr::makeOptionStatement(Where(), o, oc);
  }
  free(n);

  return ShowNewStatement("option statement:\n", foo);
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, bool check, parser_list* list)
{
  if (0==o) {
    DeleteCircular(list);
    return 0;
  }
  if (0==list)  return 0;

  // Count valid option_enums.
  int length = CircularLength(list);
  int actual_length = 0;
  for (int i=0; i<length; i++) {
    list = list->next;
    shared_string* s = smart_cast <shared_string*> (list->data);
    const char* name = s ? s->getStr() : 0;
    option_enum* oc = name ? o->FindConstant(name) : 0;
    if (oc) {
      actual_length++;
      continue;
    }
    if (name) {
      parse_error E;
      E << "Illegal value " << name << " for option " << o->Name();
      E << ", ignoring";
    }
  }

  if (actual_length<1) {
    DeleteCircular(list);
    return 0;
  }

  // Build list of option_enums
  option_enum** vlist = new option_enum*[actual_length];
  int i=0;
  for (int j=0; j<length; j++) {
    list = list->next;
    shared_string* s = smart_cast <shared_string*> (list->data);
    const char* name = s ? s->getStr() : 0;
    option_enum* oc = name ? o->FindConstant(name) : 0;
    if (0==oc) continue;

    vlist[i] = oc;
    i++;
  }
  DeleteCircular(list);

  return ShowNewStatement(
  "option statement:\n",
    expr::makeOptionStatement(Where(), o, check, vlist, length)
  );
}

// --------------------------------------------------------------
option* BuildOptionHeader(char* name)
{
  if (0==name) return 0;
  option* answer = option_manager::global().FindOption(name);

  if (0==answer) {
    parse_error E;
    E << "Unknown option " << name;
  }
  free(name);
  return answer;
}

// --------------------------------------------------------------
expr* BuildExprStatement(expr *x)
{
  if (bogus_expr::orNull(x))  return 0;
  return expr::makeExprStatement(Where(), x);
}


// ******************************************************************
// *                                                                *
// *                    Symbol-related functions                    *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
expr* BuildIntegers(char* typ, parser_list* namelist, expr* values)
{
  if (0==values) {
    DeleteCircular(namelist);
    free(typ);
    return 0;
  }
  bool oktype = (0 != strcmp(typ, "bool"));
  if (!oktype) {
    parse_error E;
    E << "no bounding needed for boolean identifiers";
    DeleteCircular(namelist);
    return 0;
  }
  int N = CircularLength(namelist);
  symbol** names = new symbol*[N];
  CopyCircular(namelist, names, N);
  return ShowNewStatement(0,
    expr::makeModelVarDecs(Where(),
      model_under_construction, type::find("int"), values, names, N)
  );
}

// --------------------------------------------------------------
expr* BuildBools(char* typ, parser_list* namelist)
{
  bool oktype = (0 != strcmp(typ, "int"));
  if (!oktype) {
    parse_error E;
    E << "unbounded integer identifiers";
    DeleteCircular(namelist);
    return 0;
  }
  int N = CircularLength(namelist);
  symbol** names = new symbol*[N];
  CopyCircular(namelist, names, N);
  return ShowNewStatement(0,
    expr::makeModelVarDecs(Where(),
      model_under_construction, type::find("bool"), 0, names, N)
  );
}


// ******************************************************************
// *                                                                *
// *                    Model-related  functions                    *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
bool IllegalModelVarName(char* ident, const char* what_am_i)
{
  DCASSERT(ModelInternal);
  if (ModelInternal->findSymbol(ident)) {
    parse_error E;
    E << "Duplicate identifier " << ident << " within model";
    free(ident);
    return true;
  }
  return false;
}

// --------------------------------------------------------------
void StartModel()
{
    const type* ICP_TYPE = type::find("dcp");
    ModelType = dynamic_cast <const formalism*> (ICP_TYPE);
    DCASSERT(ModelType);
    DCASSERT(ICP_TYPE);
    DCASSERT(0==ModelType);
    DCASSERT(0==model_under_construction);
    DCASSERT(0==ModelInternal);
    char* name = strdup(" ");
    model_under_construction =
        ModelType->makeNewModel(location::NOWHERE(), name, 0, 0);
    ModelInternal = new symbol_table;
}

// --------------------------------------------------------------
void FinishModel()
{
  DCASSERT(model_under_construction);
  expr* block = MakeStatementBlock(list_of_statements);
  int ns = ModelExternal.Length();
  ModelExternal.Sort();
  symbol** visible = ModelExternal.MakeArray();

  model_def::finishModelDef(model_under_construction, block, visible, ns);

  delete ModelInternal;
  ModelInternal = 0;

  pm->num_measures = MeasureNames.Length();
  if (pm->num_measures) {
    pm->measure_names = MeasureNames.CopyAndClear();
    pm->measure_calls = new expr*[pm->num_measures];
  }

  // Build measure calls
  for (int i=0; i<pm->num_measures; i++) {
    pm->measure_calls[i] = expr::makeMeasureCall(Where(),
      model_under_construction, 0, 0, pm->measure_names[i]);
  }

  model_under_construction = 0;
}


// --------------------------------------------------------------
parser_list* AddModelVar(parser_list* varlist, char* ident)
{
  if (0==ident)  return varlist;
  if (IllegalModelVarName(ident, "variable")) {
    return varlist;
  }

  symbol* ms = symbol::makeModelSymbol(Where(), 0, ident);
  ModelInternal->addSymbol(ms);
  ShowWhatWeBuilt("symbol ", ms);

  return AppendCircular(varlist, ms);
}


// --------------------------------------------------------------
inline expr* BuildMeasure(const type* typ, char* ident, symbol* who, expr* rhs)
{
  if (IllegalModelVarName(ident, "measure")) {
    Delete(rhs);
    return 0;
  }

  if (0==who)  {
    Delete(rhs);
    return 0;
  }
  DCASSERT(who->Next() == 0);
  expr** pass = new expr* [2];
  pass[0] = Share((expr*) model_under_construction);
  pass[1] = rhs;
  rhs = expr::makeFunctionCall(Where(), who, pass, 2);

  symbol* wrap = symbol::makeModelSymbol(Where(), typ, ident);

  // Add measure to symbol tables
  DCASSERT(ModelInternal);
  ModelInternal->addSymbol(wrap);

  ModelExternal.Insert(wrap);

  // Add this to the measure list
  MeasureNames.Append(ident);

  return ShowNewStatement("measure assignment:\n",
    expr::makeModelMeasureAssign(Where(),
        model_under_construction, wrap, rhs)
  );
}

// --------------------------------------------------------------
expr* BuildMaximize(char* ident, expr* rhs)
{
  const formalism* ICP_TYPE = dynamic_cast <formalism*> (type::find("dcp"));
  DCASSERT(ICP_TYPE);
  symbol* who = ICP_TYPE->findSymbol("maximize");
  return BuildMeasure(type::find("real"), ident, who, rhs);
}

// --------------------------------------------------------------
expr* BuildMinimize(char* ident, expr* rhs)
{
  const formalism* ICP_TYPE = dynamic_cast <formalism*> (type::find("dcp"));
  DCASSERT(ICP_TYPE);
  symbol* who = ICP_TYPE->findSymbol("minimize");
  return BuildMeasure(type::find("real"), ident, who, rhs);
}

// --------------------------------------------------------------
expr* BuildSatisfiable(char* ident, expr* rhs)
{
  const formalism* ICP_TYPE = dynamic_cast <formalism*> (type::find("dcp"));
  DCASSERT(ICP_TYPE);
  symbol* who = ICP_TYPE->findSymbol("satisfiable");
  return BuildMeasure(type::find("bool"), ident, who, rhs);
}

// ******************************************************************
// *                                                                *
// *                  Expression-related functions                  *
// *                                                                *
// ******************************************************************


// Helper for FindBest
function* scoreFuncs(symbol* find, expr** pass, int np, int &bs, bool &tie)
{
  function* best = 0;
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = smart_cast <function*> (ptr);
    if (0==f)  continue;
    int score = f->TypecheckParams(pass, np);
    if (score < 0)    continue;
    if (score == bs)  {
      tie = true;
      continue;
    }
    if ((bs < 0) || (score < bs)) {
      tie = false;
      best = f;
      bs = score;
    }
  }
  return best;
}

// Helper for FindBest
void showMatching(error_msg &E, symbol* find, expr** pass, int np, int best_score)
{
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = smart_cast <function*> (ptr);
    if (0==f)      continue;
    int score = f->TypecheckParams(pass, np);
    if (score != best_score)  continue;
    f->PrintHeader(E.stream(), true);
    E.newLine();
  } // for ptr
}

// Function/model call scoring
function* FindBest(symbol* f1, symbol* f2, expr** pass, int length, int first)
{
  if (0==f1 && 0==f2) return 0;
  const char* name = f1 ? f1->Name() : f2->Name();
  int best_score = -1;
  bool tie = false;
  function* best = scoreFuncs(f1, pass, length, best_score, tie);
  if (best_score != 0) {
    int old_best = best_score;
    function* best2 = scoreFuncs(f2, pass, length, best_score, tie);
    if (best) {
      DCASSERT(old_best>=0);
      if (best2 && old_best < best_score) best = best2;
    } else {
      best = best2;
    }
  }

  bool bailout = false;
  if (best_score < 0) {
    parse_error E;
    E << "No match for " << name << "(";
    for (int i=first; i<length; i++) {
      if (i>first)  E << ", ";
      if (pass[i])  pass[i]->PrintType(E.stream());
      else          E << "null";
    }
    E << ")";
    bailout = true;
  }

  if (tie) {
    parse_error E;
    E << "Multiple promotions with distance " << best_score;
    E << " for " << name << "(";
    for (int i=first; i<length; i++) {
      if (i>first)  E << ", ";
      pass[i]->PrintType(E.stream());
    }
    E << ")";
    E.newLine();
    E << "Possible choices:";
    E.newLine();
    showMatching(E, f1, pass, length, best_score);
    showMatching(E, f2, pass, length, best_score);
    bailout = true;
  }

  if (bailout) {
    for (int i=0; i<length; i++)  Delete(pass[i]);
    delete[] pass;
    return 0;
  }

  return best;
}


unary_op::opcode Int2Uop(int op)
{
  switch (op) {
    case NOT:     return unary_op::uop_not;
    case MINUS:   return unary_op::uop_neg;
  }
  internal_error E(__FILE__, __LINE__, Where());
  E << "Operator " << TokenName(op) << " not matched to any unary operator";
  return unary_op::uop_none;
}

binary_op::opcode Int2Bop(int op)
{
  switch (op) {
    case IMPLIES: return binary_op::bop_implies;
    case MOD:     return binary_op::bop_mod;
    case EQUALS:  return binary_op::bop_equals;
    case NEQUAL:  return binary_op::bop_nequal;
    case GT:      return binary_op::bop_gt;
    case GE:      return binary_op::bop_ge;
    case LT:      return binary_op::bop_lt;
    case LE:      return binary_op::bop_le;
  }
  internal_error E(__FILE__, __LINE__, Where());
  E << "Operator " << TokenName(op) << " not matched to any binary operator";
  return binary_op::bop_none;
}

assoc_op::opcode Int2Aop(int op)
{
  switch (op) {
    case AND:     return assoc_op::aop_and;
    case OR:      return assoc_op::aop_or;
    case PLUS:    return assoc_op::aop_plus;
    case TIMES:   return assoc_op::aop_times;
    case COLON:   return assoc_op::aop_colon;
    case SEMI:    return assoc_op::aop_semi;
    case COMMA:   return assoc_op::aop_union;
  }
  internal_error E(__FILE__, __LINE__, Where());
  E << "Operator " << TokenName(op) << " not matched to any associative operator";
  return assoc_op::aop_none;
}

// --------------------------------------------------------------
expr* BuildElementSet(expr* elem)
{
  if (bogus_expr::orNull(elem))  return elem;
  DCASSERT(elem->Type());
  const type* set_type = elem->Type()->getSetOfThis();
  if (0 == set_type) {
    parse_error E;
    E << "Sets of type ";
    elem->PrintType(E.stream());
    E << " are not allowed";
    Delete(elem);
    return bogus_expr::makeError();
  }
  return ShowWhatWeBuilt("set element: ",
    typeconv::castExpr(true, Where(), set_type, elem)
   );
}

// --------------------------------------------------------------
expr* BuildInterval(expr* start, expr* stop)
{
  return BuildInterval(start, stop, Share(ONE));
}

// --------------------------------------------------------------
expr* BuildInterval(expr* start, expr* stop, expr* inc)
{
  return ShowWhatWeBuilt("set interval: ",
    trinary_op::makeExpr(Where(),
      trinary_op::top_interval, start, stop, inc)
  );
}

// --------------------------------------------------------------
expr* BuildSummation(parser_list* list)
{
  if (0==list)  return 0;

  int length = CircularLength(list);

  // If the list has length 1, return the term (simple and common case)
  if (1==length) {
    // common and easy case
    expr_term* et = smart_cast <expr_term*> (list->data);
    expr* foo = et ? Share(et->term) : 0;
    DeleteCircular(list);
    return foo;
  }

  // Fill the list of exprs and flips.
  bool* flip = new bool[length];
  expr** opnds = new expr*[length];
  bool has_null = false;
  bool has_error = false;
  int oper = 0;
  for (int i=0; i<length; i++) {
    list = list->next;
    expr_term* et = smart_cast <expr_term*> (list->data);
    if (0==et || 0==et->term)  has_null = true;
    if (has_null) {
      opnds[i] = 0;
      continue;
    }
    if (MINUS == et->op) {
      oper = PLUS;
      flip[i] = true;
    } else {
      oper = et->op;
      flip[i] = false;
    }
    opnds[i] = Share(et->term);
    if (bogus_expr::isError(et->term))  has_error = true;
    if (has_error)  continue;
    if (0==i)       continue;

  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return bogus_expr::makeError();
  }

  return ShowWhatWeBuilt(0,
    assoc_op::makeExpr(Where(), Int2Aop(oper), opnds, flip, length)
  );
}

// --------------------------------------------------------------
expr* BuildProduct(parser_list* list)
{
  if (0==list)  return 0;

  int length = CircularLength(list);

  // If the list has length 1, return the term (simple and common case)
  if (1==length) {
    // common and easy case
    expr_term* et = smart_cast <expr_term*> (list->data);
    expr* foo = et ? Share(et->term) : 0;
    DeleteCircular(list);
    return foo;
  }

  // Fill the list of exprs and flips.
  bool* flip = new bool[length];
  expr** opnds = new expr*[length];
  bool has_null = false;
  bool has_error = false;
  int oper = 0;
  for (int i=0; i<length; i++) {
    list = list->next;
    expr_term* et = smart_cast <expr_term*> (list->data);
    if (0==et || 0==et->term)  has_null = true;
    if (has_null) {
      opnds[i] = 0;
      continue;
    }
    if (DIVIDE == et->op) {
      oper = TIMES;
      flip[i] = true;
    } else {
      oper = et->op;
      flip[i] = false;
    }
    opnds[i] = Share(et->term);
    if (bogus_expr::isError(et->term))  has_error = true;
    if (has_error)  continue;
    if (0==i)       continue;

  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return bogus_expr::makeError();
  }

  return ShowWhatWeBuilt(0,
    assoc_op::makeExpr(Where(), Int2Aop(oper), opnds, flip, length)
  );
}


// --------------------------------------------------------------
expr* BuildAssociative(int op, parser_list* list)
{
  if (0==list)  return 0;
  int length = CircularLength(list);
  if (1==length) {
    expr* foo = smart_cast <expr*> (list->data);
    RecycleCircular(list);
    return foo;
  }
  // build chunk of expressions
  expr** opnds = new expr*[length];
  CopyCircular(list, opnds, length);
  RecycleCircular(list);

  return ShowWhatWeBuilt(0,
    assoc_op::makeExpr(Where(), Int2Aop(op), opnds, 0, length)
  );
}

// --------------------------------------------------------------
expr* BuildBinary(expr* left, int op, expr* right)
{
  return binary_op::makeExpr(Where(), left, Int2Bop(op), right);
}

// --------------------------------------------------------------
expr* BuildUnary(int op, expr* opnd)
{
  return unary_op::makeExpr(Where(), Int2Uop(op), opnd);
}

// --------------------------------------------------------------
expr* MakeBoolConst(char* s)
{
  if (0==s) return 0;
  result c;
  DCASSERT(type::find("bool"));
  type::find("bool")->assignFromString(c, s);

  if (c.isNormal()) {
    free(s);
    return new value(Where(), type::find("bool"), c);
  }
  internal_error E(__FILE__, __LINE__, Where());
  E << "Bad boolean constant: " << s;
  free(s);
  return bogus_expr::makeError();
}

// --------------------------------------------------------------
expr* MakeIntConst(char* s)
{
  if (0==s)  return 0;
  result c;
  DCASSERT(type::find("int"));
  type::find("int")->assignFromString(c, s);
  expr* foo = new value(Where(), type::find("int"), c);
  free(s);
  return foo;
}

// --------------------------------------------------------------
expr* FindIdent(char* name)
{
  symbol* find = ModelInternal->findSymbol(name);
  if (find) {
    free(name);
    return Share(find);
  }
  parse_error E;
  E << "Unknown identifier: " << name;

  return bogus_expr::makeError();
}

// --------------------------------------------------------------
expr* BuildFunctionCall(char* n, parser_list* posparams)
{
  symbol* find = ModelType ? ModelType->findSymbol(n) : 0;
  symbol* find2 = 0;
  bool first = 0;
  if (find) {
    // we have a match within a model, add model to params
    expr* passmodel = Share((expr*) model_under_construction);
    posparams = PrependCircular(posparams, passmodel);
    first = 1;
  } else {
    find = ModelInternal ? ModelInternal->findSymbol(n) : 0;
    find2 = symbol_table::findGlobal(n);
  }

  if (0==find && 0==find2) {
    parse_error E;
    if (posparams)  E << "Unknown function " << n;
    else            E << "Unknown identifier: " << n;
    free(n);
    DeleteCircular(posparams);
    return bogus_expr::makeError();
  }
  free(n);

  // Dump parameters to an array
  int length = CircularLength(posparams);
  expr** pass;
  if (length) {
    pass = new expr*[length];
    CopyCircular(posparams, pass, length);
  } else {
    pass = 0;
  }
  RecycleCircular(posparams);

  function* best = FindBest(find, find2, pass, length, first);
  if (0==best)  return bogus_expr::makeError();
  return ShowWhatWeBuilt(0,
    expr::makeFunctionCall(Where(), best, pass, length)
  );
}


// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class compile_init : public initializer {
    public:
        compile_init();
    protected:
        virtual void execute();
};
static compile_init the_compile_initializer;

compile_init::compile_init() : initializer(__FILE__, 2)
{
    builds_resource("compile.cc");
    needs_resource("Debug");
}

void compile_init::execute()
{
    initialize_msg(parser_debug,
        "parser",
        "When set, very low-level parser messages are displayed.",
        get_object("Debug")
    );
    initialize_msg(compiler_debug,
        "compiler",
        "When set, low-level compiler messages are displayed.",
        get_object("Debug")
    );
#ifdef PARSER_DEBUG
    parser_debug.Activate();
#endif
#ifdef COMPILE_DEBUG
    compiler_debug.Activate();
#endif
}

// ******************************************************************
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// ******************************************************************

void InitCompiler(parse_module* parent)
{
  pm = parent;

  // init globals here.
  result one(1L);
  ONE = new value(location::NOWHERE(), type::find("int"), one);

  MeasureNames.Clear();

  // Compiler stats
  list_depth = 0;
}

int Compile(parse_module* parent)
{
  DCASSERT(parent);
  pm = parent;

  int ans = yyparse();

  if (compiler_debug.start()) {
    compiler_debug << "Done compiling\n";
    compiler_debug << "List depth: " << list_depth << "\n";
    compiler_debug.stop();
  }

  return ans;
}


