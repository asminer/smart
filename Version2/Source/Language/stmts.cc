
// $Id$

#include "stmts.h"

//@Include: stmts.h

/** @name stmts.cc
    @type File
    @args \ 

   Implementation basic statement classes.

 */

//@{

#define ALLOW_NON_VOID_EXPRSTMT

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
  virtual void show(OutputStream &s) const;
  virtual void showfancy(int depth, OutputStream &s) const;
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

#ifdef ALLOW_NON_VOID_EXPRSTMT
  if (VOID == x->Type(0)) return;

  Output << "Evaluated statement " << x << ", got:";
  switch (x->Type(0)) {
    case BOOL:
    	Output << dummy.bvalue;
	break;
    case INT:
    	Output << dummy.ivalue;
	break;
    case REAL:
    	Output << dummy.rvalue;
	break;
    default:
    	Output << "(some value)";
	break;
  }
  Output << "\n";
  Output.flush();
#endif
}

void exprstmt::show(OutputStream &s) const
{
  s << x;
}

void exprstmt::showfancy(int depth, OutputStream &s) const
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
#ifndef ALLOW_NON_VOID_EXPRSTMT
  DCASSERT(e->Type(0)==VOID);
#endif
  return new exprstmt(file, line, e);
}
  
//@}
