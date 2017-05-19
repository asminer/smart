
// $Id$

#ifndef BITVECTOR_H
#define BITVECTOR_H

#ifdef RANGE_CHECKING
#include <assert.h>
#endif

#include <stdlib.h>
#include <string.h>



/**   An array of bits.

      Use this class if you KNOW you have an array of bits.
*/  

class bitvector {
  static const int bitwidth = sizeof(long) * 8;
  static const int bitwidthm1 = bitwidth-1;
  static const unsigned long msbit = 1L << bitwidth-1;
  long size;
  unsigned long *data;
  long links;
protected:
  inline long Bits2Words(long bits) const { 
    return (bits>0) ? (((bits-1)/bitwidth)+1) : 0; 
  }
public:
  bitvector(long s) { 
    data = 0;
    size = 0;
    if (s>0) Resize(s);
    links = 1;
  }
  bitvector(const bitvector &b) {
    size = 0;
    data = 0;
    if (b.size>0) {
      Resize(b.size);
      memcpy(data, b.data, (size-1)/8+1);
    }
    links = 1;
  }
protected:
  ~bitvector() { 
    Resize(0);
  }
public:
  inline void Link() { links++; }
  inline void Unlink() { links--; if (0==links) delete this; }
  inline long numLinks() const { return links; }
  void Resize(long ns) {
    long words = Bits2Words(ns);
    long oldwords = Bits2Words(size); 
    if (words != oldwords) {
      data = (unsigned long*) realloc(data, words*sizeof(long));
    } 
    size = ns;
  }
  inline long Size() const { return size; }
  inline long MemUsed() const { return sizeof(long) * Bits2Words(size); }
  inline void Set(long n) {
#ifdef RANGE_CHECKING
    assert(n>=0);
    assert(n<size);
#endif
    data[n/bitwidth] |= (msbit >> n%bitwidth); // set bit n
  }
  inline void Unset(long n) {
#ifdef RANGE_CHECKING
    assert(n>=0);
    assert(n<size);
#endif
    data[n/bitwidth] &= ~(msbit >> n%bitwidth); // clear bit n      
  }
  inline bool IsSet(long n) const {
#ifdef RANGE_CHECKING
    assert(n>=0);
    assert(n<size);
#endif
    return (data[n/bitwidth] & (msbit >> n%bitwidth)) > 0; 
  }
  // Like set, but returns true if the bit was changed.
  inline bool SetBit_Changed(long n) {
#ifdef RANGE_CHECKING
    assert(n>=0);
    assert(n<size);
#endif
    unsigned tweak = (msbit >> n%bitwidth);
    long word = n/bitwidth;
    if (data[word] & tweak) return false;  // already set
    data[word] |= tweak; // set bit
    return true; 
  }
  // Like unset, but returns true if the bit was changed.
  inline bool UnsetBit_Changed(long n) {
#ifdef RANGE_CHECKING
    assert(n>=0);
    assert(n<size);
#endif
    unsigned tweak = (msbit >> n%bitwidth);
    long word = n/bitwidth;
    if (!(data[word] & tweak)) return false;  // already unset
    data[word] &= ~tweak; // clear bit
    return true; 
  }
  inline void UnsetAll() {
    memset(data, 0, Bits2Words(size)*sizeof(long));
  }
  inline void SetAll() {
    memset(data, 0xFF, Bits2Words(size)*sizeof(long));
  }
  /// Unset all bits from b1 to b2, inclusive
  inline void UnsetRange(long b1, long b2) {
#ifdef RANGE_CHECKING
    assert(b1>=0);
    assert(b1<size);
    assert(b1>=b1);
    assert(b2<size);
#endif
    // do the stray bits up to the word boundary
    while ((b1 <= b2) && (b1 % bitwidth > 0)) Unset(b1++);
    // do words at a time
    while (b1+bitwidthm1 <= b2) {
      data[b1/bitwidth] = 0; 
      b1 += bitwidth;
    }
    // do stray bits at the end
    while (b1 <= b2) Unset(b1++);
  }
  /// Set all bits from b1 to b2, inclusive
  inline void SetRange(long b1, long b2) {
#ifdef RANGE_CHECKING
    assert(b1>=0);
    assert(b1<size);
    assert(b1>=b1);
    assert(b2<size);
#endif
    // do the stray bits up to the word boundary
    while ((b1 <= b2) && (b1 % bitwidth > 0)) Set(b1++);
    // do words at a time
    while (b1+bitwidthm1 <= b2) {
      data[b1/bitwidth] = ~0; // 0xFFFFFFFF; 
      b1 += bitwidth;
    }
    // do stray bits at the end
    while (b1 <= b2) Set(b1++);
  }
  /// Find first bit set after n
  inline long FirstSetAfter(long n) const {
    unsigned long* dptr = data + n/bitwidth;
    while (n<size) {
      n++;
      long nmbw = n % bitwidth;
      if (nmbw) {
        if (dptr[0] & (msbit >> nmbw)) return n;
        continue;
      }
      // next word
      dptr++;
      while (0==dptr[0]) {
        n += bitwidth;
        if (n>=size) return -1;
      }
      if (dptr[0] & msbit) return n;
    } // while n < size
    return -1;
  }
  /// Find first bit unset after n
  inline long FirstUnsetAfter(long n) const {
    unsigned long* dptr = data + n/bitwidth;
    while (n<size) {
      n++;
      long nmbw = n % bitwidth;
      if (nmbw) {
        if (0==(dptr[0] & (msbit >> nmbw))) return n;
        continue;
      }
      // next word
      dptr++;
      while (0==~dptr[0]) {
        n += bitwidth;
        if (n>=size) return -1;
      }
      if (0==(dptr[0] & msbit)) return n;
    } // while n < size
    return -1;
  }
  /// This = B
  inline void FillFrom(const bitvector* B) {
    for (long w = Bits2Words(Size())-1; w>=0; w--)
      data[w] = B->data[w];  
  }
  /// This &= B
  inline void IntersectWith(const bitvector* B) {
    for (long w = Bits2Words(Size())-1; w>=0; w--)
      data[w] &= B->data[w];  
  }
  /// This |= B
  inline void UnionWith(const bitvector* B) {
    for (long w = Bits2Words(B->Size())-1; w>=0; w--)
      data[w] |= B->data[w];  
  }
  /// This -= B
  inline void DifferenceWith(const bitvector* B) {
    for (long w = Bits2Words(B->Size())-1; w>=0; w--)
      data[w] &= ~B->data[w];
  }

