
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

/// Symbol table of "constants", including user-defined
PtrTable *Constants;

/// "Symbol table" of formal parameters
List <formal_param> *FormalParams;

/** List of functions that match what we're looking for.
    Global because we'll re-use it.
*/
List <function> *matches;


bool WithinFor()
{
  return (Iterators->Length());
}

bool WithinBlock()
{
  return WithinFor();
}

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
  Internal << "bad boolean constant: " << s;
  Internal.Stop();
  return ERROR;
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
  
  if (ERROR==start || ERROR==stop) {
    Delete(start);
    Delete(stop);
    return ERROR;
  }
  
  // make sure both are integers here...
  if (start->Type(0)!=INT || stop->Type(0)!=INT) {
    Error.Start(filename, lexer.lineno());
    Error << "Step size expected for interval" << start << ".." << stop;
    Error.Stop();
    Delete(start);
    Delete(stop);
    return ERROR;
  }

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

  if (ERROR==start || ERROR==stop || ERROR==inc) {
    Delete(start);
    Delete(stop);
    Delete(inc);
    return ERROR;
  }

  bool typemismatch = false;
  if (start->Type(0)==REAL || stop->Type(0)==REAL || inc->Type(0)==REAL) {
    // If any is real, promote all to real.
    typemismatch = !Promotable(start->Type(0), REAL)
    			||
    		   !Promotable(stop->Type(0), REAL)
		   	||
    		   !Promotable(inc->Type(0), REAL);
    if (!typemismatch) {
      start = MakeTypecast(start, REAL, filename, lexer.lineno());
      stop = MakeTypecast(stop, REAL, filename, lexer.lineno());
      inc = MakeTypecast(inc, REAL, filename, lexer.lineno());
    }
  } else {
    // Make sure we are all ints.
    typemismatch = start->Type(0)!=INT 
    			|| 
		   stop->Type(0)!=INT 
		   	||
     		   inc->Type(0)!=INT;
  }
  if (typemismatch) {
    // Something strange was attempted    
    Error.Start(filename, lexer.lineno());
    Error << "Type mismatch for interval ";
    Error << start << ".." << stop << ".." << inc;
    Error.Stop();
    Delete(start);
    Delete(stop);
    Delete(inc);
    return ERROR;
  }

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

  if (ERROR==left || ERROR==right) {
    Delete(left);
    Delete(right);
    return ERROR;
  }

  bool ok = false;
  switch (left->Type(0)) {
    case SET_INT:
    	switch (right->Type(0)) {
       	  case SET_INT:		
	   	ok = true;	
	  break;
      	  case SET_REAL:	
      		left = MakeInt2RealSet(filename, lexer.lineno(), left);
		DCASSERT(left);
        	ok = true;
	  break;
    	};
	break;

    case SET_REAL:
    	switch (right->Type(0)) {
	  case SET_INT:
      		right = MakeInt2RealSet(filename, lexer.lineno(), right);
		DCASSERT(right);
        	ok = true;
	  break;
	  case SET_REAL:
	  	ok = true;
	  break;
	};
	break;
  }
  if (!ok) {
    Error.Start(filename, lexer.lineno());
    Error << "Type mismatch for set union: ";
    PrintExprType(left, Error);
    Error << " , ";
    PrintExprType(right, Error);
    Error.Stop();
    Delete(left);
    Delete(right);
    return ERROR;
  }

  expr* answer = MakeUnionOp(filename, lexer.lineno(), left, right);
  return answer;
}

