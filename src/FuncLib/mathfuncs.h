
/** \file mathfuncs.h

  Mathematical functions.

*/

#ifndef MATHFUNCS_H
#define MATHFUNCS_H

class symbol_table;
class exprman;

// void AddMathFunctions(symbol_table* st, const exprman* em);

#ifdef MATHFUNCS_DETAILED

#include "../ExprLib/functions.h"

// ******************************************************************
// *                                                                *
// *                          max_si  class                         *
// *                                                                *
// ******************************************************************

/**
  Base class for max functions.
  There are derived classes based on the argument type.
*/
class max_si : public simple_internal {
public:
  max_si(const type* args);    
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};


// ******************************************************************
// *                                                                *
// *                          min_si  class                         *
// *                                                                *
// ******************************************************************

/**
  Base class for min functions.
  There are derived classes based on the argument type.
*/
class min_si : public simple_internal {
public:
  min_si(const type* args);    
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};


// ******************************************************************
// *                                                                *
// *                         order_si  class                        *
// *                                                                *
// ******************************************************************

/**
  Base class for order functions.
  There are derived classes based on the argument type.
*/
class order_si : public simple_internal {
public:
  order_si(const type* args);    
  virtual int Traverse(traverse_data &x, expr** pass, int np);
};


#endif

#endif

