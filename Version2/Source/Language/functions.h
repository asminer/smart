
// $Id$

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

/** @name functions.h
    @type File
    @args \ 

  Functions with parameters.

  Derived from the base class, we have
  user-defined and internal functions.

  We also have arrays and models, but those are 
  defined elsewhere.

Thoughts: 
  Don't derive arrays from functions, do something separate.
  Don't use formal parameters for arrays, do something new.
  (That should clean up the function interface a bit.)

 */

#include "exprs.h"
#include "../list.h"

//@{
  

// ******************************************************************
// *                                                                *
// *                       formal_param class                       *
// *                                                                *
// ******************************************************************

/** Formal parameters.
    Most functionality here is for user-defined functions and arrays,
    but we also use them for models and internal functions.
 
    Note: if the name is NULL, we assume that the parameter is "hidden".

*/  

class formal_param : public symbol {
  /** Do we have a default?  
      This is necessary because the default might be NULL!
   */
  bool hasdefault;
  /// Default values (for user-defined functions)
  expr* deflt;
  /** Used for user-defined functions.
      The address of the pointer to the "current"
      spot in the run-time stack.
   */
  result** stack;
  /** Used for user-defined functions.
      The offset in the stack to use
      when computing our value.
   */
  int offset;
  void Construct();
public:
  /// Use this constructor for builtin functions
  formal_param(type t, char* n);
  /// Use this constructor for user-defined functions.
  formal_param(const char* fn, int line, type t, char* n);
  virtual ~formal_param();

  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);

  /** Used to "link" the formal params to 
      a user-defined function.
      (This initializes the stack pointer and offset.)
   */
  inline void LinkUserFunc(result** s, int o) {
    stack = s;
    offset = o;
  }
  inline void SetDefault(expr *d) { deflt = d; hasdefault = true; }
  inline bool HasDefault() const { return hasdefault; }
  inline expr* Default() const { return deflt; }
};

// ******************************************************************
// *                                                                *
// *                       named_param  class                       *
// *                                                                *
// ******************************************************************

/** Named parameters.
    These have been completely stripped-down and simplified in this version!
*/  

struct named_param {
  /// Name for the parameter.
  char* name;
  /// The expression we are passing.
  expr* pass;  
  /** Did we try to use the default?
      (Note: we can't simply set pass to NULL, because users might
       pass the null expression!)
   */
  bool UseDefault;

  // constructor for safety
  named_param() {
    name = NULL;
    pass = NULL;
    UseDefault = false;
  }

  // destructor
  ~named_param() {
    free(name);
    Delete(pass);
  }
};




// ******************************************************************
// *                                                                *
// *                         function class                         *
// *                                                                *
// ******************************************************************

/**   The base class of functions with parameters.

      Note: we've moved all compiler-related functionality
            OUT of this class to keep it simple.
*/  

class function : public symbol {
private:
  int* name_order;
  void SortParameters();
protected:
  /// Formal parameters (ordered, typed, and named).
  formal_param **parameters;
  /// The number of parameters.
  int num_params;
  /// The point where the parameters start to repeat.  
  int repeat_point;
  /** Is this forward defined?
      If so, we're allowed to re-assign its return value later.
  */
  bool isForward;
public:
  /// Use this constructor when there are repeating params.
  function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np, int rp);
  /// Use this constructor when there are no repeating params.
  function(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np);

  virtual ~function();

  /** Sets the return value.
      Used only by user functions; necessary to deal with forward defs.
  */
  virtual void SetReturn(expr *e);

  /** So that the compiler can do typechecking.
      @param	pl	List of parameters.  MUST NOT BE CHANGED.
      @param	np	Number of parameters.
      @param	rp	Repeat point for parameters. 
   */
  inline void GetParamList(formal_param** &pl, int &np, int &rp) const {
    pl = parameters;
    np = num_params;
    rp = repeat_point;
  }

  inline bool CanUseNamedParams() const { return name_order; }

  inline bool isForwardDefined() const { return isForward; }

  /** Parameters sorted by name (indexes).
      Returns the index of the ith parameter as ordered by name.
  */
  inline int NamedPosition(int i) const { 
    DCASSERT(name_order);
    return name_order[i];
  }

  inline bool ParamsRepeat() const { return repeat_point < num_params; }

  /** Overridden in derived classes.
      Should we use our own technique to check the passed parameter types?
      Usually this will be false.
      This should be true for internal functions that use any type of
      non-standard type-checking (such as "arcs" for Petri nets).
   */
  virtual bool HasSpecialTypechecking() const;

  /** Overridden in derived classes.
      Use our own technique to check passed (positional) parameters.
      (For classes that don't support this, we can simply do nothing!)
      
      @param	params	List of passed parameters (in positional order)

      @return	Score, as follows.
      		0	: Perfect match
		+n	: n parameters will need promotions
		-1	: parameters do not match in number/type
   */
  virtual int Typecheck(List <expr> *params) const;

  /** Overridden in derived classes.
      Should we use our own technique to "link" the passed parameters
      to the formal parameters?
      This is very, very rarely true.
      Sometimes it is necessary for very fancy internal functions.
   */
  virtual bool HasSpecialParamLinking() const;

  /** Promotes the passed parameters.
      Basically, this means that we are definitely going to use this 
      function, so any housekeeping type things are done here (such as 
      promoting the passed parameters, or checking random variable 
      independence).

      @param pp		Array of passed parameters.
      @param np		Number of passed parameters.

      @return 	true if there were no fatal errors.
   */
  virtual bool LinkParams(expr **pp, int np) const;

  /** Overridden in derived classes.
      Does this function have its own engine information?
      Yes for things like "avg_ss" within measures, no for everything else.
   */
  virtual bool HasEngineInformation() const;

  /** Determines solution engine, if available.
      Note the parameters may need to be evaluated to
      determine solution time (e.g., for transient).
      Should only be called if HasEngineInformation() returns true.

	@param	pp	Array of passed parameters.
	@param	np	Number of passed parameters.
	@param	e	If not NULL, address of struct to store engine data.

	@return	Engine type
  */
  virtual Engine_type GetEngineInfo(expr **pp, int np, engineinfo *e) const;


  virtual void Compute(expr **, int np, result &x) = 0;
  virtual void Sample(long &, expr **, int np, result &x) = 0;

  /** Return true if this function should NOT be documented.
      (Probably because it is a research function.)
  */
  virtual bool IsUndocumented() const;

  /** Returns NULL for user functions, documentation for internal funcs.
  */
  virtual const char* GetDocumentation() const;

  void ShowHeader(OutputStream &s) const;
};



