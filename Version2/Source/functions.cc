
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
  pass = NULL;
  deflt = NULL;
}

formal_param::~formal_param()
{
  Delete(deflt);
}

void formal_param::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(pass);
  x = *pass;
}

void formal_param::Sample(long &, int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(pass);
  x = *pass;
}

expr* formal_param::Substitute(int i)
{
  DCASSERT(0);   // I don't think we can ever get here.
  //
  return NULL;
}

void formal_param::show(ostream &s) const
{
  if (NULL==Name()) return;  // Don't show hidden parameters

  s << GetType(Type(0)) << " " << Name() << " := " << pass;
}

// ******************************************************************
// *                                                                *
// *                       user_func  methods                       *
// *                                                                *
// ******************************************************************

user_func::user_func(const char* fn, int line, type t, char* n, formal_param **pl, int np) : function (fn, line, t, n, pl, np, np+1)
{
  prev_stack_ptr = -1;
  return_expr = NULL;
}

user_func::~user_func()
{
  Delete(return_expr);
}

void user_func::Compute(const pos_param **pp, int np, result &x) 
{
  if (NULL==return_expr) {
    x.null = true;
    return;
  }

  // first... make sure there is enough room on the stack to save params
  if (ParamStackTop+np+3 > ParamStackSize) {
    StackOverflowPanic();
    // still alive?
    x.error = CE_StackOverflow;
    return;
  }

  int mystacktop = ParamStackTop;
  ParamStackTop += 3+np;  

  // set up our stack
  ParamStack[mystacktop  ].other  = this;  // ptr to function
  ParamStack[mystacktop+1].ivalue = prev_stack_ptr;
  prev_stack_ptr = mystacktop;
  ParamStack[mystacktop+2].ivalue = np;

  // Compute parameters, place on stack
  int i;
  for (i=0; i<np; i++) ParamStack[mystacktop+3+i].error = CE_Uncomputed;
  for (i=0; i<np; i++) {
    pp[i]->pass->Compute(0, ParamStack[mystacktop+3+i]);
    parameters[i]->SetPass(ParamStack + mystacktop + 3 + i); 
  }

  // "call" function
  return_expr->Compute(0, x);

  // Restore old parameters, if any
  prev_stack_ptr = ParamStack[mystacktop+1].ivalue;
  if (prev_stack_ptr>=0) {
    DCASSERT(ParamStack[prev_stack_ptr].other == this);
    int oldnp = ParamStack[prev_stack_ptr+2].ivalue;
    for (i=0; i<oldnp; i++) {
      parameters[i]->SetPass(ParamStack + prev_stack_ptr + 3 + i);
    }
  }
  // pop off stack
  ParamStackTop = mystacktop;
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

void internal_func::Compute(const pos_param **pp, int np, result &x)
{
  if (NULL==compute) {
    cerr << "Internal error: illegal internal function computation?\n";
    x.null = true;
    return;
  }
  compute(pp, np, x);
}

void internal_func::Sample(long &seed, const pos_param **pp, int np, result &x)
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
    int np = ParamStack[ptr+2].ivalue;
    s << "\t#parameters: " << np << "\n";
    // Eventually... display the parameters
    ptr += 3+np;
  }
}

void StackOverflowPanic()
{
  cerr << "Run-time stack overflow!\n";
  DumpRuntimeStack(cerr);
  exit(0);
}

//@}

