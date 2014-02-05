
// $Id$

#ifndef RADIXSORT_H
#define RADIXSORT_H

/** Radix sort for ordering collections whose keys are arrays of bits.
    The collection must provide functions:
      bool isBitSet(long i, int b): true iff item i has bit b set.
      void swap(long i, long j)   : exchange positions of items i and j.
    This function is recursive.
      @param  L   The collection.
      @param  a   Starting position; at top this should be 0.
      @param  b   One plus final position; at top this should be 
                  the collection size.
      @param  k   Bit number to start with (will decrease to 0).
      @param  dec Should we use a decreasing order?
*/
template <class COLLECTION>
void radix_sort(COLLECTION &L, long a, long b, int k, bool dec)
{
  if (a>=b || k<0) return;
  DCASSERT(a<b);
  // sort based on bit k.
  //
  // front of array holds entries with bit k == dec.
  // back  of array holds entries with bit k != dec.
  //

  int ap = a;
  int bp = b-1;

  // Determine front pointer, first time
  while (ap < b) {
    if (L.isBitSet(ap, k) != dec) break;
    ap++;
  }
  if (ap >= b-1) {
    radix_sort(L, a, ap, k-1, dec);
    return;
  }

  for (;;) {

    // find smallest entry != dec
    while (L.isBitSet(ap, k) == dec) {
      ap++;
      if (ap >= bp) {
        radix_sort(L, a, ap, k-1, dec);
        radix_sort(L, ap, b, k-1, dec);
        return;
      }
    } // while

    // find largest entry == dec
    while (L.isBitSet(bp, k) != dec) {
      bp--;
      if (ap >= bp) {
        radix_sort(L, a, ap, k-1, dec);
        radix_sort(L, ap, b, k-1, dec);
        return;
      }
    } // while

    L.swap(ap, bp);
    
  } // infinite loop
}

#endif
