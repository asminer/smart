
// $Id$

#ifndef CASTING_H
#define CASTING_H

/** \file casting.h
    Expressions that deal with typecasting between types.
 */

#include "type.h"
#include "expr.h"
#include "unary.h"

// ******************************************************************
// *                                                                *
// *                        base_conv  class                        *
// *                                                                *
// ******************************************************************

/**  Abstract base class for all type conversion rules.
*/
class base_conv {
protected:
  static const int RANGE_EXPAND = 1;  // e.g., int -> bigint
  static const int SIMPLE_CONV = 2;   // e.g., int -> real
  static const int MAKE_PHASE = 3;
  static const int MAKE_RAND = 6;
  static const int MAKE_PROC = 9;
  static const int MAKE_SET = 20;
  const exprman* em;  
  friend class superman;
public:
  base_conv();
  virtual ~base_conv();

  /** Convert an expression according to this rule.
        @param  file  Filename causing the promotion.
        @param  line  Linenumber causing the promotion.
        @param  src   Original expression.
        @param  dest  Desired type.  Can be the target type, or
                      anything that is a "no op" promotion
                      from the target type.
        @return 0, if it is impossible to convert \a src to type \a dest,
                at least according to this rule.  Otherwise, 
                convert src to the new type.
  */
  virtual expr* convert(const char* file, int line,
      expr* src, const type* dest) const = 0;
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
class general_conv : public base_conv {
public:

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
                If src is not promotable to dest, behavior is undefined.
  */
  virtual bool requiresConversion(const type* src, const type* dest) const = 0;
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
class specific_conv : public base_conv {
  bool is_cast;
public:
  specific_conv(bool cast);

  inline bool isPromotion() const { return !is_cast; }
  inline bool isCast() const { return is_cast; }

  /** According to this "rule", get distance from the source type
      to whatever type it will be converted to.
      (Used primarily to determine rules that do not apply.)
        @param  src  Starting type.
        @return Distance of the promotion from src to the destination, or
                -1 if impossible, according to this rule.
  */ 
  virtual int getDistance(const type* src) const = 0;
  
  /** According to this "rule", get destination type for the given source type.
        @param  src  Starting type.
        @return Destination type, or 0 if the rule does not apply.
  */
  virtual const type* promotesTo(const type* src) const = 0;
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
protected:
  /// For display only.
  bool silent;
public:
  typecast(const char* fn, int line, const type* newt, expr* x);
  virtual bool Print(OutputStream &s, int) const;
  virtual void Compute(traverse_data &x);
protected:
  // required for unary ops
  virtual expr* buildAnother(expr* x) const;
};


#endif

