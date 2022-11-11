
#ifndef UNARY_H
#define UNARY_H

/** \file unary.h

  Base class for all unary operations.

 */

#include "exprman.h"

class unary;

// ******************************************************************
// *                                                                *
// *                         unary_op class                         *
// *                                                                *
// ******************************************************************

/**  The base class for unary operations.
     These are registered with the expression manager,
     and invoked whenever a unary expression is constructed.
*/
class unary_op {
  exprman::unary_opcode opcode;
protected:
  const exprman* em;
public:
  unary_op(exprman::unary_opcode opcode);
  virtual ~unary_op();

  inline exprman::unary_opcode getOpcode() const { return opcode; }

  // Define these in the derived class

  /** If we apply this operator to an expression, what is the resulting type?
        @param  t  Type of operand.
        @return Type of the expression "opcode x", will be 0 if undefined.
  */
  virtual const type* getExprType(const type* t) const = 0;

  /** Are operations for the specified operand type \a t handled by us?
        @param  t  Type of operand.
        @return true,   if we can build a legal expression "opcode x"
                        where x has type \a t.
                false,  otherwise.
  */
  inline bool isDefinedForType(const type* t) const {
    return getExprType(t);
  }

  /** Build an expression "opcode x".
        @param  W   Where the expression was in input.
        @param  x   Operand.
        @return A new expression "opcode x", or
                0 if an error occurred.
                Will return 0 if "isDefinedForType()" returns false
                for the type of \a x.
  */
  virtual unary* makeExpr(const location& W, expr* x) const = 0;

private:
  const  unary_op* next;
  friend class superman;
};

// ******************************************************************
// *                                                                *
// *                          unary  class                          *
// *                                                                *
// ******************************************************************

/**  The base class for unary expressions.
     Deriving from this class will save you from having
     to implement a few things.
 */

class unary : public expr {
protected:
  expr* opnd;
public:
  unary(const location& W, const type* t, expr* x);
protected:
  virtual ~unary();
public:
  virtual void Traverse(traverse_data &x);
  /** Used for Substitution.
      Whatever kind of unary operation we are, make another one;
      except we try to make a shallow copy if possible.
      The filename and line number should be copied.
   */
  inline expr* MakeAnother(expr* newopnd) {
    if (0==newopnd) return 0;
    if (newopnd==opnd) {
      Delete(newopnd);
      return Share(this);
    }
    return buildAnother(newopnd);
  }
protected:
  virtual expr* buildAnother(expr* newopnd) const = 0;
};

// ******************************************************************
// *                                                                *
// *                          negop  class                          *
// *                                                                *
// ******************************************************************

/** The base class for negation.
    Note: this includes binary "not"
    Negation expressions should be derived from this one.
*/

class negop : public unary {
  // So we know if it is "!" or "-".
  exprman::unary_opcode opcode;
public:
  negop(const location& W, exprman::unary_opcode oc, const type* t, expr* x);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Traverse(traverse_data &x);
};

// ******************************************************************
// *                                                                *
// *                    unary_temporal_op  class                    *
// *                                                                *
// ******************************************************************

/** The base class for temporal temporal expression.
*/

class unary_temporal_expr : public unary {
  exprman::unary_opcode opcode;
public:
  unary_temporal_expr(const location& W, exprman::unary_opcode oc, const type* t, expr* x);
  virtual bool Print(OutputStream &s, int) const;

  exprman::unary_opcode GetOpCode() const
  {
    return opcode;
  }
};

#endif

