
// $Id$

#include "exprs.h"

/** @name exprs.cc
    @type File
    @args \ 

   Implementation of simple expression classes.

 */

//@{


// ******************************************************************
// *                                                                *
// *                        aggregates class                        *
// *                                                                *
// ******************************************************************

/**   The class used to aggregate expressions.
 
      Note: instead of using several binary operations, like we used to,
      we group all the aggregates together in one place.
      Why?  Speed, of course.
*/  

class aggregates : public expr {
  /// The individual expressions.
  expr** items;
  /// The number of individual expressions.
  int numitems;
public:
  /// Empty constructor.
  aggregates();

  /** Constructor.
      Note: if left or right is already an aggregate, its entries
            will be copied, and it will be destroyed.
   */
  aggregates(expr* left, expr* right);

  virtual ~aggregates();

  virtual expr* Copy() const;
  virtual int NumComponents() const { return numitems; }
  virtual expr* GetComponent(int i) { 
    CHECK_RANGE(0, i, numitems);
    return items[i]; 
  }
  virtual type Type(int i) const {
    CHECK_RANGE(0, i, numitems);
    return items[i]->Type(0);
  }
  virtual void Compute(int i, result &x) const {
    CHECK_RANGE(0, i, numitems);
    items[i]->Compute(0, x);
  }
  virtual void Sample(long &seed, int i, result &x) const {
    CHECK_RANGE(0, i, numitems);
    items[i]->Sample(seed, 0, x);
  }

protected:
  virtual void TakeAggregates();
};

aggregates::aggregates() : expr()
{
  items = NULL;
  numitems = 0;
}

aggregates::aggregates(expr* left, expr* right) : expr()
{
  int leftcount = 1;
  if (left) leftcount = left->NumComponents();
  int rightcount = 1;
  if (rightcount) rightcount = right->NumComponents();
  // make space for us
  numitems = leftcount+rightcount;
  items = new expr* [numitems];
  int i,j;
  // Copy left items
  if (left) {
    if (1==leftcount) {
      items[0] = left;
      i = 1;
    } else {
      // we have aggregates
      for (i=0; i<leftcount; i++) 
	items[i] = left->GetComponent(i);
      left->TakeAggregates();
    }
  } else {
    items[0] = NULL;
    i = 1;
  }
  // Copy right items
  if (right) {
    if (1==rightcount) {
      items[i] = right;
    } else {
      // we have aggregates
      for (j=0; j<leftcount; j++) 
	items[j+i] = right->GetComponent(j);
      right->TakeAggregates();
    }
  } else {
    items[i] = NULL;
  }
  // Cleanup
  delete left;
  delete right;
}

aggregates::~aggregates()
{
  int i;
  for (i=0; i<numitems; i++) delete items[i];
  delete[] items;
}

expr* aggregates::Copy() const
{
  int i;
  aggregates *answer = new aggregates();
  answer->numitems = numitems;
  answer->items = new expr* [numitems];
  for (i=0; i<numitems; i++)
    answer->items[i] = items[i]->Copy();
  return answer;
}

void aggregates::TakeAggregates()
{
  // Our items have been copied, so set them to NULL so we don't
  // delete them in the destructor
  int i;
  for (i=0; i<numitems; i++) items[i] = NULL;
}

// ******************************************************************
// *                                                                *
// *                        boolconst  class                        *
// *                                                                *
// ******************************************************************

/** A boolean constant expression.
 */
class boolconst : public expr {
  bool value;
  public:
  boolconst(const char* fn, int line, bool v) : expr (fn, line) {
    value = v;
  }

  virtual expr* Copy() const { 
    return new boolconst(Filename(), Linenumber(), value); 
  }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return BOOL;
  }

  virtual void Compute(int i, result &x) const {
    DCASSERT(0==i);
    x.Clear();
    x.bvalue = value;
  }
};

// ******************************************************************
// *                                                                *
// *                         intconst class                         *
// *                                                                *
// ******************************************************************

/** An integer constant expression.
 */
class intconst : public expr {
  int value;
  public:
  intconst(const char* fn, int line, int v) : expr (fn, line) {
    value = v;
  }

  virtual expr* Copy() const { 
    return new intconst(Filename(), Linenumber(), value); 
  }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return INT;
  }

  virtual void Compute(int i, result &x) const {
    DCASSERT(0==i);
    x.Clear();
    x.ivalue = value;
  }
};

// ******************************************************************
// *                                                                *
// *                        realconst  class                        *
// *                                                                *
// ******************************************************************

/** A real constant expression.
 */
class realconst : public expr {
  double value;
  public:
  realconst(const char* fn, int line, double v) : expr (fn, line) {
    value = v;
  }

  virtual expr* Copy() const { 
    return new realconst(Filename(), Linenumber(), value); 
  }

  virtual type Type(int i) const {
    DCASSERT(0==i);
    return REAL;
  }

  virtual void Compute(int i, result &x) const {
    DCASSERT(0==i);
    x.Clear();
    x.rvalue = value;
  }
};

// ******************************************************************
// *                                                                *
// *             Global functions  to build expressions             *
// *                                                                *
// ******************************************************************

expr* MakeConstExpr(bool c, const char* file, int line) 
{
  return new boolconst(file, line, c);
}

expr* MakeConstExpr(int c, const char* file, int line) 
{
  return new intconst(file, line, c);
}

expr* MakeConstExpr(double c, const char* file, int line) 
{
  return new realconst(file, line, c);
}

expr* SimpleTypecast(expr *e, type newtype);
expr* SimpleUnaryOp(int op, expr *opnd);
expr* SimpleBinaryOr(expr *left, int op, expr *right);

//@}

