
#ifndef ASSOC_H
#define ASSOC_H

/** \file assoc.h

  Base class for all associative operations.
*/

#include "expr.h"

// ******************************************************************
// *                                                                *
// *                         assoc_op class                         *
// *                                                                *
// ******************************************************************

/**  The base class for associative operations.
     These are registered with the expression manager,
     and invoked whenever an associative expression is constructed.
*/
class assoc_op {
    public:
        enum opcode {
            /// Boolean AND
            aop_and    = 0,
            /// Boolean OR
            aop_or    = 1,
            /// Addition
            aop_plus  = 2,
            /// Multiplication
            aop_times  = 3,
            /// Aggregation
            aop_colon  = 4,
            /// Aggregation of void expressions
            aop_semi  = 5,
            /// Union of sets
            aop_union  = 6,
            /// no operation (placeholder).
            aop_none  = 7
        };

    public:
        assoc_op(opcode code);
        virtual ~assoc_op();

        inline opcode getOpcode() const { return code; }

        //
        // Statics, for the entire registry of assoc ops
        //

        /// Get the lexeme for a given opcode
        static const char* getOp(bool flip, opcode code);
        /// Get documentation for a given opcode
        static const char* documentOp(bool flip, opcode code);

        /** Determine the type of an associative operation (sub)expression.

                @param  left  The left operand type.
                @param  flip  Do we "flip" the operation.
                @param  op    The operation to perform.
                @param  right The right operand type.
                @return 0,  on any kind of error;
                        the type of the operation, otherwise.
        */
        static const type* getTypeOf(const type* left, bool flip, opcode op,
                const type* right);

        /** Make an associative operation expression.

                @param  W     Where defined.
                @param  op    The operation to perform.
                @param  opnds Array of operands.
                @param  f     For each operand, should it be "flipped"?
                            If missing (null), we assume that no
                            operands should be flipped.
                @param  nops  Number of operands.
                @return NULL,   if any opnd is NULL.
                        ERROR,  if any opnd is ERROR,
                                or if the operand cannot be applied
                                (i.e., type mismatch).
                        a new expression, otherwise.
        */
        static expr* makeExpr(const location& W, opcode op,
                expr** opnds, bool* f, int nops);

    protected:
        // Define these in the derived class

        /** Total promotion distance, if any, for operands.
            @param  flip  Do we flip the operator.
            @param  lt    Type of left operand.
            @param  rt    Type of right operand.
            @return Sum of promotion distances required if we want
                        to apply "lt opcode rt".
                    -1  if we cannot promote lt or rt to satisfy the operator.
        */
        virtual int getPromoteDistance(bool flip, const type* lt,
            const type* rt) const = 0;

        /** Total promotion distance, if any, for operands.
            @param  list  List of operands.
            @param  flip  For flippable operands, designation of
                        "flipped or not" for each operand.
                        Can be 0 to indicate "none flipped".
            @param  N      Number of operands.
            @return Sum of promotion distances required, if possible.
                    -1  if we cannot promote to satisfy the operator.
        */
        virtual int getPromoteDistance(expr** list, bool* flip, int N)
            const = 0;

        /** Are operations for the specified operand types handled by us?
            @param  list  List of operands.
            @param  flip  For flippable operands, designation of
                        "flipped or not" for each operand.
                        Can be 0 to indicate "none flipped".
            @param  N     Number of operands.
            @return true,  if we can build a legal expression.
                    false,  otherwise.
        */
        inline bool isDefinedForTypes(expr** list, bool* flip, int N) const {
            return getPromoteDistance(list, flip, N) >= 0;
        }

        /** If we apply this operator, what is the resulting type?
                @param  flip  Do we flip the operator.
                @param  lt    Type of left operand.
                @param  rt    Type of right operand.
                @return Type of the expression, will be 0 if undefined.
        */
        virtual const type* getExprType(bool flip, const type* lt,
                const type* rt) const = 0;

        /** Build an expression.
            The operands are promoted as necessary.
                @param  W     Location of expression.
                @param  list  List of operands.
                @param  flip  For flippable operands, designation of
                              "flipped or not" for each operand.
                              Can be 0 to indicate "none flipped".
                @param  N  Number of operands.
                @return A new expression, or 0 if an error occurred.
                        Will return 0 if "isDefinedForTypes()" returns false.
        */
        virtual expr* makeExpr(const location& W, expr** list,
                bool* flip, int N) const = 0;

    private:
        static void registerOp(assoc_op* op);

    private:
        opcode code;
        const  assoc_op* next;

        static const assoc_op** registry;
};


