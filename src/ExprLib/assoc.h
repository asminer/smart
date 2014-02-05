
// $Id$

#ifndef ASSOC_H
#define ASSOC_H

/** \file assoc.h

  Base class for all associative operations.
*/

#include "exprman.h"

class assoc;

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
  exprman::assoc_opcode opcode;
protected:
  const exprman* em;
public:
  assoc_op(exprman::assoc_opcode opcode);
  virtual ~assoc_op();

  inline exprman::assoc_opcode getOpcode() const { return opcode; }

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
  virtual int getPromoteDistance(expr** list, bool* flip, int N) const = 0;

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
        @param  fn    Filename of expression.
        @param  ln    Line number of expression.
        @param  list  List of operands.
        @param  flip  For flippable operands, designation of
                      "flipped or not" for each operand.
                      Can be 0 to indicate "none flipped".
        @param  N  Number of operands.
        @return A new expression, or 0 if an error occurred.
                Will return 0 if "isDefinedForTypes()" returns false.
  */
  virtual assoc* makeExpr(const char* fn, int ln, expr** list, 
        bool* flip, int N) const = 0;

private:
  const  assoc_op* next;
  friend class superman;
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
  exprman::assoc_opcode opcode;
public:
  assoc(const char* fn, int line, exprman::assoc_opcode oc,
        const type* t, expr **x, int n);
  assoc(const char* fn, int line, exprman::assoc_opcode oc,
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
  flipassoc(const char* fn, int line, exprman::assoc_opcode oc, 
    const type* t, expr** x, bool* f, int n);
protected:
  virtual ~flipassoc();
public:
  virtual void Traverse(traverse_data &x);
  virtual bool Print(OutputStream &s, int) const;
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
  summation(const char* fn, int line, exprman::assoc_opcode oc, 
    const type* t, expr** x, bool* f, int n);
protected:
  inline void inftyMinusInfty(const expr* opnd) const {
    DCASSERT(opnd);
    if (em->startError()) {
      em->causedBy(opnd->Filename(), opnd->Linenumber());
      em->cerr() << "Undefined operation (infty-infty) due to ";
      opnd->Print(em->cerr(), 0);
      em->stopIO();
    }
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
  product(const char* fn, int line, exprman::assoc_opcode oc,
    const type* t, expr** x, bool* f, int n);
  virtual void Traverse(traverse_data &x);
protected:
  inline void divideByZero(const expr* opnd) const {
    DCASSERT(opnd);
    if (em->startError()) {
      em->causedBy(opnd->Filename(), opnd->Linenumber());
      em->cerr() << "Undefined operation (divide by 0) due to ";
      opnd->Print(em->cerr(), 0);
      em->stopIO();
    }
  }
  inline void zeroTimesInfty(const expr* opnd) const {
    DCASSERT(opnd);
    if (em->startError()) {
      em->causedBy(opnd->Filename(), opnd->Linenumber());
      em->cerr() << "Undefined operation (0 * infty) due to ";
      opnd->Print(em->cerr(), 0);
      em->stopIO();
    }
  } 
  inline void inftyTimesZero(bool flip, const expr* opnd) const {
    DCASSERT(opnd);
    if (em->startError()) {
      em->causedBy(opnd->Filename(), opnd->Linenumber());
      em->cerr() << "Undefined operation (infty ";
      if (flip) em->cerr() << "/"; else em->cerr() << "*";
      em->cerr() << "0) due to ";
      opnd->Print(em->cerr(), 0);
      em->stopIO();
    }
  }
  inline void inftyDivInfty(const expr* opnd) const {
    DCASSERT(opnd);
    if (em->startError()) {
      em->causedBy(opnd->Filename(), opnd->Linenumber());
      em->cerr() << "Undefined operation (infty / infty) due to ";
      opnd->Print(em->cerr(), 0);
      em->stopIO();
    }
  }
};


#endif
