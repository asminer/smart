
// $Id$

#include "operators.h"
//@Include: operators.h

/** @name operators.cc
    @type File
    @args \ 

   Implementation of operator classes.

 */

//@{


// ******************************************************************
// *                                                                *
// *                         addbool  class                         *
// *                                                                *
// ******************************************************************

/** Or of two boolean expressions.
 */
class addbool : public expr {
  expr *left;
  expr *right;
  public:
  addbool(const char* fn, int line, expr *l, expr *r) : expr (fn, line) {
    left = l;
    right = r;
  }

  virtual expr* Copy() const { 
    return new addbool(Filename(), Linenumber(), 
	               CopyExpr(left), CopyExpr(right));
  }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }

  virtual void Compute(int i, result &x) const;

  virtual void show(ostream &s) const {
    s << "(" << left << " | " << right << ")";
  }
};

void addbool::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  result l;
  result r;
  if (left) left->Compute(0, l);
  if (right) right->Compute(0, r);

  if (l.error) {
  }
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* SimpleUnaryOp(int op, expr *opnd);
expr* SimpleBinaryOr(expr *left, int op, expr *right);

//@}

