
// $Id$

#include "functions.h"
//@Include: functions.h

/** @name functions.cc
    @type File
    @args \ 

   Implementation of functions with parameters.

 */

//@{

/// The Run-time stack
result* ParamStack;
/// Top of the run-time stack
int ParamStackTop;
/// Size of the Run-time stack
int ParamStackSize;

// ******************************************************************
// *                                                                *
// *                      formal_param methods                      *
// *                                                                *
// ******************************************************************

formal_param::formal_param(type t, char* n)
  : symbol(NULL, -1, t, n)
{
  Construct();
}

formal_param::formal_param(const char* fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  Construct();
}

void formal_param::Construct()
{
  hasdefault = false;
  deflt = NULL;
  stack = NULL;
  offset = 0;
  // SetSubstitution(false);   // Not sure about this yet
}

formal_param::~formal_param()
{
  Delete(deflt);
}

void formal_param::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  x = stack[0][offset];  
  x.notFreeable();    // this is a shallow copy
}

void formal_param::Sample(long &, int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  x = stack[0][offset];  
  x.notFreeable();
}

// ******************************************************************
// *                                                                *
// *                        function methods                        *
// *                                                                *
// ******************************************************************

void function::SortParameters()
{
  name_order = NULL;
  if (NULL==parameters) return;
  // First: check that all formal parameters have names
  int i;
  for (i=0; i<num_params; i++) {
    DCASSERT(parameters[i]);
    if (NULL==parameters[i]->Name()) return;
  }
  // Selection sort parameters by name
  name_order = new int[num_params];
  for (i=0; i<num_params; i++) name_order[i] = i;
  for (i=0; i<num_params-1; i++) {
    // find smallest in remaining
    int min = i;
    const char* minname = parameters[name_order[i]]->Name();
    for (int j=i+1; j<num_params; j++) {
      int cmp = strcmp(parameters[name_order[j]]->Name(), minname);
      if (0==cmp) {
	Internal.Start(__FILE__, __LINE__);
	Internal << "Function " << Name();
	Internal << " has two parameters named " << minname;
	Internal.Stop();
      }
      if (cmp<0) { // new smallest
        min = j;
	minname = parameters[name_order[j]]->Name();
      }
    }
    if (min!=i) SWAP(name_order[i], name_order[min]);
  }
/*
  Output << "Done sorting parameters:\n";
  for (i=0; i<num_params; i++) {
    Output << "\t" << parameters[name_order[i]] << "\n";
  }
  Output.flush();
*/
}

function::function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np) : symbol(fn, line, t, n)
{
  parameters = pl;
  num_params = np;
  repeat_point = np+1;
  SortParameters();
}

function::function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np, int rp) : symbol(fn, line, t, n)
{
  parameters = pl;
  num_params = np;
  repeat_point = rp;
  name_order = NULL;
}

function::~function()
{
  int i;
  for (i=0; i<num_params; i++)
    delete parameters[i];
  delete[] parameters;
  delete[] name_order;
}

bool function::HasSpecialTypechecking() const 
{ 
  return false; 
}

int function::Typecheck(List <expr> *) const
{
  DCASSERT(0);
  return 0;
}

bool function::HasSpecialParamLinking() const 
{ 
  return false; 
}

bool function::LinkParams(expr **pp, int np) const
{
  DCASSERT(0);
  return false;
}

bool function::IsUndocumented() const
{
  // default
  return false;
}

const char* function::GetDocumentation() const 
{
  // default...
  return NULL;
}

void function::ShowHeader(OutputStream &s) const
{
  s << GetType(Type(0)) << " " << Name();
  if (num_params<1) return;
  s << "(";
  for (int i=0; i<num_params; i++) {
    if (repeat_point==i) s << "...";
    if (NULL==parameters[i]) s << "null";
    else {
      // print type of parameter i
      for (int j=0; j<parameters[i]->NumComponents(); j++) {
	if (j) s << ":";
	s << GetType(parameters[i]->Type(j));
      }
      s << " " << parameters[i];
      if (parameters[i]->HasDefault()) {
	s << ":=" << parameters[i]->Default();
      }
    }
    if (i<num_params-1) s << ", ";
  }
  if (repeat_point<=num_params) s << ",...";
  s << ")";
}

