
// $Id$

#include "functions.h"
#include "measures.h"
#include "../Base/memtrack.h"

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
  ALLOC("formal_param", sizeof(formal_param));
  Construct();
}

formal_param::formal_param(type *t, int tlen, char* n)
  : symbol(NULL, -1, t, tlen, n)
{
  ALLOC("formal_param", sizeof(formal_param));
  Construct();
}

formal_param::formal_param(const char* fn, int line, type t, char* n)
  : symbol(fn, line, t, n)
{
  ALLOC("formal_param", sizeof(formal_param));
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
  FREE("formal_param", sizeof(formal_param));
  Delete(deflt);
}

void formal_param::Compute(int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  CopyResult(Type(i), x, stack[0][offset]);
}

void formal_param::Sample(Rng &, int i, result &x)
{
  DCASSERT(i==0);
  DCASSERT(stack);
  DCASSERT(stack[0]);   
  CopyResult(Type(i), x, stack[0][offset]);
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
  ALLOC("function", sizeof(function));
  parameters = pl;
  num_params = np;
  repeat_point = np+1;
  SortParameters();
  isInsideModel = false;
}

function::function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np, int rp) : symbol(fn, line, t, n)
{
  ALLOC("function", sizeof(function));
  parameters = pl;
  num_params = np;
  repeat_point = rp;
  name_order = NULL;
  isInsideModel = false;
}

function::~function()
{
  FREE("function", sizeof(function));
  int i;
  for (i=0; i<num_params; i++)
    delete parameters[i];
  delete[] parameters;
  delete[] name_order;
}

void function::SetReturn(expr *)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Bad function modification?\n";
  Internal.Stop();
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

bool function::HasEngineInformation() const
{
  return false;
}

Engine_type function::GetEngineInfo(expr **pp, int np, engineinfo *e) const
{
  DCASSERT(0);
  return ENG_Error;
}

int function::GetRewardParameter() const
{
  return -1;
}

void function::Compute(expr **, int, result &)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Illegal function computation";
  Internal.Stop();
}

void function::Sample(Rng &, expr **, int, result &)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Illegal function sample";
  Internal.Stop();
}

void function::Compute(const state &, expr **, int, result &)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Illegal function computation";
  Internal.Stop();
}

void function::Sample(Rng &, const state &, expr **, int, result &)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Illegal function sample";
  Internal.Stop();
}

