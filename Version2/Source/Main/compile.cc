
// $Id$

#include "../Base/api.h"
#include "compile.h"
#include "../list.h"
#include "tables.h"
#include "fnlib.h"

#include <stdio.h>
#include <FlexLexer.h>

//#define COMPILE_DEBUG

void DumpPassed(OutputStream &s, List <expr> *pass)
{
  if (NULL==pass) return;
  if (pass->Length()==0) return;
  s << "(";
  int i;
  for (i=0; i<pass->Length(); i++) {
    if (i) s << ", ";
    PrintExprType(pass->Item(i), s);
  }
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

/// Symbol table of "constants"
PtrTable *Constants;

/// "Symbol table" of formal parameters
List <formal_param> *FormalParams;

/** List of functions that match what we're looking for.
    Global because we'll re-use it.
*/
List <function> *matches;


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
  Output << "MakeType(";
  if (modif) Output << modif; else Output << "null";
  Output << ", " << tp << ") returned " << GetType(answer) << "\n";
  Output.flush();
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

expr* MakeStringConst(char *s)
{
  char *scopy = strdup(s);
  return MakeConstExpr(scopy, filename, lexer.lineno());
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
  Output << "Built interval, start: " << start << " stop: " << stop << "\n";
  Output << "\tGot: " << answer << "\n";
  Output.flush();
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
  Output << "Built interval, start: " << start;
  Output << " stop: " << stop << " inc: " << inc << "\n";
  Output << "\tGot: " << answer << "\n";
  Output.flush();
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
  Output << "Built iterator: " << ans << "\n";
  Output.flush();
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
#ifdef COMPILE_DEBUG
  Output << "Building binary expression " << left << GetOp(op) << right << "\n";
#endif

  expr* answer = MakeBinaryOp(left, op, right, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Got " << answer << "\n";
#endif
  return answer;
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
  expr** parts = foo->MakeArray();
  delete foo;
  expr* answer = MakeAggregate(parts, size, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Built aggregate expression: " << answer << "\n";
  Output.flush();
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
  Output << "Adding " << i << " to Iterators\n";
#endif
  return 1;
}

statement* BuildForLoop(int count, void *stmts)
{
#ifdef COMPILE_DEBUG
  Output << "Popping " << count << " Iterators\n";
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
    Output << "Empty for loop statement, skipping\n";
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
    block = foo->MakeArray();
    delete foo;
  }

  statement *f = MakeForLoop(i, count, block, bsize, filename, lexer.lineno());

#ifdef COMPILE_DEBUG
  Output << "Built for loop statement: ";
  f->showfancy(0, Output);
  Output << "\n";
  Output.flush();
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

statement* BuildFuncStmt(user_func *f, expr *r)
{
  if (f) {
    // Check type!
    f->SetReturn(r);
  }
  delete FormalParams;
  FormalParams = NULL;
  return NULL;
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
  Output << "Formal index stack: ";
  for (int i=0; i<foo->Length(); i++) {
    char* id = foo->Item(i);
    Output << id << " ";
  }
  Output << "\n";
  Output.flush();
#endif
  return foo;
}

template <class PARAM>
inline void* Template_AddParameter(void* list, PARAM *p)
{
  List <PARAM> *foo = (List <PARAM> *)list;
  if (NULL==foo) 
    foo = new List <PARAM> (256);
  foo->Append(p);
  return foo;
}

void* AddParameter(void* list, expr* pass)
{
  return Template_AddParameter(list, pass);
}

void* AddParameter(void* list, formal_param* fp)
{
  return Template_AddParameter(list, fp);
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

user_func* BuildFunction(type t, char*n, void* list)
{
  if (NULL==n) return NULL;
  if (NULL==list) return NULL;  // No parameters? build a const func...

  // TO DO STILL: search symbol table for existing functions with this name!

  List <formal_param> *fpl = (List <formal_param> *)list;
  user_func *f = new user_func(filename, lexer.lineno(), t, n,
  			fpl->Copy(), fpl->Length());

  // Save the formal parameters to a "symbol table"
  FormalParams = fpl;

  InsertFunction(Builtins, f);
  return f;
}

formal_param* BuildFormal(type t, char* name)
{
  return new formal_param(filename, lexer.lineno(), t, name);
}

formal_param* BuildFormal(type t, char* name, expr* deflt)
{
  formal_param *f = new formal_param(filename, lexer.lineno(), t, name);

  expr* d = NULL;
  // check return type of deflt
  if (deflt) {
    if (!Promotable(deflt->Type(0), t)) {
      Error.Start(filename, lexer.lineno());
      Error << "default type does not match parameter " << f;
      Error.Stop();
      Delete(deflt);
    } else {
      d = MakeTypecast(deflt, t, filename, lexer.lineno());
    }
  } 
  f->SetDefault(d);

  return f;
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
  
  // check formal parameters
  if (FormalParams) for (d=0; d<FormalParams->Length(); d++) {
    formal_param *i = FormalParams->Item(d);
    if (NULL==i) continue; // can this happen?
    if (strcmp(name, i->Name())==0)
      return Copy(i);
  }

  // check constants
  constfunc* find = (constfunc*) Constants->FindName(name);
  if (find) return Copy(find);

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

/**  Check types;  tricky only because of aggregates.
     If types match perfectly, "perfect" will be set to true.
     If passed expression can be promoted to match perfectly, 
     "promote" will be set to true.
*/
void MatchParam(formal_param *p, expr* pass, bool &perfect, bool &promote)
{
  if (!promote) {
    DCASSERT(!perfect);
    return;
  }
  if (NULL==pass) return;
  DCASSERT(p);
  if (p->NumComponents() != pass->NumComponents()) {
    perfect = promote = false;
    return;
  }
  int i;
  for (i=0; i<p->NumComponents(); i++) {
    type tfp = p->Type(i);
    type tpass = pass->Type(i);
    if (tfp != tpass) perfect = false;
    if (!Promotable(tpass, tfp)) {
      promote = false;
      return;
    }
  }
}

/**  Score how well this function matches the passed parameters.
     @param	f	The function to check
     @param	params	List of passed parameters (in positional order).
     @return 	Score, as follows.
     		0	: Perfect match
		+n	: n parameters will need to be promoted
		-1	: Parameters do not match in number/type
*/
int ScoreFunction(function *f, List <expr> *params)
{
  bool perfect = true;
  bool promote = true;
  formal_param **fpl;
  int np;
  int rp;
  f->GetParamList(fpl, np, rp);
  int fptr = 0;
  int pptr = 0;
  int numpromote = 0;
  while (promote) {
    if ((fptr == np) && (pptr == params->Length())) break;  // end of params

    if (pptr == params->Length()) return -1;  // not enough passed

    if (fptr == np) {  // too many passed, maybe...
      // can we repeat?
      if (f->ParamsRepeat()) 
	fptr = rp;  // might be ok
      else
	return -1;  // too many passed parameters
    }

    // Compare formal param #fptr with passed param #pptr
    MatchParam(fpl[fptr], params->Item(pptr), perfect, promote);
    if (!perfect) numpromote++;
    fptr++;
    pptr++;
  }
  if (promote) return numpromote;
  return -1;
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

  // Find best match in symbol table
  int bestmatch = params->Length()+2;
  int i;
  for (i=flist->Length()-1; i>=0; i--) {
    int score = ScoreFunction(flist->Item(i), params);
    if ((score>=0) && (score<bestmatch)) {
      // better match, clear old list
      matches->Clear();
      bestmatch = score;
    }
    if (score==bestmatch) {
      // Add to list
      matches->Append(flist->Item(i));
    }
  }

  if (bestmatch > params->Length()) {
    Error.Start(filename, lexer.lineno());
    Error << "No match for " << n;
    DumpPassed(Error, params);
    Error.Stop();
    // dump candidates?
    return NULL;
  }

  if (matches->Length()>1) {
    DCASSERT(bestmatch>0);
    Error.Start(filename, lexer.lineno());
    Error << "Multiple promotions possible for " << n;
    DumpPassed(Error, params);
    Error.Stop();
    // dump matching candidates?
    return NULL;
  }

  // Good to go
  function* find = matches->Item(0);
  int np = params->Length();
  expr** pp = params->MakeArray();
  delete params; 
  expr *fcall = MakeFunctionCall(find, pp, np, filename, lexer.lineno());
  return fcall;
}

expr* BuildNamedFunctionCall(const char *, void*)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Named parameters not done yet, sorry";
  Internal.Stop();
  return NULL;
}

// ==================================================================
// |                                                                |
// |                             Options                            | 
// |                                                                |
// ==================================================================

option* BuildOptionHeader(char* name)
{
  option* answer = FindOption(name);
  if (NULL==answer) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown option " << name;
    Error.Stop();
  }
  return answer;
}

statement* BuildOptionStatement(option* o, expr* v)
{
  if (NULL==o) return NULL;
  if (NULL==v) return NULL;
  // check types
  if (!Promotable(v->Type(0), o->Type())) {
    Error.Start(filename, lexer.lineno());
    Error << "Option " << o;
    if (o->Type()==VOID) {
      // this is an enumerated option
      Error << " is enumerated";
    } else {
      // type mismatch
      Error << " expects type " << GetType(o->Type());
    }
    Error.Stop();
    return NULL;
  }

  expr *e = MakeTypecast(v, o->Type(), filename, lexer.lineno());

  statement *ans = MakeOptionStatement(o, e, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Built option statement: " << ans << "\n";
#endif
  ans->Execute();
  return ans;
}

statement* BuildOptionStatement(option* o, char* n)
{
  if (NULL==o) return NULL;
  if (NULL==n) return NULL;

  if (o->Type()!=VOID) {
    Error.Start(filename, lexer.lineno());
    Error << "Option " << o << " expects type " << GetType(o->Type());
    Error.Stop();
    return NULL;
  }

  const option_const *v = o->FindConstant(n);
  if (NULL==v) {
    Error.Start(filename, lexer.lineno());
    Error << "Illegal value " << n << " for option " << o;
    Error.Stop();
    return NULL;
  }

  statement *ans = MakeOptionStatement(o, v, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Built option statement: " << ans << "\n";
#endif
  ans->Execute();
  return ans;
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
    function *f = bar->Item(i);
    f->ShowHeader(Output);
    Output << "\n";
  }
}
#endif

void InitCompiler()
{
  Iterators = new List <array_index> (256);
  Arrays = new PtrTable();
  Builtins = new PtrTable();
  Constants = new PtrTable();
  FormalParams = NULL;
  matches = new List <function> (32);

  InitBuiltinFunctions(Builtins); 
  InitBuiltinConstants(Constants);

#ifdef COMPILE_DEBUG
  Output << "Initialized compiler data\n";
  Output << "Builtin Functions:\n";
  Builtins->Traverse(ShowSymbols);
  Output << "ready to rock.\n";
#endif
}