// ******************************************************************
// *                                                                *
// *                       user_func  methods                       *
// *                                                                *
// ******************************************************************

user_func::user_func(const char* fn, int line, type t, char* n, formal_param **pl, int np) : function (fn, line, t, n, pl, np)
{
  return_expr = NULL;
  stack_ptr = NULL;
  // link the parameters
  int i;
  for (i=0; i<np; i++) {
    pl[i]->LinkUserFunc(&stack_ptr, i);
  }
}

user_func::~user_func()
{
  Delete(return_expr);
}

void user_func::Compute(expr **pp, int np, result &x) 
{
  if (NULL==return_expr) {
    x.setNull();
    return;
  }

  // first... make sure there is enough room on the stack to save params
  if (ParamStackTop+np > ParamStackSize) {
    Error.Start(Filename(), Linenumber());
    Error << "Stack overflow in function call " << Name();
    Error.Stop();
    x.error = CE_StackOverflow;
    return;
  }

  int oldstacktop = ParamStackTop;  
  result* oldstackptr = stack_ptr;
  result* newstackptr = ParamStack + ParamStackTop;

  ParamStackTop += np; 

  // Compute parameters, place on stack
  int i;
  for (i=0; i<np; i++) newstackptr[i].error = CE_Uncomputed;
  for (i=0; i<np; i++) SafeCompute(pp[i], 0, newstackptr[i]); 

  // "call" function
  stack_ptr = newstackptr;
  SafeCompute(return_expr, 0, x);

  if (x.error) {
    // check option?
    Error.Continue(pp[0]->Filename(), pp[0]->Linenumber());
    Error << "function call " << Name();
    if (np) Error << "(";
    for (i=0; i<np; i++) {
      if (i) Error << ", ";
      Error << parameters[i] << "=";
      PrintResult(Error, parameters[i]->Type(0), newstackptr[i]);
    }
    if (np) Error << ")";
    Error.Stop();
  }

  // free parameters, in case they're strings or other bulky items
  for (i=0; i<np; i++) DeleteResult(pp[i]->Type(0), newstackptr[i]);

  // pop off stack
  ParamStackTop = oldstacktop;
  stack_ptr = oldstackptr;
}

void user_func::Sample(long &s, expr **pp, int np, result &x) 
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Sample not yet done.  (Copy from compute, eventually.)\n";
  Internal.Stop();
}

void user_func::show(OutputStream &s) const
{
  DCASSERT(Name());
  s << GetType(Type(0)) << " " << Name() << "(";
  int i;
  for (i=0; i<num_params; i++) {
    if (i) s << ", ";
    if (parameters[i]->Name()) {
      s << GetType(parameters[i]->Type(0)) << " " << parameters[i];
      if (parameters[i]->HasDefault()) {
	s << " := " << parameters[i]->Default();
      }
    }
  }
  s << ") := " << return_expr;
}

// ******************************************************************
// *                                                                *
// *                     internal_func  methods                     *
// *                                                                *
// ******************************************************************

internal_func::internal_func(type t, char *n, 
   compute_func c, sample_func s, formal_param **pl, int np, const char* d) 
 : function(NULL, -1, t, n, pl, np)
{
  compute = c;
  sample = s;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
}

internal_func::internal_func(type t, char *n, 
   compute_func c, sample_func s, formal_param **pl, int np, int rp, 
   const char* d) 
 : function(NULL, -1, t, n, pl, np, rp)
{
  compute = c;
  sample = s;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
}

void internal_func::Compute(expr **pp, int np, result &x)
{
  if (NULL==compute) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Illegal internal function computation";
    Internal.Stop();
    x.setNull();
    return;
  }
  compute(pp, np, x);
}

void internal_func::Sample(long &seed, expr **pp, int np, result &x)
{
  if (NULL==sample) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Illegal internal function sample";
    Internal.Stop();
    x.setNull();
    return;
  }
  sample(seed, pp, np, x);
}

void internal_func::show(OutputStream &s) const
{
  if (NULL==Name()) return; // Hidden?
  s << GetType(Type(0)) << " " << Name();
}

void internal_func::HideDocs()
{
  hidedocs = true;
}

bool internal_func::IsUndocumented() const
{
#ifdef DEVELOPMENT_CODE
  return false;
#else
  return hidedocs;
#endif
}