void function::Instantiate(expr **, int, result &, const char*, int)
{
  Internal.Start(__FILE__, __LINE__);
  Internal << "Illegal model instantiaton";
  Internal.Stop();
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
  // skip first parameter if we are a model function
  for (int i= (isWithinModel()) ? 1 : 0; i<num_params; i++) {
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
  isForward = true;
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
    x.setError();
    return;
  }

  int oldstacktop = ParamStackTop;  
  result* oldstackptr = stack_ptr;
  result* newstackptr = ParamStack + ParamStackTop;

  ParamStackTop += np; 

  // Compute parameters, place on stack
  int i;
  for (i=0; i<np; i++) newstackptr[i].setError();
  for (i=0; i<np; i++) SafeCompute(pp[i], 0, newstackptr[i]); 

  // "call" function
  stack_ptr = newstackptr;
  SafeCompute(return_expr, 0, x);

  if (x.isError()) {
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

void user_func::Sample(Rng &s, expr **pp, int np, result &x) 
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
    x.setError();
    return;
  }

  int oldstacktop = ParamStackTop;  
  result* oldstackptr = stack_ptr;
  result* newstackptr = ParamStack + ParamStackTop;

  ParamStackTop += np; 

  // Compute parameters, place on stack
  int i;
  for (i=0; i<np; i++) newstackptr[i].setError();
  for (i=0; i<np; i++) SafeSample(pp[i], s, 0, newstackptr[i]); 

  // "call" function
  stack_ptr = newstackptr;
  SafeSample(return_expr, s, 0, x);

  if (x.isError()) {
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

void user_func::SetReturn(expr *e)
{
  return_expr = e; 
  isForward = false; 
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
// *                     engine_wrapper methods                     *
// *                                                                *
// ******************************************************************

engine_wrapper::engine_wrapper(type t, char *n, engine_func e, 
                formal_param **pl, int np, int rew, const char* doc)
 : function(NULL, -1, t, n, pl, np)
{
  documentation = doc;
  getengine = e;
  reward = rew;
}

void engine_wrapper::show(OutputStream &s) const
{
  if (NULL==Name()) return; // Hidden?
  s << GetType(Type(0)) << " " << Name();
}

void engine_wrapper::HideDocs()
{
  hidedocs = true;
}

bool engine_wrapper::IsUndocumented() const
{
#ifdef DEVELOPMENT_CODE
  return false;
#else
  return hidedocs;
#endif
}

const char* engine_wrapper::GetDocumentation() const
{
  return documentation;
}

bool engine_wrapper::HasEngineInformation() const
{
  return (getengine != NULL);
}

Engine_type engine_wrapper::GetEngineInfo(expr **pp, int np, engineinfo *e) const
{
  if (getengine) return getengine(pp, np, e);
  DCASSERT(0);
  return ENG_Error;
}

int engine_wrapper::GetRewardParameter() const
{
  return reward;
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
  comp_proc = NULL;
  samp_proc = NULL;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
  isForward = false;
  hidedocs = false;
}

internal_func::internal_func(type t, char *n, 
   compute_func c, sample_func s, formal_param **pl, int np, int rp, 
   const char* d) 
 : function(NULL, -1, t, n, pl, np, rp)
{
  compute = c;
  sample = s;
  comp_proc = NULL;
  samp_proc = NULL;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
  isForward = false;
  hidedocs = false;
}

internal_func::internal_func(type t, char *n, 
   compute_proc c, sample_proc s, formal_param **pl, int np, const char* d) 
 : function(NULL, -1, t, n, pl, np)
{
  compute = NULL;
  sample = NULL;
  comp_proc = c;
  samp_proc = s;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
  isForward = false;
  hidedocs = false;
}

internal_func::internal_func(type t, char *n, 
   compute_proc c, sample_proc s, formal_param **pl, int np, int rp,
   const char* d) 
 : function(NULL, -1, t, n, pl, np, rp)
{
  compute = NULL;
  sample = NULL;
  comp_proc = c;
  samp_proc = s;
  documentation = d;
  typecheck = NULL;
  linkparams = NULL;
  isForward = false;
  hidedocs = false;
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

void internal_func::Sample(Rng &seed, expr **pp, int np, result &x)
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

void internal_func::Compute(const state &s, expr **pp, int np, result &x)
{
  if (NULL==comp_proc) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Illegal internal function computation";
    Internal.Stop();
    x.setNull();
    return;
  }
  comp_proc(s, pp, np, x);
}

void internal_func::Sample(Rng &s, const state &m, expr **p, int np, result &x)
{
  if (NULL==samp_proc) {
    Internal.Start(__FILE__, __LINE__);
    Internal << "Illegal internal function sample";
    Internal.Stop();
    x.setNull();
    return;
  }
  samp_proc(s, m, p, np, x);
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
  virtual void ClearCache();
  virtual void Compute(int i, result &x);
  virtual void Sample(Rng &, int i, result &x);
  virtual void Compute(const state &, int i, result &x);
  virtual void Sample(Rng &, const state &, int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, List <symbol> *syms=NULL);
  virtual void show(OutputStream &s) const;
  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* GetRewardExpr();
  virtual expr* SplitEngines(List <measure> *mlist);
};

// ******************************************************************
// *                         fcall  methods                         *
// ******************************************************************


fcall::fcall(const char *fn, int line, function *f, expr **p, int np)
  : expr (fn, line)
{
  ALLOC("fcall", sizeof(fcall));
  func = f;
  pass = p;
  numpass = np;
}

fcall::~fcall()
{
  FREE("fcall", sizeof(fcall));
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

void fcall::ClearCache()
{
  int i;
  for (i=0; i < numpass; i++) if (pass[i]) pass[i]->ClearCache(); 
}

void fcall::Compute(int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Compute(pass, numpass, x);
}

void fcall::Sample(Rng &s, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Sample(s, pass, numpass, x);
}

void fcall::Compute(const state &m, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Compute(m, pass, numpass, x);
}

void fcall::Sample(Rng &s, const state &m, int i, result &x)
{
  DCASSERT(0==i);
  DCASSERT(func);
  func->Sample(s, m, pass, numpass, x);
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

int fcall::GetSymbols(int i, List <symbol> *syms)
{
  DCASSERT(0==i);
  int n;
  int answer = 0;
  for (n=0; n<numpass; n++) {
    answer += pass[n]->GetSymbols(0, syms);
  }
  return answer;
}

void fcall::show(OutputStream &s) const
{
  if (func->Name()==NULL) return;  // Hidden?
  s << func->Name();
  int start = (func->isWithinModel()) ? 1 : 0;
  if (numpass <= start) return;
  s << "(";
  int i;
  for (i=start; i<numpass; i++) {
    if (i>start) s << ", ";
    s << pass[i];
  }
  s << ")";
}

Engine_type fcall::GetEngine(engineinfo *e)
{
  if (func->HasEngineInformation()) 
    return func->GetEngineInfo(pass, numpass, e);

  // "ordinary" function, engine depends only on params
  Engine_type foo = ENG_None;
  int i;
  for (i=0; i<numpass; i++) {
    DCASSERT(pass[i]);
    Engine_type bar = pass[i]->GetEngine(NULL);
    if (ENG_Error == bar) {
      if (e) e->engine = ENG_Error;
      return ENG_Error;
    }
    if (bar != ENG_None) foo = ENG_Mixed;
  }
  if (e) e->engine = foo;
  return foo;
}

expr* fcall::GetRewardExpr()
{
  int rw = func->GetRewardParameter();
  if (rw<0) return ERROR;
  DCASSERT(rw<numpass);
  return Copy(pass[rw]);
}

expr* fcall::SplitEngines(List <measure> *mlist)
{
  if (GetEngine(NULL) != ENG_Mixed) return Copy(this);

  expr** newpass = new expr* [numpass];
  int i;
  for (i=0; i<numpass; i++) {
    DCASSERT(pass[i]);
    Engine_type et = pass[i]->GetEngine(NULL);
    measure *m;
    switch (et) {
      case ENG_None:
    	newpass[i] = Copy(pass[i]);
	break;

      case ENG_Mixed: 
    	newpass[i] = pass[i]->SplitEngines(mlist);
	break;

      default:  // A "leaf"
     	m = new measure(pass[i]->Filename(), pass[i]->Linenumber(), 
			pass[i]->Type(0), NULL);
	m->SetReturn(Copy(pass[i]));
	mlist->Append(m);
	newpass[i] = m;
    } // switch
  } // for i
  return new fcall(Filename(), Linenumber(), func, newpass, numpass);
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

