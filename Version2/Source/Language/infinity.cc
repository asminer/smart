
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

  virtual void Compute(Rng *, const state *, int i, result &x) {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = sign;
    x.setInfinity();
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
  if (x.isNull()) return NULL;
  if (x.isError()) return NULL;
  if (x.isInfinity()) return MakeInfinityExpr(x.ivalue, file, line);
  switch (t) {
    case BOOL:		
    case RAND_BOOL:		
    case PROC_BOOL:		
    case PROC_RAND_BOOL:		
			return MakeConstExpr(t, x.bvalue, file, line);

    case INT: 		
    case RAND_INT: 		
    case PROC_INT: 		
    case PROC_RAND_INT: 		
			return MakeConstExpr(t, x.ivalue, file, line);

    case REAL:		
    case RAND_REAL:		
    case PROC_REAL:		
    case PROC_RAND_REAL:		
			return MakeConstExpr(t, x.rvalue, file, line);

    case STRING:	return MakeConstExpr((char*)x.other, file, line);
  }
  //
  Internal.Start(__FILE__, __LINE__, file, line);
  Internal << "Illegal type for MakeConstExpr";
  Internal.Stop();
  return NULL;
}

//@}

