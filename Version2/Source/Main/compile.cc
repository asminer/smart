
// $Id$

#include "../Base/api.h"
#include "compile.h"
#include "../list.h"
#include "tables.h"
#include "fnlib.h"

#include <stdio.h>
#include <FlexLexer.h>

#define COMPILE_DEBUG

void DumpParam(OutputStream &s, formal_param *p)
{
  if (NULL==p) {
    s << "null";
    return;
  }
  int i;
  for (i=0; i<p->NumComponents(); i++) {
    if (i) s << ":";
    s << GetType(p->Type(i));
  }
  s << " " << p->Name();
}

void DumpHeader(OutputStream &s, function *f)
{
  s << f;
  formal_param **fp;
  int np;
  int rp;
  f->GetParamList(fp, np, rp);  
  if (np<1) return;
  s << "(";
  int i;
  for (i=0; i<np; i++) {
    if (rp==i) s << "...";
    DumpParam(s, fp[i]);
    if (i<np-1) s << ", ";
  }
  if (rp<=np) s << ",...";
  s << ")";
}


// ==================================================================
// |                                                                |
// |                      Global compiler data                      | 
// |                                                                |
// ==================================================================

/// Current stack of for-loop iterators
List <array_index> *Iterators;

/// Symbol table of arrays
PtrTable *Arrays;

/// Symbol table of functions
PtrTable *Builtins;

// ==================================================================
// |                                                                |
// |                          Lexer  hooks                          | 
// |                                                                |
// ==================================================================

extern char* filename;
extern yyFlexLexer lexer;

char* Filename() { return filename; }

int LineNumber() { return lexer.lineno(); }

// ==================================================================
// |                                                                |
// |                           Type stuff                           | 
// |                                                                |
// ==================================================================

type MakeType(const char* modif, const char* tp)
{
  int m = 0;
  if (modif) {
    m = FindModif(modif);
    if (m<0) {
      Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
      Internal << "Bad type modifier: " << modif << ", using void";
      Internal.Stop();
      return VOID;
    }
  }
  int t = FindType(tp);
  if (t<0) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Bad type: " << tp << ", using void";
    Internal.Stop();
    return VOID;
  }
  type answer;
  if (modif) answer = ModifyType(m, t); else answer = t;
#ifdef COMPILE_DEBUG
  cout << "MakeType(";
  if (modif) cout << modif; else cout << "null";
  cout << ", " << tp << ") returned " << int(answer) << endl;
#endif
  return answer;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Expression  construction                    | 
// |                                                                |
// |                                                                |
// ==================================================================

expr* MakeBoolConst(char* s)
{
  if (strcmp(s, "true") == 0) {
    return MakeConstExpr(true, filename, lexer.lineno());
  }
  if (strcmp(s, "false") == 0) {
    return MakeConstExpr(false, filename, lexer.lineno());
  }
  // error
  Internal.Start(filename, lexer.lineno());
  Internal << "bad boolean constant";
  Internal.Stop();
  return NULL;
}

expr* MakeIntConst(char* s)
{
  int value;
  sscanf(s, "%d", &value);
  return MakeConstExpr(value, filename, lexer.lineno());
}

expr* MakeRealConst(char* s)
{
  double value;
  sscanf(s, "%lf", &value);
  return MakeConstExpr(value, filename, lexer.lineno());
}

expr* BuildInterval(expr* start, expr* stop)
{
  if (NULL==start || NULL==stop) {
    Delete(start);
    Delete(stop);
    return NULL;
  }
  
  // make sure both are integers here...

  expr* answer = MakeInterval(filename, lexer.lineno(), 
                      start, stop, MakeConstExpr(1, filename, lexer.lineno()));

#ifdef COMPILE_DEBUG
  cout << "Built interval, start: " << start << " stop: " << stop << endl;
  cout << "\tGot: " << answer << endl;
#endif

  return answer;
}

