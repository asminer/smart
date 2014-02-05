
// $Id$

#ifndef OPS_INT_H
#define OPS_INT_H

#include "assoc.h"

/** \file ops_int.h

   Operator classes, for integers (int, proc int, rand int, proc rand int)

 */

// ******************************************************************
// *                                                                *
// *                       int_assoc_op class                       *
// *                                                                *
// ******************************************************************

/** Base class for integer associative operators.
    Handles all cases, but note that for this "rule" we never
    build anything with type "phase int"; rand int is used instead.
*/
class int_assoc_op : public assoc_op {
public:
  int_assoc_op(exprman::assoc_opcode op);
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
};

// ******************************************************************
// *                                                                *
// *                        int_add_op class                        *
// *                                                                *
// ******************************************************************

/** Class for integer addition and subtraction operations.
*/
class int_add_op : public int_assoc_op {

protected:
  /// The actual addition / subtraction expression.
  class expression : public summation {
  public:
    expression(const char* fn, int line, const type* t, 
    expr **x, bool* f, int n);
    virtual void Compute(traverse_data &x);
  protected:
    virtual expr* buildAnother(expr **x, bool* f, int n) const;
  };

public:
  int_add_op();
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                                                                *
// *                       int_mult_op  class                       *
// *                                                                *
// ******************************************************************

class int_mult_op : public int_assoc_op {
protected:
  /// The actual multiply expression; only products.
  class expression : public product {
    public:
      expression(const char* fn, int line, const type* t, expr **x, int n);
      virtual void Compute(traverse_data &x);
    protected:
      virtual expr* buildAnother(expr **x, bool* f, int n) const;
  };
public:
  int_mult_op();
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const;
  virtual int getPromoteDistance(bool f, const type* lt, const type* rt) const;
  virtual const type* getExprType(bool f, const type* l, const type* r) const;
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

class exprman;
void InitIntegerOps(exprman* em);

#endif

