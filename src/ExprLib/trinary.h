
#ifndef TRINARY_H
#define TRINARY_H

/** \file trinary.h

  Base class for all trinary operations.

  These include:
  the set operator: start .. stop .. increment
  the if-then-else operator: test ? tval : fval

*/

#include "exprman.h"
#include "result.h"

class trinary;

// ******************************************************************
// *                                                                *
// *                        trinary_op class                        *
// *                                                                *
// ******************************************************************

/**  The base class for trinary operations.
     These are registered with the expression manager,
     and invoked whenever a trinary expression is constructed.
*/
class trinary_op {
  exprman::trinary_opcode opcode;
protected:
  const exprman* em;
public:
  trinary_op(exprman::trinary_opcode opcode);
  virtual ~trinary_op();

  inline exprman::trinary_opcode getOpcode() const { return opcode; }

  // Define these in the derived class

  /** Total promotion distance, if any, for operands.
        @param  lt  Type of left operand.
        @param  mt  Type of middle operand.
        @param  rt  Type of right operand.
        @return Sum of promotion distances required, if possible;
                -1  if we cannot promote to satisfy the operator.
  */
  virtual int getPromoteDistance(const type* lt, const type* mt, const type* rt) const = 0;

  /** Are operations for the specified operand types handled by us?
        @param  lt  Type of left operand.
        @param  mt  Type of middle operand.
        @param  rt  Type of right operand.
        @return true,  if we can build a legal expression.
                false, otherwise.
  */
  inline bool isDefinedForTypes(const type* lt, const type* mt, const type* rt) const {
    return getPromoteDistance(lt, mt, rt) >= 0;
  }

  /** If we apply this operator, what is the resulting type?
        @param  lt  Type of left operand.
        @param  mt  Type of middle operand.
        @param  rt  Type of right operand.
        @return Type of the expression, will be 0 if undefined.
  */
  virtual const type* getExprType(const type* lt, const type* mt, const type* rt) const = 0;

  /** Build an expression.
      The operands are promoted as necessary.
        @param  W       Location of expression.
        @param  left    Left operand.
        @param  middle  Middle operand.
        @param  right   Right operand.
        @return A new expression "left opcode right", or
                0 if an error occurred.
                Will return 0 if "isDefinedForTypes()" returns false.
  */
  virtual trinary* makeExpr(const location& W, expr* left,
        expr* middle, expr* right) const = 0;

private:
  const  trinary_op* next;
  friend class superman;
};

// ******************************************************************
// *                                                                *
// *                         trinary  class                         *
// *                                                                *
// ******************************************************************

/**  The base class for trinary operations.
     Deriving from this class will save you from having
     to implement a few things.
 */

class trinary : public expr {
protected:
  expr* left;
  expr* middle;
  expr* right;
public:
  trinary(const location &W, const type* t, expr* l, expr* m, expr* r);
protected:
  virtual ~trinary();
public:
  /// Default traversals: recursively traverse left, middle, right.
  virtual void Traverse(traverse_data &x);
  /** Used for Substitution.
      Whatever kind of trinary operation we are, make another one.
      The filename and line number should be copied.
   */
  inline expr* MakeAnother(expr* newl, expr* newm, expr* newr) {
    if (0==newl || 0==newm || 0==newr) {
      Delete(newl);
      Delete(newm);
      Delete(newr);
      return 0;
    }
    if (left==newl && middle==newm && right==newr) {
      Delete(newl);
      Delete(newm);
      Delete(newr);
      return Share(this);
    }
    return buildAnother(newl, newm, newr);
  }
protected:
  virtual expr* buildAnother(expr* newl, expr* newm, expr* newr) const = 0;
  /// First step of most trinary operations :^)
  inline void LMRCompute(result &l, result &m, result &r, traverse_data &x) {
    DCASSERT(x.answer);
    DCASSERT(0==x.aggregate);
    result* answer = x.answer;
    x.answer = &l;
    SafeCompute(left, x);
    x.answer = &m;
    SafeCompute(middle, x);
    x.answer = &r;
    SafeCompute(right, x);
    x.answer = answer;
  }
};


#endif

