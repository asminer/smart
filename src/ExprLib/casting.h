
#ifndef CASTING_H
#define CASTING_H

/** \file casting.h
    Expressions that deal with typecasting between types.
 */

#include "type.h"
#include "expr.h"
#include "unary.h"

class general_conv;
class specific_conv;

// ******************************************************************
// *                                                                *
// *                         typeconv class                         *
// *                                                                *
// ******************************************************************

/**  Abstract base class for all type conversion rules.
     Also serves as a registry for type conversions,
     with methods for changing types of expressions.

     All type conversions can be achieved through at most
     one "specific" conversion, followed by at most one "general" conversion.

     For a given input type, a specific conversion has at most one
     output type (with no outputs when the conversion is not applicable).

     A general conversion will either apply or not for each
     input, output pair.
*/
class typeconv {
    public:
        typeconv();
        virtual ~typeconv();

        /** Convert an expression according to this rule.
                @param  W       Where the promotion is happening.
                @param  src     Original expression.
                @param  dest    Desired type.  Can be the target type, or
                                anything that is a "no op" promotion
                                from the target type.
                @return 0,  if it is impossible to convert \a src to
                            type \a dest, at least according to this rule.
                            Otherwise, convert src to the new type.
        */
        virtual expr* convert(const location& W,
            expr* src, const type* dest) const = 0;

        //
        // Statics, for type conversions in general
        //

        /** Returns the "promotion distance" from type t1 to t2.
                @param  t1  Original type
                @param  t2  Target type

                @return -1, if the promotion is impossible
                        0,  if the types are equal
                        n,  the cost of the conversion(s), designed
                            to minimize costly conversions.
        */
        static int getPromoteDistance(const type* t1, const type* t2);

        /// Returns true if type t1 can be promoted to type t2.
        static inline bool isPromotable(const type* t1, const type* t2) {
            return getPromoteDistance(t1, t2) >= 0;
        }

        /** Least common type that types a and b can be promoted to.
            Useful for, say, determining the type of
                rand int + proc real
            (which is proc rand real).

            @param  a  First type.
            @param  b  Second type.

            @return If such a type \a c exists, return the \a c that
                    minimizes the (non-negative) promotion distances
                    from a to c and from b to c.
                    Otherwise, return 0.
        */
        static const type* getLeastCommonType(const type* a, const type* b);


        /** Returns true if type t1 can be cast to type t2.
            This includes both promotions and explicit casts.

            @param  t1  Original type
            @param  t2  Target type

            @return true  iff it is possible to change from
                          type \a t1 to type \a t2.
        */
        static bool isCastable(const type* t1, const type* t2);

        /** Convert an expression to another type.

            @param  promote_only    If true, we do not consider casting rules.
            @param  W               Where defined.
            @param  newt            The desired type.
            @param  e               The original expression.

            @return 0,              if e is 0.
                    ERROR,          if e is ERROR, or if the change of type
                                    is impossible.
                    e,              if e already has type \a newt.
                    a new expression, otherwise.
        */
        static expr* castExpr(bool promote_only, const location &W,
                const type* newt, expr* e);

        /** Convert an expression for passing as a parameter.
            Like castExpr() with promote_only set to true, but
                (1) it will print a warning (disabled by default)
                    if the types are different;
                (2) it takes care of aggregate expressions;
                (3) the new expression will have the same location
                    as the original.

            @param  e       The original expression.
            @param  prc     Should fp components be promoted to proc.
            @param  rnd     Should fp components be promoted to rand.
            @param  fp      An expression (e.g., formal parameter)
                            whose type we want to match.
            @return 0,      if e is 0.
                    ERROR,  if e is ERROR, or if no promotion is possible.
                    e,      if e already has type \a newtype.
                    a new expression, otherwise.
        */
        static expr* promoteExpr(expr* e, bool prc, bool rnd, const expr* fp);

    protected:
        static const int RANGE_EXPAND = 1;  // e.g., int -> bigint
        static const int SIMPLE_CONV = 2;   // e.g., int -> real
        static const int MAKE_PHASE = 3;
        static const int MAKE_RAND = 6;
        static const int MAKE_PROC = 9;
        static const int MAKE_SET = 20;

        static void registerConv(general_conv* c);
        static void registerConv(specific_conv* c);
        static const general_conv* findGeneral(const type* oldt,
                const type* newt);

        static void findPair(const specific_conv* &list,
            const general_conv* &gc, const type* oldt, const type* newt);

    private:
        static general_conv* general_list;
        static specific_conv* promote_list;
        static specific_conv* cast_list;
        static warning_msg promote_arg;

        friend class casting_init;
};

// ******************************************************************
// *                                                                *
// *                       general_conv class                       *
// *                                                                *
// ******************************************************************

/**  Abstract base class for general type promotion rules.
     This allows the expression manager to make type promotions
     without "knowing" how they work.
     A "general" type promotion is one that works with changes
     not to the base type, e.g., int -> rand int, int -> {int}, etc.
*/
class general_conv : public typeconv {
    public:
        general_conv();

        /** According to this "rule", get distance between types.
            If -1, the rule does not handle promotions of this form.
                @param  src   Starting type.
                @param  dest  Desired type to reach.
                @return  Distance of the promotion from src to dest,
                        -1 if impossible, according to this rule.
        */
        virtual int getDistance(const type* src, const type* dest) const = 0;

        /** Is a conversion necessary for this promotion.
            If not, we can do a generic typecast that only changes the type,
            not how the expression is computed.
                @param  src   Starting type.
                @param  dest  Desired type to reach.
                @return If src is promotable to dest, returns true iff
                        a type conversion is required.
                        If src is not promotable to dest,
                        behavior is undefined.
        */
        virtual bool requiresConversion(const type* src, const type* dest)
            const = 0;
    private:
        general_conv* next;
        friend class typeconv;
};

// ******************************************************************
// *                                                                *
// *                      specific_conv  class                      *
// *                                                                *
// ******************************************************************

/**  Abstract base class for specific type conversion rules.
     This allows the expression manager to make type promotions
     without "knowing" how they work.
     A "specific" conversion is one that, for a given input type,
     there is only one output type.
     Examples: int -> real, rand int -> rand real, etc.
*/
class specific_conv : public typeconv {
    public:
        specific_conv(bool cast);

        inline bool isPromotion() const { return !is_cast; }
        inline bool isCast() const { return is_cast; }

        /** According to this "rule", get distance from the source type
            to whatever type it will be converted to.
            (Used primarily to determine rules that do not apply.)
                @param  src  Starting type.
                @return Distance to promote from src to the destination, or
                        -1 if impossible, according to this rule.
        */
        virtual int getDistance(const type* src) const = 0;

        /** According to this "rule", get destination type
            for the given source type.
                @param  src  Starting type.
                @return Destination type, or 0 if the rule does not apply.
        */
        virtual const type* promotesTo(const type* src) const = 0;
    private:
        bool is_cast;
        specific_conv* next;
        friend class typeconv;
};

// ******************************************************************
// *                                                                *
// *                         typecast class                         *
// *                                                                *
// ******************************************************************

/**  Base class for all typecast operations (expressions).
     Also a useful default, catch-all type converter
     for promotions that require "no operation".
*/
class typecast : public unary {
    public:
        typecast(const location &W, const type* newt, expr* x);
        virtual bool Print(std::ostream &s, int w=0) const;
        virtual void Compute(traverse_data &x);
    protected:
        // required for unary ops
        virtual expr* buildAnother(expr* x) const;
    protected:
        /// For display only.
        bool silent;
};


#endif