const char* internal_func::GetDocumentation() const
{
  return documentation;
}

void internal_func::SetSpecialTypechecking(typecheck_func t) 
{
  typecheck = t;
}

bool internal_func::HasSpecialTypechecking() const
{
  return (typecheck!=NULL);
}

int internal_func::Typecheck(List <expr> *params) const
{
  if (NULL==typecheck) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Bad use of custom type checking";
    Internal.Stop();
  }
  return typecheck(params);
}

void internal_func::SetSpecialParamLinking(link_func t)
{
  linkparams = t;
}

bool internal_func::HasSpecialParamLinking() const
{
  return (linkparams!=NULL);
}

bool internal_func::LinkParams(expr** p, int np) const
{
  if (NULL==linkparams) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Bad use of custom parameter linking";
    Internal.Stop();
  }
  return linkparams(p, np);
}

// ******************************************************************
// *                                                                *
// *                          fcall  class                          *
// *                                                                *
// ******************************************************************

/**  An expression used to compute a function call.
 */

class fcall : public expr {
protected:
  function *func;
  expr **pass;
  int numpass;
public:
  fcall(const char *fn, int line, function *f, expr **p, int np);
  virtual ~fcall();
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual void show(OutputStream &s) const;
};

// fcall  methods

fcall::fcall(const char *fn, int line, function *f, expr **p, int np)
  : expr (fn, line)
{
  func = f;
  pass = p;
  numpass = np;
}

fcall::~fcall()
{
  // don't delete func
  int i;
  for (i=0; i < numpass; i++) Delete(pass[i]);
  delete[] pass;
}

type fcall::Type(int i) const
{
  DCASSERT(0==i);
  return func->Type(0);
}

void fcall::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Compute(pass, numpass, x);
}

void fcall::Sample(long &s, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Sample(s, pass, numpass, x);
}

expr* fcall::Substitute(int i)
{
  DCASSERT(0==i);
  if (0==numpass) return Copy(this);

  // substitute each passed parameter...
  expr** pass2 = new expr*[numpass];
  int n;
  for (n=0; n<numpass; n++) {
    pass2[n] = pass[n]->Substitute(0);
  }
  return new fcall(Filename(), Linenumber(), func, pass2, numpass);
}

int fcall::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(0==i);
  // check each passed parameter
  return 0;
}

void fcall::show(OutputStream &s) const
{
  if (func->Name()==NULL) return;  // Hidden?
  s << func->Name();
  if (0==numpass) return;
  s << "(";
  int i;
  for (i=0; i<numpass; i++) {
    if (i) s << ", ";
    s << pass[i];
  }
  s << ")";
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

void CreateRuntimeStack(int size)
{
  ParamStack = (result*) malloc(size * sizeof(result));
  ParamStackSize = size;
  ParamStackTop = 0;
}

void DestroyRuntimeStack()
{
  DCASSERT(ParamStackTop==0);  // Otherwise we're mid function call!
  free(ParamStack);
  ParamStack = NULL;
  ParamStackSize = 0;
}

bool ResizeRuntimeStack(int newsize)
{
  DCASSERT(ParamStackTop<newsize);
  if (0==newsize) {
    DestroyRuntimeStack();
    return true;
  }
  void *foo = realloc(ParamStack, newsize * sizeof(result));
  if (NULL==foo) return false; // there was a problem.
  ParamStackSize = newsize;
  ParamStack = (result*) foo;
  return true;
}

/*

void DumpRuntimeStack(OutputStream &s)
{
  s << "Stack dump:\n";
  int ptr = 0;
  while (ptr<ParamStackTop) {
    function *f = (function*) ParamStack[ptr].other; 
    s << "\t" << f << "\n";
    int np = ParamStack[ptr+1].ivalue;
    s << "\t#parameters: " << np << "\n";
    // Eventually... display the parameters
    ptr += 2+np;
  }
}

void StackOverflowPanic()
{
  Error.Start(NULL, -1);
  Error << "Run-time stack overflow!\n";
  DumpRuntimeStack(Error);
}

*/

expr* MakeFunctionCall(function *f, expr **p, int np, const char*fn, int line)
{
  return new fcall(fn, line, f, p, np);
}

//@}

