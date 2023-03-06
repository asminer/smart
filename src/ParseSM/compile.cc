
#include "compile.h"
#include "lexer.h"

#include "../Options/optman.h"
#include "../Options/options.h"
#include "../Options/opt_enum.h"

#include "../Utils/strings.h"
#include "../Utils/init_opts.h"

#include "../ExprLib/values.h"
#include "../ExprLib/symbols.h"
#include "../ExprLib/functions.h"
#include "../ExprLib/mod_def.h"
#include "../ExprLib/formalism.h"
#include "../ExprLib/symb_tab.h"
#include "../ExprLib/bogus.h"
#include "../ExprLib/unary.h"
#include "../ExprLib/binary.h"
#include "../ExprLib/trinary.h"
#include "../ExprLib/assoc.h"
#include "../ExprLib/casting.h"

#include "../include/heap.h"
#include "parse_sm.h"
#include <string.h>
#include <stdlib.h>

// Put this one last.
#include "ParseSM/smartyacc.hh"

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


    TO DO:

      Currently, user-defined functions are added to the
      same symbol table as builtin functions.  That means
      that whoever invokes the parser can "keep" these
      functions, which may not be the desired behavior.
      Need to think more about the parser interface...
*/

// should be built by bison.
int yyparse();

//
// Helper class - stack of options
//
class optstack {
    const option_enum* data[16]; // max depth
    int top_index;
  public:
    optstack() {
      top_index = 0;
    }
    inline bool isEmpty() const { return top_index < 0; }
    inline const option_enum* top() const {
      return (top_index > 0) ? data[top_index] : 0;
    }
    inline bool push(const option_enum* oc) {
      if (top_index >= 15) return false;
      data[++top_index] = oc;
      return true;
    }
    inline const option_enum* pop() {
      if (top_index<0) return 0;
      return data[top_index--];
    }
};

/* =====================================================================

  Global variables.

   ===================================================================== */

parse_module* pm;
debugging_msg parser_debug;
debugging_msg compiler_debug;

// Expression for the integer constant 1.
expr* ONE = 0;

/// Current stack of options
optstack Options;

/// Current stack of for-loop iterators
symbol* Iterators = nullptr;

/// Symbol table of arrays.
symbol_table* Arrays = 0;

/// Symbol table of "constants", i.e., functions without parameters.
symbol_table* Constants = 0;

/// Depth of converges
int converge_depth;

/// Current function under construction (for finding formal parameters)
function* function_under_construction = 0;

/// Type of model under construction, if any.
const formalism* ModelType = 0;

/// Model under construction
model_def* model_under_construction = 0;

/// Internal symbols for model under construction.
symbol_table* ModelInternal = 0;

/// External (visible) symbols for model under construction.
HeapOfPointers <symbol> ModelExternal;

inline bool WithinFor() { return (Iterators); }

inline bool WithinConverge() { return (converge_depth>0); }

inline bool WithinModel() { return ModelType; }

inline bool WithinBlock() {
  return WithinFor() || WithinConverge() || WithinModel();
}

inline bool ignoringBadModelDecl() {
  return WithinModel() && (0==model_under_construction);
}

// Compiler stats:

long list_depth;

inline int Compare(const symbol* a, const symbol* b)
{
  DCASSERT(a);
  DCASSERT(b);
  return strcmp(a->Name(), b->Name());
}

/* =====================================================================

  Stack of iterators "symbol table"

   ===================================================================== */

// --------------------------------------------------------------
int chainLength(symbol* list)
{
    int i;
    for (i=0; list; list=list->Next()) i++;
    return i;
}

// --------------------------------------------------------------
void fillFromChain(symbol** indexes, int dim, symbol* list)
{
    int i = dim-1;
    for (; list; list=list->Next()) {
        CHECK_RANGE(__FILE__, __LINE__, 0, i, dim);
        indexes[i] = Share(list);
        i--;
    }
}

// --------------------------------------------------------------
symbol* findInChain(symbol* list, const char* name)
{
    for (; list; list=list->Next()) {
        if (0==strcmp(list->Name(), name)) return list;
    }
    return nullptr;
}

/* =====================================================================

  I/O and such.

   ===================================================================== */

