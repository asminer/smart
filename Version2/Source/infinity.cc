
// $Id$

#include "infinity.h"
//@Include: infinity.h

/** @name infinity.cc
    @type File
    @args \ 

   Implementation of type casting between simple types.

 */

//@{

// ******************************************************************
// *                                                                *
// *                      infinityconst  class                      *
// *                                                                *
// ******************************************************************

/** An infinity constant expression.
    The sign tells us if we are positive or negative infinity.
    Note the type of infinity is integer.
    (That way it can be promoted to real, if necessary.)
 */
class infinityconst : public constant {
  int sign;
  public:
  infinityconst(const char* fn, int line, int s) : constant (fn, line, INT) {
    sign = s;
  }

  virtual void Compute(int i, result &x) const {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = sign;
    x.infinity = true;
  }

  virtual void Sample(long &, int i, result &x) const {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = sign;
    x.infinity = true;
  }

  virtual void show(ostream &s) const {
    if (sign<0) s << "-";
    s << "infinity";       //<<<<<<<<<<<<<<<<<<<<<<<FIX THIS LATER!!!!!!!!!!!
  }
};

expr* MakeInfinityExpr(int sign, const char* file=NULL, int line=0)
{
  return new infinityconst(file, line, sign);
}


//@}

