
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
    For-loop iterators
    User-defined functions with no parameters

 */

#include "exprs.h"

//@{
  




// ******************************************************************
// *                                                                *
// *                        constfunc  class                        *
// *                                                                *
// ******************************************************************


/** Constant functions (with no parameters).
    These are used often as building blocks for more complex items.
    So, some functionality here is provided for derived classes.
 */
class constfunc : public symbol {
  // Function_State state;
  /// The return expression for the function.
protected:
  expr* return_expr;
public:
  constfunc(const char *fn, int line, type t, char *n);
  virtual ~constfunc();
  virtual void show(ostream &s) const;

  inline void SetReturn(expr *e) { return_expr = e; }
};

// ******************************************************************
// *                                                                *
// *                        determfunc class                        *
// *                                                                *
// ******************************************************************


/** Constant functions that are true constants.
    Thus, eventually, the value will be computed,
    and once it has, we can trash the return expression.
 */
class determfunc : public constfunc {
  bool computed_already;
  result value;
public:
  determfunc(const char *fn, int line, type t, char *n);
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************


//@}

#endif