parse_error::parse_error(bool err) : error_msg(err ? "ERROR" : "WARNING")
{
    Out << ' ' << Where() << ':';
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

const type* MakeType(bool proc, char* modif, const type* t)
{
  modifier m = 0;
  if (modif) {
    m = type::findModifier(modif);
    if (NO_SUCH_MODIFIER == m) {
      internal_error E(__FILE__, __LINE__, Where());
      E << "Bad type modifier: " << modif;
      return 0;
    }
  }
  const type* answer;
  if (modif) answer = ModifyType(m, t); else answer = t;
  if (proc) answer = ProcifyType(answer);
  free(modif);
  return answer;
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

void ShowPosCall(std::ostream &s, const char* name, expr** pass, int first, int length)
{
  s << name << "(";
  for (int i=first; i<length; i++) {
    if (i>first)  s << ", ";
    if (pass[i])  pass[i]->PrintType(s);
    else          s << "null";
  }
  s << ")";
}

void ShowNamedCall(std::ostream &s, const char* name, symbol** pass, int length)
{
  s << name << "(";
  bool comma = false;
  for (int i=0; i<length; i++) {
    if (0==pass) continue;  // possible?
    const char* n = pass[i]->Name();
    if (0==n) continue;
    if ('-' == n[0]) continue;  // hidden parameter
    if (comma)  s << ", ";
    pass[i]->PrintType(s);
    s << ' ' << n;
    comma = true;
  }
  s << ")";
}

/* =====================================================================

  Struct for model calls

   ===================================================================== */

class model_call_data : public shared_object {
public:
  model_def* model1;
  symbol* model2;
  expr** pass;
  int np;
public:
  model_call_data(model_def* m, expr** p, int n);
  model_call_data(symbol* m);
  virtual ~model_call_data();

  void Trash();

  virtual bool Print(std::ostream &s, int) const;
};

model_call_data::model_call_data(model_def* m, expr** p, int n)
 : shared_object()
{
  model1 = m;
  model2 = 0;
  pass = p;
  np = n;
}

model_call_data::model_call_data(symbol* m)
 : shared_object()
{
  model1 = 0;
  model2 = m;
  pass = 0;
  np = 0;
}

model_call_data::~model_call_data()
{
}

void model_call_data::Trash()
{
  for (int i=0; i<np; i++)  Delete(pass[i]);
  delete[] pass;
  pass = 0;
  np = 0;
}

bool model_call_data::Print(std::ostream &s, int) const
{
  DCASSERT(0);
  return false;
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
  for (parser_list* ptr = list->next; ptr != list; ptr=ptr->next) {
    length++;
  }
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
  return assoc_op::makeExpr(
      Where(), assoc_op::aop_semi, opnds, 0, length
  );
}


bool BadIteratorList(char* n, parser_list* list)
{
    // Convert circular list to linear one
    if (list) {
        parser_list* next = list->next;
        list->next = nullptr;
        list = next;
    }
    // Reverse the linear list
    parser_list* reversed = nullptr;
    while (list) {
        parser_list* next = list->next;
        list->next = reversed;
        reversed = list;
        list = next;
        shared_string* forml = smart_cast <shared_string*> (reversed->data);
        DCASSERT(forml);
    }

    // Make sure iterator names match reversed list names
    //
    // Keep track of which symbols mismatched
    const symbol* it_mismatch = nullptr;
    const shared_string* pl_mismatch = nullptr;
    parser_list* curr = reversed;

    for (symbol* nth = Iterators; nth; nth=nth->Next()) {
        if (!curr) {
            // Out of list elements
            it_mismatch = nth;
            pl_mismatch = nullptr;
            break;
        }
        shared_string* forml = smart_cast <shared_string*> (curr->data);
        DCASSERT(forml);
        if (strcmp(nth->Name(), forml->getStr())) {
            it_mismatch = nth;
            pl_mismatch = forml;
            // List is reversed, so keep going, so we
            // can show the first mismatch
        }
        curr = curr->next;
    }
    if (curr) {
        // Too many list elements
        it_mismatch = nullptr;
        pl_mismatch = smart_cast <shared_string*> (curr->data);
    }

    //
    // Display errors as appropriate
    //
    if (it_mismatch || pl_mismatch) {
        parse_error E;
        if (!it_mismatch || !pl_mismatch) {
            E << "Dimension of array " << n << " does not match iterators";
        } else {
            E << "Array " << n << " expecting index ";
            E << *it_mismatch << ", got " << *pl_mismatch;
        }
        free(n);
        n = nullptr;
    }

    //
    // Delete list
    //
    while (reversed) {
        parser_list* next = reversed->next;
        delete reversed;
        reversed = next;
    }

    // name not deleted? was not bad so return false;
    // otherwise, null name means bad so return true.
    return !n;
}

/* =====================================================================

  Singleton class for named parameters

   ===================================================================== */

class named_paramarray {
  symbol** pass;
  int passalloc;
  int passlen;

private:
  named_paramarray();
  ~named_paramarray();

public:
  static named_paramarray& theNamedList() {
    static named_paramarray* TheOne = 0;
    if (0==TheOne) TheOne = new named_paramarray();
    return *TheOne;
  }

  // not the best encapsulation, but better than before.
  inline symbol** getList() const { return pass; }
  inline int getLength() const { return passlen; }

  // Initialize from a list, which is destroyed
  void initFromList(parser_list* &namedparams);

  // Recycle the current list
  void recycle();
};

named_paramarray::named_paramarray()
{
  pass = 0;
  passalloc = 0;
  passlen = 0;
}

named_paramarray::~named_paramarray()
{
  free(pass);
}

void named_paramarray::initFromList(parser_list* &namedparams)
{
  //
  // Determine list length
  //
  passlen = CircularLength(namedparams);

  //
  // Enlarge our static array?
  //
  if (passlen > passalloc) {
    passalloc = 16;
    while (passlen > passalloc) passalloc *= 2;
    pass = (symbol**) realloc(pass, passalloc * sizeof(symbol*));
  }

  //
  // Dump parameters to our static array
  //
  if (passlen) {
    CopyCircular(namedparams, pass, passlen);
  }
  RecycleCircular(namedparams);
}

void named_paramarray::recycle()
{
  for (int i=0; i<passlen; i++) {
    Delete(pass[i]);
  }
  passlen = 0;
}

/* =====================================================================

  Singleton class for positional parameters

   ===================================================================== */

class pos_paramarray {
  expr** pass;
  int passalloc;

private:
  pos_paramarray();
  ~pos_paramarray();

public:
  static pos_paramarray& thePosList() {
    static pos_paramarray* TheOne = 0;
    if (0==TheOne) TheOne = new pos_paramarray();
    return *TheOne;
  }

  inline void alloc(int reqd) {
    if (reqd > passalloc) Expand(reqd);
  }

private:
  void Expand(int reqd);

public:
  // not the best encapsulation, but better than before.
  inline expr** getList() const { return pass; }
  inline int maxList() const { return passalloc; }

  expr** Compactify(int len);
  void recycle(int len);
};

pos_paramarray::pos_paramarray()
{
  pass = 0;
  passalloc = 0;
}

pos_paramarray::~pos_paramarray()
{
  free(pass);
}

void pos_paramarray::Expand(int reqd)
{
  passalloc = 16;
  while (passalloc < reqd) passalloc *= 2;
  pass = (expr**) realloc(pass, passalloc * sizeof(expr*));
}

expr** pos_paramarray::Compactify(int len)
{
  if (0==len) return 0;

  DCASSERT(len <= passalloc);

  expr** compact = new expr*[len];
  for (int i=0; i<len; i++) {
    compact[i] = pass[i];
    pass[i] = 0;
  }
  return compact;
}

void pos_paramarray::recycle(int len)
{
  DCASSERT(len <= passalloc);
  for (int i=0; i<len; i++) {
    Delete(pass[i]);
  }
}


// ******************************************************************
// *                                                                *
// *                     List-related functions                     *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
parser_list* AppendStatement(parser_list* list, expr* s)
{
  if (bogus_expr::orNull(s))  return list;

  // Do we need to save the statement, or can we just execute it?
  if (!WithinBlock()) {
    traverse_data x(traverse_data::Compute);
    result answer(0L);
    x.answer = &answer;
    s->Compute(x);
    Delete(s);
    return list;
  }

  // Save the statement
  return AppendCircular(list, s);
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
        if (bogus_expr::orNull(item)) {
          // collapse the list
          DeleteCircular(list);
          return AppendCircular(0, item);
        }
        if (bogus_expr::orNull((expr*)list->data)) {
          // list is already collapsed
          Delete(item);
          return list;
        }
        return AppendCircular(list, item);
  } // switch
  return 0;
}

// --------------------------------------------------------------
parser_list* AppendName(parser_list* list, char* ident)
{
  shared_string* i = new shared_string(ident);
  return AppendCircular(list, i);
}

// --------------------------------------------------------------
parser_list* AppendSymbol(parser_list* list, symbol* p, const char* kind)
{
  if (bogus_expr::orNull(p))  return list;
  // Check for duplicate names
  int length = CircularLength(list);
  parser_list* ptr = list;
  const char* pname = p->Name();
  DCASSERT(pname);
  for (int i=0; i<length; i++) {
    ptr = ptr->next;
    symbol* fp = smart_cast <symbol*> (ptr->data);
    DCASSERT(fp);
    const char* fpname = fp->Name();
    DCASSERT(fpname);
    if (0==strcmp(pname, fpname)) {
      parse_error E;
      E << "Duplicate " << kind << " `" << fpname << "'";
      Delete(p);
      return list;
    }
  }

  // no duplicates, go ahead and add
  return AppendCircular(list, p);
}

// --------------------------------------------------------------
parser_list* AppendGeneric(parser_list* list, void* obj)
{
  return AppendCircular(list, (shared_object*) obj);
}

// --------------------------------------------------------------
parser_list* AppendTerm(parser_list* list, int op, expr* t)
{
  // TBD: what about null or error expressions?
  expr_term* foo = new expr_term(op, t);
  return AppendCircular(list, foo);
}


// ******************************************************************
// *                                                                *
// *                  Statement-related  functions                  *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
expr* BuildForLoop(int count, parser_list* stmts)
{
  // Construct iterators.
  symbol** iters = 0;
  if (count>0) {
    iters = new symbol*[count];
    for (int d=count-1; d>=0; d--) {
      iters[d] = Iterators;
      Iterators = Iterators ? Iterators->Next() : nullptr;
    };
    // check for stack underflow
    if (0==iters[0]) {
      internal_error E(__FILE__, __LINE__, Where());
      E << "Iterator stack underflow";
      for (int d=0; d<count; d++) Delete(iters[d]);
      delete[] iters;
      return 0;
    }
  }

  // Construct statement block.
  expr* block = MakeStatementBlock(stmts);

  // Construct For Loop.
  return ShowNewStatement(
      "for loop:\n",
      expr::makeForLoop(Where(), iters, count, block)
  );
}


// --------------------------------------------------------------
expr* FinishConverge(parser_list* stmts)
{
  converge_depth--;

  // Construct statement block.
  expr* block = MakeStatementBlock(stmts);

  // Construct converge statement.
  return ShowNewStatement(
      "converge:\n",
      expr::makeConverge(Where(), block, (0==converge_depth))
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

// Helper for BuildOptionStatement and StartOptionBlock
option_enum* FindOptionConstant(option* o, char* n)
{
  option_enum* oc = o ? o->FindConstant(n) : 0;
  if (0==oc) {
    if (o) {
      parse_error E;
      E << "Illegal value " << n << " for option " << o->Name();
      E << ", ignoring";
    }
  }
  free(n);
  return oc;
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, char* n)
{
  option_enum* oc = FindOptionConstant(o, n);
  expr* foo;
  if (oc) {
    foo = expr::makeOptionStatement(Where(), o, oc);
  } else {
    foo = 0;
  }
  return ShowNewStatement("option statement:\n", foo);
}

// --------------------------------------------------------------
expr* StartOptionBlock(option* o, char* n)
{
  option_enum* oc = FindOptionConstant(o, n);
  expr* foo;
  if (oc) {
    if (!Options.push(oc)) {
      parse_error E;
      E << "Nesting of option statements is too deep";
    }
    foo = expr::makeOptionStatement(Where(), o, oc);
  } else {
    foo = 0;
  }
  return ShowNewStatement("option statement:\n", foo);
}

// --------------------------------------------------------------
expr* FinishOptionBlock(expr* os, parser_list* list)
{
  Options.pop();
  list = PrependCircular(list, os);
  return ShowNewStatement("option block:\n",
    MakeStatementBlock(list)
  );
}

// --------------------------------------------------------------
expr* BuildOptionStatement(option* o, bool check, parser_list* list)
{
  if (0==o) {
    DeleteCircular(list);
    return 0;
  }
  if (0==list)  return 0;

  // Build another circular list of option_enums.
  int length = CircularLength(list);
  parser_list* oclist = 0;
  for (int i=0; i<length; i++) {
    list = list->next;
    shared_string* s = smart_cast <shared_string*> (list->data);
    const char* name = s ? s->getStr() : 0;
    option_enum* oc = name ? o->FindConstant(name) : 0;
    if (oc) {
      oclist = AppendGeneric(oclist, oc);
      continue;
    }
    if (name) {
      parse_error E;
      E << "Illegal value " << name << " for option " << o->Name();
      E << ", ignoring";
    }
  }
  DeleteCircular(list);

  length = CircularLength(oclist);
  if (length<1)  return 0;

  option_enum** vlist = new option_enum*[length];
  for (int i=0; i<length; i++) {
    oclist = oclist->next;
    vlist[i] = (option_enum*) oclist->data;
  }
  RecycleCircular(oclist);

  return ShowNewStatement(
    "option statement:\n",
    expr::makeOptionStatement(Where(), o, check, vlist, length)
  );
}

// --------------------------------------------------------------
expr* BuildExprStatement(expr *x)
{
  if (bogus_expr::orNull(x))  return 0;
  return expr::makeExprStatement(Where(), x);
}

// --------------------------------------------------------------
void StartConverge()
{
  converge_depth++;
}

// --------------------------------------------------------------
option* BuildOptionHeader(char* name)
{
  if (0==name) return 0;

  option* answer;

  const option_enum* oc = Options.top();

  const option_manager* om = 0;
  if (oc) {
    om = oc->readSettings();
  } else {
    om = & option_manager::global();
  }
  answer = om ? om->FindOption(name) : 0;

  if (0==answer) {
    parse_error E;
    E << "Unknown option " << name;
    if (oc) E << " within " << oc->Name();
  }
  free(name);
  return answer;
}

// --------------------------------------------------------------
int AddIterator(symbol* i)
{
    if (bogus_expr::orNull(i))  return 0;
    if (findInChain(Iterators, i->Name())) {
        parse_error E;
        E << "Duplicate iterator named " << i->Name();
        Delete(i);
        return 0;
    }
    // Push i
    i->LinkTo(Iterators);
    Iterators = i;

    return 1;
}

// --------------------------------------------------------------
expr* BuildFuncStmt(symbol* f, expr* r)
{
  DCASSERT(function_under_construction==f);
  expr* foo = DefineUserFunction(
    Where(), f, r, model_under_construction
  );
  function_under_construction = 0;
  return ShowNewStatement("function statement:\n", foo);
}

// --------------------------------------------------------------
bool IllegalModelVarName(char* ident, const char* what_am_i)
{
  DCASSERT(WithinModel());
  if (model_under_construction) {
    if (model_under_construction->FindFormal(ident)) {
      parse_error E;
      E << "Model " << what_am_i << " ";
      E << ident << " has same name as parameter";
      free(ident);
      return true;
    }
  }
  if (ModelInternal) if (ModelInternal->findSymbol(ident)) {
    parse_error E;
    E << "Duplicate identifier " << ident << " within model";
    free(ident);
    return true;
  }
  return false;
}

// --------------------------------------------------------------
/** Build a measure statement.
    This handles statements of the form (within a model):
        type ident := rhs;
    The measure is added to the model's symbol tables.
      @param  typ     Type of measure.
      @param  ident   Name of measure.
      @param  rhs     Definition of measure.
      @return measure construction statement,
              or 0 on error (will make noise).
*/
expr* BuildMeasure(const type* typ, char* ident, expr* rhs)
{
  if (ignoringBadModelDecl()) {
    free(ident);
    Delete(rhs);
    return 0;
  }

  if (IllegalModelVarName(ident, "measure")) {
    Delete(rhs);
    return 0;
  }

  /* Initialize internal symbol table if necessary */
  if (0==ModelInternal) {
    ModelInternal = new symbol_table;
  }
  symbol* wrap = 0;

  if (typ && typ->hasProc()) {

    /* Special case - proc "constant" in a model;
       implement this as a function with 0 parameters */
    wrap = MakeUserConstFunc(
        Where(), typ, ident, true
    );
    ModelInternal->addSymbol(wrap);

    return ShowNewStatement("const func statement:\n",
      DefineUserFunction(
        Where(), wrap, rhs, model_under_construction
      )
    );

  } else {

    /* Ordinary measure */
    wrap = symbol::makeModelSymbol(Where(), typ, ident);
    ModelExternal.Insert(wrap);
    ModelInternal->addSymbol(wrap);

    return ShowNewStatement("measure assignment:\n",
      expr::makeModelMeasureAssign(
        Where(), model_under_construction, wrap, rhs
      )
    );
  }
}

// --------------------------------------------------------------
expr* BuildVarStmt(const type* typ, char* id, expr* ret)
{
  if (bogus_expr::isError(ret)) {
    free(id);
    return 0;
  }

  if (WithinModel()) {
    return BuildMeasure(typ, id, ret);
  }

  DCASSERT(typ);
  if (! typ->canDefineVarOfThis()) {
    parse_error E;
    E << "Constants of type " << *typ << " are not allowed";
    free(id);
    Delete(ret);
    return 0;
  }

  symbol* find = 0;
  function* match = 0;
  // Check for functions / models with no parameters of this name
  for (find = symbol_table::findGlobal(id); find; find = find->Next()) {
    function* f = smart_cast <function*> (find);
    if (0==f)    continue;
    int score = f->TypecheckParams(0, 0);
    if (score != 0)  continue;
    match = f;
    break;
  }
  if (match) {
    parse_error E;
    E << "Constant declaration conflicts with existing identifier:";
    E.Out.incIndent();
    E.newLine();
    match->PrintHeader(E.stream(), true);
    E << " declared " << match->Where();
    E.Out.decIndent();
    free(id);
    Delete(ret);
    return 0;
  }

  // Check that the name is unique among constants
  find = Constants->findSymbol(id);
  if (find) {
    free(id);

    if (find->isDefined()) {
      parse_error E;
      E << "Re-definition of constant " << find->Name();
      Delete(ret);
      return 0;
    }
  } else {
    // Make the symbol
    if (WithinConverge()) {
      find = symbol::makeCvgVar(Where(), typ, id);
    } else {
      find = symbol::makeConstant(Where(), typ, id, ret, 0);
    }
    // Add to symbol table
    if (0==find)  return 0;
    Constants->addSymbol(find);
    ShowWhatWeBuilt("variable: ", find);
  }

  if (WithinConverge())
    return ShowNewStatement("assignment:\n",
      expr::makeCvgAssign(Where(), find, ret)
    );

  return 0;
}

// --------------------------------------------------------------
expr* BuildGuessStmt(const type* typ, char* id, expr* ret)
{
  if (bogus_expr::isError(ret)) {
    free(id);
    return 0;
  }
  if (!WithinConverge()) {
    parse_error E;
    E << "Guess for " << id << " outside converge";
    free(id);
    Delete(ret);
    return 0;
  }
  // check that this is not already defined
  symbol* find = Constants->findSymbol(id);
  if (find) {
    free(id);
    if (find->isGuessed()) {
      parse_error E;
      E << "Duplicate guess for identifier " << find->Name();
      return nullptr;
    }
  } else {
    find = symbol::makeCvgVar(Where(), typ, id);
    if (0==find)  return 0;
    Constants->addSymbol(find);
    ShowWhatWeBuilt("variable: ", find);
  }

  return ShowNewStatement("guess:\n",
    expr::makeCvgGuess(Where(), find, ret)
  );
}

// --------------------------------------------------------------
expr* BuildArrayStmt(symbol *a, expr *ret)
{
  if (WithinModel())
    return ShowNewStatement("measure array assignment:\n",
      symbol::makeModelMeasureArray(Where(),
        model_under_construction, a, ret)
    );

  if (WithinConverge())
    return ShowNewStatement("converge array assignment:\n",
      expr::makeArrayCvgAssign(Where(), a, ret)
    );

  // ordinary array
  return ShowNewStatement("array assignment:\n",
    expr::makeArrayAssign(Where(), a, ret)
  );
}

// --------------------------------------------------------------
expr* BuildArrayGuess(symbol* a, expr* ret)
{
  if (0==a) {
    Delete(ret);
    return 0;
  }
  expr* stmt;
  if (WithinConverge()) {
    if (a->isGuessed()) {
      parse_error E;
      E << "Duplicate guess for identifier " << a->Name();
      Delete(ret);
      return 0;
    }
    stmt = expr::makeArrayCvgGuess(Where(), a, ret);
  } else {
    parse_error E;
    E << "Guess for " << a->Name() << " outside converge";
    stmt = 0;
    Delete(ret);
  }
  return ShowNewStatement("array guess:\n", stmt);
}


// ******************************************************************
// *                                                                *
// *                    Symbol-related functions                    *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
symbol* BuildIterator(const type* typ, char* n, expr* values)
{
  return ShowWhatWeBuilt("iterator: ",
    symbol::makeIterator(Where(), typ, n, values)
   );
}

// --------------------------------------------------------------
void DoneWithFunctionHeader()
{
  function_under_construction = 0;
}



// Helper for BuildFunction
// --------------------------------------------------------------
function* findFirstMatch(symbol_table* st, char* n, expr** pass, int nfp)
{
  if (0==st) return 0;
  for (symbol* find = st->findSymbol(n); find; find = find->Next()) {
    function* f = smart_cast <function*> (find);
    if (0==f)    continue;
    int score = f->TypecheckParams(pass, nfp);
    if (score != 0)  continue;
    return f;
  }
  return 0;
}

// Helper for BuildFunction
// --------------------------------------------------------------
void duplicationError(bool warning_only, function* f, const char* how)
{
  if (0==f) return;
  parse_error E(!warning_only);
  E << "Function declaration " << how << " existing identifier:";
  E.Out.incIndent();
  E.newLine();
  f->PrintHeader(E.stream(), true);
  E.newLine();
  E << "declared " << f->Where();
  E.Out.decIndent();
}

// Check for named parameter conflicts
// --------------------------------------------------------------
bool hasNamedParamConflicts(const char* n, symbol** fp, int np)
{
  // Check *all* functions of this name, for function call
  // ambiguity when passing named parameters.
  static int* scratch = 0;
  static int  scrsize = 0;

  if (np>scrsize) {
    delete[] scratch;
    scrsize = 16;
    while (scrsize < np) scrsize *= 2;  // fails if np is huge
    scratch = new int[scrsize];
  }

  bool conflicts = false;
  const symbol* findlist = symbol_table::findGlobal(n);

  //
  // Check for conflicts
  //
  for (const symbol* find = findlist; find; find = find->Next()) {
    const function* f = smart_cast <const function*> (find);
    if (0==f)   continue;
    if (!f->HasNameConflict(fp, np, scratch))  continue;
    conflicts = true;
    break;
  }
  if (!conflicts) return false;

  //
  // Print the conflicts
  //
  parse_error E;
  E << "Parameter names for `" << n << "' conflict with existing:";
  E.Out.incIndent();
  E.newLine();
  for (const symbol* find = findlist; find; find = find->Next()) {
    const function* f = smart_cast <const function*> (find);
    if (0==f)   continue;
    if (!f->HasNameConflict(fp, np, scratch))  continue;

    f->PrintHeader(E.stream(), true);
    E.newLine();
  }
  E.Out.decIndent();
  return true;
}

// --------------------------------------------------------------
symbol* BuildFunction(const type* typ, char* n, parser_list* list)
{
  DCASSERT(0==function_under_construction);
  if (0==list) {
    free(n);
    return 0;
  }
  if (WithinFor() || WithinConverge()) {
    parse_error E;
    E << "Function " << n << " defined within a for/converge";
    free(n);
    DeleteCircular(list);
    return 0;
  }
  DCASSERT(typ);
  if (! typ->canDefineFuncOfThis()) {
    parse_error E;
    E << "Functions of type " << *typ << " are not allowed";
    free(n);
    DeleteCircular(list);
    return 0;
  }

  int nfp = CircularLength(list);
  symbol** fp = nfp ? new symbol*[nfp] : 0;
  CopyCircular(list, fp, nfp);
  RecycleCircular(list);

  // check for forward definitions, duplications, etc.
  function* match = findFirstMatch(
    (model_under_construction) ? ModelInternal : &symbol_table::global(), n, (expr**) fp, nfp
  );
  if (match) {
    if (!match->HeadersMatch(typ, fp, nfp)) {
      duplicationError(0, match, "conflicts with");
      match = 0;
    }
    free(n);
    ResetUserFunctionParams(Where(), match, fp, nfp);
    function_under_construction = match;
    return match;
  }
  if (model_under_construction) {
    // warn if we are hiding a "global" function
    duplicationError(1, findFirstMatch(&symbol_table::global(), n, (expr**) fp, nfp), "hides");
  }

  //
  // This is a brand-new function.
  //
  if (hasNamedParamConflicts(n, fp, nfp)) {
    // Already printed an error message.
    // Cleanup and bail out.
    free(n);
    for (int i=0; i<nfp; i++) Delete(fp[i]);
    delete[] fp;
    return 0;
  }

  //
  // All clear; build the function
  //
  function_under_construction = MakeUserFunction(
      Where(), typ, n, fp, nfp, model_under_construction
  );

  if (model_under_construction) {
    if (0==ModelInternal) ModelInternal = new symbol_table;
    ModelInternal->addSymbol(function_under_construction);
  } else {
      symbol_table::addGlobal(function_under_construction);
  }
  return ShowWhatWeBuilt("function: ", function_under_construction);
}

// --------------------------------------------------------------
symbol* BuildMeasureArray(const type* typ, char* n, parser_list* list)
{
  if (ignoringBadModelDecl()) {
    free(n);
    DeleteCircular(list);
    return 0;
  }

  if (IllegalModelVarName(n, "measure")) {
    DeleteCircular(list);
    return 0;
  }

  if (BadIteratorList(n, list))  return 0;

  // Build a new array.  First, construct the iterator list.
  int dim = chainLength(Iterators);
  symbol** indexes = new symbol*[dim];
  fillFromChain(indexes, dim, Iterators);

  symbol* wrap = symbol::makeModelArray(
      Where(), typ, n, indexes, dim
  );

  // Add measure to symbol tables
  if (0==ModelInternal) {
    ModelInternal = new symbol_table;
  }
  ModelInternal->addSymbol(wrap);

  ModelExternal.Insert(wrap);

  return ShowWhatWeBuilt("measure array: ", wrap);
}

// --------------------------------------------------------------
symbol* BuildArray(const type* typ, char* n, parser_list* list)
{
  if (0==list) { // can this happen?
    free(n);
    return 0;
  }

  if (WithinModel())   return BuildMeasureArray(typ, n, list);

  // Check if the array already exists
  symbol* find = Arrays->findSymbol(n);
  if (find) {
    if (find->isDefined()) {
      parse_error E;
      E << "Array " << n << " already defined";
      DeleteCircular(list);
      free(n);
      return 0;
    }
  }

  if (BadIteratorList(n, list))  return 0;

  // If existing array, return it.
  if (find)  {
    free(n);
    return find;
  }

  // Build a new array.  First, construct the iterator list.
  int dim = chainLength(Iterators);
  symbol** indexes = new symbol*[dim];
  fillFromChain(indexes, dim, Iterators);
  symbol* f = symbol::makeArray(Where(), typ, n, indexes, dim);
  if (f) Arrays->addSymbol(f);
  return ShowWhatWeBuilt("array: ", f);
}

// --------------------------------------------------------------
symbol* BuildFormal(const type* typ, char* name)
{
  return MakeFormalParam(Where(),
                          typ, name, model_under_construction);
}

// --------------------------------------------------------------
symbol* BuildFormal(const type* typ, char* name, expr* deflt)
{
  return MakeFormalParam(Where(),
                          typ, name, deflt, model_under_construction);
}

// --------------------------------------------------------------
symbol* BuildNamed(char* name, expr* pass)
{
  symbol* s = MakeNamedParam(Where(), name, pass);
  return ShowWhatWeBuilt("named parameter: ", s);
}


// ******************************************************************
// *                                                                *
// *                    Model-related  functions                    *
// *                                                                *
// ******************************************************************

// --------------------------------------------------------------
void BuildModelStmt(symbol* m, parser_list* block)
{
  DCASSERT(m==(symbol*) model_under_construction);
  expr* stmt_block;
  if (model_under_construction) {
    stmt_block = MakeStatementBlock(block);
    int ns = ModelExternal.Length();
    ModelExternal.Sort();
    symbol** visible = ModelExternal.MakeArray();
    model_def::finishModelDef(model_under_construction, stmt_block, visible, ns);
    symbol_table::addGlobal(m);
  } else {
    DeleteCircular(block);
    stmt_block = 0;
  }
  ModelType = 0;
  model_under_construction = 0;
  delete ModelInternal;
  ModelInternal = 0;

  ShowWhatWeBuilt("model ", m);
}

// --------------------------------------------------------------
symbol* BuildModel(const type* typ, char* n, parser_list* list)
{
  DCASSERT(pm);
  DCASSERT(typ);
  DCASSERT(0==model_under_construction);
  DCASSERT(!WithinModel());

  ModelType = dynamic_cast <const formalism*> (typ);
  if (0==ModelType) {
    internal_error E(__FILE__, __LINE__, Where());
    E << "Type " << *typ << " is not a formalism!";
    return 0;
  }

  if (WithinFor() || WithinConverge()) {
    parse_error E;
    E << "Model " << n << " defined within a for/converge; ignoring";
    free(n);
    DeleteCircular(list);
    return 0;
  }

  int num_Formals = CircularLength(list);
  symbol** Formals = 0;
  if (num_Formals) {
    Formals = new symbol*[num_Formals];
    CopyCircular(list, Formals, num_Formals);
    RecycleCircular(list);
  }

  // check that the name is unique

  symbol* find = 0;
  if (0==num_Formals) {
    // No parameters - check "constants"
    find = Constants->findSymbol(n);
    if (find) {
      parse_error E;
      E << "Model declaration conflicts with existing identifier:";
      E.Out.incIndent();
      E.newLine();
      find->PrintType(E.stream());
      E << " " << find->Name() << " declared " << find->Where();
      E.Out.decIndent();
      free(n);
      return 0;
    }
  }

  // Check functions and other models
  expr** pass = (expr**) Formals;
  for (find = symbol_table::findGlobal(n); find; find = find->Next()) {
    function* f = smart_cast <function*> (find);
    DCASSERT(f);
    int score = f->TypecheckParams(pass, num_Formals);
    if (score != 0)    continue;
    // perfect match, that's bad!
    parse_error E;
    E << "Model declaration conflicts with existing identifier:";
    E.Out.incIndent();
    E.newLine();
    f->PrintHeader(E.stream(), true);
    E << " declared " << f->Where();
    E.Out.decIndent();
    free(n);
    for (int i=0; i<num_Formals; i++) {
      Delete(Formals[i]);
    }
    delete[] Formals;
    return 0;
  }

  // Check against name conflicts
  if (hasNamedParamConflicts(n, Formals, num_Formals)) {
    // Already printed an error message.
    // Cleanup and bail out.
    free(n);
    for (int i=0; i<num_Formals; i++) {
      Delete(Formals[i]);
    }
    delete[] Formals;
    return 0;
  }


  const formalism* fml = dynamic_cast <const formalism*> (typ);
  if (fml) {
    model_under_construction
        = fml->makeNewModel(Where(), n, Formals, num_Formals);
    DCASSERT(model_under_construction);
  } else {
    parse_error E;
    E << "Couldn't make model of type " << *typ;
    free(n);
    // Should Delete each element?
    delete[] Formals;
  }

  return (symbol*) model_under_construction;
}

// --------------------------------------------------------------
expr* BuildModelVarStmt(const type* typ, parser_list* list)
{
  DCASSERT(WithinModel());

  if (0==list)  return 0;
  if (0==typ || 0==model_under_construction) {
    DeleteCircular(list);
    return 0;
  }
  int numsyms = CircularLength(list);
  symbol** slist = new symbol*[numsyms];
  CopyCircular(list, slist, numsyms);
  RecycleCircular(list);

  expr* stmt;
  if (WithinFor()) {
    stmt = expr::makeModelArrayDecs(
        Where(), model_under_construction,
        typ, slist, numsyms
    );
  } else {
    stmt = expr::makeModelVarDecs(
        Where(), model_under_construction,
        typ, 0, slist, numsyms
    );
  }
  return ShowNewStatement("model decl:\n", stmt);
}

// --------------------------------------------------------------
parser_list* AddModelVar(parser_list* varlist, char* ident)
{
  if (0==ident)  return varlist;
  if (WithinFor()) {
    parse_error E;
    E << "Expecting array for model variable " << ident;
    free(ident);
    return varlist;
  }
  if (IllegalModelVarName(ident, "variable")) {
    return varlist;
  }

  symbol* ms = symbol::makeModelSymbol(Where(), 0, ident);
  if (0==ModelInternal) {
    ModelInternal = new symbol_table;
  }
  ModelInternal->addSymbol(ms);

  return AppendCircular(varlist, ms);
}

// --------------------------------------------------------------
parser_list* AddModelArray(parser_list* varlist, char* ident, parser_list* indexlist)
{
  if (0==ident)  {
    DeleteCircular(indexlist);
    return varlist;
  }
  if (!WithinFor()) {
      parse_error E;
      E << "Model array variable "<< ident <<" outside of for loop";
      free(ident);
      DeleteCircular(indexlist);
      return varlist;
  }
  if (IllegalModelVarName(ident, "array variable")) {
    return varlist;
  }
  if (BadIteratorList(ident, indexlist)) {
    return varlist;
  }

  // copy the iterator list.
  int dim = chainLength(Iterators);
  symbol** indexes = new symbol*[dim];
  fillFromChain(indexes, dim, Iterators);

  symbol* ms = symbol::makeModelArray(
      Where(), 0, ident, indexes, dim
  );
  if (0==ModelInternal) {
    ModelInternal = new symbol_table;
  }
  ModelInternal->addSymbol(ms);

  return AppendCircular(varlist, ms);
}

// ******************************************************************
// *                                                                *
// *                  Expression-related functions                  *
// *                                                                *
// ******************************************************************

//
// Positional parameter matching
//

// Helper for FindBest
function* scoreFuncs(symbol* find, expr** pass, int np, int &bs, bool &tie)
{
  if (find) if (compiler_debug.start()) {
    compiler_debug << "matching positional call ";
    ShowPosCall(compiler_debug.stream(), find->Name(), pass, 0, np);
    compiler_debug.stop();
  }
  function* best = 0;
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = dynamic_cast <function*> (ptr);
    if (0==f)  continue;
    int score = f->TypecheckParams(pass, np);
    if (compiler_debug.start()) {
      compiler_debug << "scored ";
      f->PrintHeader(compiler_debug.stream(), false);
      compiler_debug << ": " << score;
      compiler_debug.stop();
    }
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
void showMatching(parse_error &E, symbol* find,
        expr** pass, int np, int best_score)
{
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = dynamic_cast <function*> (ptr);
    if (0==f)      continue;
    int score = f->TypecheckParams(pass, np);
    if (score != best_score)  continue;
    f->PrintHeader(E.stream(), true);
    E.newLine();
  } // for ptr
}

// Function/model call scoring (positional parameters)
function* FindBest(symbol* f1, symbol* f2, expr** pass,
  int length, int first, bool no_match_error)
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

  if (best_score < 0) {
    if (no_match_error) {
      parse_error E;
      E << "No match for ";
      ShowPosCall(E.stream(), name, pass, first, length);
    }
    return nullptr;
  }

  if (tie) {
    parse_error E;
    E << "Multiple promotions with distance " << best_score << " for ";
    ShowPosCall(E.stream(), name, pass, first, length);
    E.newLine();
    E << "Possible choices:";
    E.Out.incIndent();
    E.newLine();
    showMatching(E, f1, pass, length, best_score);
    showMatching(E, f2, pass, length, best_score);
    E.Out.decIndent();
    return nullptr;
  }

  return best;
}


//
// Named parameter matching
//

// Helper for FindBest
function* scoreFuncs(symbol* find, symbol** pass, int np, int &bs, bool &tie)
{
  pos_paramarray &ppa = pos_paramarray::thePosList();

  if (find) if (compiler_debug.start()) {
    compiler_debug << "matching named call ";
    ShowNamedCall(compiler_debug.stream(), find->Name(), pass, np);
    compiler_debug.stop();
  }
  function* best = 0;
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = dynamic_cast <function*> (ptr);
    if (0==f)  continue;

    //
    // Convert named to positional, if we can for this function
    //
    int mnp = f->maxNamedParams();
    if (mnp < 0) {
      if (compiler_debug.start()) {
        compiler_debug << "named params not supported by ";
        f->PrintHeader(compiler_debug.stream(), false);
        compiler_debug.stop();
      }

      // can't call this function with named params
      continue;
    }
    ppa.alloc(mnp);
    int ntp = f->named2Positional(pass, np, ppa.getList(), ppa.maxList());
    if (ntp < 0) {
      //
      // Couldn't convert; bad name or missing something required
      //
      if (compiler_debug.start()) {
        compiler_debug << "failed conversion to ";
        f->PrintHeader(compiler_debug.stream(), false);
        compiler_debug << ": " << ntp;
        compiler_debug.stop();
      }
      continue;
    }

    //
    // Now, typecheck the positional, as usual
    //
    int score = f->TypecheckParams(ppa.getList(), ntp);
    ppa.recycle(ntp);   // cleanup
    if (compiler_debug.start()) {
      compiler_debug << "scored ";
      f->PrintHeader(compiler_debug.stream(), false);
      compiler_debug << ": " << score;
      compiler_debug.stop();
    }
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
void showMatching(parse_error &E, symbol* find, symbol** pass, int np, int best_score)
{
  pos_paramarray &ppa = pos_paramarray::thePosList();
  for (symbol* ptr = find; ptr; ptr = ptr->Next()) {
    function* f = dynamic_cast <function*> (ptr);
    if (0==f)      continue;
    //
    // Convert named to positional, if we can for this function
    //
    int mnp = f->maxNamedParams();
    if (mnp < 0) continue;    // can't call this function with named params
    ppa.alloc(mnp);
    int ntp = f->named2Positional(pass, np, ppa.getList(), ppa.maxList());
    if (ntp < 0) continue;    // can't convert

    int score = f->TypecheckParams(ppa.getList(), ntp);
    ppa.recycle(ntp);
    if (score != best_score)  continue;
    f->PrintHeader(E.stream(), true);
    E.newLine();
  } // for ptr
}


// Function/model call scoring (named parameters)
function* FindBest(symbol* f1, symbol* f2, symbol** pass,
  int length, bool no_match_error)
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

  if (best_score < 0) {
    if (no_match_error) {
      parse_error E;
      E << "No match for ";
      ShowNamedCall(E.stream(), name, pass, length);
    }
    return nullptr;
  }

  if (tie) {
    parse_error E;
    E << "Multiple promotions with distance " << best_score << " for ";
    ShowNamedCall(E.stream(), name, pass, length);
    E.newLine();
    E << "Possible choices:";
    E.Out.incIndent();
    E.newLine();
    showMatching(E, f1, pass, length, best_score);
    showMatching(E, f2, pass, length, best_score);
    E.Out.decIndent();
    return 0;
  }

  return best;
}

unary_op::opcode Int2Uop(int op)
{
  switch (op) {
    case NOT:       return unary_op::uop_not;
    case MINUS:     return unary_op::uop_neg;
    case FORALL:    return unary_op::uop_forall;
    case EXISTS:    return unary_op::uop_exists;
    case FUTURE:    return unary_op::uop_future;
    case GLOBALLY:  return unary_op::uop_globally;
    case NEXT:      return unary_op::uop_next;
  }
  internal_error E(__FILE__, __LINE__, Where());
  E << "Operator " << TokenName(op) << " not matched to any unary operator";
  return unary_op::uop_none;
}

binary_op::opcode Int2Bop(int op)
{
  switch (op) {
    case IMPLIES:   return binary_op::bop_implies;
    case MOD:       return binary_op::bop_mod;
    case SET_DIFF:  return binary_op::bop_diff;
    case EQUALS:    return binary_op::bop_equals;
    case NEQUAL:    return binary_op::bop_nequal;
    case GT:        return binary_op::bop_gt;
    case GE:        return binary_op::bop_ge;
    case LT:        return binary_op::bop_lt;
    case LE:        return binary_op::bop_le;
    case UNTIL:     return binary_op::bop_until;
    case TEMPORALAND:  return binary_op::bop_and;
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
  E << "Operator " << TokenName(op)
    << " not matched to any associative operator";
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
      trinary_op::top_interval, start, stop, inc
    )
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
    if (has_error)    continue;
    if (0==i)         continue;
  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return bogus_expr::makeError();
  }

  assoc_op::opcode aop = Int2Aop(oper);
  return ShowWhatWeBuilt(0,
      assoc_op::makeExpr(Where(), aop, opnds, flip, length)
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
    if (has_error)    continue;
    if (0==i)         continue;
  } // for i

  DeleteCircular(list);
  if (has_null || has_error) {
    for (int i=0; i<length; i++)  Delete(opnds[i]);
    delete[] opnds;
    delete[] flip;
    if (has_null)  return 0;
    return bogus_expr::makeError();
  }

  assoc_op::opcode aop = Int2Aop(oper);
  return ShowWhatWeBuilt(0,
      assoc_op::makeExpr(Where(), aop, opnds, flip, length)
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
  assoc_op::opcode aop = Int2Aop(op);

  return ShowWhatWeBuilt(0,
      assoc_op::makeExpr(Where(), aop, opnds, 0, length)
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
expr* BuildTypecast(const type* newtype, expr* opnd)
{
  if (ignoringBadModelDecl()) {
    Delete(opnd);
    return 0;
  }

  DCASSERT(newtype);
  symbol* find = symbol_table::findGlobal(newtype->getStr());
  function* best = find ? FindBest(find, 0, &opnd, 1, 0, false) : 0;

  if (best) {
    //
    // Use a function call, actually
    //
    expr** pass = new expr*[1];
    pass[0] = opnd;
    return ShowWhatWeBuilt(0,
      expr::makeFunctionCall(Where(), best, pass, 1)
    );
  } else {
    // Either no function exists, or no parameter match
    // Use an ordinary typecast
    return ShowWhatWeBuilt("typecast: ",
      typeconv::castExpr(false, Where(), newtype, opnd)
    );
  }
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
  expr* answer = 0;
  if (c.isNull()) {
    // did we overflow?  try bigints
    if (type::find("bigint")) {
      type::find("bigint")->assignFromString(c, s);
      answer = new value(Where(), type::find("bigint"), c);
    }
  } else {
    answer = new value(Where(), type::find("int"), c);
  }
  free(s);
  return answer;
}

// --------------------------------------------------------------
expr* MakeRealConst(char* s)
{
  if (0==s)  return 0;
  result c;
  DCASSERT(type::find("real"));
  type::find("real")->assignFromString(c, s);
  expr* foo = new value(Where(), type::find("real"), c);
  free(s);
  return foo;
}

// --------------------------------------------------------------
expr* MakeStringConst(char *s)
{
  result c;
  DCASSERT(type::find("string"));
  type::find("string")->assignFromString(c, s);
  expr* foo = new value(Where(), type::find("string"), c);
  free(s);
  return foo;
}

// --------------------------------------------------------------

expr* MakeMCall(shared_object* mcall, char* m)
{
  model_call_data* mcd = dynamic_cast <model_call_data*> (mcall);
  if (0==mcd) {
    free(m);
    return bogus_expr::makeError();
  }

  expr* foo = 0;
  if (mcd->model1) {
    foo = model_def::makeMeasureCall(Where(),
      mcd->model1, mcd->pass, mcd->np, m);
  } else {
    foo = expr::makeMeasureCall(Where(), mcd->model2, m);
  }
  free(m);
  Delete(mcall);
  return ShowWhatWeBuilt(0, foo);
}

// --------------------------------------------------------------
expr* MakeAMCall(char* n, parser_list* ind, char* m)
{
  symbol* find = 0;
  if (n) find = Arrays->findSymbol(n);
  if (0==find) {
    if (n) {
      parse_error E;
      E << "Unknown array " << n;
    }
    free(n);
    DeleteCircular(ind);
    free(m);
    return bogus_expr::makeError();
  }
  free(n);

  int ni = CircularLength(ind);
  DCASSERT(ni);
  expr** i = new expr*[ni];
  CopyCircular(ind, i, ni);
  RecycleCircular(ind);
  expr* foo = expr::makeMeasureCall(Where(), find, i, ni, m);
  free(m);
  return ShowWhatWeBuilt(0, foo);
}

// --------------------------------------------------------------
expr* MakeMACall(shared_object* mcall, char* m, parser_list* ind)
{
  model_call_data* mcd = dynamic_cast <model_call_data*> (mcall);
  if (0==mcd) {
    free(m);
    DeleteCircular(ind);
    return bogus_expr::makeError();
  }
  int length = CircularLength(ind);
  DCASSERT(length);
  expr** I = new expr*[length];
  CopyCircular(ind, I, length);
  RecycleCircular(ind);
  expr* foo = 0;
  if (mcd->model1) {
    foo = model_def::makeMeasureCall(Where(),
      mcd->model1, mcd->pass, mcd->np, m, I, length);
  } else {
    foo = expr::makeMeasureCall(Where(),
      mcd->model2, m, I, length);
  }
  free(m);
  Delete(mcall);
  return ShowWhatWeBuilt(0, foo);
}

// --------------------------------------------------------------
expr* MakeAMACall(char* n, parser_list* ind, char* m, parser_list* ind2)
{
  symbol* find = 0;
  if (n) find = Arrays->findSymbol(n);
  if (0==find) {
    if (n) {
      parse_error E;
      E << "Unknown array " << n;
    }
    free(n);
    DeleteCircular(ind);
    free(m);
    return bogus_expr::makeError();
  }
  free(n);

  int ni = CircularLength(ind);
  DCASSERT(ni);
  expr** i = new expr*[ni];
  CopyCircular(ind, i, ni);
  RecycleCircular(ind);

  int nj = CircularLength(ind2);
  DCASSERT(nj);
  expr** j = new expr*[nj];
  CopyCircular(ind2, j, nj);
  RecycleCircular(ind2);

  expr* foo = expr::makeMeasureCall(
      Where(), find, i, ni, m, j, nj
  );
  free(m);
  return ShowWhatWeBuilt(0, foo);
}

// --------------------------------------------------------------
shared_object* MakeModelCallPP(char* n, parser_list* list)
{
  if (ignoringBadModelDecl()) {
    free(n);
    DeleteCircular(list);
    return 0;
  }

  symbol* find;
  if (0==list) {
    // try vars of type "model" first...
    find = Constants->findSymbol(n);
    if (find) {
      free(n);
      return new model_call_data(find);
    }
  }

  find = symbol_table::findGlobal(n);
  if (0==find) {
    parse_error E;
    E << "Unknown model " << n;
    free(n);
    DeleteCircular(list);
    return 0;
  }
  free(n);

  // Dump parameters to an array
  int length = CircularLength(list);
  expr** pass;
  if (length) {
    pass = new expr*[length];
    CopyCircular(list, pass, length);
    RecycleCircular(list);
  } else {
    pass = 0;
  }

  function* best = FindBest(find, 0, pass, length, 0, true);

  if (0==best) {
    for (int i=0; i<length; i++)  Delete(pass[i]);
    delete[] pass;
    return 0;
  }

  // make sure this is a model!
  model_def* parent = dynamic_cast<model_def*>(best);
  if (0==parent) {
    parse_error E;
    if (0==length) {
        E << best->Name() << " is not a model";
    } else {
        E << "Expected model for call ";
        ShowPosCall(E.stream(), best->Name(), pass, 0, length);
        E << ", but it matches";
        E.Out.incIndent();
        E.newLine();
        best->PrintHeader(E.stream(), true);
        E << " declared " << best->Where();
        E.Out.decIndent();
    } // length
    return 0;
  }

  return new model_call_data(parent, pass, length);
}

// --------------------------------------------------------------
shared_object* MakeModelCallNP(char* n, parser_list* list)
{
  if (ignoringBadModelDecl()) {
    free(n);
    DeleteCircular(list);
    return 0;
  }

  symbol* find;
  if (0==list) {
    // try vars of type "model" first...
    find = Constants->findSymbol(n);
    if (find) {
      free(n);
      return new model_call_data(find);
    }
  }

  find = symbol_table::findGlobal(n);
  if (0==find) {
    parse_error E;
    E << "Unknown model " << n;
    free(n);
    DeleteCircular(list);
    return 0;
  }
  free(n);

  // Dump parameters to our temporary array
  named_paramarray& npa = named_paramarray::theNamedList();
  npa.initFromList(list);

  //
  // Find best match
  //
  function* best = FindBest(find, 0, npa.getList(), npa.getLength(), true);
  if (0==best) {
    npa.recycle();
    return 0;
  }

  //
  // make sure this is a model!
  //
  model_def* parent = dynamic_cast<model_def*>(best);
  if (0==parent) {
    parse_error E;
    if (0==npa.getLength()) {
        E << best->Name() << " is not a model";
    } else {
        E << "Expected model for call ";
        ShowNamedCall(E.stream(), best->Name(), npa.getList(), npa.getLength());
        E << ", but it matches";
        E.Out.incIndent();
        E.newLine();
        best->PrintHeader(E.stream(), true);
        E << " declared " << best->Where();
        E.Out.decIndent();
    } // length
    return 0;
  }

  //
  // Build positional parameter equivalents
  //
  pos_paramarray& ppa = pos_paramarray::thePosList();
  int np = best->named2Positional(
    npa.getList(), npa.getLength(), ppa.getList(), ppa.maxList()
  );
  DCASSERT(ppa.maxList() >= np);
  npa.recycle();
  expr** pass = ppa.Compactify(np);
  return new model_call_data(parent, pass, np);
}

// --------------------------------------------------------------
expr* FindIdent(char* name)
{
  if (ignoringBadModelDecl()) {
    free(name);
    return 0;
  }

  // Check for loop iterators.
  symbol* find = findInChain(Iterators, name);

  // Check function formal parameters, if any
  if (!find) if (function_under_construction) {
    find = function_under_construction->FindFormal(name);
  }

  // Check model formal parameters, if any
  if (!find) if (model_under_construction) {
    find = model_under_construction->FindFormal(name);
  }

  // Check model built-ins
  if (!find) if (ModelType) {
    find = ModelType->findSymbol(name);
  }

  // Check model variables
  if (!find) if (ModelInternal) {
    find = ModelInternal->findSymbol(name);
  }

  // Any others to check?

  // Check "constants"
  if (!find) find = Constants->findSymbol(name);

  if (find) {
    free(name);
    return Share(find);
  }

  // Check no-parameter functions
  return BuildFuncCallPP(name, 0);
}

// --------------------------------------------------------------
expr* BuildArrayCall(char* n, parser_list* ind)
{
  symbol* find = 0;
  // Check model variables
  if (ModelInternal) {
    find = ModelInternal->findSymbol(n);
  }
  if (0==find) {
    find = Arrays->findSymbol(n);
  }

  if (0==find) {
    parse_error E;
    E << "Unknown array " << n;
    free(n);
    DeleteCircular(ind);
    return bogus_expr::makeError();
  }
  free(n);
  int length = CircularLength(ind);
  DCASSERT(length);
  expr** pass = new expr*[length];
  CopyCircular(ind, pass, length);
  RecycleCircular(ind);
  return ShowWhatWeBuilt(0,
    expr::makeArrayCall(Where(), find, pass, length)
  );
}

// --------------------------------------------------------------
expr* BuildFuncCallPP(char* n, parser_list* posparams)
{
  if (ignoringBadModelDecl()) {
    free(n);
    DeleteCircular(posparams);
    return 0;
  }

  symbol* find = ModelType ? ModelType->findSymbol(n) : nullptr;
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

  function* best = FindBest(find, find2, pass, length, first, true);
  if (0==best) {
    for (int i=0; i<length; i++)  Delete(pass[i]);
    delete[] pass;
    return bogus_expr::makeError();
  }
  return ShowWhatWeBuilt(0,
    expr::makeFunctionCall(Where(), best, pass, length)
  );
}

// --------------------------------------------------------------
expr* BuildFuncCallNP(char* n, parser_list* namedparams)
{
  if (ignoringBadModelDecl()) {
    free(n);
    DeleteCircular(namedparams);
    return 0;
  }

  named_paramarray& npa = named_paramarray::theNamedList();

  symbol* find = ModelType ? ModelType->findSymbol(n) : nullptr;
  symbol* find2 = 0;
  if (find) {
    // we have a match within a model, add model to params
    symbol* passmodel = BuildNamed(
      strdup("-m"), Share((expr*) model_under_construction)
    );
    namedparams = PrependCircular(namedparams, passmodel);
  } else {
    find = ModelInternal ? ModelInternal->findSymbol(n) : 0;
    find2 = symbol_table::findGlobal(n);
  }

  if (0==find && 0==find2) {
    parse_error E;
    if (namedparams)  E << "Unknown function " << n;
    else              E << "Unknown identifier: " << n;
    free(n);
    DeleteCircular(namedparams);
    return bogus_expr::makeError();
  }
  free(n);

  //
  // Initialize the named parameter list
  //
  npa.initFromList(namedparams);

  //
  // Find best match
  //
  function* best = FindBest(find, find2, npa.getList(), npa.getLength(), true);
  if (0==best) {
    npa.recycle();
    return bogus_expr::makeError();
  }

  //
  // Build call and cleanup
  //
  pos_paramarray& ppa = pos_paramarray::thePosList();
  int np = best->named2Positional(
    npa.getList(), npa.getLength(), ppa.getList(), ppa.maxList()
  );
  DCASSERT(ppa.maxList() >= np);
  npa.recycle();

  expr** posparams = ppa.Compactify(np);
  return ShowWhatWeBuilt(0,
    expr::makeFunctionCall(Where(), best, posparams, np)
  );
}

// --------------------------------------------------------------
expr*  Default()
{
  return bogus_expr::makeDefault();
}

// ******************************************************************
// *                                                                *
// *                         Initialization                         *
// *                                                                *
// ******************************************************************

class compile_initializer : public initializer {
    public:
        compile_initializer();
    protected:
        virtual void execute();
};
static compile_initializer the_compile_initializer;

compile_initializer::compile_initializer() : initializer(__FILE__, 2)
{
    builds_resource("compile.cc");
    needs_resource("Debug");
}

void compile_initializer::execute()
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

    // Init symbol tables
    Iterators = nullptr;
    Arrays = new symbol_table;
    Constants = new symbol_table;

    // init globals and such here.
    result one(1L);
    ONE = new value(location::NOWHERE(), type::find("int"), one);

    result dk;
    dk.setUnknown();
    Constants->addSymbol(
      symbol::makeConstant(location::NOWHERE(), type::find("int"), strdup("DontKnow"),
          new value(location::NOWHERE(), type::find("int"), dk), 0
      )
    );

    result inf;
    inf.setInfinity(1);
    Constants->addSymbol(
      symbol::makeConstant(location::NOWHERE(), type::find("int"), strdup("infinity"),
        new value(location::NOWHERE(), type::find("int"), inf), 0
      )
    );

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


