
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

#include "sets.h"
#include "stmts.h"
#include "variables.h"

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
protected:
  /// Expression (set type) for the values of the iterator.
  expr *values;
  /// Current range of values for the iterator.
  set_result *current;
  /// Index in the above set for the current value.
  int index;
public:
  array_index(const char *fn, int line, type t, char *n, expr *v);
  virtual ~array_index(); 

  virtual void ClearCache() { } // No cache
  virtual void Compute(int i, result &x);
  void showfancy(OutputStream &s) const;

  virtual Engine_type GetEngine(engineinfo *e);
  virtual expr* SplitEngines(List <measure> *); 

  // Used by for loops and arrays:

  /// Compute the current range of values for the iterator.
  inline void ComputeCurrent() {
    Delete(current);
    result x;
    SafeCompute(values, 0, x);
    if (x.isNull() || x.isError()) current = NULL;  // print an error?
    else current = (set_result*) x.other;
  }

  /// Copy the current range of values.
  inline set_result* CopyCurrent() {
    return Copy(current);
  }

  /** Fix our value to the first element of the current set.
      @return true on success.
   */
  inline bool FirstIndex() { 
    if (NULL==current) return false;
    if (current->Size() < 1) return false;
    index = 0;
    return true;
  }

  /** Fix our value to the next element of the current set.
      @return true on success.
   */
  inline bool NextValue() { 
    DCASSERT(current);
    index++;
    return (index < current->Size());
  }

  /** The element number of the current set that we are "on".
   */
  inline int Index() { 
    return index; 
  }
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
    will be to constfuncs.  (see variables.h)
*/  

struct array_desc {
  /// The values that can be assumed here.
  set_result *values;
  /** Pointers to the next dimension (another array_desc)
      or to the array values (a constfunc).
      Note that the dimension of this array is equal to the
      size of the set "values".
   */
  void** down;

  array_desc(set_result *v) {
    values = v;
    down = new void*[values->Size()];
    for (int i=0; i<values->Size(); i++) down[i] = NULL;
  }
  ~array_desc() {
    Delete(values);
    delete[] down;
  }
};




// ******************************************************************
// *                                                                *
// *                          array  class                          *
// *                                                                *
// ******************************************************************

/**   The base class of arrays.
      
      Do we need to derive anything from this?

      Note: we've moved all compiler-related functionality
            OUT of this class to keep it simple.
*/  

class array : public symbol {
protected:
  /// Info about enclosing for iterators.
  array_index **index_list;
  /// The number of iterators.
  int dimension;
  /// The descriptor.
  array_desc *descriptor;

  /// Recursive function to clear out descriptor
  void Clear(int level, void *desc);
public:
  /// Used to catch compile errors with converges and such
  const_state state;
public:
  array(const char* fn, int line, type t, char* n, array_index **il, int dim);

  /// When we don't know the type yet (used by models)
  array(const char* fn, int line, char* n, array_index **il, int dim);

  /// For delayed type definition (by models)
  inline void SetType(type t) {
    DCASSERT(state == CS_Untyped);
    mytype = t;
    state = CS_Undefined;
  }

  virtual ~array();

  /** Clear out the array elements but keep everything else.
      Used for models to allow "reinstantiation".
  */
  void Clear();

  /** So that the compiler can do typechecking.
      @param	il	List of indices.  MUST NOT BE CHANGED.
      @param	dim	Dimension (number of indices).
   */
  inline void GetIndexList(array_index** &il, int &dim) const {
    il = index_list;
    dim = dimension;
  }

  /** For the current values of the iterators,
      set the return "value" of the array.
   */
  void SetCurrentReturn(symbol *retvalue);

  /** For the current values of the iterators,
      obtain the return value function.
      Returns null if the return value has not been set yet.
      (Used for converge array assignments)
   */
  symbol* GetCurrentReturn();

  /** Determine our current "name", which
      will be written to the specified stream.
   */
  void GetName(OutputStream &s) const;

  virtual void ClearCache() { } // No cache

  /** For the given indices (as expressions),
      find the array "value".
      Actually, we return a constfunc.
      Used directly by "acall".
      @param	il	Indices to use.  Must be exactly of size "dimension".
      @param	x	Where we return the function (in the "other" field).
      Note: this way we can set the error values appropriately!
   */
  void Compute(expr** il, result &x);

  /** Like compute, but we sample the indices instead.
   */
  void Sample(Rng &, expr **il, result &x);

  virtual void show(OutputStream &s) const;
};






// ******************************************************************
// *                                                                *
// *                                                                *
// *                          Global stuff                          *
// *                                                                *
// *                                                                *
// ******************************************************************

/** Make an expression to call an array.
 */
expr* MakeArrayCall(array *f, expr **p, int np, const char *fn, int l);

/** Make a for-loop statement.
    We only handle one dimension at a time.
 */
statement* MakeForLoop(array_index **i, int dim, statement** block, int bsize, 
                       const char *fn, int line);

/// "Normal" array assignments.
statement* MakeArrayAssign(array *f, expr* retval, 
                           const char *fn, int line);

//@}

#endif