expr* BuildInterval(expr* start, expr* stop, expr* inc)
{
  if (NULL==start || NULL==stop || NULL==inc) {
    Delete(start);
    Delete(stop);
    Delete(inc);
    return NULL;
  }

  // type checking and promotion here...

  expr* answer = MakeInterval(filename, lexer.lineno(), start, stop, inc);

#ifdef COMPILE_DEBUG
  cout << "Built interval, start: " << start;
  cout << " stop: " << stop << " inc: " << inc << endl;
  cout << "\tGot: " << answer << endl;
#endif

  return answer;
}

expr* AppendSetElem(expr* left, expr* right)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }

  // type checking goes here
  expr* answer = MakeUnionOp(filename, lexer.lineno(), left, right);
  return answer;
}

array_index* BuildIterator(type t, char* n, expr* values)
{
  if (NULL==values) return NULL;

  // type checking
  type vt = values->Type(0);
  bool match = false;
  switch (t) {
    case INT:	match = (vt == SET_INT); 	break;
    case REAL:	match = (vt == SET_REAL);	break;
  }

  // To do still: promote int sets to reals, if t is real
  
  if (!match) {
    Error.Start(filename, lexer.lineno());
    Error << "Type mismatch: iterator " << n << " expects set of type " << GetType(t);
    Error.Stop();
    return NULL;
  }

  array_index* ans = new array_index(filename, lexer.lineno(), t, n, values);

#ifdef COMPILE_DEBUG
  cout << "Built iterator: " << ans << endl;
#endif

  return ans;
}

expr* BuildUnary(int op, expr* opnd)
{
  if (NULL==opnd) return NULL;

  // type checking here

  return MakeUnaryOp(op, opnd, filename, lexer.lineno());
}

expr* BuildBinary(expr* left, int op, expr* right)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }

  // type checking goes here

  return MakeBinaryOp(left, op, right, filename, lexer.lineno());
}

expr* BuildTypecast(expr* opnd, type newtype)
{
  if (NULL==opnd) return NULL;

  // type checking here
  
  return MakeTypecast(opnd, newtype, filename, lexer.lineno());
}

void* StartAggregate(expr* a, expr* b)
{
  if (NULL==a || NULL==b) {
    Delete(a);
    Delete(b);
    return NULL;
  }
  List <expr> *foo = new List <expr> (256);
  foo->Append(a);
  foo->Append(b);
  return foo;
}

void* AddAggregate(void* x, expr* b)
{
  if (NULL==x) {
    Delete(b);
    return NULL;
  }
  List <expr> *foo = (List <expr> *)x;
  if (NULL==b) {
    delete foo;
    return NULL;
  }
  foo->Append(b);
  return foo;
}

