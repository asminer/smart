
// $Id$

#include "../Base/api.h"
#include "compile.h"
#include "../Templates/list.h"
#include "../Templates/heap.h"
#include "tables.h"
#include "fnlib.h"
#include "../Formalisms/api.h"
#include "../Language/measures.h"

#include <stdio.h>
#include <FlexLexer.h>

//#define COMPILE_DEBUG

// ==================================================================
// |                                                                |
// |                          Lexer  hooks                          |
// |                                                                |
// ==================================================================

extern char* filename;
extern yyFlexLexer lexer;

char* Filename() { return filename; }

int LineNumber() { return lexer.lineno(); }

bool IsInteractive() { return (strcmp("-", filename)==0); }

// ==================================================================
// |                                                                |
// |                      Global compiler data                      | 
// |                                                                |
// ==================================================================

/// Current stack of for-loop iterators
List <array_index> Iterators(256);

/// Current stack of array indexes
List <char> Indexes(256);

/// For allocating func_call structs
Manager <func_call> FuncCallPile(16);

/// Depth of converges
int converge_depth;

/// Symbol table of arrays
PtrTable Arrays;

/// Symbol table of functions
PtrTable Builtins;

/// Symbol table of models
PtrTable Models;

/// Symbol table of "constants", including user-defined
PtrTable Constants;

/// "Symbol table" of formal parameters
List <formal_param> *FormalParams;

/// Model block we are in (switch to stack if we allow nesting)
model *model_under_construction;

/// Internal model symbol table
PtrTable *ModelInternal;

/// External model symbol table
HeapOfPointers <symbol> ModelExternal(16);

/** List of functions that match what we're looking for.
    Global because we'll re-use it.
*/
List <function> matches(16);

bool WithinFor() { return (Iterators.Length()); }

bool WithinConverge() { return (converge_depth>0); }

bool WithinModel() { return (ModelInternal != NULL); }

bool WithinBlock() { return WithinFor() || WithinConverge() || WithinModel(); }

// ==================================================================
// |                                                                |
// |                       Handy output stuff                       |
// |                                                                |
// ==================================================================

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

void DumpPassed(OutputStream &s, List <named_param> *pass)
{
  if (NULL==pass) return;
  if (pass->Length()==0) return;
  s << "(";
  int i;
  for (i=0; i<pass->Length(); i++) {
    if (i) s << ", ";
    named_param *np = pass->Item(i);
    if (!np->UseDefault) {
      PrintExprType(np->pass, s);
      s << " ";
    }
    s << pass->Item(i)->name;
    if (np->UseDefault) {
      s << ":=default";
    }
  }
  s << ")";
}

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
    if (NO_SUCH_MODIF == m) {
      Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
      Internal << "Bad type modifier: " << modif;
      Internal.Stop();
      // shouldn't get here
      return VOID;
    }
  }
  int t = FindType(tp);
  if (NO_SUCH_TYPE == t) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Bad type: " << tp;
    Internal.Stop();
    // shouldn't get here
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
    return MakeConstExpr(BOOL, true, filename, lexer.lineno());
  }
  if (strcmp(s, "false") == 0) {
    return MakeConstExpr(BOOL, false, filename, lexer.lineno());
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
  return MakeConstExpr(INT, value, filename, lexer.lineno());
}

expr* MakeRealConst(char* s)
{
  double value;
  sscanf(s, "%lf", &value);
  return MakeConstExpr(REAL, value, filename, lexer.lineno());
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

  expr* answer = MakeInterval(filename, lexer.lineno(), start, stop, 
			MakeConstExpr(INT, 1, filename, lexer.lineno()));

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

/**  Performs type promotion for left and right operators
     as necessary for the given operator.
     If this is impossible, return false.
     If we are successful, return true.
*/
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
    case RAND_BOOL:
	switch (op) {
	  case OR:
	  case AND:
	  case EQUALS:
	  case NEQUAL:
		switch (rt) {
		  case BOOL:
		  	// promote right
			right = MakeTypecast(right, lt, file, line);
			return true;
		  case RAND_BOOL:	
			return true;
		  case PROC_BOOL:
		  case PROC_RAND_BOOL:
		  	// promote left
			left = MakeTypecast(left, rt, file, line);
		  	return true;
		}
	} // end of switch op for rand bool
	return false;

    // ========================================================
    case RAND_INT:
    	switch (op) {
	  case PLUS:
	  case MINUS:
	  case TIMES:
	  case DIVIDE:
	  case EQUALS:
	  case NEQUAL:
	  case GT:
	  case GE:
	  case LT:
	  case LE:
	  	switch (rt) {
		  case INT:
		  case PH_INT:
		  	// promote right
			right = MakeTypecast(right, lt, file, line);
			return true;

		  case RAND_INT:
		  	return true;

		  case RAND_REAL:
		  case PROC_INT:
		  case PROC_REAL:
		  case PROC_RAND_INT:
		  case PROC_RAND_REAL:
		  	// promote left
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		}
	} // end of switch op for rand int
	return false;

    // ========================================================
    case RAND_REAL:
    	switch (op) {
	  case PLUS:
	  case MINUS:
	  case TIMES:
	  case DIVIDE:
	  case EQUALS:
	  case NEQUAL:
	  case GT:
	  case GE:
	  case LT:
	  case LE:
	  	switch (rt) {
		  case INT:
		  case REAL:
		  case EXPO:
		  case PH_INT:
		  case PH_REAL:
		  case RAND_INT:
		  	// promote right
			right = MakeTypecast(right, lt, file, line);
			return true;

		  case RAND_REAL:
		  	return true;

		  case PROC_REAL:
		  case PROC_RAND_REAL:
		  	// promote left
		  	left = MakeTypecast(left, rt, file, line);
			return true;
		}
	} // end of switch op for rand int
	return false;

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

void* StartSequence(expr* a, expr* b)
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
  if (a->Type(0)!=VOID || b->Type(0)!=VOID) {
    Error.Start(filename, lexer.lineno());
    Error << "Illegal binary operation: ";
    PrintExprType(a, Error);
    Error << " ; ";
    PrintExprType(b, Error);
    Error.Stop();
    Delete(a);
    Delete(b);
    return ERROR;
  } 
  List <expr> *foo = new List <expr> (256);
  foo->Append(a);
  foo->Append(b);
  return foo;
}

