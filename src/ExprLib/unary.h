
#ifndef UNARY_H
#define UNARY_H

/** \file unary.h

  Base class for all unary operations.

 */

#include "expr.h"
// #include "exprman.h"

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
    public:
        enum opcode {
            /// Boolean negation.
            uop_not       = 0,
            /// Arithmetic negation.
            uop_neg       = 1,
            /// Temporal operator "A".
            uop_forall    = 2,
            /// Temporal operator "E".
            uop_exists    = 3,
            /// Temporal operator "F"
            uop_future    = 4,
            /// Temporal operator "G"
            uop_globally  = 5,
            /// Temporal operator "X"
            uop_next      = 6,
            /// Custom operations
            uop_custom    = 7,
            /// no operation (placeholder).  MUST BE THE LARGEST INTEGER.
            uop_none      = 8
        };

    public:
        unary_op(opcode code);
        virtual ~unary_op();

        inline opcode getOpcode() const { return code; }

        //
        // Statics, for the entire registry of unary ops
        //

        /**
            Get the lexeme for a given opcode.
        */
        static const char* getOp(opcode code);

        /**
            Get documentation for a given opcode.
        */
        static const char* documentOp(opcode code);

        /** Determine the type of a unary operation expression.

            @param  op    The operation to perform.
            @param  opnd  The operand type.
            @return 0,  on any kind of error;
                        the type of the operation, otherwise.
        */
        static const type* getTypeOf(opcode op, const type* x);

        /** Make a unary operation expression.

            @param  W     Where defined.
            @param  op    The operation to perform.
            @param  opnd  The operand.
            @return NULL,   if opnd is NULL.
                    ERROR,  if opnd is ERROR, or if there is a type mismatch.
                            a new expression, otherwise.
        */
        static expr* makeExpr(const location &W, opcode op, expr* opnd);

    protected:
        /** Are operations for the specified operand type \a t handled by us?
            @param  t  Type of operand.
            @return true,   if we can build a legal expression "opcode x"
                            where x has type \a t.
                    false,  otherwise.
        */
        inline bool isDefinedForType(const type* t) const {
            return getExprType(t);
        }

  // Define these in the derived class

        /** If we apply this operator to an expression,
            what is the resulting type?
            @param  t  Type of operand.
            @return Type of the expression "opcode x", will be 0 if undefined.
        */
        virtual const type* getExprType(const type* t) const = 0;

        /** Build an expression "opcode x".
            @param  W   Where the expression was in input.
            @param  x   Operand.
            @return A new expression "opcode x", or
                    0 if an error occurred.
                    Will return 0 if "isDefinedForType()" returns false
                    for the type of \a x.
        */
        virtual expr* makeExpr(const location& W, expr* x) const = 0;

    private:
        static const unary_op* bestMatch(opcode op, const type* x);
        void registerOp(unary_op* op);

    private:
        opcode code;
        const  unary_op* next;

        static const unary_op** registry;
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
    unary_op::opcode code;
protected:
    expr* opnd;
public:
    unary(const location& W, unary_op::opcode code, const type* t, expr* x);
protected:
    virtual ~unary();
public:
    virtual bool Print(std::ostream &s, int width=0) const;
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
    inline unary_op::opcode GetOpCode() const {
        return code;
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
public:
    negop(const location& W, unary_op::opcode code, const type* t, expr* x);
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
public:
    unary_temporal_expr(const location& W, unary_op::opcode code,
            const type* t, expr* x);
};

#endif

