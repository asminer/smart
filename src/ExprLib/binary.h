
#ifndef BINARY_H
#define BINARY_H

/** \file binary.h

  Base class for all binary operations.

 */

#include "result.h"
#include "expr.h"

// ******************************************************************
// *                                                                *
// *                        binary_op  class                        *
// *                                                                *
// ******************************************************************

/**  The base class for binary operations.
     These are registered with the expression manager,
     and invoked whenever a binary expression is constructed.
*/
class binary_op {
    public:
        enum opcode {
            /// Boolean implication
            bop_implies   = 0,
            /// Modulo operator
            bop_mod       = 1,
            /// Set difference
            bop_diff      = 2,
            /// Check for equality
            bop_equals    = 3,
            /// Check for inequality
            bop_nequal    = 4,
            /// Greater than
            bop_gt        = 5,
            /// Greater or equal
            bop_ge        = 6,
            /// Less than
            bop_lt        = 7,
            /// Less or equal
            bop_le        = 8,
            /// Temporal operator "U"
            bop_until     = 9,
            /// Temporal operator "AND"
            bop_and       = 10,
            /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
            bop_none      = 11
        };

    public:
        binary_op(opcode code);
        virtual ~binary_op();

        inline opcode getOpcode() const { return code; }

        //
        // Statics, for the entire registry of binary ops
        //

        /**
            Get the lexeme for a given opcode.
        */
        static const char* getOp(opcode code);

        /**
            Get documentation for a given opcode.
        */
        static const char* documentOp(opcode code);

        /** Determine the type of a binary operation expression.

            @param  left  The left operand type.
            @param  op    The operation to perform.
            @param  right The right operand type.
            @return 0,  on any kind of error;
                        the type of the operation, otherwise.
        */
        static const type* getTypeOf(const type* l, opcode op, const type* r);

        /** Make a binary operation expression.

            @param  W     Where defined.
            @param  left  The left operand.
            @param  op    The operation to perform.
            @param  rt    The right operand.
            @return NULL,   if either opnd is NULL.
                    ERROR,  if either opnd is ERROR,
                            or if the operand cannot be applied
                            (i.e., type mismatch).
                    a new expression, otherwise.
        */
        static expr* makeExpr(const location& W, expr* left,
                opcode op, expr* rt);

    protected:
        /** Are operations for the specified operand type \a t handled by us?
            @param  lt  Type of left operand.
            @param  rt  Type of right operand.
            @return true,   if we can build a legal expression "x opcode y"
                            where x has type \a lt, y has type \a rt.
                    false,  otherwise.
        */
        inline bool isDefinedForTypes(const type* lt, const type* rt) const {
            return getPromoteDistance(lt, rt) >= 0;
        }

    // Define these in the derived class

        /** Total promotion distance, if any, for operands.
            @param  lt  Type of left operand.
            @param  rt  Type of right operand.
            @return Sum of promotion distances required if we want
                        to apply "lt opcode rt".
                    -1  if we cannot promote lt or rt to satisfy the operator.
        */
        virtual int getPromoteDistance(const type* lt, const type* rt)
            const = 0;

        /** If we apply this operator, what is the resulting type?
            @param  lt  Type of left operand.
            @param  rt  Type of right operand.
            @return  Type of the expression, will be 0 if undefined.
        */
        virtual const type* getExprType(const type* lt, const type* rt)
            const = 0;

        /** Build an expression "left opcode right".
            The left and right operands are promoted as necessary.
            @param  W     Location of the expression.
            @param  left  Left operand.
            @param  right Right operand.
            @return A new expression "left opcode right", or
                    0 if an error occurred.
                    Will return 0 if "isDefinedForTypes()" returns false.
        */
        virtual expr* makeExpr(const location& W, expr* left, expr* right)
            const = 0;

    private:
        static const binary_op* bestMatch(const type* lt, opcode op,
                const type* rt);
        void registerOp(binary_op* op);

    private:
        opcode code;
        const  binary_op* next;

        static const binary_op** registry;
};


// ******************************************************************
// *                                                                *
// *                          binary class                          *
// *                                                                *
// ******************************************************************

/**  The base class for binary operations.
     Deriving from this class will save you from having
     to implement a few things.
 */

