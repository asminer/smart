
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

formal_param::formal_param(const char* fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  hasdefault = false;
  deflt = NULL;
  stack = NULL;
  offset = 0;
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
}

void formal_param::Sample(long &, int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  x = stack[0][offset];  
}

expr* formal_param::Substitute(int i)
{
  DCASSERT(0);   // I don't think we can ever get here.
  //
  return NULL;
}

void formal_param::show(ostream &s) const
{
  s << Name();
}

// ******************************************************************
// *                                                                *
// *                        function methods                        *
// *                                                                *
// ******************************************************************

function::function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np, int rp) : symbol(fn, line, t, n)
{
  parameters = pl;
  num_params = np;
  repeat_point = rp;
}

function::~function()
{
  int i;
  for (i=0; i<num_params; i++)
    delete parameters[i];
  delete[] parameters;
}

expr* function::Substitute(int i) 
{ 
  // This should never get called.
  DCASSERT(0); 
  return NULL; 
}

bool function::HasSpecialTypechecking() const 
{ 
  return false; 
}

bool function::Typecheck(const expr** pp, int np, ostream &error) const
{
  DCASSERT(0);
  return false;
}

bool function::HasSpecialParamLinking() const 
{ 
  return false; 
}

bool function::LinkParams(expr **pp, int np, ostream &error) const
{
  DCASSERT(0);
  return false;
}

// ******************************************************************
// *                                                                *
// *                       user_func  methods                       *
// *                                                                *
// ******************************************************************

user_func::user_func(const char* fn, int line, type t, char* n, formal_param **pl, int np) : function (fn, line, t, n, pl, np, np+1)
{
  return_expr = NULL;
  stack_ptr = NULL;
  // link the parameters
  int i;
  for (i=0; i<np; i++) {
    pl[i]->LinkUserFunc(&stack_ptr, i+2);
  }
}

user_func::~user_func()
{
  Delete(return_expr);
}

void user_func::Compute(expr **pp, int np, result &x) 
{
  if (NULL==return_expr) {
    x.null = true;
    return;
  }

  // first... make sure there is enough room on the stack to save params
  if (ParamStackTop+np+2 > ParamStackSize) {
    StackOverflowPanic();
    // still alive?
    x.error = CE_StackOverflow;
    return;
  }

  int oldstacktop = ParamStackTop;  
  result* oldstackptr = stack_ptr;
  result* newstackptr = ParamStack + ParamStackTop;

  // Set up our stack
  newstackptr[0].other = this;  // ptr to function
  newstackptr[1].ivalue = np;
  ParamStackTop += 2+np;  

  // Compute parameters, place on stack
  int i;
  for (i=0; i<np; i++) newstackptr[2+i].error = CE_Uncomputed;
  for (i=0; i<np; i++) pp[i]->Compute(0, newstackptr[2+i]); 

  // "call" function
  stack_ptr = newstackptr;
  return_expr->Compute(0, x);

  // pop off stack
  ParamStackTop = oldstacktop;
  stack_ptr = oldstackptr;
}

void user_func::Sample(long &s, expr **pp, int np, result &x) 
{
  cout << "Sample not yet done.  (Copy from compute, eventually.)\n";
}

void user_func::show(ostream &s) const
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
   compute_func c, sample_func s, formal_param **pl, int np, int rp) 
 : function(NULL, -1, t, n, pl, np, rp)
{
  compute = c;
  sample = s;
}

void internal_func::Compute(expr **pp, int np, result &x)
{
  if (NULL==compute) {
    cerr << "Internal error: illegal internal function computation?\n";
    x.null = true;
    return;
  }
  compute(pp, np, x);
}

void internal_func::Sample(long &seed, expr **pp, int np, result &x)
{
  if (NULL==sample) {
    cerr << "Internal error: illegal internal function sampling?\n";
    x.null = true;
    return;
  }
  sample(seed, pp, np, x);
}

void internal_func::show(ostream &s) const
{
  if (NULL==Name()) return; // Hidden?
  s << GetType(Type(0)) << " " << Name();
}

// ******************************************************************
// *                                                                *
// *                         fcall  methods                         *
// *                                                                *
// ******************************************************************

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
  for (i=0; i < numpass; i++) delete pass[i];
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
  func->Compute(pass, numpass, x);
}

void fcall::Sample(long &s, int i, result &x)
{
  DCASSERT(0==i);
  func->Sample(s, pass, numpass, x);
}

expr* fcall::Substitute(int i)
{
  DCASSERT(0==i);
  // substitute each passed parameter...
  return NULL;
}

int fcall::GetSymbols(int i, symbol **syms, int N, int offset)
{
  DCASSERT(0==i);
  // check each passed parameter
  return 0;
}

void fcall::show(ostream &s) const
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
  ParamStack = new result[size];
  ParamStackSize = size;
  ParamStackTop = 0;
}

void DestroyRuntimeStack()
{
  DCASSERT(ParamStackTop==0);  // Otherwise we're mid function call!
  delete[] ParamStack;
  ParamStack = NULL;
}

void DumpRuntimeStack(ostream &s)
{
  s << "Error: Stack overflow\n";
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
  cerr << "Run-time stack overflow!\n";
  DumpRuntimeStack(cerr);
  exit(0);
}

//@}