void* AddToSequence(void* x, expr* b)
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
  if (b->Type(0)!=VOID) {
    Error.Start(filename, lexer.lineno());
    Error << "Illegal binary operation: VOID ; ";
    PrintExprType(b, Error);
    Error.Stop();
    Delete(b);
    delete foo;
    return ERROR;
  }
  foo->Append(b);
  return foo;
}

expr* BuildSequence(void* x)
{
  if (NULL==x) return NULL;
  if (ERROR==x) return ERROR;
  List <expr> *foo = (List <expr> *)x;
  int size = foo->Length();
  expr** parts = foo->MakeArray();
  delete foo;
  expr* answer = MakeAssocOp(SEMI, parts, size, filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Built sequence expression: " << answer << "\n";
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
  for (int n=0; n<Iterators.Length(); n++) {
    if (strcmp(Iterators.Item(n)->Name(), i->Name())==0) {
      // Something fishy
      Error.Start(filename, lexer.lineno());
      Error << "Duplicate iterator named " << i;
      Error.Stop();
      Delete(i);
      return 0;
    }
  }
  Iterators.Append(i);
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
  if (count > Iterators.Length()) {
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Iterator stack underflow";
    Internal.Stop();

    Iterators.Clear();
    return NULL;
  }

  if (NULL==stmts) {
#ifdef COMPILE_DEBUG
    Output << "Empty for loop statement, skipping\n";
    Output.flush();
#endif
    // Remove iterators 
    for (d=0; d<count; d++) Iterators.Pop();
    return NULL;
  }

  array_index **i = new array_index*[count];
  int first = Iterators.Length() - count;
  for (d=0; d<count; d++) {
    i[d] = Iterators.Item(first+d);
  }
  for (d=0; d<count; d++) Iterators.Pop();

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
    // this is the proper thing even within a converge
  }
  if (!Promotable(e->Type(0), a->Type(0))) {
    Error.Start(filename, lexer.lineno());
    Error << "Type mismatch in assignment for array " << a->Name();
    Error.Stop();
    return NULL;
  }
  Optimize(0, e);
  expr* ne = MakeTypecast(e, a->Type(0), filename, lexer.lineno());
  
  if (WithinConverge()) 
    return MakeArrayCvgAssign(a, ne, filename, lexer.lineno());

  if (WithinModel()) 
    return MakeMeasureArrayAssign(
    		model_under_construction,
		a, ne, filename, lexer.lineno()
	   );

  // ordinary array

  return MakeArrayAssign(a, ne, filename, lexer.lineno());
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

void DoneWithFunctionHeader(user_func* )
{
  // do we need to do anything to the function?
  delete FormalParams;
  FormalParams = NULL;
}

statement* BuildVarStmt(type t, char* id, expr* ret)
{
  if (ERROR==ret) return NULL;

  if (WithinConverge()) {
    // Make sure we're REAL
    if (t != REAL) {
      Error.Start(filename, lexer.lineno());
      Error << "Converge variable " << id << " must have type real";
      Error.Stop();
      Delete(ret);
      return NULL;
    }
  }

  variable* find = NULL;

  // see if the variable exists...
  if (WithinModel()) {
    // Just within this model
    DCASSERT(ModelInternal);
    if (ModelInternal->ContainsName(id)) {
      Error.Start(filename, lexer.lineno());
      Error << "Identifier " << id << " already used in model";
      Error.Stop();
      return NULL;
    }
  } else {
    // Globally
    find = (variable*) Constants.FindName(id);
    if (find) {
      bool error = true;
      if (WithinConverge()) {
        error = (find->state != CS_HasGuess);
      } 
      if (error) {
        Error.Start(filename, lexer.lineno());
        Error << "Re-definition of constant " << find;
        Error.Stop();
        return NULL;
      }
    }
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

  if (WithinConverge()) {
    // Idents within a converge
    cvgfunc* v; 
    if (find) {
      v = dynamic_cast<cvgfunc*> (find);
      if (!v) {
	Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
	Internal << "Bad already guessed converge var " << find;
	Internal.Stop();
      }
    } else {
      v = MakeConvergeVar(t, id, filename, lexer.lineno());
      Constants.AddNamePtr(id, v); 
    }
    statement *s = MakeAssignStmt(v, ans, filename, lexer.lineno());
    return s;
  } 

  if (WithinModel()) {
    // Idents within a model (must be a measure)
    measure *m = new measure(filename, lexer.lineno(), t, id);
    m->SetReturn(ans);
    ModelInternal->AddNamePtr(id, m);
    ModelExternal.Insert(m);
    return MakeMeasureAssign(
    		model_under_construction, 
		m, filename, lexer.lineno()
	   );
  }

  // Normal idents (i.e., constants)
  constfunc* v;
  v = MakeConstant(t, id, filename, lexer.lineno());
  v->SetReturn(ans);
  Constants.AddNamePtr(id, v); 
  return NULL;
}

statement* BuildGuessStmt(type t, char* id, expr* ret)
{
  if (ERROR==ret) return NULL;

  if (!WithinConverge()) {
    Error.Start(filename, lexer.lineno());
    Error << "Guess for " << id << " outside converge";
    Error.Stop();
    Delete(ret);
    return NULL;
  }

  // Make sure we're REAL
  if (t != REAL) {
    Error.Start(filename, lexer.lineno());
    Error << "Converge variable " << id << " must have type real";
    Error.Stop();
    Delete(ret);
    return NULL;
  }

  // see if the variable exists already
  variable* find = (variable*) Constants.FindName(id);
  if (find) {
    Error.Start(filename, lexer.lineno());
    Error << "Re-definition of constant " << find;
    Error.Stop();
    Delete(ret);
    return NULL;
  }
  // name ok, check type consistency
  DCASSERT(NumComponents(ret)==1);
  if (!Promotable(Type(ret, 0), t)) {
    Error.Start(filename, lexer.lineno());
    Error << "Return type for identifier " << id;
    Error << " should be " << GetType(t);
    Error.Stop();
    Delete(ret);
    return NULL;
  }
  expr* ans = MakeTypecast(ret, t, filename, lexer.lineno());

  cvgfunc *v = MakeConvergeVar(t, id, filename, lexer.lineno());
  Constants.AddNamePtr(id, v); 
  statement* s = MakeGuessStmt(v, ans, filename, lexer.lineno());
  return s;
}

statement* BuildArrayGuess(array* id, expr* ret)
{
  if (NULL==id || ERROR==id) return NULL;
  if (ERROR==ret) return NULL;

  if (!WithinConverge()) {
    Error.Start(filename, lexer.lineno());
    Error << "Guess for " << id << " outside converge";
    Error.Stop();
    Delete(ret);
    return NULL;
  }

  // check type consistency
  DCASSERT(NumComponents(ret)==1);
  if (!Promotable(Type(ret, 0), REAL)) {
    Error.Start(filename, lexer.lineno());
    Error << "Return type for identifier " << id;
    Error << " should be " << GetType(REAL);
    Error.Stop();
    Delete(ret);
    return NULL;
  }

  // guess already?
  if (id->state != CS_Undefined) {
    Error.Start(filename, lexer.lineno());
    Error << "Duplicate guess for identifier " << id;
    Error.Stop();
    return NULL;
  }

  // good to go
  if (ret) {
    Optimize(0, ret);
    expr* nr = MakeTypecast(ret, REAL, filename, lexer.lineno());
    statement* s = MakeArrayCvgGuess(id, nr, filename, lexer.lineno());
    return s;
  }
  return MakeArrayCvgGuess(id, NULL, filename, lexer.lineno());
}

void* AppendStatement(void* list, statement* s)
{
  if (NULL==s) return list;

  // Top level: execute and forget
  if (!WithinBlock()) {
    s->Execute();
    s->Affix();
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

void StartConverge()
{
  converge_depth++;
}

statement* FinishConverge(void* list)
{
  converge_depth--;
  List <statement> *foo = (List <statement> *) list;
  statement** block = NULL;
  int bsize = 0;
  if (foo) {
    bsize = foo->Length();
    block = foo->MakeArray();
  }
  delete foo;
  if (NULL==block) return NULL;

  return MakeConverge(block, bsize, filename, lexer.lineno());
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
  if (NULL==list) {
    Indexes.Clear();
    list = &Indexes;
  } 
  DCASSERT(list == &Indexes);
  Indexes.Append(n);
#ifdef COMPILE_DEBUG
  Output << "Formal index stack: ";
  for (int i=0; i<Indexes.Length(); i++) {
    char* id = Indexes.Item(i);
    Output << id << " ";
  }
  Output << "\n";
  Output.flush();
#endif
  return list;
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

void TrashIndexList()
{
  int length = Indexes.Length();
  for (length--; length>=0; length--) 
    delete[] Indexes.Item(length);
  Indexes.Clear();
}

/**
    Returns true if they match, false if they don't.
    @param	n	The array we're defining (used for errors)
    @param	itlist	List of index names
*/
bool CompareIterators(char* n, void* itlist)
{
  if (NULL==itlist) return false;
  DCASSERT(itlist == &Indexes);
  int dim = Iterators.Length();

  // check the iterators dimensions
  if (Indexes.Length() != dim) {
    Error.Start(filename, lexer.lineno());
    Error << "Dimension of array " << n << " does not match iterators";
    Error.Stop();
    return false;
  }

  // compare iterator names
  int i;
  for (i=0; i<dim; i++) {
    array_index* it = Iterators.Item(i);
    DCASSERT(it);
    const char* exname = it->Name();
    DCASSERT(exname);
    char* myname = Indexes.Item(i);
    DCASSERT(myname);
    if (strcmp(exname, myname)!=0) {
      Error.Start(filename, lexer.lineno());
      Error << "Array " << n << " expecting index " << exname;
      Error << ", got " << myname;
      Error.Stop();
      return false;
    }
  }

  return true;
}

array* BuildArray(type t, char*n, void* list)
{
  if (NULL==n) return NULL;
  if (NULL==list) return NULL;

  if (WithinConverge()) {
    // Make sure we're REAL
    if (t != REAL) {
      Error.Start(filename, lexer.lineno());
      Error << "Converge variable " << n << " must have type real";
      Error.Stop();
      TrashIndexList();
      return NULL;
    }
  }

  // Does the array / name exist already?
  array* find = NULL;
  if (WithinModel()) {
    // look at model symbols
    DCASSERT(ModelInternal);
    if (ModelInternal->ContainsName(n)) {
        Error.Start(filename, lexer.lineno());
        Error << "Identifier " << n << " already used in model";
        Error.Stop();
        TrashIndexList();
        return NULL;
    }
  } else {
    // look at global symbols
    find = (array*) Arrays.FindName(n);

    // Allow guesses in converge, otherwise duplication = error
    if (find) {
      bool error = true;
      if (WithinConverge()) {
        // might be ok
        error = (find->state == CS_Defined) || (find->state == CS_Computed);
      }
      if (error) {
        Error.Start(filename, lexer.lineno());
        Error << "Array " << n << " already defined";
        Error.Stop();
        TrashIndexList();
        return NULL;
      }
    }
  }
  
  // Name is ok, check iterators
  if (!CompareIterators(n, list)) {
    // Problem with iterators, bail out
    TrashIndexList();
    return NULL;
  }

  // If this is an existing array, then return it
  if (find) return find;

  // Build "copies" of iterators 
  int dim = Iterators.Length();
  array_index **il = Iterators.Copy();
  for (int i=0; i<dim; i++) {
    Copy(il[i]);  // increment counter
  }

  // build array
  array *A = new array(filename, lexer.lineno(), t, n, il, dim);

  if (WithinModel()) {
    // Add array (measure) to model symbol table
    ModelInternal->AddNamePtr(n, A);
    ModelExternal.Insert(A);
  } else {
    // Add array to array symbol table
    Arrays.AddNamePtr(n, A); 
  }

  // Done with indexes
  TrashIndexList();

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
    // this counts as a perfect match for VOID, and promotable otherwise
    perfect = (p->NumComponents()==1) && (p->Type(0)==VOID);
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

bool CanDistinguishPositionalParams(function *f, List <formal_param> *fpb)
{
  // Not sure how to deal with this case, so don't (for now)
  if (f->HasSpecialTypechecking())
    return false;

  formal_param **fpa;
  int npa;
  int dummy;
  f->GetParamList(fpa, npa, dummy);
  
  int npb = fpb->Length();

  // Check positions and types
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
      return false;
    }
  }
  return true; 
}

bool CanDistinguishNamedParams(function *f, List <formal_param> *fpb)
{
  // Not sure how to deal with this case, so don't (for now)
  if (f->HasSpecialTypechecking())
    return false;

  // not implemented yet ;^)
  return true;
}

bool PerfectFormalMatch(function *f, List <formal_param> *fpb)
{
  if (f->HasSpecialTypechecking()) return false;

  formal_param **fpa;
  int npa;
  int dummy;
  f->GetParamList(fpa, npa, dummy);

  if (npa != fpb->Length()) return false;

  int i;
  for (i=0; i<npa; i++) {
    // Check defaults later!

    DCASSERT(fpa[i]);
    if (fpa[i]->Matches(fpb->Item(i))) continue;
    // mismatch
    return false;
  }
  return true;
}

bool HasADefault(function *f, List <formal_param> *fpb)
{
  formal_param **fpa;
  int npa;
  int dummy;
  f->GetParamList(fpa, npa, dummy);
  int i;
  for (i=0; i<npa; i++) {
    DCASSERT(fpa[i]);
    if (fpa[i]->HasDefault()) return true;
  }
  
  for (i=0; i<fpb->Length(); i++) {
    formal_param *fp = fpb->Item(i);
    if (fp->HasDefault()) return true;
  }
  return false;
}

/** Check for proper overloading and forward defs.
    @param	table	The symbol table to check
    @param	t	Function (or model) type to be declared
    @param	n	Function name
    @param	FormalParams	(Global variable) for the formal parameters.
    @param	out	The matched function or NULL if none (legal)

    @return	true 	If we should continue and construct a new function
*/
bool CheckFunctionDecl(PtrTable *table, type t, char *n, function* &out)
{
  List <function> *flist = FindFunctions(table, n);
  int i = (flist) ? flist->Length() : 0;
  for (i--; i>=0; i--) {
    // Check functions with this name (if any)
    function *f = flist->Item(i);

    if (CanDistinguishPositionalParams(f, FormalParams) &&
        CanDistinguishNamedParams(f, FormalParams)) 
      continue;  // properly overloaded function
      
    // Check: Is this a forward-defined function?
    if (f->isForwardDefined()) {
      // we had better match perfectly
      if ((!PerfectFormalMatch(f, FormalParams)) || (t!=f->Type(0))) {
	Error.Start(filename, lexer.lineno());
	Error << "Forward declaration mismatch with function:\n\t";
	f->ShowHeader(Error);
	if (f->Filename()) {
          Error << " declared in file " << f->Filename();
	  Error << " near line " << f->Linenumber();
	} 
	Error.Stop();
	delete FormalParams;
	FormalParams = NULL;
	out = NULL;
	return false;
      }
      bool need_warning = HasADefault(f, FormalParams);
      // Replace us with the forward declaration 
      out = f;
      // Swap parameters...
      FormalParams->Clear();
      f->FillFormal(FormalParams);
      if (need_warning) {
        Warning.Start(filename, lexer.lineno());
        Warning << "Using original defaults for forward-defined function:\n\t";
        f->ShowHeader(Warning);
        Warning.Stop();
      }
      return false;
    } else {
      // Not forward defined; bad overloading
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
      delete FormalParams;
      FormalParams = NULL;
      out = NULL;
      return false;
    } // if (forward defined)
  } // for i
  out = NULL;
  return true;
}

user_func* BuildFunction(type t, char*n, void* list)
{
  if (NULL==n) return NULL;
  if (NULL==list) return NULL;  // No parameters? build a const func...

  FormalParams = (List <formal_param> *)list;

  if (WithinBlock()) {
    Error.Start(filename, lexer.lineno());
    Error << "Function " << n << " defined within a for/converge";
    Error.Stop();
    delete FormalParams;
    FormalParams = NULL;
    return NULL;
  }

  function *out;
  // Make sure this function isn't a duplicate of an existing function
  if (CheckFunctionDecl(&Builtins, t, n, out)) {
    // This is a new function.
    user_func *f = new user_func(filename, lexer.lineno(), t, n,
    			FormalParams->Copy(), FormalParams->Length());

    InsertFunction(&Builtins, f);
    return f;
  } else {
    if (out) {
      // found a forward decl
      user_func *f = dynamic_cast<user_func*> (out);
      if (NULL==f) {
	Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
	Internal << "Bad forward definition match?";
	Internal.Stop();
      }
      return f;
    } 
  }
  return NULL;
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

/**  Score how well this function matches the passed parameters.
     @param	f	The function to check
     @param	hidden	Type model we're in (for model functions)
			or VOID to use normal functions.
     @param	params	List of passed parameters (in positional order).
     @return 	Score, as follows.
     		0	: Perfect match
		+n	: n parameters will need to be promoted
		-1	: Parameters do not match in number/type
*/
int ScoreFunction(function *f, type hidden, List <expr> *params)
{
  bool promote = true;
  formal_param **fpl;
  int np;
  int rp;
  f->GetParamList(fpl, np, rp);
  int fptr = 0;
  int pptr = 0;
  int numpromote = 0;

  // Typecheck the hidden parameter, if any
  if (hidden != VOID) {
    DCASSERT(f->isWithinModel());
    DCASSERT(IsModelType(hidden));

    if (0==np) return -1;  // no parameters for this function!

    // Compare formal param #fptr with hidden passed param
    if (!MatchesTemplate(fpl[fptr]->Type(0), hidden)) return -1;
    
    fptr++;
  } else {
    DCASSERT(!f->isWithinModel());
  }

  if (NULL==params) {
    // no passed params
    if (fptr == np) return numpromote;  
    else return -1; // wrong number of params
  }

  // Check the rest of the passed parameters
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

// Used for both function and model calls ;^)
// returns NULL on error, a nice struct on success.
func_call* MatchFunctionCallPos(PtrTable *syms, const char* n, void* poslist, bool quietly)
{
  List <expr> *params = (List <expr> *)poslist;
  int params_length = (params) ? (params->Length()) : 0;
  
  // If we are within a model, check model functions too
  List <function> *flist_mod = NULL;
  if (model_under_construction) {
    flist_mod = FindModelFunctions(model_under_construction->Type(0), n);
  }

  // find symbol table entry
  List <function> *flist = FindFunctions(syms, n);

  if ((NULL==flist_mod) && (NULL==flist)) {
    // Never heard of it
    if (!quietly) {
      Error.Start(filename, lexer.lineno());
      Error << "Unknown function " << n;
      Error.Stop();
    }
    delete params;
    return NULL;
  }

  int bestmatch = params_length+2;
  int i, last;
  bool is_normal_match = true;

  // Find best match in "ordinary" symbol table
  last = (flist) ? flist->Length()-1 : -1;
  for (i=last; i>=0; i--) {
    function *f = flist->Item(i);
    int score;
    if (f->HasSpecialTypechecking()) 
      score = f->Typecheck(params);
    else
      score = ScoreFunction(f, VOID, params);

#ifdef COMPILE_DEBUG
    Output << "Function " << f << " got score " << score << "\n";
    Output.flush();
#endif

    if ((score>=0) && (score<bestmatch)) {
      // better match, clear old list
      matches.Clear();
      bestmatch = score;
    }
    if (score==bestmatch) {
      // Add to list
      matches.Append(flist->Item(i));
    }
  } // for i

  // Find best match in model symbol table
  last = (flist_mod) ? flist_mod->Length()-1 : -1;
  for (i=last; i>=0; i--) {
    // same as above except we have to add the "hidden" parameter
    function *f = flist_mod->Item(i);
    int score;
    if (f->HasSpecialTypechecking()) 
      score = f->Typecheck(params);
    else
      score = ScoreFunction(f, model_under_construction->Type(0), params);

#ifdef COMPILE_DEBUG
    Output << "Function " << f << " got score " << score << "\n";
    Output.flush();
#endif

    if ((score>=0) && (score<bestmatch)) {
      // better match, clear old list
      matches.Clear();
      bestmatch = score;
      is_normal_match = false;
    }
    if (score==bestmatch) {
      if (is_normal_match) {
        // tie between ordinary function and model function...
        // Is this even possible?  If so, model function wins
        matches.Clear();
        is_normal_match = false;
      }
      // Add to list
      matches.Append(flist_mod->Item(i));
    }
  } // for i

  // Did we get any hits?
  if (bestmatch > params_length) {
    if (!quietly) {
      Error.Start(filename, lexer.lineno());
      Error << "No match for " << n;
      DumpPassed(Error, params);
      Error.Stop();
    }
    // dump candidates?
    delete params;
    return NULL;
  }

  if (matches.Length()>1) {
    DCASSERT(bestmatch>0);
    Error.Start(filename, lexer.lineno());
    Error << "Multiple promotions possible for " << n;
    DumpPassed(Error, params);
    Error.Stop();
    delete params;
    // dump matching candidates?
    return NULL;
  }

  // Fill struct for matching function call
  func_call *foo = FuncCallPile.NewObject();
  foo->find = matches.Item(0);

  if (params) {
    // If necessary, add hidden parameter
    if (!is_normal_match) {
      DCASSERT(model_under_construction);
      params->InsertAt(0, Copy(model_under_construction));
#ifdef COMPILE_DEBUG
      Output << "Added parameter " << model_under_construction << "\n";
      Output.flush();
#endif
    }
    // Good to go, fill struct
    foo->np = params->Length();
    foo->pass = params->MakeArray();
    delete params; 
  } else {
    // This should be a model function with one hidden parameter
    DCASSERT(!is_normal_match);
    DCASSERT(model_under_construction);
    foo->np = 1;
    foo->pass = new expr*[1];
    foo->pass[0] = Copy(model_under_construction);
  }

  // Promote params
  bool ok;
  if (foo->find->HasSpecialParamLinking())
    ok = foo->find->LinkParams(foo->pass, foo->np);
  else
    ok = LinkFunction(foo->find, foo->pass, foo->np);

  if (!ok) {
    // should delete foo->pass
    FuncCallPile.FreeObject(foo);
    return NULL;
  }

  return foo;
}

expr* BuildFunctionCall(const char* n, void* posparams)
{
  func_call *x = MatchFunctionCallPos(&Builtins, n, posparams, false);
  if (NULL==x) return ERROR;
  expr *fcall = MakeFunctionCall(x->find, x->pass, x->np, 
  				filename, lexer.lineno());
#ifdef COMPILE_DEBUG
  Output << "Built function call:\n\t" << fcall << "\n";
  Output.flush();
#endif
  FuncCallPile.FreeObject(x);
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
  // Also don't allow hidden named parameters

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
      // Match
      // Check defaults
      if (x->UseDefault) if (!y->HasDefault()) return -1; 
      // Check types.
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

expr** NamedToPositions(function *f, List <named_param> *named, int& np)
{
  DCASSERT(f);
  DCASSERT(named);
  formal_param **fpl;
  int dummy;
  f->GetParamList(fpl, np, dummy);
  expr** params = new expr* [np];
  // Traverse sorted lists
  int fptr = 0;
  int pptr = 0;
  formal_param* y = fpl[f->NamedPosition(0)];
  named_param* x = named->Item(0);
  while (x || y) {
    int cmp;
    if (x && y) cmp = strcmp(y->Name(), x->name);
    else if (y) cmp = -1;
    else cmp = 1;

    DCASSERT(cmp<=0);  // Otherwise there is an extra named parameter!

    if (cmp<0) {
      // formal parameter but no named, use default
      DCASSERT(y->HasDefault());
      params[f->NamedPosition(fptr)] = DEFLT;
      // advance formal ptr
      fptr++;
      if (fptr < np) y = fpl[f->NamedPosition(fptr)];
      else y = NULL;
    } else {
      // Match
      if (x->UseDefault) 
        params[f->NamedPosition(fptr)] = DEFLT;
      else	
        params[f->NamedPosition(fptr)] = x->pass;
      x->pass = NULL;
      // advance named ptr
      pptr++;
      if (pptr < named->Length()) x = named->Item(pptr);
      else x = NULL;
      // advance formal ptr
      fptr++;
      if (fptr < np) y = fpl[f->NamedPosition(fptr)];
      else y = NULL;
    }
  }

  return params;
}

func_call* MatchFunctionCallNamed(PtrTable *syms, const char* n, void* list)
{
  List <named_param> * params = (List <named_param> *) list;
  if (NULL == params) return NULL;
  int i;
#ifdef COMPILE_DEBUG
  Output << "Named parameters:\n";
  for (i=0; i<params->Length(); i++) {
    Output << "\t" << params->Item(i)->name << "\n";
  }
  Output.flush();
#endif

  // find symbol table entry
  List <function> *flist = FindFunctions(syms, n);
  if (NULL==flist) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown function " << n;
    Error.Stop();
    delete params;
    return NULL;
  }

  // Find best match in symbol table
  int bestmatch = params->Length()+2;
  for (i=flist->Length()-1; i>=0; i--) {
    function *f = flist->Item(i);
    if (!f->CanUseNamedParams()) continue;
    int score;
    if (f->HasSpecialTypechecking()) {
      // can we allow named parameters here?
      // For now: no.
      continue;
    } else {
      score = ScoreFunction(f, params);
    }

#ifdef COMPILE_DEBUG
    Output << "Function " << f << " got score " << score << "\n";
    Output.flush();
#endif

    if ((score>=0) && (score<bestmatch)) {
      // better match, clear old list
      matches.Clear();
      bestmatch = score;
    }
    if (score==bestmatch) {
      // Add to list
      matches.Append(flist->Item(i));
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

  if (matches.Length()>1) {
    DCASSERT(bestmatch>0);
    Error.Start(filename, lexer.lineno());
    Error << "Multiple promotions possible for " << n;
    DumpPassed(Error, params);
    Error.Stop();
    // dump matching candidates?
    return NULL;
  }

  // Good to go; convert to positional params
  func_call *foo = FuncCallPile.NewObject();
  foo->find = matches.Item(0);
  foo->pass = NamedToPositions(foo->find, params, foo->np);
  delete params; 

  // Promote params
  bool ok;
  if (foo->find->HasSpecialParamLinking())
    ok = foo->find->LinkParams(foo->pass, foo->np);
  else
    ok = LinkFunction(foo->find, foo->pass, foo->np);

  if (!ok) {
    FuncCallPile.FreeObject(foo);
    return NULL;
  }

  return foo;
}

expr* BuildNamedFunctionCall(const char *n, void* x)
{
  func_call *foo = MatchFunctionCallNamed(&Builtins, n, x);
  if (NULL==foo) return ERROR;
  expr *fcall = MakeFunctionCall(foo->find, foo->pass, foo->np, 
  				filename, lexer.lineno());
  FuncCallPile.FreeObject(foo);
  return fcall;
}

expr* FindIdent(char* name)
{
  // Check for loop iterators
  int d;
  for (d=0; d<Iterators.Length(); d++) {
    array_index *i = Iterators.Item(d);
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

  // check model variables
  if (ModelInternal) {
    expr* f = (expr*) ModelInternal->FindName(name);
    if (f) { // found
      if (dynamic_cast<array*>(f)) {
        Error.Start(filename, lexer.lineno());
        Error << "Model identifier " << name << " is an array\n";
        Error.Stop();
        return NULL;
      }
      return Copy(f);
    }
  }

  // check model functions with no visible parameters
  // overkill, but it will work
  func_call *x = MatchFunctionCallPos(NULL, name, NULL, true);
  if (x) {
    // match!
    expr *fcall = MakeFunctionCall(x->find, x->pass, x->np, 
  				filename, lexer.lineno());
    FuncCallPile.FreeObject(x);
    return fcall;
  }

  // check constants
  variable* find = (variable*) Constants.FindName(name);
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
  array* entry = NULL;
  // Check model first
  if (ModelInternal) {
    expr* match = (expr*) ModelInternal->FindName(n);
    if (match) {
      entry = dynamic_cast<array*>(match);
      if (!entry) {
	// The model has a variable of this name, but it's not an array
	Error.Start(filename, lexer.lineno());
	Error << "Variable " << n << " is not an array";
	Error.Stop();
	delete foo;
	return ERROR;
      }
    }
  }
  // Check symbol table
  if (NULL==entry) entry = (array*) (Arrays.FindName(n));
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
  return ans;
}

// ==================================================================
// |                                                                |
// |                                                                |
// |                       Model construction                       | 
// |                                                                |
// |                                                                |
// ==================================================================

void ShowSymbolNames(void *x)
{
  PtrTable::splayitem *foo = (PtrTable::splayitem *)x;
  Output << "\t" << foo->name << "\n";
}

void StartModelTables()
{
  ModelInternal = new PtrTable();
}

void KillModelTables()
{
#ifdef DEBUG_MODEL
  Output << "Destroying symbol tables for model ";
  if (model_under_construction) Output << model_under_construction; 
  Output << "\nInternal symbols:\n";
  if (ModelInternal) ModelInternal->Traverse(ShowSymbolNames);
#endif

  delete ModelInternal;
  ModelInternal = NULL;
}

model* BuildModel(type t, char* n, void* list)
{
  if (NULL==n) return NULL;

  FormalParams = (List <formal_param> *)list;

  if (WithinBlock()) {
    Error.Start(filename, lexer.lineno());
    Error << "Model " << n << " defined within another block";
    Error.Stop();
    delete FormalParams;
    FormalParams = NULL;
    return NULL;
  }


  function *out;
  model *f = NULL;
  // Make sure this model isn't a duplicate of an existing model
  // (at least in terms of parameters)
  if (CheckFunctionDecl(&Models, t, n, out)) {
    // This is a new model.
    if (FormalParams) 
  	f = MakeNewModel(filename, lexer.lineno(), t, n,
  			FormalParams->Copy(), FormalParams->Length());
    else
  	f = MakeNewModel(filename, lexer.lineno(), t, n, NULL, 0);

    InsertFunction(&Models, f);
  } else {
    if (out) {
      // found a forward decl
      f = dynamic_cast<model*> (out);
      if (NULL==f) {
	Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
	Internal << "Bad forward definition match?";
	Internal.Stop();
      }
    } 
  }
  StartModelTables();
  return (model_under_construction = f);
}

statement* BuildModelStmt(model *m, void* block)
{
  List <statement> *sb = (List <statement>*)block;
  int num_stmts = sb->Length();
  statement** stmts = sb->MakeArray();
  int num_syms = ModelExternal.Length();
  ModelExternal.Sort();
  symbol** syms = ModelExternal.MakeArray();
  if (model_under_construction) {
    model_under_construction->SetStatementBlock(stmts, num_stmts);
    model_under_construction->SetSymbolTable(syms, num_syms);
  }
  KillModelTables();
  model_under_construction = NULL;
  return NULL;
}

void* AddModelVariable(void* list, char* ident)
{
  DCASSERT(ModelInternal);
  if (NULL==ident) return list;
  if (WithinFor()) {
    Error.Start(filename, lexer.lineno());
    Error << "Expecting array for model variable " << ident;
    Error.Stop();
    return list;
  }
  // Is the name used already?
  if (ModelInternal->ContainsName(ident)) {
    Error.Start(filename, lexer.lineno());
    Error << "Duplicate identifier " << ident << " within model";
    Error.Stop();
    return list;
  }
  // Check parameter names
  if (model_under_construction) {
    formal_param** mpl;
    int mnp, mrp;
    model_under_construction->GetParamList(mpl, mnp, mrp);
    for (mnp--; mnp>=0; mnp--) {
      if (strcmp(mpl[mnp]->Name(), ident)==0) {
	Error.Start(filename, lexer.lineno());
	Error << "Model variable " << ident << " has same name as parameter";
	Error.Stop();
	return list;
      }
    }
  }

  // Everything checks out, add to symbol table
  expr *wr = MakeEmptyWrapper(filename, lexer.lineno());
  ModelInternal->AddNamePtr(ident, wr);
  // add to list
  List <char>* foo = (List <char> *)list;
  if (NULL==foo) { foo = new List <char> (256); }
  foo->Append(ident);
  return foo;
}

void* AddModelArray(void* list, char* ident, void* indexlist)
{
  DCASSERT(WithinModel()); 
  DCASSERT(ModelInternal);
  if (NULL==ident) return list;
  if (!WithinFor()) {
    Error.Start(filename, lexer.lineno());
    Error << "Array outside of for loop for model variable " << ident;
    Error.Stop();
    return list;
  }
  // Is the name used already?
  if (ModelInternal->ContainsName(ident)) {
    Error.Start(filename, lexer.lineno());
    Error << "Duplicate identifier " << ident << " within model";
    Error.Stop();
    return list;
  }
  // Check parameter names
  if (model_under_construction) {
    formal_param** mpl;
    int mnp, mrp;
    model_under_construction->GetParamList(mpl, mnp, mrp);
    for (mnp--; mnp>=0; mnp--) {
      if (strcmp(mpl[mnp]->Name(), ident)==0) {
	Error.Start(filename, lexer.lineno());
	Error << "Model variable " << ident << " has same name as parameter";
	Error.Stop();
	return list;
      }
    }
  }

  if (!CompareIterators(ident, indexlist)) {
    // Problem with iterators, bail out
    TrashIndexList();
    return NULL;
  }
  TrashIndexList();

  // Build "copies" of iterators 
  int dim = Iterators.Length();
  array_index **il = Iterators.Copy();
  for (int i=0; i<dim; i++) {
    Copy(il[i]);  // increment counter
  }

  // Create an array of unknown type
  array *A = new array(filename, lexer.lineno(), ident, il, dim);

  // Add to symbol table 
  ModelInternal->AddNamePtr(ident, A);  

  // Add to list
  List <char>* foo = (List <char> *)list;
  if (NULL==foo) { foo = new List <char> (256); }
  foo->Append(ident);
  return foo;
}

statement* BuildModelVarStmt(type t, void* list)
{
  DCASSERT(WithinModel()); 
  if (NULL==list) return NULL;

  List <char>* foo = (List <char> *)list;
  int length = foo->Length();

  // make sure our model is not null
  if (NULL==model_under_construction) {
    delete foo;
    return NULL; 
  }
  
  // make sure the type is compatible with the model
  if (!CanDeclareType(model_under_construction->Type(0), t)) {
      Error.Start(filename, lexer.lineno());
      Error << "Cannot declare variable of type " << GetType(t);
      Error << "within " << GetType(model_under_construction->Type(0));
      Error.Stop();
      delete foo;
      return NULL; 
  }

  int i;
  if (WithinFor()) {
    // Set return type for arrays

    array** alist = new array*[length];
    for (i=0; i<length; i++) {
      expr* e = (expr*) ModelInternal->FindName(foo->Item(i));
      DCASSERT(e);
      alist[i] = dynamic_cast<array*>(e);
      DCASSERT(alist[i]);
      alist[i]->SetType(t);
    }
    delete foo;

    return MakeModelArrayStmt(model_under_construction, alist, length,
  			filename, lexer.lineno());
  } 

  // Not within for loops
  char** namelist = foo->MakeArray();
  delete foo;
  expr** ws = new expr*[length];
  for (i=0; i<length; i++) {
    ws[i] = (expr*) ModelInternal->FindName(namelist[i]);
    DCASSERT(ws[i]);
  }
  return MakeModelVarStmt(model_under_construction, t, namelist, ws, length, 
  			filename, lexer.lineno());
}

func_call* MakeModelCall(char* n)
{
  // find model n
  List <function> *flist = FindFunctions(&Models, n);
  if (NULL==flist) {
    Error.Start(filename, lexer.lineno());
    Error << "Unknown model " << n;
    Error.Stop();
    return NULL;
  }
  int i;
  matches.Clear();
  for (i=flist->Length()-1; i>=0; i--) {
    function *f = flist->Item(i);
    if (f->NumParams()==0) matches.Append(f);
  }
  if (matches.Length() == 0) {
    // no matches
    Error.Start(filename, lexer.lineno());
    Error << "No match for model " << n << " with no parameters";
    Error.Stop();
    return NULL;
  }
  if (matches.Length() > 1) {
    // This should never happen
    Internal.Start(__FILE__, __LINE__, filename, lexer.lineno());
    Internal << "Two models named " << n << " with no parameters?\n";
    Internal.Stop();
    return NULL;  // won't get here
  }

  // only one match, use it
  func_call *foo = FuncCallPile.NewObject();
  function *f = matches.Item(0);
  foo->find = dynamic_cast<model*>(f);
  foo->pass = NULL;
  foo->np = 0;
  matches.Clear();

  return foo;
}

func_call* MakeModelCallPos(char* n, void* list)
{
  return MatchFunctionCallPos(&Models, n, list, false);
}

func_call* MakeModelCallNamed(char* n, void* list)
{
  return MatchFunctionCallNamed(&Models, n, list);
}

expr* MakeMCall(func_call *fc, char* msr)
{
  if (NULL==fc) return ERROR;

  model *m = dynamic_cast<model*>(fc->find);
  if (NULL==m) {
    Internal.Start(__FILE__, __LINE__, Filename(), lexer.lineno());
    Internal << "Function within model symbol table?\n";
    Internal.Stop();
    return ERROR;
  }
  symbol *s = m->FindVisible(msr);
  if (NULL==s) {
    Error.Start(Filename(), lexer.lineno());
    Error << "Measure " << msr << " does not exist in model " << m;
    Error.Stop();
    return ERROR;
  }
  measure *thing = dynamic_cast<measure*>(s);
  if (NULL==thing) {
    // Symbol must be an array then
    Error.Start(Filename(), lexer.lineno());
    Error << "Measure " << msr << " within model " << m << " is an array";
    Error.Stop();
    return ERROR;
  }
  // Everything is great
  expr* mc = MakeMeasureCall(m, fc->pass, fc->np, thing, 
  				Filename(), lexer.lineno());

  FuncCallPile.FreeObject(fc);
  return mc;
}

expr* MakeMACall(func_call *fc, char* msr, void* list)
{
  if (NULL==fc) return ERROR;

  List <expr> *foo = (List <expr> *)list;

  model *m = dynamic_cast<model*>(fc->find);
  if (NULL==m) {
    Internal.Start(__FILE__, __LINE__, Filename(), lexer.lineno());
    Internal << "Function within model symbol table?\n";
    Internal.Stop();
    return ERROR;
  }
  symbol *s = m->FindVisible(msr);
  if (NULL==s) {
    Error.Start(Filename(), lexer.lineno());
    Error << "Measure " << msr << " does not exist in model " << m;
    Error.Stop();
    delete foo;
    return ERROR;
  }
  array *thing = dynamic_cast<array*>(s);
  if (NULL==thing) {
    // Symbol must not be an array then
    Error.Start(Filename(), lexer.lineno());
    Error << "Measure " << msr << " within model " << m << " is not an array";
    Error.Stop();
    delete foo;
    return ERROR;
  }
  // check type, dimension of indexes
  int numindx = foo->Length();
  int i;
  array_index **il;
  int dim;
  thing->GetIndexList(il, dim);
  if (numindx!=dim) {
    Error.Start(filename, lexer.lineno());
    Error << "Measure array " << msr << " has dimension " << dim;
    Error.Stop();
    delete foo;
    return ERROR;
  }
  // types
  for (i=0; i<dim; i++) {
    expr* me = foo->Item(i);
    if (!Promotable(me->Type(0), il[i]->Type(0))) {
      Error.Start(filename, lexer.lineno());
      Error << "Measure array " << msr << " expects type ";
      Error << GetType(il[i]->Type(0));
      Error << " for index " << il[i]->Name();
      Error.Stop();
      delete foo;
      return ERROR;
    }
  }

  // Ok, build the array call
  expr** indx = new expr*[dim];
  for (i=0; i<dim; i++) {
    expr* x = foo->Item(i);
    Optimize(0, x);
    indx[i] = MakeTypecast(x, il[i]->Type(0), filename, lexer.lineno());
  }
  delete foo;
  expr* answer = MakeMeasureArrayCall(m, fc->pass, fc->np, thing, indx, numindx, filename, lexer.lineno());
  return answer;
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
  FormalParams = NULL;
  converge_depth = 0;
  model_under_construction = NULL;
  ModelInternal = NULL;

  InitBuiltinFunctions(&Builtins); 
  InitBuiltinConstants(&Constants);
  InitModels();  // defined in Formalisms/api.h

#ifdef COMPILE_DEBUG
  Output << "Initialized compiler data\n";
  Output << "Builtin Functions:\n";
  Builtins.Traverse(ShowSymbols);
  Output << "ready to rock.\n";
#endif
}

