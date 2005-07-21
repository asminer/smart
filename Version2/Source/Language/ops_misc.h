
// $Id$

#include "baseops.h"


/** @name ops_misc.h
    @type File
    @args \ 

   Operator classes, for other types.

 */

//@{

//#define DEBUG_DEEP


// ******************************************************************
// *                                                                *
// *                       Operators for void                       *
// *                                                                *
// *         Yes, there is one, for "sequencing", i.e., ";"         *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                         void_seq class                         *
// ******************************************************************

/** Sequence of void expressions, separated by semicolon.
 */
class void_seq : public assoc {
public:
  void_seq(const char* fn, int ln, expr** x, int n) : assoc(fn, ln, x, n) { }
  virtual void show(OutputStream &s) const {
    assoc_show(s, ";");
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return VOID;
  }
  virtual void Compute(compute_data &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new void_seq(Filename(), Linenumber(), x, n);
  }
};


// ******************************************************************
// *                                                                *
// *                    Operators for proc_state                    *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                      proc_state_seq class                      *
// ******************************************************************

/** Sequence of state changes, separated by semicolon.
    Want to consider this as a list of products
    (for "kronecker consistency" and such)
    so it is derived from class "multop".
 */
class proc_state_seq : public multop {
public:
  proc_state_seq(const char* fn, int ln, expr** x, int n) : multop(fn, ln, x, n) { }
  virtual void show(OutputStream &s) const {
    assoc_show(s, ";");
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_STATE;
  }
  virtual void NextState(const state &c, state &n, result &x);
protected:
  virtual expr* MakeAnother(expr **x, int n) {
    return new proc_state_seq(Filename(), Linenumber(), x, n);
  }
};





//@}

