
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
  virtual void show(OutputStream &s) const;
  virtual void ShowHeader(OutputStream &s) const;

  inline void SetReturn(expr *e) { return_expr = e; }
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
*/
constfunc* MakeConstant(type t, char* id, const char* file, int line);

//@}

#endif

