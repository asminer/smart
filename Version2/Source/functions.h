
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

 */

#include "exprs.h"

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
  /** For passed parameters.
      This is a pointer to the proper position in the run-time stack.
   */
  result* pass;  
  /** Do we have a default?  
      This is necessary because the default might be NULL!
   */
  bool hasdefault;
  /// Default values (for user-defined functions)
  expr* deflt;
public:
  formal_param(const char* fn, int line, type t, char* n);
  virtual ~formal_param();

  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
  virtual void show(ostream &s) const;

  inline void SetPass(result *p) { pass = p; }
  inline void SetDefault(expr *d) { deflt = d; hasdefault = true; }
  inline bool HasDefault() const { return hasdefault; }
  inline expr* Default() const { return deflt; }

};

// ******************************************************************
// *                                                                *
// *                        pos_param  class                        *
// *                                                                *
// ******************************************************************

/** Positional parameters.
    These have been completely stripped-down and simplified in this version!
*/  

struct pos_param {
  /// The expression we are passing.
  expr* pass;  
  /// The file where this is happening 
  const char* filename;
  /// The line number where this is happening
  int linenumber;
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
  /// The file where this is happening 
  const char* filename;
  /// The line number where this is happening
  int linenumber;
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
protected:
  /// Formal parameters (ordered, typed, and named).
  formal_param **parameters;
  /// The number of parameters.
  int num_params;
  /** The point where the parameters start to repeat.  
      If there is no repeating, set to num_params+1.
   */
  int repeat_point;
public:
  function(const char* fn, int line, type t, char* n, 
           formal_param *pl, int np, int rp);
  virtual ~function();

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

  /// Overrided by Arrays.
  virtual bool IsArray() const { return false; }

  /** Overridden in derived classes.
      Should we use our own technique to check the passed parameter types?
      Usually this will be false.
      This should be true for internal functions that use any type of
      non-standard type-checking (such as "arcs" for Petri nets).
   */
  virtual bool HasSpecialTypechecking() const { return false; }

  /** Overridden in derived classes.
      Use our own technique to check passed (positional) parameters.
      (For classes that don't support this, we can simply do nothing!)
      
      @param	pp	Array of positional parameters
      @param	np	Number of parameters
      @param	error	Stream where any typechecking errors should be written.

      @return	true if the parameters match (or can be promoted). 
   */
  virtual bool Typecheck(const pos_param** pp, int np, ostream &error) const{}

  /** Overridden in derived classes.
      Should we use our own technique to "link" the passed parameters
      to the formal parameters?
      This is very, very rarely true.
      Sometimes it is necessary for very fancy internal functions.
   */
  virtual bool HasSpecialParamLinking() const { return false; }

  /** Promotes the passed parameters.
      Basically, this means that we are definitely going to use this 
      function, so any housekeeping type things are done here (such as 
      promoting the passed parameters, or checking random variable 
      independence).

      @param pp		Array of passed positional parameters.
      @param np		Number of passed parameters.
      @param error	Stream where any errors should be written.

      @return 	true if there were no fatal errors.

      \end{tabular}
   */
  virtual bool LinkParams(pos_param **pp, int np, ostream &error) const{}

  virtual void Compute(const pos_param **, int np, result &x) = 0;
  virtual void Sample(long &, const pos_param **, int np, result &x) = 0;
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
  int prev_stack_ptr;
protected:
  expr* return_expr;
public:
  user_func(const char* fn, int line, type t, char* n, 
           formal_param *pl, int np);
  virtual ~user_func();

  virtual void Compute(const pos_param **, int np, result &x);
  virtual void Sample(long &, const pos_param **, int np, result &x);

  inline void SetReturn(expr *e) { return_expr = e; }
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

void DumpRuntimeStack(ostream &s);  // Handy for run-time errors

/** Called if there is a run-time stack overflow.
    Usually this is caused by too much recursion.
    It can also be caused by extremely large input files
    with a very large number of parameters passed to user-defined functions.
 */
void StackOverflowPanic();

//@}

#endif