// ******************************************************************
// *                                                                *
// *                          assoc  class                          *
// *                                                                *
// ******************************************************************

/**  The base class for associative operations.
     This allows us to string together things like sums
     into a single operator (for efficiency).
     Deriving from this class will save you from having
     to implement a few things.
 */

class assoc : public expr {
protected:
  int opnd_count;
  expr** operands;
  assoc_op::opcode opcode;
public:
  assoc(const location &W, assoc_op::opcode oc,
        const type* t, expr **x, int n);
  assoc(const location &W, assoc_op::opcode oc,
        typelist* t, expr **x, int n);
protected:
  virtual ~assoc();
public:
  virtual void Traverse(traverse_data &x);
  /** Used for Substitution.
      Whatever kind of associative operation we are, make another one;
      except we make a shallow copy if possible.
      The filename and line number should be copied.
   */
  expr* MakeAnother(expr **newx, int newn);
protected:
  virtual expr* buildAnother(expr **newx, int newn) const = 0;
};


// ******************************************************************
// *                                                                *
// *                        flipassoc  class                        *
// *                                                                *
// ******************************************************************

/**  Slightly fancier associative operators, in which we
     are allowed to "invert" or "flip" some of the operands.
     Allows us to do things like
        a + b - c + d;
 */

class flipassoc : public assoc {
protected:
  /// Can be NULL to signify "no flips".
  bool* flip;
public:
  flipassoc(const location &W, assoc_op::opcode oc,
    const type* t, expr** x, bool* f, int n);
protected:
  virtual ~flipassoc();
public:
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;
  /** Used for Substitution.
      Whatever kind of flip associative operation we are, make another one;
      except we make a shallow copy if possible.
      The filename and line number should be copied.
   */
  expr* MakeAnother(expr **newx, bool* newf, int newn);
protected:
  virtual expr* buildAnother(expr **newx, int newn) const;
  virtual expr* buildAnother(expr **newx, bool* newf, int newn) const = 0;
  static inline bool* checkFlip(bool* f, int n) {
    if (0==f) return 0;
    for (int i=n-1; i>=0; i--) {
      if (f[i]) return f;
    }
    delete[] f;
    return 0;
  }
};

// ******************************************************************
// *                                                                *
// *                        summation  class                        *
// *                                                                *
// ******************************************************************

/**   The base class of addition classes.

      This saves you from having to implement a few of
      the virtual functions, because they are all the
      same for addition.

      Note: this includes logical or

      This is now a fancier operation: we can "negate" some
      of the operands.  I.e., this class can handle expressions:
        a + b - c + d;
*/

class summation : public flipassoc {
public:
    summation(const location &W, assoc_op::opcode oc,
        const type* t, expr** x, bool* f, int n);
protected:
    inline void inftyMinusInfty(const expr* opnd, result* ans) const {
        DCASSERT(opnd);
        expr_error E(opnd, ans);
        E << "Undefined operation (infty-infty) due to ";
        opnd->Print(E.stream(), 0);
    }
};

// ******************************************************************
// *                                                                *
// *                         product  class                         *
// *                                                                *
// ******************************************************************

/**   The base class of multiplication classes.

      This saves you from having to implement a few of
      the virtual functions, because they are all the
      same for multiplication.

      Note: this includes logical and
*/

class product : public flipassoc {
public:
    product(const location &W, assoc_op::opcode oc,
        const type* t, expr** x, bool* f, int n);
    virtual void Traverse(traverse_data &x);
protected:
    inline void divideByZero(const expr* opnd, result* ans) const {
        DCASSERT(opnd);
        expr_error E(opnd, ans);
        E << "Undefined operation (divide by 0) due to ";
        opnd->Print(E.stream(), 0);
    }
    inline void zeroTimesInfty(const expr* opnd, result* ans) const {
        DCASSERT(opnd);
        expr_error E(opnd, ans);
        E << "Undefined operation (0 * infty) due to ";
        opnd->Print(E.stream(), 0);
    }
    inline void inftyTimesZero(bool flip, const expr* opnd, result* ans) const {
        DCASSERT(opnd);
        expr_error E(opnd, ans);
        E << "Undefined operation (infty " << (flip ? '/' : '*')
          << "0) due to ";
        opnd->Print(E.stream(), 0);
    }
    inline void inftyDivInfty(const expr* opnd, result* ans) const {
        DCASSERT(opnd);
        expr_error E(opnd, ans);
        E << "Undefined operation (infty / infty) due to ";
        opnd->Print(E.stream(), 0);
    }
};


#endif
