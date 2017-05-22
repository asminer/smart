
#ifndef CIRCULAR_H
#define CIRCULAR_H

/** Find a spot for an item in an ordered, circular list.
    index is the "key" to search for.
    tail is the last element of the list, and tail->next is the front.
    Return a pointer p such that:
        if the list is empty, p is -1.
        if index is in the list, then indexes[p] = index.
        if index is not in the list, then it would be added after p.
*/
inline long FindInCircular(long index, long tail, long* indexes, long* next)
{
  if (-1 == tail) return -1;
  if (index >= indexes[tail]) return tail;  // common and easy case
  long prev = tail;
  long curr = next[prev];
  while (1) {
    if (index < indexes[curr])   return prev;
    if (index == indexes[curr]) return curr;
    prev = curr;
    curr = next[prev];
  }
}

/** Defragment a list.
    @param ptr      The list, can be empty.
    @param i        Input & output: next slot to use in arrays.
    @param indexes  Index array.
    @param values   Value array.
    @param next     Next array.
*/
template <class DATA>
inline void Defragment(long ptr, long &i, long* indexes, DATA* values, long* next)
{
  while (ptr >= 0) {
    // traverse forwarding arcs if necessary
    while (ptr < i) ptr = next[ptr];
    long nextptr = next[ptr];
    if (i!=ptr) {
      next[ptr] = next[i];  // partial swap
      next[i] = ptr;      // forwarding info
      SWAP(indexes[i], indexes[ptr]);
      SWAP(values[i], values[ptr]);
    } 
    ptr = nextptr;
    i++;
  } // while ptr
}


#endif
