
// $Id$

#include "ops_extra.h"

#ifndef OPS_EXTRA_H
#define OPS_EXTRA_H

#include "exprs.h"


/** @name ops_extra.cc
    @type File
    @args \ 

   Special, internally-used operator classes.

   The necessary backend of ops_const.h
   Classes are defined here.

 */

//@{



// ******************************************************************
// *                       int_eq_expr  class                       *
// ******************************************************************

/** INT equal to PROC_INT
*/
class int_eq_expr : public unary {
  int leftside;
public:
  int_eq_expr(const char* fn, int line, int lhs, expr* x) : unary (fn, line, x) {
    leftside = lhs;
  }
  virtual void show(OutputStream &s) const {
    s << leftside << '=' << opnd;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &s, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new int_eq_expr(Filename(), Linenumber(), leftside, x);
  }
};

void int_eq_expr::Compute(const state &s, int i, result &x)
{
  DCASSERT(0==i);
  SafeCompute(opnd, s, 0, x); 
  if (x.isNormal()) {
    x.bvalue = leftside == x.ivalue;
    return;
  }
  if (x.isInfinity()) {
    x.bvalue = false;
    x.Clear();
  }
  // unknown or other error, let it propogate
}

// ******************************************************************
// *                       int_gt_expr  class                       *
// ******************************************************************

/** INT greater than PROC_INT
*/
class int_gt_expr : public unary {
  int leftside;
public:
  int_gt_expr(const char* fn, int line, int lhs, expr* x) : unary (fn, line, x) {
    leftside = lhs;
  }
  virtual void show(OutputStream &s) const {
    s << leftside << '>' << opnd;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &s, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new int_gt_expr(Filename(), Linenumber(), leftside, x);
  }
};

void int_gt_expr::Compute(const state &s, int i, result &x)
{
  DCASSERT(0==i);
  SafeCompute(opnd, s, 0, x); 
  if (x.isNormal()) {
    x.bvalue = leftside > x.ivalue;
    return;
  }
  if (x.isInfinity()) {
    x.bvalue = (x.ivalue<0);  // true if -infinity, false otherwise
    x.Clear();
  }
  // unknown or other error, let it propogate
}

// ******************************************************************
// *                       int_le_expr  class                       *
// ******************************************************************

/** INT less or equal to PROC_INT
*/
class int_le_expr : public unary {
  int leftside;
public:
  int_le_expr(const char* fn, int line, int lhs, expr* x) : unary (fn, line, x) {
    leftside = lhs;
  }
  virtual void show(OutputStream &s) const {
    s << leftside << "<=" << opnd;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &s, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new int_le_expr(Filename(), Linenumber(), leftside, x);
  }
};

void int_le_expr::Compute(const state &s, int i, result &x)
{
  DCASSERT(0==i);
  SafeCompute(opnd, s, 0, x); 
  if (x.isNormal()) {
    x.bvalue = leftside <= x.ivalue;
    return;
  }
  if (x.isInfinity()) {
    x.bvalue = (x.ivalue>0);  // true if +infinity, false otherwise
    x.Clear();
  }
  // unknown or other error, let it propogate
}

// ******************************************************************
// *                       expr_between class                       *
// ******************************************************************

/** INT <= PROC_INT <= INT
*/
class expr_between : public unary {
  int lower;
  int upper;
public:
  expr_between(const char* fn, int ln, int L, expr*x, int U) : unary (fn, ln, x) {
    lower = L;
    upper = U;
  }
  virtual void show(OutputStream &s) const {
    s << lower << "<=" << opnd << "<=" << upper;
  }
  virtual type Type(int i) const {
    DCASSERT(0==i);
    return PROC_BOOL;
  }
  virtual void Compute(const state &s, int i, result &x);
protected:
  virtual expr* MakeAnother(expr *x) {
    return new expr_between(Filename(), Linenumber(), lower, x, upper);
  }
};

void expr_between::Compute(const state &s, int i, result &x)
{
  DCASSERT(0==i);
  SafeCompute(opnd, s, 0, x); 
  if (x.isNormal()) {
    x.bvalue = (lower <= x.ivalue) && (x.ivalue <= upper);
    return;
  }
  if (x.isInfinity()) {
    x.bvalue = false; 
    x.Clear();
  }
  // unknown or other error, let it propogate
}


// ******************************************************************
// *                                                                *
// *                        Global frontends                        *
// *                                                                *
// ******************************************************************

expr* MakeConstCompare(int left, int op, expr* right, const char* f, int ln)
{
  DCASSERT(right);
  DCASSERT(right->Type(0)==PROC_INT);
  switch (op) {
    case EQ:	return new int_eq_expr(f, ln, left, right);
    case GT:	return new int_gt_expr(f, ln, left, right);
    case LE:	return new int_le_expr(f, ln, left, right);
    // Not sure if the others are necessary
    default:
	DCASSERT(0);
  }
  return NULL;
}

expr* MakeConstBounds(int lower, expr* opnd, int upper, const char* f, int ln)
{
  DCASSERT(opnd);
  DCASSERT(opnd->Type(0)==PROC_INT);
  DCASSERT(lower <= upper);
  if (lower==upper) return new int_eq_expr(f, ln, lower, opnd);

  return new expr_between(f, ln, lower, opnd, upper);
}


#endif

//@}