expr* BuildAggregate(void* x)
{
  if (NULL==x) return NULL;
  List <expr> *foo = (List <expr> *)x;
  int size = foo->Length();
  expr** parts = foo->Copy();
  delete foo;
  expr* answer = MakeAggregate(parts, size, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  cout << "Built aggregate expression: " << answer << endl;
#endif
  return answer;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Statement   construction                    | 
// |                                                                |
// |                                                                |
// ==================================================================

int AddIterator(array_index *i)
{
  if (NULL==i) return 0;
  Iterators->Append(i);
#ifdef COMPILE_DEBUG
  cout << "Adding " << i << " to Iterators\n";
#endif
  return 1;
}

statement* BuildForLoop(int count, void *stmts)
{
#ifdef COMPILE_DEBUG
  cout << "Popping " << count << " Iterators\n";
#endif
  if (count > Iterators->Length()) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Iterator stack underflow";
    Internal.Stop();

    Iterators->Clear();
    return NULL;
  }

  if (NULL==stmts) {
#ifdef COMPILE_DEBUG
    cout << "Empty for loop statement, skipping\n";
#endif
    return NULL;
  }

  array_index **i = new array_index*[count];
  int d;
  int first = Iterators->Length() - count;
  for (d=0; d<count; d++) {
    i[d] = Iterators->Item(first+d);
  }
  for (d=0; d<count; d++) Iterators->Pop();

  statement** block = NULL;
  int bsize = 0;
  if (stmts) {
    List <statement> *foo = (List <statement> *)stmts;
    bsize = foo->Length();
    block = foo->Copy();
    delete foo;
  }

  statement *f = MakeForLoop(i, count, block, bsize, filename, lexer.lineno());

#ifdef COMPILE_DEBUG
  cout << "Built for loop statement: ";
  f->showfancy(0, cout);
  cout << endl;
#endif

  f->Execute();
  
  return f;
}

statement* BuildExprStatement(expr *x)
{
  if (NULL==x) return NULL;
  Optimize(0, x);
  statement* s = MakeExprStatement(x, filename, lexer.lineno());
  if (NULL==s) return NULL;

  // remove this eventually...
  s->Execute();

  return s;
}

statement* BuildArrayStmt(array *a, expr *e)
{
  if (NULL==a) return NULL;
  if (NULL==e) {
    return MakeArrayAssign(a, NULL, filename, lexer.lineno());
  }
  if (!Promotable(e->Type(0), a->Type(0))) {
    Error.Start(filename, lexer.lineno());
    Error << "type mismatch in assignment for array " << a->Name();
    Error.Stop();
    return NULL;
  }
  Optimize(0, e);
  expr* ne = MakeTypecast(e, a->Type(0), filename, lexer.lineno());
  statement *s = MakeArrayAssign(a, ne, filename, lexer.lineno());
  return s; 
}

void* AppendStatement(void* list, statement* s)
{
  if (NULL==s) return list;
  List <statement> *foo = (List <statement> *)list;
  if (NULL==foo) 
    foo = new List <statement> (256);
  foo->Append(s);
  return foo;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                        Parameter  lists                        | 
// |                                                                |
// |                                                                |
// ==================================================================

void* AddFormalIndex(void* list, char* n)
{
  if (NULL==n) return list;
  List <char> *foo = (List <char> *)list;
  if (NULL==foo) 
    foo = new List <char> (256);
  foo->Append(n);
#ifdef COMPILE_DEBUG
  cout << "Formal index stack: ";
  for (int i=0; i<foo->Length(); i++) {
    char* id = foo->Item(i);
    cout << id << " ";
  }
  cout << endl;
#endif
  return foo;
}

void* AddParameter(void* list, expr* e)
{
  List <expr> *foo = (List <expr> *)list;
  if (NULL==foo) 
    foo = new List <expr> (256);
  foo->Append(e);
#ifdef COMPILE_DEBUG
  cout << "Parameter list: ";
  for (int i=0; i<foo->Length(); i++) {
    expr* p = foo->Item(i);
    cout << p << " ";
  }
  cout << endl;
#endif
  return foo;
}

array* BuildArray(type t, char*n, void* list)
{
  if (NULL==n) return NULL;
  if (NULL==list) return NULL;

  // Check that this array name is ok
  if (Arrays->FindName(n)) {
    Error.Start(filename, lexer.lineno());
    Error << "Array " << n << " already defined";
    Error.Stop();
    return NULL;
  }
  
  List <char> *foo = (List <char> *)list;
  int dim = Iterators->Length();

  // check the iterators dimensions
  if (foo->Length() != dim) {
    Error.Start(filename, lexer.lineno());
    Error << "Dimension of array " << n << " does not match iterators";
    Error.Stop();
    return NULL;
  }

  // compare iterator names
  int i;
  for (i=0; i<dim; i++) {
    array_index* it = Iterators->Item(i);
    DCASSERT(it);
    const char* exname = it->Name();
    DCASSERT(exname);
    char* myname = foo->Item(i);
    DCASSERT(myname);
    if (strcmp(exname, myname)!=0) {
      Error.Start(filename, lexer.lineno());
      Error << "Array " << n << " expecting index " << exname;
      Error << ", got " << myname;
      Error.Stop();
      return NULL;
    }
  }

  // Build "copies" of iterators 
  array_index **il = Iterators->Copy();
  for (i=0; i<dim; i++) {
    Copy(il[i]);  // increment counter
  }

  // build array
  array *A = new array(filename, lexer.lineno(), t, n, il, dim);

  // Add array to array symbol table
  Arrays->AddNamePtr(n, A); 

  return A;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                         Function calls                         | 
// |                                                                |
// |                                                                |
// ==================================================================

expr* FindIdent(char* name)
{
  // Check for loop iterators
  int d;
  for (d=0; d<Iterators->Length(); d++) {
    array_index *i = Iterators->Item(d);
    DCASSERT(i);
    if (strcmp(name, i->Name())==0)
      return Copy(i);
  }

  // Couldn't find it.
  Error.Start(filename, lexer.lineno());
  Error << "Unknown identifier: " << name;
  Error.Stop();
  return NULL;
}

expr* BuildArrayCall(const char* n, void* ind)
{
  List <expr> *foo = (List <expr> *)ind;
  // find symbol table entry
  array* entry = (array*) (Arrays->FindName(n));
  if (NULL==entry) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown array " << n;
    Error.Stop();
    delete foo;
    return NULL;
  }
  // check type, dimension of indexes
  int size = foo->Length();
  int i;
  array_index **il;
  int dim;
  entry->GetIndexList(il, dim);
  if (size!=dim) {
    Error.Start(filename, lexer.lineno());
    Error << "Array " << n << " has dimension " << dim;
    Error.Stop();
    delete foo;
    return NULL;
  }
  // types
  for (i=0; i<dim; i++) {
    expr* me = foo->Item(i);
    if (!Promotable(me->Type(0), il[i]->Type(0))) {
      Error.Start(filename, lexer.lineno());
      Error << "Array " << n << " expects type ";
      Error << GetType(il[i]->Type(0));
      Error << " for index " << il[i]->Name();
      Error.Stop();
      delete foo;
      return NULL;
    }
  }

  // Ok, build the array call
  expr** pass = new expr*[dim];
  for (i=0; i<dim; i++) {
    expr* x = foo->Item(i);
    Optimize(0, x);
    pass[i] = MakeTypecast(x, il[i]->Type(0), filename, lexer.lineno());
  }
  delete foo;
  expr* answer = MakeArrayCall(entry, pass, size, filename, lexer.lineno());
  return answer;
}

expr* BuildFunctionCall(const char* n, void* posparams)
{
  List <expr> *params = (List <expr> *)posparams;
  // find symbol table entry
  List <function> *flist = FindFunctions(Builtins, n);
  if (NULL==flist) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown function " << n;
    Error.Stop();
    delete params;
    return NULL;
  }

  // check parameters
  return NULL;
}

expr* BuildNamedFunctionCall(const char *, void*)
{
  cerr << "Named parameters not done yet, sorry\n";
  return NULL;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Initialize compiler data                    | 
// |                                                                |
// |                                                                |
// ==================================================================

#ifdef COMPILE_DEBUG
void ShowSymbols(void *x)
{
  PtrTable::splayitem *foo = (PtrTable::splayitem *)x;
  Output << foo->name << "\n";
  List <function> *bar = (List <function> *)foo->ptr;
  int i;
  for (i=0; i<bar->Length(); i++) {
    Output << "\t";
    DumpHeader(Output, bar->Item(i));
    Output << "\n";
  }
}
#endif

void InitCompiler()
{
  Iterators = new List <array_index> (256);
  Arrays = new PtrTable();
  Builtins = new PtrTable();
  InitBuiltinFunctions(Builtins); 
#ifdef COMPILE_DEBUG
  cout << "Initialized compiler data\n";
  cout << "Builtin Functions:\n";
  Builtins->Traverse(ShowSymbols);
  cout << "ready to rock.\n";
#endif
}

