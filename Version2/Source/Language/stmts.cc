
// $Id$

#include "stmts.h"

//@Include: stmts.h

/** @name stmts.cc
    @type File
    @args \ 

   Implementation basic statement classes.

 */

//@{

// ******************************************************************
// *                                                                *
// *                         exprstmt class                         *
// *                                                                *
// ******************************************************************

/** An expression statement.
    Used for function calls, like "print".
 */
class exprstmt : public statement {
  expr *x;
public:
  exprstmt(const char* fn, int line, expr *e);
  virtual ~exprstmt();
  virtual void Execute();
  virtual void show(ostream &s);
  virtual void showfancy(int depth, ostream &s);
};

exprstmt::exprstmt(const char* fn, int line, expr *e) : statement(fn,line)
{
  x = e;
}

exprstmt::~exprstmt()
{
  Delete(x);
}

void exprstmt::Execute()
{
  // this should be eliminated at compile time
  DCASSERT(x);

  result dummy;
  x->Compute(0, dummy);

  // hmmm... do we check for errors here?
}

void exprstmt::show(ostream &s)
{
  s << x;
}

void exprstmt::showfancy(int depth, ostream &s)
{
  int i;
  for (i=depth; i; i--) s << " ";
  s << x << ";\n";
}

// ******************************************************************
// *                                                                *
// *              Global functions to build statements              *
// *                                                                *
// ******************************************************************

statement* MakeExprStatement(expr* e, const char* file, int line)
{
  if (NULL==e) return NULL;
  DCASSERT(e->Type(0)==VOID);
  return NULL;
}
  
//@}
