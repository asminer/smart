
#include "compile.h"
#include "lexer.h"
#include "../Options/options.h"
#include "../Streams/streams.h"
#include "../ExprLib/exprman.h"
#include "../ExprLib/functions.h"
#include "../SymTabs/symtabs.h"
#include "../ExprLib/strings.h"
#include "../ExprLib/formalism.h"
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
exprman* em;
named_msg parser_debug;
named_msg compiler_debug;

// Expression for the integer constant 1.
expr* ONE = 0;

/// Symbol table of built-in functions
symbol_table* Funcs = 0;

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

void yyerror(const char *msg)
{
  DCASSERT(pm);
  if (pm->startError()) {
    pm->cerr() << msg;
    pm->stopError();
  }
}

void Reducing(const char* msg)
{
  DCASSERT(pm);
  if (parser_debug.startReport()) {
    parser_debug.report() << "reducing rule:\n\t\t";
    parser_debug.report().Put(msg);
    parser_debug.report().Put('\n');
    parser_debug.stopIO();
  }
}

inline expr* ShowNewStatement(const char* what, expr* f)
{
  // Make noise as appropriate
  if (compiler_debug.startReport()) {
    compiler_debug.report() << "built ";
    if (what)   compiler_debug.report() << what;
    else        compiler_debug.report() << "statement: ";
    if (f)  f->Print(compiler_debug.report(), 4);
    else    compiler_debug.report().Put("    null\n");
    compiler_debug.stopIO();
  }
  return f;
}