  /// This = A - B
  inline void Difference(const bitvector* A, const bitvector* B) {
    long s = (A->Size() < B->Size()) ? A->Size() : B->Size();
    Resize(s);
    for (long w = Bits2Words(s)-1; w>=0; w--)
      data[w] = A->data[w] & (~B->data[w]);
  }

  /// This = ~A
/*
  inline void Complement(const bitvector* A) {
    Resize(A->Size());
    for (long w = Bits2Words(Size())-1; w>=0; w--)
      data[w] = ~A->data[w];
  }
*/

  /// This = (~A) union B
/*
  inline void Implies(const bitvector* A, const bitvector *B) {
    long s = (A->Size() < B->Size()) ? A->Size() : B->Size();
    Resize(s);
    for (long w = Bits2Words(s)-1; w>=0; w--)
      data[w] = (~A->data[w]) | B->data[w];
  }
*/

  /// does This == B?
  inline bool Equals(const bitvector* B) const {
    long w = Bits2Words(Size())-1;
    // check the last word, igoring bits past size
    if ((data[w] ^ B->data[w]) & (~0 << bitwidth-Size()%bitwidth))
      return false;
    for (w--; w>=0; w--) if (data[w] != B->data[w]) return false;
    return true;
  } 

  /// is This a subset of B?  (This <= B?)
  inline bool SubsetOf(const bitvector* B) const {
    long w = Bits2Words(Size())-1;
    // check the last word, ignoring bits past size
    if ((data[w] & (~B->data[w])) & (~0 << bitwidth-Size()%bitwidth))
      return false;
    for (w--; w>=0; w--) if (data[w] & (~B->data[w])) return false;
    return true;
  }

  /// is This a proper subset of B?  (This < B?)
  inline bool ProperSubsetOf(const bitvector* B) const {
  }

};

#endif

