
// $Id$

#include "infinity.h"
//@Include: infinity.h

/** @name infinity.cc
    @type File
    @args \ 

   Implementation of type casting between simple types.

 */

//@{

option* infinity_string;

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

  virtual void Compute(int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = sign;
    x.infinity = true;
  }

  virtual void Sample(long &, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = sign;
    x.infinity = true;
  }

  virtual void show(OutputStream &s) const {
    if (sign<0) s << "-";
    DCASSERT(infinity_string);
    s << infinity_string->GetString();
  }
};

expr* MakeInfinityExpr(int sign, const char* file, int line)
{
  return new infinityconst(file, line, sign);
}

expr* MakeConstExpr(type t, const result &x, const char* file, int line)
{
  if (x.null) return NULL;
  if (x.error) return NULL;
  if (x.infinity) return MakeInfinityExpr(x.ivalue, file, line);
  switch (t) {
    case BOOL:		return MakeConstExpr(x.bvalue, file, line);
    case INT: 		return MakeConstExpr(x.ivalue, file, line);
    case REAL:		return MakeConstExpr(x.rvalue, file, line);
    case STRING:	return MakeConstExpr((char*)x.other, file, line);
  }
  //
  Internal.Start(__FILE__, __LINE__, file, line);
  Internal << "Illegal type for MakeConstExpr";
  Internal.Stop();
  return NULL;
}

//@}