template <class EXPR>
inline EXPR* ShowWhatWeBuilt(const char* what, EXPR* f)
{
  // Make noise as appropriate
  if (compiler_debug.startReport()) {
    compiler_debug.report() << "built ";
    if (what)   compiler_debug.report() << what;
    else        compiler_debug.report() << "expression: ";
    if (f)  f->Print(compiler_debug.report(), 0);
    else    compiler_debug.report().Put("null");
    compiler_debug.report() << "\t type: ";
    const type* t = f ? f->Type() : 0;
    if (t)  compiler_debug.report().Put(t->getName());
    else    compiler_debug.report().Put("null");
    compiler_debug.report().Put('\n');
    compiler_debug.stopIO();
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
  
  virtual bool Print(OutputStream &s, int width) const;
  virtual bool Equals(const shared_object* o) const;
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

bool expr_term::Print(OutputStream &s, int) const
{
  s << "(";
  s << TokenName(op);
  s << ", ";
  if (term)   term->Print(s, 0);
  else        s.Put("null");
  s << ")";
  return true;
}

bool expr_term::Equals(const shared_object* o) const
{
  return (this == o);
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
    em->makeAssocOp(Filename(), Linenumber(), exprman::aop_semi, 
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
  if (!em->isOrdinary(s))  return;

  list_of_statements = AppendCircular(list_of_statements, s);
}

// --------------------------------------------------------------
parser_list* AppendExpression(int behv, parser_list* list, expr* item)
{
  switch (behv) {
    case 0:  // add regardless.
        return AppendCircular(list, item);

    case 1:  // add, unless item is null or error
        if (em->isOrdinary(item))
          return AppendCircular(list, item);
        return list;
  
    case 2:  // collapse on null or error.
        if (0==list)
          return AppendCircular(0, item);  // correct regardless
        if (! em->isOrdinary(item) ) {
          // collapse the list
          DeleteCircular(list);
          return AppendCircular(0, item);
        }
        if (! em->isOrdinary((expr*) list->data) ) {
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
  if (!em->isOrdinary(x))  return 0;
  DCASSERT(x->Type());
  if (! x->Type()->matches("bool")) {
    if (pm->startError()) {
      pm->cerr() << "Expected boolean constraint, got ";
      x->PrintType(pm->cerr());
      pm->cerr() << ", ignoring";
      pm->stopError();
    }
    return 0;
  }
  const type* ICP_TYPE = em->findOWDType("dcp");
  symbol* best = em->findFunction(ICP_TYPE, "constraint");
  if (0==best)  return 0;
  DCASSERT(best->Next() == 0);
  expr** pass = new expr* [2];
  pass[0] = Share((expr*) model_under_construction);
  pass[1] = x;
  return ShowWhatWeBuilt(0,
    em->makeFunctionCall(Filename(), Linenumber(), best, pass, 2)
  );
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, expr* v)
{
  return ShowNewStatement(
    "option statement:\n",
    em->makeOptionStatement(Filename(), Linenumber(), o, v)
  );
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, char* n)
{
  expr* foo;
  option_const* oc = o ? o->FindConstant(n) : 0;
  if (0==oc) {
    if (o && pm->startError()) {
      pm->cerr() << "Illegal value " << n << " for option " << o->Name();
      pm->cerr() << ", ignoring";
      pm->stopError();
    }
    foo = 0;
  } else {
    foo = em->makeOptionStatement(Filename(), Linenumber(), o, oc);
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

  // Count valid option_consts.
  int length = CircularLength(list);
  int actual_length = 0;
  for (int i=0; i<length; i++) {
    list = list->next;
    shared_string* s = smart_cast <shared_string*> (list->data);
    const char* name = s ? s->getStr() : 0;
    option_const* oc = name ? o->FindConstant(name) : 0;
    if (oc) {
      actual_length++;
      continue;
    }
    if (name) if (pm->startError()) {
      pm->cerr() << "Illegal value " << name << " for option " << o->Name();
      pm->cerr() << ", ignoring";
      pm->stopError();
    }
  }

  if (actual_length<1) {
    DeleteCircular(list);
    return 0;
  }

  // Build list of option_consts
  option_const** vlist = new option_const*[actual_length];
  int i=0;
  for (int j=0; j<length; j++) {
    list = list->next;
    shared_string* s = smart_cast <shared_string*> (list->data);
    const char* name = s ? s->getStr() : 0;
    option_const* oc = name ? o->FindConstant(name) : 0;
    if (0==oc) continue;

    vlist[i] = oc;
    i++;
  }
  DeleteCircular(list);

  return ShowNewStatement(
  "option statement:\n",
    em->makeOptionStatement(Filename(), Linenumber(), o, check, vlist, length)
  );
}

// --------------------------------------------------------------
option* BuildOptionHeader(char* name)
{
  if (0==name) return 0;
  option* answer = pm ? pm->findOption(name) : 0;

  if (0==answer) if (pm->startError()) {
    pm->cerr() << "Unknown option " << name;
    pm->stopError();
  }
  free(name);
  return answer;
}

// --------------------------------------------------------------
expr* BuildExprStatement(expr *x)
{
  if (!em->isOrdinary(x))  return 0;
  return em->makeExprStatement(Filename(), Linenumber(), x);
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
    if (pm->startError()) {
      pm->cerr() << "no bounding needed for boolean identifiers";
      pm->stopError();
    }
    DeleteCircular(namelist);
    return 0;
  }
  int N = CircularLength(namelist);
  symbol** names = new symbol*[N];
  CopyCircular(namelist, names, N);
  return ShowNewStatement(0,
    em->makeModelVarDecs(Filename(), Linenumber(), 
      model_under_construction, em->INT, values, names, N)
  );
}

// --------------------------------------------------------------
expr* BuildBools(char* typ, parser_list* namelist)
{
  bool oktype = (0 != strcmp(typ, "int"));
  if (!oktype) {
    if (pm->startError()) {
      pm->cerr() << "unbounded integer identifiers";
      pm->stopError();
    }
    DeleteCircular(namelist);
    return 0;
  }
  int N = CircularLength(namelist);
  symbol** names = new symbol*[N];
  CopyCircular(namelist, names, N);
  return ShowNewStatement(0,
    em->makeModelVarDecs(Filename(), Linenumber(), 
      model_under_construction, em->BOOL, 0, names, N)
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
  if (ModelInternal->FindSymbol(ident)) {
    if (pm->startError()) {
      pm->cerr() << "Duplicate identifier " << ident << " within model";
      pm->stopError();
    }
    free(ident);
    return true;
  }
  return false;
}

// --------------------------------------------------------------
void StartModel()
{
  if (em) {
    const type* ICP_TYPE = em->findOWDType("dcp");
    DCASSERT(0==ModelType);
    DCASSERT(0==model_under_construction);
    DCASSERT(0==ModelInternal);
    char* name = strdup(" ");
    model_under_construction = em->makeModel(0, -1, ICP_TYPE, name, 0, 0);
    ModelInternal = MakeSymbolTable();
    ModelType = smart_cast <const formalism*> (ICP_TYPE);
    DCASSERT(ModelType);
  }
}

// --------------------------------------------------------------
void FinishModel()
{
  DCASSERT(model_under_construction);
  expr* block = MakeStatementBlock(list_of_statements);
  int ns = ModelExternal.Length();
  ModelExternal.Sort();
  symbol** visible = ModelExternal.MakeArray();

  em->finishModelDef(model_under_construction, block, visible, ns);
  
  delete ModelInternal;
  ModelInternal = 0;
  
  pm->num_measures = MeasureNames.Length();
  if (pm->num_measures) {
    pm->measure_names = MeasureNames.CopyAndClear();
    pm->measure_calls = new expr*[pm->num_measures];
  }

  // Build measure calls
  for (int i=0; i<pm->num_measures; i++) {
    pm->measure_calls[i] = em->makeMeasureCall(Filename(), Linenumber(),
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

  symbol* ms = em->makeModelSymbol(Filename(), Linenumber(), 0, ident);
  ModelInternal->AddSymbol(ms);
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
  rhs = em->makeFunctionCall(Filename(), Linenumber(), who, pass, 2);
 
  symbol* wrap = em->makeModelSymbol(Filename(), Linenumber(), typ, ident);

  // Add measure to symbol tables
  DCASSERT(ModelInternal);
  ModelInternal->AddSymbol(wrap);
  
  ModelExternal.Insert(wrap);

  // Add this to the measure list
  MeasureNames.Append(ident);

  return ShowNewStatement("measure assignment:\n",
    em->makeModelMeasureAssign(Filename(), Linenumber(),
        model_under_construction, wrap, rhs)
  );
}

// --------------------------------------------------------------
expr* BuildMaximize(char* ident, expr* rhs)
{
  const type* ICP_TYPE = em->findOWDType("dcp");
  symbol* who = em->findFunction(ICP_TYPE, "maximize");
  return BuildMeasure(em->REAL, ident, who, rhs);
}

// --------------------------------------------------------------
expr* BuildMinimize(char* ident, expr* rhs)
{
  const type* ICP_TYPE = em->findOWDType("dcp");
  symbol* who = em->findFunction(ICP_TYPE, "minimize");
  return BuildMeasure(em->REAL, ident, who, rhs);
}

// --------------------------------------------------------------
expr* BuildSatisfiable(char* ident, expr* rhs)
{
  const type* ICP_TYPE = em->findOWDType("dcp");
  symbol* who = em->findFunction(ICP_TYPE, "satisfiable");
  return BuildMeasure(em->BOOL, ident, who, rhs);
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
void showMatching(symbol* find, expr** pass, int np, int best_score)
{
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = smart_cast <function*> (ptr);
    if (0==f)      continue;
    int score = f->TypecheckParams(pass, np);
    if (score != best_score)  continue;
    f->PrintHeader(pm->cerr(), true);
    pm->newLine();
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
    if (pm->startError()) {
      pm->cerr() << "No match for " << name << "(";
      for (int i=first; i<length; i++) {
        if (i>first)  pm->cerr() << ", ";
        if (pass[i])  pass[i]->PrintType(pm->cerr());
        else          pm->cerr().Put("null");
      }
      pm->cerr() << ")";
      pm->stopError();
    }
    bailout = true;
  }

  if (tie) {
    if (pm->startError()) {
      pm->cerr() << "Multiple promotions with distance " << best_score;
      pm->cerr() << " for " << name << "(";
      for (int i=first; i<length; i++) {
        if (i>first)  pm->cerr() << ", ";
        pass[i]->PrintType(pm->cerr());
      }
      pm->cerr() << ")";
      pm->newLine();
      pm->cerr() << "Possible choices:";
      pm->newLine();
      showMatching(f1, pass, length, best_score);
      showMatching(f2, pass, length, best_score);
      pm->stopError();
    }
    bailout = true; 
  }

  if (bailout) {
    for (int i=0; i<length; i++)  Delete(pass[i]);
    delete[] pass;
    return 0;
  }

  return best;
}


exprman::unary_opcode Int2Uop(int op) 
{
  switch (op) {
    case NOT:     return exprman::uop_not;
    case MINUS:   return exprman::uop_neg;
  }
  if (pm->startInternal(__FILE__, __LINE__)) {
    pm->internal() << "Operator " << TokenName(op);
    pm->internal() << " not matched to any unary operator";
    pm->stopError();
  }
  return exprman::uop_none;
}

exprman::binary_opcode Int2Bop(int op) 
{
  switch (op) {
    case IMPLIES: return exprman::bop_implies;
    case MOD:     return exprman::bop_mod;
    case EQUALS:  return exprman::bop_equals;
    case NEQUAL:  return exprman::bop_nequal;
    case GT:      return exprman::bop_gt;
    case GE:      return exprman::bop_ge;
    case LT:      return exprman::bop_lt;
    case LE:      return exprman::bop_le;
  }
  if (pm->startInternal(__FILE__, __LINE__)) {
    pm->internal() << "Operator " << TokenName(op);
    pm->internal() << " not matched to any binary operator";
    pm->stopError();
  }
  return exprman::bop_none;
}

exprman::assoc_opcode Int2Aop(int op) 
{
  switch (op) {
    case AND:     return exprman::aop_and;
    case OR:      return exprman::aop_or;
    case PLUS:    return exprman::aop_plus;
    case TIMES:   return exprman::aop_times;
    case COLON:   return exprman::aop_colon;
    case SEMI:    return exprman::aop_semi;
    case COMMA:   return exprman::aop_union;
  }
  if (pm->startInternal(__FILE__, __LINE__)) {
    pm->internal() << "Operator " << TokenName(op);
    pm->internal() << " not matched to any associative operator";
    pm->stopError();
  }
  return exprman::aop_none;
}

// --------------------------------------------------------------
expr* BuildElementSet(expr* elem)
{
  if (0==elem || em->isError(elem))  return elem;
  DCASSERT(elem->Type());
  const type* set_type = elem->Type()->getSetOfThis();
  if (0 == set_type) {
    if (pm->startError()) {
      pm->cerr() << "Sets of type ";
      elem->PrintType(pm->cerr());
      pm->cerr() << " are not allowed";
      pm->stopError();
    }
    Delete(elem);
    return em->makeError();
  }
  return ShowWhatWeBuilt("set element: ",
    em->makeTypecast(Filename(), Linenumber(), set_type, elem)
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
    em->makeTrinaryOp(Filename(), Linenumber(), 
      exprman::top_interval, start, stop, inc)
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
    if (em->isError(et->term))  has_error = true;
    if (has_error)  continue;
    if (0==i)       continue;

  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return em->makeError();
  }

  exprman::assoc_opcode aop = Int2Aop(oper);
  return ShowWhatWeBuilt(0, 
    em->makeAssocOp(Filename(), Linenumber(), aop, opnds, flip, length)
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
    if (em->isError(et->term))  has_error = true;
    if (has_error)  continue;
    if (0==i)       continue;

  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return em->makeError();
  }

  exprman::assoc_opcode aop = Int2Aop(oper);
  return ShowWhatWeBuilt(0, 
    em->makeAssocOp(Filename(), Linenumber(), aop, opnds, flip, length)
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
  exprman::assoc_opcode aop = Int2Aop(op);

  return ShowWhatWeBuilt(0,
    em->makeAssocOp(Filename(), Linenumber(), aop, opnds, 0, length)
  );
}

// --------------------------------------------------------------
expr* BuildBinary(expr* left, int op, expr* right)
{
  exprman::binary_opcode bop = Int2Bop(op);
  return em->makeBinaryOp(Filename(), Linenumber(), left, bop, right);
}

// --------------------------------------------------------------
expr* BuildUnary(int op, expr* opnd)
{
  exprman::unary_opcode uop = Int2Uop(op);
  return em->makeUnaryOp(Filename(), Linenumber(), uop, opnd);
}

// --------------------------------------------------------------
expr* MakeBoolConst(char* s)
{
  if (0==s) return 0;
  result c;
  DCASSERT(em->BOOL);
  em->BOOL->assignFromString(c, s);

  if (c.isNormal()) {
    free(s);
    return em->makeLiteral(Filename(), Linenumber(), em->BOOL, c);
  }
  if (pm->startInternal(__FILE__, __LINE__)) {
    pm->internal() << "Bad boolean constant: " << s;
    pm->stopError();
  }
  free(s);
  return em->makeError();
}

// --------------------------------------------------------------
expr* MakeIntConst(char* s)
{
  if (0==s)  return 0;
  result c;
  DCASSERT(em->INT);
  em->INT->assignFromString(c, s);
  expr* foo = em->makeLiteral(Filename(), Linenumber(), em->INT, c);
  free(s);
  return foo;
}

// --------------------------------------------------------------
expr* FindIdent(char* name)
{
  symbol* find = ModelInternal->FindSymbol(name);
  if (find) {
    free(name);
    return Share(find);
  }
  if (pm->startError()) {
    pm->cerr() << "Unknown identifier: " << name;
    pm->stopError();
  }

  return em->makeError();
}

// --------------------------------------------------------------
expr* BuildFunctionCall(char* n, parser_list* posparams)
{
  symbol* find = em->findFunction(ModelType, n);
  symbol* find2 = 0;
  bool first = 0;
  if (find) {
    // we have a match within a model, add model to params
    expr* passmodel = Share((expr*) model_under_construction);
    posparams = PrependCircular(posparams, passmodel);
    first = 1;
  } else {
    find = ModelInternal ? ModelInternal->FindSymbol(n) : 0;
    find2 = Funcs->FindSymbol(n);
  }

  if (0==find && 0==find2) {
    if (pm->startError()) {
      if (posparams)  pm->cerr() << "Unknown function ";
      else            pm->cerr() << "Unknown identifier: ";
      pm->cerr() << n;
      pm->stopError();
    }
    free(n);
    DeleteCircular(posparams);
    return em->makeError();
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
  if (0==best)  return em->makeError();
  return ShowWhatWeBuilt(0,
    em->makeFunctionCall(Filename(), Linenumber(), best, pass, length)
  );
}


// ******************************************************************
// *                                                                *
// *                      Front-end  functions                      *
// *                                                                *
// ******************************************************************

void InitCompiler(parse_module* parent)
{
  pm = parent;
  em = pm ? pm->em : 0;

  // init globals here.
  if (em) {
    result one(1L);
    ONE = em->makeLiteral(0, -1, em->INT, one);
  } else {
    ONE = 0;
  }

  MeasureNames.Clear();

  option* debug = pm ? pm->findOption("Debug") : 0;
  parser_debug.Initialize(debug,
    "parser_debug",
    "When set, very low-level parser messages are displayed.",
    false
  );
  compiler_debug.Initialize(debug,
    "compiler_debug",
    "When set, low-level compiler messages are displayed.",
    false
  );
#ifdef PARSER_DEBUG
  parser_debug.active = true;
#endif
#ifdef COMPILE_DEBUG
  compiler_debug.active = true;
#endif

  // Compiler stats
  list_depth = 0;
}

int Compile(parse_module* parent)
{
  DCASSERT(parent);
  pm = parent;

  int ans = yyparse();

  if (compiler_debug.startReport()) {
    compiler_debug.report() << "Done compiling\n";
    compiler_debug.report() << "List depth: " << list_depth << "\n";
    compiler_debug.stopIO();
  }

  return ans;
}


