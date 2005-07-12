
// $Id$

#ifndef VARIABLES_H
#define VARIABLES_H

/** @name variables.h
    @type File
    @args \ 

    Basic symbols that don't require parameters.

    Ok, so "variable" isn't exactly the most accurate
    description, but it works.

    Classes defined here:
    User-defined functions with no parameters
    Measures

 */

#include "exprs.h"

//@{

/// Possible states of a constant
enum const_status {
  /// No type yet! 
  CS_Untyped,
  /// Not yet defined
  CS_Undefined,
  /// Given an initial guess
  CS_HasGuess,
  /// Has a return expression
  CS_Defined,
  /// Has been computed (for deterministic)
  CS_Computed
};
  
// ******************************************************************
// *                                                                *
// *                         variable class                         *
// *                                                                *
// ******************************************************************


/** Variables.
    An abstract base class that includes constfunc and cvgfunc.
 */
class variable : public symbol {
public:
  /// State; used by compiler.
  const_status status;
public:
  variable(const char *fn, int line, type t, char *n);
  virtual ~variable();
  virtual void show(OutputStream &s) const;
};

// ******************************************************************
// *                                                                *
// *                        constfunc  class                        *
// *                                                                *
// ******************************************************************


/** Constant functions (with no parameters).
    These are used often as building blocks for more complex items.
    So, some functionality here is provided for derived classes.
 */
class constfunc : public variable {
protected:
  /// The return expression for the function.
  expr* return_expr;
  /// Do we have a cached value?
  bool have_cached;
  /// The cached value
  result cache; 
public:
  constfunc(const char *fn, int line, type t, char *n);
  virtual ~constfunc();
  virtual void ShowHeader(OutputStream &s) const;

  virtual void ClearCache();
  virtual void Compute(int i, result &x);
  virtual void Sample(Rng &, int i, result &x);

  inline void SetReturn(expr *e) { 
    status = CS_Defined;
    return_expr = e; 
  }

  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *mlist);
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

/** Make a constant function.
    For functions within a converge, use something else.  
    Never use this within a model.
*/
constfunc* MakeConstant(type t, char* id, const char* file, int line);


//@}

#endif

