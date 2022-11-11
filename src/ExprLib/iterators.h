
#ifndef ITERATORS_H
#define ITERATORS_H

/** \file iterators.h

  For loop iterators.
  Since this is required by both for loops and arrays,
  we define the class here and keep it hidden from the rest.
*/

#include "symbols.h"
#include "sets.h"

// ******************************************************************
// *                                                                *
// *                         iterator class                         *
// *                                                                *
// ******************************************************************

/** For loop iterator variable.
    Also used as "formal parameters for arrays".
*/

class iterator : public symbol {
protected:
  /// Expression (set type) for the values of the iterator.
  expr *values;
  /// Current range of values for the iterator.
  shared_set *current;
  /// Index in the above set for the current value.
  long index;
public:
  iterator(const location &W, const type* t, char *n, expr *v);
protected:
  virtual ~iterator();
public:
  virtual void Compute(traverse_data &x);

  /** Prints the iterator name, and the set of values.
      Required by for loops.
  */
  void PrintAll(OutputStream &s) const;

  /** Compute the current range of values for the iterator.
      Required by for loops.
  */
  void ComputeCurrent(traverse_data &x);

  /** Done with current range of values for the iterator.
      Required by for loops.
  */
  inline void DoneCurrent() {
    Delete(current);
    current = 0;
  }

  /** Fix our value to the first element of the current set.
      Required by for loops.
        @return true on success.
   */
  inline bool FirstIndex() {
    if (0==current) return false;
    if (current->Size() < 1) return false;
    index = 0;
    return true;
  }

  /** Fix our value to the next element of the current set.
      Required by for loops.
        @return true on success.
   */
  inline bool NextValue() {
    DCASSERT(current);
    index++;
    return (index < current->Size());
  }

  /** The element number of the current set that we are "on".
      Required for arrays.
   */
  inline long Index() {
    return index;
  }

  /** Copy the current range of values.
      Required for arrays.
  */
  inline shared_set* CopyCurrent() {
    return Share(current);
  }

  /// For debugging.
  void ShowAssignment(OutputStream &s) const;
};



#endif