// ******************************************************************
// *                                                                *
// *                        user_func  class                        *
// *                                                                *
// ******************************************************************

/**   Class for user-defined functions.

*/  

class user_func : public function {
private:
  result* stack_ptr;
protected:
  expr* return_expr;
public:
  user_func(const char* fn, int line, type t, char* n, 
           formal_param **pl, int np);
  virtual ~user_func();

  virtual void Compute(expr **, int np, result &x);
  virtual void Sample(long &, expr **, int np, result &x);

  virtual void SetReturn(expr *e);

  virtual void show(OutputStream &s) const;

  void FillFormal(List <formal_param>* fpl) const;
};

// ******************************************************************
// *                                                                *
// *                      internal_func  class                      *
// *                                                                *
// ******************************************************************

/** For special type checking.
    Use the following declaration:

    int MyTypecheck(List <expr> *params, int np);

 */
typedef int (*typecheck_func) (List <expr> *params);

/** For special parameter linking.
    Use the following declaration:

    bool MyParamLink(expr** p, int np);

 */
typedef bool (*link_func) (expr** p, int np);

/** For computing internal functions.
    Use the following declaration:

    void MyFunc(expr **pp, int np, result &x);

 */
typedef void (*compute_func) (expr **pp, int np, result &x);

/** For sampling internal functions.
    Use the following declaration:

    void MyFunc(long &seed, expr **pp, int np, result &x);

 */
typedef void (*sample_func) (long &seed, expr **pp, int np, result &x);

/** For determining engine information.
    Use the following declaration:

    Engine_type MyFunc(expr **pp, int np, engineinfo *e);

*/
typedef Engine_type (*engine_func) (expr **pp, int np, engineinfo *e);

/**   Class for internal functions.
      Used for internally declared functions (such as sqrt).
    
      Write a C function (of format compute_func) which computes the desired 
      result.  Pass the address of the function to the constructor.
    
      For distributions, define a similar function for creating a sample.
    
      To do still: add fancy type checking and engines.
*/  

class internal_func : public function {
protected:
  compute_func compute;
  sample_func sample;
  const char* documentation;
  bool hidedocs;
  typecheck_func typecheck;
  link_func linkparams;
  engine_func getengine;
public:
  /** Constructor.
      @param t	The type.
      @param n	The name.
      @param c	The C-function to call to compute the result.
      @param s	The C-function to call to sample the result
      @param pl	The parameter list.
      @param np	The number of formal parameters (no repetition).
      @param doc Documentation
   */
  internal_func(type t, char *n, compute_func c, sample_func s, 
                formal_param **pl, int np, const char* doc);

  /** Constructor.
      @param t	The type.
      @param n	The name.
      @param c	The C-function to call to compute the result.
      @param s	The C-function to call to sample the result
      @param pl	The parameter list.
      @param np	The number of formal parameters.
      @param rp	The point to start repeating parameters
      @param doc Documentation
   */
  internal_func(type t, char *n, compute_func c, sample_func s, 
                formal_param **pl, int np, int rp, const char* doc);

  virtual void Compute(expr **, int np, result &x);
  virtual void Sample(long &, expr **, int np, result &x);

  virtual void show(OutputStream &s) const;

  void HideDocs();
  virtual bool IsUndocumented() const;
  virtual const char* GetDocumentation() const;

  // For custom parameter checks
  void SetSpecialTypechecking(typecheck_func t);
  virtual bool HasSpecialTypechecking() const;
  virtual int Typecheck(List <expr> *params) const;

  // For custom parameter linking
  void SetSpecialParamLinking(link_func t);
  virtual bool HasSpecialParamLinking() const;
  virtual bool LinkParams(expr** p, int np) const;

  // For solution engine stuff
  void SetEngineInformation(engine_func e);
  virtual bool HasEngineInformation() const;
  virtual Engine_type GetEngineInfo(expr **pp, int np, engineinfo *e) const;
};


// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

void CreateRuntimeStack(int size);
void DestroyRuntimeStack();
/// Returns true on success.
bool ResizeRuntimeStack(int newsize);

void DumpRuntimeStack(OutputStream &s);  // Handy for run-time errors

/** Called if there is a run-time stack overflow.
    Usually this is caused by too much recursion.
    It can also be caused by extremely large input files
    with a very large number of parameters passed to user-defined functions.
 */
void StackOverflowPanic();

/** Make an expression to call a function.
    Passed parameters must match exactly in type.
    @param	f	The function to call.  Can be user-defined, 
      			internal, or pretty much anything.

    @param	p	The parameters to pass, as an array of expressions.
    @param	np	Number of passed parameters.

    @param	fn	Filename we are declared in.
    @param	line	line number we are declared on.
 */
expr* MakeFunctionCall(function *f, expr **p, int np, const char *fn, int line);

//@}

#endif