array_index* BuildIterator(type t, char* n, expr* values)
{
  if (ERROR==values) return NULL;

  if (NULL==values) {
    Warning.Start(filename, lexer.lineno());
    Warning << "Empty set for iterator " << n;
    Warning.Stop();
  } else {
    // type checking
    type vt = values->Type(0);
    bool match = false;
    switch (t) {
      case INT:	match = (vt == SET_INT); 	break;
      case REAL:	
        if (vt == SET_INT) {
	    values = MakeInt2RealSet(filename, lexer.lineno(), values);
	    vt = values->Type(0);
	}
    	match = (vt == SET_REAL);	break;
      default:
    	Error.Start(filename, lexer.lineno());
	Error << "Illegal type for iterator " << n;
	Error.Stop();
	Delete(values);
	return NULL;
    }

    if (!match) {
      Error.Start(filename, lexer.lineno());
      Error << "Type mismatch: iterator " << n;
      Error << " expects set of type " << GetType(t);
      Error.Stop();
      Delete(values);
      return NULL;
    }
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
  if (ERROR==opnd) return ERROR;

  // type checking here

  return MakeUnaryOp(op, opnd, filename, lexer.lineno());
}

bool PromoteForOp(expr* &left, int op, expr* &right)
{
  DCASSERT(left);
  DCASSERT(right);
  DCASSERT(left!=ERROR);
  DCASSERT(right!=ERROR);

  // these should be enforced by the compiler (the language itself)
  DCASSERT(left->NumComponents()==1);
  DCASSERT(right->NumComponents()==1);

  type lt = left->Type(0);
  type rt = right->Type(0);

  // A triple-nested switch.  Unpleasant, but clear.
  const char* file = filename;
  int line = lexer.lineno();

  switch (lt) {
    // ========================================================
    case BOOL:
	switch (op) {
	  case OR:
	  case AND:
	  case EQUALS:
	  case NEQUAL:
		switch (rt) {
		  case BOOL:	
			return true;
		  case RAND_BOOL:
		  case PROC_BOOL:
		  case PROC_RAND_BOOL:
		  	// promote left
			left = MakeTypecast(left, rt, file, line);
		  	return true;
		}
	} // end of switch op for bool
	return false;

    // ========================================================
    case INT:
    	switch (op) {
	  case TIMES:
	  	switch (rt) {
		  case INT:
		  case PH_INT: 
		  	return true;  
		  case PROC_INT:
		  case PROC_PH_INT:
		  	left = MakeTypecast(left, PROC_INT, file, line);
			return true;
		  case REAL:
		  case PH_REAL:
		  	left = MakeTypecast(left, REAL, file, line);
			return true;  // real * ph real is ok!
		  case PROC_REAL:
		  case PROC_PH_REAL:
		  	left = MakeTypecast(left, PROC_REAL, file, line);
			return true;  
		  case RAND_INT:
		  case RAND_REAL:
		  case PROC_RAND_INT:
		  case PROC_RAND_REAL:
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		};
	  return false;

	  case PLUS:
	  	switch (rt) {
		  case INT:
		  	return true;
		  case REAL:
		  case PH_INT:
		  case RAND_INT:
		  case RAND_REAL:
		  case PROC_INT:
		  case PROC_REAL:
		  case PROC_PH_INT:
		  case PROC_RAND_INT:
		  case PROC_RAND_REAL:
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		}; 
	  return false;

	  case MINUS:
	  case DIVIDE:
	  case EQUALS:
	  case NEQUAL:
	  case GT:
	  case GE:
	  case LT:
	  case LE:
	  	switch (rt) {
		  case INT:
		  	return true;	
		  case REAL:
		  case RAND_INT:
		  case RAND_REAL:
		  case PROC_INT:
		  case PROC_REAL:
		  case PROC_RAND_INT:
		  case PROC_RAND_REAL:
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		}
	} // end of switch op for int
	return false;

    // ========================================================
    case REAL:
    	switch (op) {
	  case TIMES:
	  	switch (rt) {
		  case INT:
		  	right = MakeTypecast(right, REAL, file, line);
			return true;
		  case PROC_INT:
			left = MakeTypecast(left, PROC_REAL, file, line);
		  	right = MakeTypecast(right, PROC_REAL, file, line);
			return true;
		  case REAL:
		  case PH_REAL:
			return true;  // real * ph real is ok!
		  case PROC_REAL:
		  case PROC_PH_REAL:
		  	left = MakeTypecast(left, PROC_REAL, file, line);
			return true;  
		  case RAND_INT:
		  	left = MakeTypecast(left, RAND_REAL, file, line);
			right = MakeTypecast(right, RAND_REAL, file, line);
			return true;
		  case PROC_RAND_INT:
		  	left = MakeTypecast(left, PROC_RAND_REAL, file, line);
			right = MakeTypecast(right, PROC_RAND_REAL, file, line);
			return true;
		  case RAND_REAL:
		  case PROC_RAND_REAL:
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		};
	  return false;

	  case PLUS:
	  case MINUS:
	  case DIVIDE:
	  case EQUALS:
	  case NEQUAL:
	  case GT:
	  case GE:
	  case LT:
	  case LE:
	  	switch (rt) {
		  case INT:
			right = MakeTypecast(right, REAL, file, line);
		  	return true;	
		  case RAND_INT:
		  	left = MakeTypecast(left, RAND_REAL, file, line);
			right = MakeTypecast(right, RAND_REAL, file, line);
			return true;
		  case PROC_INT:
		  	left = MakeTypecast(left, PROC_REAL, file, line);
			right = MakeTypecast(right, PROC_REAL, file, line);
			return true;
		  case PROC_RAND_INT:
		  	left = MakeTypecast(left, PROC_RAND_REAL, file, line);
			right = MakeTypecast(right, PROC_RAND_REAL, file, line);
			return true;
		  case REAL:
		  case RAND_REAL:
		  case PROC_REAL:
		  case PROC_RAND_REAL:
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		}
	} // end of switch op for real
	return false;
    	
    // ========================================================
    case STRING:
	switch (op) {
	  case EQUALS:
	  case NEQUAL:
	  case GT:
	  case GE:
	  case LT:
	  case LE:
	  case PLUS:
		switch (rt) {
		  case STRING:	
			return true;
		}
	} // end of switch op for string
	return false;

    // ========================================================
  }

  return false;
}

expr* BuildBinary(expr* left, int op, expr* right)
{
  if (NULL==left || NULL==right) {
    Delete(left);
    Delete(right);
    return NULL;
  }

  if (ERROR==left || ERROR==right) {
    Delete(left);
    Delete(right);
    return ERROR;
  }

#ifdef COMPILE_DEBUG
  Output << "Building binary expression " << left << GetOp(op) << right << "\n";
#endif

  // type checking
  bool ok = PromoteForOp(left, op, right);
  if (!ok) {
    Error.Start(filename, lexer.lineno());
    Error << "Illegal binary operation: ";
    PrintExprType(left, Error);
    Error << " " << GetOp(op) << " ";
    PrintExprType(right, Error);
    Error.Stop();
    Delete(left);
    Delete(right);
    return ERROR;
  }

  expr* answer = MakeBinaryOp(left, op, right, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Got " << answer << "\n";
  Output.flush();
#endif
  return answer;
}

expr* BuildTypecast(expr* opnd, type newtype)
{
  if (NULL==opnd) return NULL;
  if (ERROR==opnd) return ERROR;

  // type checking here
  if (!Castable(opnd->Type(0), newtype)) {
    Error.Start(filename, lexer.lineno());
    Error << "Illegal typecast from " << GetType(opnd->Type(0));
    Error << " to " << GetType(newtype);
    Error.Stop();
    Delete(opnd);
    return ERROR;
  }
  
  return MakeTypecast(opnd, newtype, filename, lexer.lineno());
}

void* StartAggregate(expr* a, expr* b)
{
  if (NULL==a || NULL==b) {
    Delete(a);
    Delete(b);
    return NULL;
  }
  if (ERROR==a || ERROR==b) {
    Delete(a);
    Delete(b);
    return ERROR;
  }
  List <expr> *foo = new List <expr> (256);
  foo->Append(a);
  foo->Append(b);
  return foo;
}

void* AddAggregate(void* x, expr* b)
{
  if (NULL==x || ERROR==x) {
    Delete(b);
    return x;
  }
  List <expr> *foo = (List <expr> *)x;
  if (NULL==b || ERROR==b) {
    delete foo;
    return b;
  }
  foo->Append(b);
  return foo;
}

expr* BuildAggregate(void* x)
{
  if (NULL==x) return NULL;
  if (ERROR==x) return ERROR;
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
  if (NULL==i || ERROR==i) return 0;
  // Check for duplicates!
  for (int n=0; n<Iterators->Length(); n++) {
    if (strcmp(Iterators->Item(n)->Name(), i->Name())==0) {
      // Something fishy
      Error.Start(filename, lexer.lineno());
      Error << "Duplicate iterator named " << i;
      Error.Stop();
      Delete(i);
      return 0;
    }
  }
  Iterators->Append(i);
#ifdef COMPILE_DEBUG
  Output << "Adding " << i << " to Iterators\n";
  Output.flush();
#endif
  return 1;
}

statement* BuildForLoop(int count, void *stmts)
{
  int d;
#ifdef COMPILE_DEBUG
  Output << "Popping " << count << " Iterators\n";
  Output.flush();
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
    Output.flush();
#endif
    // Remove iterators 
    for (d=0; d<count; d++) Iterators->Pop();
    return NULL;
  }

  array_index **i = new array_index*[count];
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

  return f;
}

statement* BuildExprStatement(expr *x)
{
  if (NULL==x || ERROR==x) return NULL;
  Optimize(0, x);
  statement* s = MakeExprStatement(x, filename, lexer.lineno());
  return s;
}

statement* BuildArrayStmt(array *a, expr *e)
{
  if (NULL==a || ERROR==a) return NULL;
  if (ERROR==e) return NULL;
  if (NULL==e) {
    return MakeArrayAssign(a, NULL, filename, lexer.lineno());
  }
  if (!Promotable(e->Type(0), a->Type(0))) {
    Error.Start(filename, lexer.lineno());
    Error << "Type mismatch in assignment for array " << a->Name();
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
  if (f && r!=ERROR) {
    // Check type!
    DCASSERT(r->NumComponents()==1);
    if (!Promotable(Type(r, 0), f->Type(0))) {
      Error.Start(filename, lexer.lineno());
      Error << "Return type for function " << f->Name();
      Error << " should be " << GetType(f->Type(0));
      Error.Stop();
    } else {
      expr* ans = MakeTypecast(r, f->Type(0), filename, lexer.lineno());
      f->SetReturn(ans);
    }
  }
  delete FormalParams;
  FormalParams = NULL;
  return NULL;
}

statement* BuildVarStmt(type t, char* id, expr* ret)
{
  if (ERROR==ret) return NULL;

  // eventually... check if we're in a converge

  // first... see if the variable exists already
  constfunc* find = (constfunc*) Constants->FindName(id);
  if (find) {
    // actually, we'll need (eventually) to check state, 
    // because this might be a guess.
    Error.Start(filename, lexer.lineno());
    Error << "Re-definition of constant " << find;
    Error.Stop();
    return NULL;
  }
  // name ok, check type consistency
  DCASSERT(NumComponents(ret)==1);
  if (!Promotable(Type(ret, 0), t)) {
    Error.Start(filename, lexer.lineno());
    Error << "Return type for identifier " << id;
    Error << " should be " << GetType(t);
    Error.Stop();
    return NULL;
  }
  expr* ans = MakeTypecast(ret, t, filename, lexer.lineno());

  // check that we're deterministic...

  constfunc *v = new determfunc(filename, lexer.lineno(), t, id);
  v->SetReturn(ans);
  Constants->AddNamePtr(id, v); 
  return NULL;
}

void* AppendStatement(void* list, statement* s)
{
  if (NULL==s) return list;

  // Top level: execute and forget
  if (!WithinBlock()) {
    s->Execute();
    delete s;
    return NULL;
  }

  // We're within some block; save this statement for later
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

inline bool ListContainsName(void* list, const char* name)
{
  // I wonder if this works?
  List <symbol> *foo = (List <symbol> *)list;
  if (NULL==foo) return false;
  int i;
  for (i=0; i<foo->Length(); i++) {
    if (strcmp(foo->Item(i)->Name(), name)==0) return true;
  }
  return false;
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
  if (ListContainsName(list, fp->Name())) {
    Error.Start(filename, lexer.lineno());
    Error << "Duplicate formal parameter named " << fp;
    Error.Stop();
    Delete(fp);
    return list;
  }
  return Template_AddParameter(list, fp);
}

void* AddParameter(void* list, named_param* np)
{
  if (NULL==np) return list;
  List <named_param>* foo = (List <named_param> *)list;
  if (NULL==foo) {
    foo = new List <named_param> (256);
    foo->Append(np);
    return foo;
  }
  // Find the spot to insert np
  int length = foo->Length();
  int spot;
  for (spot = length; spot; spot--) {
    int cmp = strcmp(foo->Item(spot-1)->name, np->name);
    if (0==cmp) {
      // error, duplicate name
      Error.Start(filename, lexer.lineno());
      Error << "Duplicate passed parameter with name " << np->name;
      Error.Stop();
      return foo;
    }
    if (cmp<0) break;  // order is correct
  }
  foo->InsertAt(spot, np);
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
  if (NULL==pass) {
    // this counts as a perfect match
    return;
  }
  if (ERROR==pass) {
    perfect = promote = false;
    return;
  }
  if (DEFLT==pass) {
    if (p->HasDefault()) return;
    // We're trying to pass a default for a parameter that doesn't have one
    perfect = promote = false;
    return;
  }
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

/**  Make sure this function's parameters differ enough
     from the given formal parameters.
     (Used to check that a newly defined function is not a duplicate of an
     existing function)
     @param	f	The function to check
     @param	params	List of passed parameters (in positional order).
     @return 	The following code.
     		0	: The functions can always be distinguished
		1	: Positional parameters match perfectly
		2	: Named parameters match perfectly
*/
int CantDistinguishFunction(function *f, List <formal_param> *fpb)
{
  // Not sure how to deal with repeats, so bail for now.
  if (f->ParamsRepeat()) {
    return 0;
  }

  formal_param **fpa;
  int npa;
  int dummy;
  f->GetParamList(fpa, npa, dummy);
  
  int npb = fpb->Length();

  // First: check positions and types
  if (npa == npb) {
    // Only check if the number of parameters is the same
    int i;
    for (i = 0; i < npb; i++) {
      bool perfect = true;
      bool promote = true;
      MatchParam(fpa[i], fpb->Item(i), perfect, promote);
      if (!perfect) {
	// These parameters don't have the same type.
	if (!fpa[i]->HasDefault()) break;
	if (!fpb->Item(i)->HasDefault()) break;
	// Still here? They both have defaults, which means
	// they cannot be distinguished if the defaults are passed
      }
    }
    if (i>=npb) {
      // ALL matched, bail
      return 1;
    }
  }

  // Check names (eventually)

  return 0;
}

user_func* BuildFunction(type t, char*n, void* list)
{
  if (NULL==n) return NULL;
  if (NULL==list) return NULL;  // No parameters? build a const func...

  // Save the formal parameters to a "symbol table" (deleted later)
  List <formal_param> *fpl = (List <formal_param> *)list;
  FormalParams = fpl;

  // Make sure this function isn't a duplicate of an existing function
  List <function> *flist = FindFunctions(Builtins, n);
  int i = (flist) ? flist->Length() : 0;
  for (i--; i>=0; i--) {
    // Check functions with this name (if any)
    function *f = flist->Item(i);
    bool error = false;
    if (f->HasSpecialTypechecking()) {
      // Don't allow this yet
      error = true;
    } else {
      error = CantDistinguishFunction(f, fpl);
    }
    if (error) {
      Error.Start(filename, lexer.lineno());
      Error << "Function declaration conflicts with existing function:\n\t";
      f->ShowHeader(Error);
      Error << " declared";
      if (f->Filename()) {
        Error << " in file " << f->Filename() << " near line " << f->Linenumber();
      } else {
	Error << " internally";
      }
      Error.Stop();
      return NULL;
    }
  }

  user_func *f = new user_func(filename, lexer.lineno(), t, n,
  			fpl->Copy(), fpl->Length());

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

named_param* BuildNamedParam(char* name, expr* pass)
{
  if (ERROR == pass) return NULL;
  named_param* answer = new named_param;
  answer->name = name;
  answer->pass = pass;
  answer->UseDefault = false;
  return answer;
}

named_param* BuildNamedDefault(char* name)
{
  named_param* answer = new named_param;
  answer->name = name;
  answer->pass = NULL;
  answer->UseDefault = true;
  return answer;
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
  return ERROR;
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
    return ERROR;
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
    return ERROR;
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
      return ERROR;
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
    bool perfect = true;
    MatchParam(fpl[fptr], params->Item(pptr), perfect, promote);
    if (!perfect) numpromote++;
    fptr++;
    pptr++;
  }
  if (promote) return numpromote;
  return -1;
}

/** Promote passed parameters to formal types, as necessary.
    Assumes that type checking has been performed already. 
*/
bool LinkFunction(function *f, expr** params, int np)
{
  formal_param **fpl;
  int nfp;
  int rfp;
  f->GetParamList(fpl, nfp, rfp);
  int fptr = 0;
  for (int pptr=0; pptr<np; pptr++) {
    if (fptr == nfp) {
      if (f->ParamsRepeat())
	fptr = rfp;
      else {
	Internal.Start(__FILE__, __LINE__);
	Internal << "Can't link parameters";
	Internal.Stop();
	return false;
      }
    }

    // promote each component as necessary
    expr *prom = NULL;
    if (DEFLT==params[pptr]) {
      // Replace defaults
      prom = Copy(fpl[fptr]->Default());
    } else {
      // Promote parameter as necessary
      if (fpl[fptr]->NumComponents()==1) {
        // no aggregation
        prom = MakeTypecast(params[pptr], fpl[fptr]->Type(0),
      				filename, lexer.lineno());
      } else {
        prom = MakeTypecast(params[pptr], fpl[fptr], filename, lexer.lineno());
      }
    }
    params[pptr] = prom;
    fptr++;
  }
  return true;
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
    return ERROR;
  }

  // Find best match in symbol table
  int bestmatch = params->Length()+2;
  int i;
  for (i=flist->Length()-1; i>=0; i--) {
    function *f = flist->Item(i);
    int score;
    if (f->HasSpecialTypechecking()) 
      score = f->Typecheck(params);
    else
      score = ScoreFunction(f, params);

#ifdef COMPILE_DEBUG
    Output << "Function " << f << " got score " << score << "\n";
    Output.flush();
#endif

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
    return ERROR;
  }

  if (matches->Length()>1) {
    DCASSERT(bestmatch>0);
    Error.Start(filename, lexer.lineno());
    Error << "Multiple promotions possible for " << n;
    DumpPassed(Error, params);
    Error.Stop();
    // dump matching candidates?
    return ERROR;
  }

  // Good to go
  function* find = matches->Item(0);
  int np = params->Length();
  expr** pp = params->MakeArray();
  delete params; 

  // Promote params
  bool ok;
  if (find->HasSpecialParamLinking())
    ok = find->LinkParams(pp, np);
  else
    ok = LinkFunction(find, pp, np);

  if (!ok) return ERROR;

  expr *fcall = MakeFunctionCall(find, pp, np, filename, lexer.lineno());
  return fcall;
}

/**  Score how well this function matches the passed named parameters.
     @param	f	The function to check
     @param	params	List of parameters (in name order).
     @return 	Score, as follows.
     		0	: Perfect match
		+n	: n parameters will need to be promoted
		-1	: Some fatal error (e.g.,
			  missing name, type mismatch, etc.)
*/
int ScoreFunction(function *f, List <named_param> *params)
{
  if (f->ParamsRepeat()) return -1;
  // For now, don't allow named parameters with repeating lists.

  bool promote = true;
  formal_param **fpl;
  int dummy;
  int np;
  f->GetParamList(fpl, np, dummy);
  // Traverse f's sorted list, check against ours
  int fptr = 0;
  int pptr = 0;
  int numpromote = 0;
  formal_param* y = fpl[f->NamedPosition(0)];
  named_param* x = params->Item(0);
  while (promote) {
    if ((NULL==x) && (NULL==y)) break;
    int cmp;
    if (x && y) cmp = strcmp(y->Name(), x->name);
    else if (y) cmp = -1;
    else cmp = 1;
    if (cmp<0) {
      // formal parameter not passed, make sure there's a default
      if (!y->HasDefault()) {
	return -1;  // critical parameter is missing
      }
      // there's a default; advance formal
      fptr++;
      if (fptr < np) y = fpl[f->NamedPosition(fptr)];
      else y = NULL;
    } else if (cmp>0) {
      // extra named parameter, that's bad
      return -1;
    } else {
      // Match; check types.
      bool perfect = true;
      MatchParam(y, x->pass, perfect, promote);
      if (!perfect) numpromote++;
      // advance named
      pptr++;
      if (pptr < params->Length()) x = params->Item(pptr);
      else x = NULL;
      // advance formal
      fptr++;
      if (fptr < np) y = fpl[f->NamedPosition(fptr)];
      else y = NULL;
    }
  }
  if (promote) return numpromote;
  return -1;
}


expr* BuildNamedFunctionCall(const char *n, void* x)
{
  List <named_param> * foo = (List <named_param> *) x;
  if (NULL == foo) return NULL;
  int i;
  Output << "Named parameters:\n";
  for (i=0; i<foo->Length(); i++) {
    Output << "\t" << foo->Item(i)->name << "\n";
  }
  Output.flush();

  // find symbol table entry
  List <function> *flist = FindFunctions(Builtins, n);
  if (NULL==flist) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown function " << n;
    Error.Stop();
    delete foo;
    return ERROR;
  }

  Internal.Start(__FILE__, __LINE__);
  Internal << "Named parameters not done yet, sorry";
  Internal.Stop();
  return ERROR;
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
  if (NULL==v || ERROR==v) return NULL;
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
  Output.flush();
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
  Output.flush();
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

