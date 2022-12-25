
#ifndef TRINARY_H
#define TRINARY_H

/** \file trinary.h

  Base class for all trinary operations.

  These include:
  the set operator: start .. stop .. increment
  the if-then-else operator: test ? tval : fval

*/

#include "expr.h"
#include "result.h"

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
    public:
        enum opcode {
            /// Set intervals.
            top_interval  = 0,
            /// If-then-else
            top_ite    = 1,
            /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
            top_none  = 2
        };

    public:
        trinary_op(opcode oc);
        virtual ~trinary_op();

        inline opcode getOpcode() const { return code; }

        //
        // Statics, for the entire registry of binary ops
        //

        /// Get the lexeme for the first operator
        static const char* getFirst(opcode op);
        /// Get the lexeme for the second operator
        static const char* getSecond(opcode op);
        /// Get documentation for the given opcode
        static const char* documentOp(opcode op);

        /** Determine the type of a trinary operation expression.
                @param  op      The operation to perform.
                @param  left    The left operand type.
                @param  middle  The middle operand type.
                @param  right   The right operand type.
                @return 0,  on any kind of error;
                            the type of the operation, otherwise.
        */
        static const type* getTypeOf(opcode op, const type* left,
            const type* middle, const type* right);

        /** Make a trinary operation expression.
                @param  W     Where defined.
                @param  op  The operation to perform.
                @param  l   The left operand.
                @param  m   The middle operand.
                @param  r   The right operand.
                @return ERROR,  if any opnd is ERROR,
                                or if the operand cannot be applied
                                (i.e., type mismatch).
                        a new expression, otherwise.
        */
        static expr* makeExpr(const location& W, opcode op,
            expr* l, expr* m, expr* r);

    protected:
        /** Are operations for the specified operand types handled by us?
                @param  lt  Type of left operand.
                @param  mt  Type of middle operand.
                @param  rt  Type of right operand.
                @return true,  if we can build a legal expression.
                        false, otherwise.
        */
        inline bool isDefinedForTypes(const type* lt, const type* mt,
                const type* rt) const
        {
            return getPromoteDistance(lt, mt, rt) >= 0;
        }

    // Define these in the derived class

        /** Total promotion distance, if any, for operands.
                @param  lt  Type of left operand.
                @param  mt  Type of middle operand.
                @param  rt  Type of right operand.
                @return Sum of promotion distances required, if possible;
                        -1  if we cannot promote to satisfy the operator.
        */
        virtual int getPromoteDistance(const type* lt, const type* mt,
                const type* rt) const = 0;

        /** If we apply this operator, what is the resulting type?
                @param  lt  Type of left operand.
                @param  mt  Type of middle operand.
                @param  rt  Type of right operand.
                @return Type of the expression, will be 0 if undefined.
        */
        virtual const type* getExprType(const type* lt, const type* mt,
                const type* rt) const = 0;

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
        virtual expr* makeExpr(const location& W, expr* left,
                expr* middle, expr* right) const = 0;

    private:
        static const trinary_op* bestMatch(opcode op, const type* left,
            const type* middle, const type* right);

        static void registerOp(trinary_op* op);

    private:
        opcode code;
        const  trinary_op* next;

        static const trinary_op** registry;
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

