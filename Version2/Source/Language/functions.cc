
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
}

void formal_param::Sample(long &, int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  x = stack[0][offset];  
}

// ******************************************************************
// *                                                                *
// *                        function methods                        *
// *                                                                *
// ******************************************************************

function::function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np) : symbol(fn, line, t, n)
{
  parameters = pl;
  num_params = np;
  repeat_point = np+1;
}

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
   compute_func c, sample_func s, formal_param **pl, int np, const char* d) 
 : function(NULL, -1, t, n, pl, np)
{
  compute = c;
  sample = s;
  documentation = d;
}

internal_func::internal_func(type t, char *n, 
   compute_func c, sample_func s, formal_param **pl, int np, int rp, 
   const char* d) 
 : function(NULL, -1, t, n, pl, np, rp)
{
  compute = c;
  sample = s;
  documentation = d;
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
  virtual void show(ostream &s) const;
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

expr* MakeFunctionCall(function *f, expr **p, int np, const char*fn, int line)
{
  return new fcall(fn, line, f, p, np);
}

//@}
