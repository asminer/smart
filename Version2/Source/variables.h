
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
// *                         iterator class                         *
// *                                                                *
// ******************************************************************

/**   Symbols used within for loops.

      Temporarily... use simple integer for loops.
      Eventually... allow those complex sets.
*/  

class iterator : public symbol {
  /// Starting value
  int start;
  /// Stopping value
  int stop;
  /// increment
  int inc;
  /// Index (useful for arrays)
  int index;
  /// Current value
  int value;
public:
  iterator(const char *fn, int line, type t, char *n);
  virtual void Compute(int i, result &x) const;
  virtual expr* Substitute(int i);
  virtual void show(ostream &s) const;

  /** Set our value to the first item.
      Return true on success.
      (This will fail if we are iterating over an empty set.)
   */
  inline bool FirstValue() { 
    value = start; 
    if (value>stop) return false;
    index = 0; 
    return true;
  }
  /** Set our value to the next item.
      Return true on success.
      (This fails when we pass the last item.)
   */
  inline bool NextValue() { 
    value += inc;
    if (value>stop) return false;
    index++;
    return true;
  }
  /** Item number we are on.
      Used by arrays (this is the array index).
   */
  inline int Index() { return index; }

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
class constfunc : public symbol {
  // Function_State state;
  /// The return expression for the function.
protected:
  expr* return_expr;
public:
  constfunc(const char *fn, int line, type t, char *n);
  virtual ~constfunc();
  virtual expr* Substitute(int i);
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

