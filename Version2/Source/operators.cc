
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
// *                         bool_not class                         *
// *                                                                *
// ******************************************************************

/** Negation of a boolean expression.
 */
class bool_not : public expr {
  expr *opnd;
  public:
  bool_not(const char* fn, int line, expr *x) : expr (fn, line) {
    opnd = x;
  }
  virtual ~bool_not() {
    delete opnd;
  }
  virtual expr* Copy() const { 
    return new bool_not(Filename(), Linenumber(), CopyExpr(opnd));
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x) const;
  virtual void show(ostream &s) const {
    s << "!(" << opnd << ")";
  }
};

void bool_not::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  if (opnd) opnd->Compute(0, x); else x.null = true;

  // Trace errors?
  if (x.error) return;
  if (x.null) return;

  x.bvalue = !x.bvalue;
}

// ******************************************************************
// *                                                                *
// *                         bool_or  class                         *
// *                                                                *
// ******************************************************************

/** Or of two boolean expressions.
 */
class bool_or : public expr {
  expr *left;
  expr *right;
  public:
  bool_or(const char* fn, int line, expr *l, expr *r) : expr (fn, line) {
    left = l;
    right = r;
  }
  virtual ~bool_or() {
    delete left;
    delete right;
  }
  virtual expr* Copy() const { 
    return new bool_or(Filename(), Linenumber(), 
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

void bool_or::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  result l;
  result r;
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

  if (l.error) {
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
    return;
  }
  x.bvalue = l.bvalue || r.bvalue;
}

// ******************************************************************
// *                                                                *
// *                         bool_and class                         *
// *                                                                *
// ******************************************************************

/** And of two boolean expressions.
 */
class bool_and : public expr {
  expr *left;
  expr *right;
  public:
  bool_and(const char* fn, int line, expr *l, expr *r) : expr (fn, line) {
    left = l;
    right = r;
  }
  virtual ~bool_and() {
    delete left;
    delete right;
  }
  virtual expr* Copy() const { 
    return new bool_and(Filename(), Linenumber(), 
	               CopyExpr(left), CopyExpr(right));
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }
  virtual void Compute(int i, result &x) const;
  virtual void show(ostream &s) const {
    s << "(" << left << " & " << right << ")";
  }
};

void bool_and::Compute(int i, result &x) const
{
  DCASSERT(0==i);
  result l;
  result r;
  if (left) left->Compute(0, l); else l.null = true;
  if (right) right->Compute(0, r); else r.null = true;

  if (l.error) {
    // some option about error tracing here, I guess...
    x.error = l.error;
    return;
  }
  if (r.error) {
    x.error = r.error;
    return;
  }
  if (l.null || r.null) {
    x.null = true;
    return;
  }
  x.bvalue = l.bvalue && r.bvalue;
}

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* SimpleUnaryOp(int op, expr *opnd);
expr* SimpleBinaryOr(expr *left, int op, expr *right);

//@}

