
// $Id$

#ifndef STATEHEAP_H
#define STATEHEAP_H

#include "../Language/results.h"

/** @name stateheap.h
    @type File
    @args \

    State definition and minimalist front-end. 
    
    Someday we may have a fancy backend for efficiently
    allocating temporary full states.

*/

/** Discrete state.
    For now, everything is public because we know what we're doing ;^)
*/
class state {
  /// If true, this state shares space with another state.
  bool am_substate;  
  /// Length (# array elements of data to consider)
  int size;
  /// Data.  Allocation/deletion should be handled by functions below.
  result* data; 
public:  
  state() { size = 0; data = NULL; }
  inline int Size() const { return size; }
  inline result& operator[](int n) {
    DCASSERT(n>=0);
    DCASSERT(n<size);
    DCASSERT(data);
    return data[n];
  }
  inline result Read(int n) const {
    DCASSERT(n>=0);
    DCASSERT(n<size);
    DCASSERT(data);
    return data[n];
  }
  void Show(OutputStream& s) const;
  friend bool AllocState(state &s, int length);
  friend void FreeState(state &s);
  friend void MakeSubstate(state &sub, const state &s, int pos, int len);
};

/** Stream overloading for state */
inline OutputStream& operator<< (OutputStream& s, const state &st)
{
  st.Show(s);
  return s;
}

/** Set up space for state s, from the state "heap".
    Returns true on success.
*/
bool AllocState(state &s, int length);

/** Return space for state s to the state "heap".
*/
void FreeState(state &s);

/** Make a (contiguous) substate.
    @param	sub	Output; substate to create (NULL on range error)
    @param	s	State to take from
    @param	pos	Starting position within state s
    @param	len	Length
*/
void MakeSubstate(state &sub, const state &s, int pos, int len);

#endif

