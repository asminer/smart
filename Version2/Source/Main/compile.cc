
// $Id$

#include "../defines.h"
#include "compile.h"
#include "../Base/api.h"
#include "../list.h"
#include "tables.h"

#include <stdio.h>
#include <FlexLexer.h>

#define COMPILE_DEBUG


// ==================================================================
// |                                                                |
// |                      Global compiler data                      | 
// |                                                                |
// ==================================================================

/// Current stack of for-loop iterators
List <array_index> *Iterators;

/// Symbol table of arrays
PtrTable *Arrays;

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
  array_index **il = new array_index*[dim];
  for (i=0; i<dim; i++) {
    il[i] = Iterators->Item(i);
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

// ==================================================================
// |                                                                |
// |                                                                |
// |                    Initialize compiler data                    | 
// |                                                                |
// |                                                                |
// ==================================================================

void InitCompiler()
{
  Iterators = new List <array_index> (256);
  Arrays = new PtrTable();
#ifdef COMPILE_DEBUG
  cout << "Initialized compiler data\n";
#endif
}

