
// $Id$

#include "converge.h"
//@Include: converge.h

/** @name converge.cc
    @type File
    @args \ 

   Implementation of all things converge related.

 */

//@{

// ******************************************************************
// *                                                                *
// *                       convergefunc class                       *
// *                                                                *
// ******************************************************************

/** Real variables within a converge.
    Members are public because they're used by converge statements.
 */
class convergefunc : public constfunc {
public:
  bool is_fixed;
  result current;
public:
  convergefunc(const char *fn, int line, type t, char *n);
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
  virtual void ShowHeader(OutputStream &s) const;
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

cvgfunc* MakeConvergeVar(type t, char* id, const char* file, int line)
{
  DCASSERT(t==REAL);

  // Do something here
  return NULL;
}


//@}


