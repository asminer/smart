
// $Id$

#ifndef ARRAYS_H
#define ARRAYS_H

/** @name arrays.h
    @type File
    @args \ 

  User-defined arrays.

  Goal: make arrays fast and simple.

Thoughts: derive classes from array.
   simple_array: for ordinary arrays.
converge_array: arrays within a converge statement.

 */

#include "exprs.h"

//@{
  

// ******************************************************************
// *                                                                *
// *                       array_index  class                       *
// *                                                                *
// ******************************************************************

/** Array index class.
    These are like formal parameters, but for arrays.

    Note: these are used directly by for loops.
    So, some of the functionality here is for them.
*/  

class array_index : public symbol {
  // set_expr *values;
  // set_result *current;
  // int index;
public:
  array_index(const char *fn, int line, type t, char *n);
  virtual ~array_index(); 
};

/*

class iterator : public symbol {
  /// Starting value
  int start;
  /// Stopping value
  int stop;
  /// increment
  int inc;
  /// Index (useful for arrays)
  int index;
  /// Current value
  int value;
public:
  iterator(const char *fn, int line, type t, char *n);
  virtual void Compute(int i, result &x) const;
  virtual expr* Substitute(int i);
  virtual void show(ostream &s) const;

  inline bool FirstValue() { 
    value = start; 
    if (value>stop) return false;
    index = 0; 
    return true;
  }
  inline bool NextValue() { 
    value += inc;
    if (value>stop) return false;
    index++;
    return true;
  }
  inline int Index() { return index; }

};

*/

// ******************************************************************
// *                                                                *
// *                      array_element  class                      *
// *                                                                *
// ******************************************************************

/** Array element struct.
    Used as part of the descriptor structure within an array.
    This struct will be at the "leaves".
*/  

struct array_element {
  bool already_computed;
  result value;
  expr* return_expr;
};




// ******************************************************************
// *                                                                *
// *                        array_desc class                        *
// *                                                                *
// ******************************************************************

/** Array descriptor struct.
    Used as part of the descriptor structure within an array.
    If there is another dimension below this one, then all the
    down pointers will be to other descriptors, otherwise they
    will be to elements.
*/  

struct array_desc {
  // set_result *values;
  void** down;
};




// ******************************************************************
// *                                                                *
// *                          array  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of arrays.

      Note: we've moved all compiler-related functionality
            OUT of this class to keep it simple.
*/  

class array : public symbol {
protected:
  /// Info about enclosing for iterators.
  array_index **index_list;
  /// The number of iterators.
  int dimension;
public:
  array(const char* fn, int line, type t, char* n, array_index **il, int dim);
  virtual ~array();

  virtual expr* Substitute(int i);

  /** So that the compiler can do typechecking.
      @param	il	List of indices.  MUST NOT BE CHANGED.
      @param	dim	Dimension (number of indices).
   */
  inline void GetIndexList(array_index** &il, int &dim) const {
    il = index_list;
    dim = dimension;
  }

  virtual void Compute(expr **, int np, result &x) = 0;
  virtual void Sample(long &, expr **, int np, result &x) = 0;
protected:

};





// ******************************************************************
// *                                                                *
// *                          acall  class                          *
// *                                                                *
// ******************************************************************

/**  An expression used to obtain an array element.
 */

class acall : public expr {
protected:
  array *func;
  expr **pass;
  int numpass;
public:
  acall(const char *fn, int line, array *f, expr **p, int np);
  virtual ~acall();
  virtual type Type(int i) const;
  virtual void Compute(int i, result &x);
  virtual void Sample(long &, int i, result &x);
  virtual expr* Substitute(int i);
  virtual int GetSymbols(int i, symbol **syms=NULL, int N=0, int offset=0);
  virtual void show(ostream &s) const;
};


//@}

#endif