class binary : public expr {
protected:
  expr* left;
  expr* right;
  binary_op::opcode opcode;
public:
  binary(const location &W, binary_op::opcode oc,
   const type* t, expr* l, expr* r);
protected:
  virtual ~binary();
public:
  virtual void Traverse(traverse_data &x);
  virtual bool Print(std::ostream &s, int) const;
  /** Used for Substitution.
      Whatever kind of binary operation we are, make another one;
      except we try to make a shallow copy if possible.
      The filename and line number should be copied.
   */
  inline expr* MakeAnother(expr* l, expr* r) {
    if (0==l || 0==r) {
      Delete(left);
      Delete(right);
      return 0;
    }
    if (left==l && right==r) {
      Delete(left);
      Delete(right);
      return Share(this);
    }
    return buildAnother(l, r);
  };
protected:
  virtual expr* buildAnother(expr* newleft, expr* newright) const = 0;
  /// First step of most binary operations :^)
  inline void LRCompute(result &l, result &r, traverse_data &x) {
    DCASSERT(x.answer);
    DCASSERT(0==x.aggregate);
    DCASSERT(left);
    DCASSERT(right);
    result* answer = x.answer;
    x.answer = &l;
    left->Compute(x);
    x.answer = &r;
    right->Compute(x);
    x.answer = answer;
  }
};

// ******************************************************************
// *                                                                *
// *                          modulo class                          *
// *                                                                *
// ******************************************************************

/**   The base class of modulo classes.

      This saves you from having to implement a few of
      the virtual functions, because they are all the
      same for modulo.
*/

class modulo : public binary {
public:
  modulo(const location &W, const type* t, expr* l, expr* r);
};

// ******************************************************************
// *                                                                *
// *                           eqop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for equality check, for numerical non-constants
*/

class eqop : public binary {
public:
  eqop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all eqops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed).
        @param r  The value of the right operand (already computed).
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign, definitely not equal
      x.answer->setBool(false);
      return;
    }
    if (l.isInfinity() || r.isInfinity()) {
      // one infinity, one not: definitely not equal
      x.answer->setBool(false);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    // Can we get here?
    DCASSERT(0);
  }
};



// ******************************************************************
// *                                                                *
// *                          neqop  class                          *
// *                                                                *
// ******************************************************************

/**   The base class for inequality check, for non-constants
*/

class neqop : public binary {
public:
  neqop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all neqops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed).
        @param r  The value of the right operand (already computed).
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign, definitely not equal
      x.answer->setBool(true);
      return;
    }
    if (l.isInfinity() || r.isInfinity()) {
      // one infinity, one not: definitely not equal
      x.answer->setBool(true);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    // can we get here?
    DCASSERT(0);
  }
};



// ******************************************************************
// *                                                                *
// *                           gtop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater than check, for non-constants
*/

class gtop : public binary {
public:
  gtop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all gtops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed).
        @param r  The value of the right operand (already computed).
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->setBool(l.signInfinity()>0);
      return;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->setBool(l.signInfinity()>0);
      return;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->setBool(r.signInfinity()<0);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    // Still here?
    DCASSERT(0);
  }
};



// ******************************************************************
// *                                                                *
// *                           geop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for greater-equal check, for non-constants
*/

class geop : public binary {
public:
  geop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all geops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed)
        @param r  The value of the right operand (already computed)
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->setBool(l.signInfinity()>0);
      return;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->setBool(l.signInfinity()>0);
      return;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->setBool(r.signInfinity() < 0);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    DCASSERT(0);
  }
};



// ******************************************************************
// *                                                                *
// *                           ltop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less than check, for non-constants
*/

class ltop : public binary {
public:
  ltop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all ltops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed)
        @param r  The value of the right operand (already computed)
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign.
      x.answer->setBool(r.signInfinity() > 0);
      return;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->setBool(l.signInfinity() < 0);
      return;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->setBool(r.signInfinity() > 0);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    DCASSERT(0);
  }
};



// ******************************************************************
// *                                                                *
// *                           leop class                           *
// *                                                                *
// ******************************************************************

/**   The base class for less-equal check, for non-constants
*/

class leop : public binary {
public:
  leop(const location &W, const type* t, expr* l, expr* r);
protected:
  /** Common to all leops.
      Handles non-normal cases of operands.
        @param l  The value of the left operand (already computed)
        @param r  The value of the right operand (already computed)
        @param x  The result.  (Can be set on error conditions, etc.)
  */
  inline void Special(const result &l, const result &r, traverse_data &x) const {
    if (l.isNull() || r.isNull()) {
      x.answer->setNull();
      return;
    }
    if (l.isInfinity() && r.isInfinity()) {
      // both infinity
      if (l.signInfinity() == r.signInfinity()) {
        // same sign infinity, this is undefined
        x.answer->setNull();
        return;
      }
      // different sign.  If the first is positive, it is greater.
      x.answer->setBool(r.signInfinity() > 0);
      return;
    }
    if (l.isInfinity()) {
      // first is infinity, the other isn't; check sign
      x.answer->setBool(l.signInfinity() < 0);
      return;
    }
    if (r.isInfinity()) {
      // second is infinity, the other isn't; check sign
      x.answer->setBool(r.signInfinity() > 0);
      return;
    }
    if (l.isUnknown() || r.isUnknown()) {
      x.answer->setUnknown();
      return;
    }
    DCASSERT(0);
  }
};


#endif

