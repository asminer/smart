
// $Id$

#ifndef COLL_H
#define COLL_H

#include "statelib.h"

// ******************************************************************
// *                                                                *
// *                       Macros and such                          *
// *                                                                *
// ******************************************************************

// Useful for debugging.
#ifdef RANGE_CHECK
  #include <assert.h>
  #define DCASSERT(X) assert(X)
  inline void CheckRange(int min, int value, int max)
  {
    assert(value<max);
    assert(value>=min);
  }
  #define CHECK_RANGE(MIN, VALUE, MAX)  CheckRange(MIN, VALUE, MAX)
#else
  #define CHECK_RANGE(MIN, VALUE, MAX)
  #define DCASSERT(X)
#endif

// ******************************************************************
// *                                                                *
// *                       bitstream struct                         *
// *                                                                *
// ******************************************************************

/** Struct used to keep track of our current 
    location in a stream of bits.
    Used within the class coll_base.
*/
struct bitstream {
  long byteptr;
  int bitptr;
public:
  bitstream() {
    Rewind();
  };

  bitstream(long byp) {
    MoveTo(byp);
  };

  /// Advance the pointer the specified number of bits.
  inline void Advance(long numbits) {
    byteptr += numbits / 8;
    bitptr -= numbits % 8;
    if (bitptr < 0) {
      bitptr += 8;
      byteptr++;
    }
    CHECK_RANGE(0, bitptr, 8);
  };

  inline void Rewind() {
    byteptr = 0;
    bitptr = 7;
  }

  inline void MoveTo(long byp) {
    byteptr = byp;
    bitptr = 7;
  }

  inline long NextByte() const {
    if (7==bitptr) return byteptr;
    return byteptr+1;
  }
};


// ******************************************************************
// *                                                                *
// *                       coll_base  class                         *
// *                                                                *
// ******************************************************************

/** More useful base class for our state collections.

    Implementation of low-level memory stuff
    (i.e., writing and reading some number of bits).
*/

class coll_base : public StateLib::state_coll {
  /// Memory space for actual states
  unsigned char *mem;
  /// Current dimension of the \a mem array.
  long memsize;
public:
  coll_base();
  virtual ~coll_base();  

  /** Enlarge the memory array, if necessary. 
      Throws an error if malloc fails.

        @param  newsize   Desired size.  Memory will be expanded
                          if necessary but not shrunk.

  */
  void EnlargeMem(long newsize);

  /** The workhorse of the whole thing.
      So it better be efficient!
      Pulls off the next \emph{bits} bits from the "bit stream",
      stores the integer result in \emph{x}.

        @param  s     Pointer to current bit in memory.
                      Will be advanced as we read bits.

        @param  bits  Number of bits to read.

        @param  x     Where to store the resulting integer.
   */
  void ReadInt(bitstream &s, char bits, int &x) const;

  /** The opposite of ReadInt.
      Writes the integer \emph{x} onto the "bit stream", using
      \emph{bits} bits.

      @param  s     Pointer to the current bit in memory.
                    Will be advanced as we write bits.

      @param  bits  Number of bits to write.

      @param  x     Integer to write into the bits.
   */
  void WriteInt(bitstream &s, char bits, int x);

  inline const unsigned char* GetPtr(long h) const { return mem + h; }
  inline const unsigned char GetByte(long h) const { return mem[h]; }

  virtual long ReportMemTotal() const {
    return memsize;
  }

  /** For debugging.
      Dump raw memory to standard output.
  */
  void DumpMemory(FILE* s, long starth, long stoph) const;
};


// ******************************************************************
// *                                                                *
// *                     coll_base_test class                       *
// *                                                                *
// ******************************************************************

/** A class used to test the abstract base class, coll_base.

    All the required virtual functions are no-ops.
*/
class coll_base_test : public coll_base {
public:
  coll_base_test() : coll_base() { }
  virtual ~coll_base_test() { }

  virtual bool StateSizesAreStored() const {
    return false;
  }

  virtual void Clear() { }
  virtual long AddState(const int* state, int size) { 
    return -1; 
  }
  virtual bool PopLast(long hndl) { 
    return false; 
  }
  virtual long GetStateKnown(long hndl, int* state, int size) const {
    return -1;
  }
  virtual int GetStateUnknown(long hndl, int* state, int size) const {
    return -1;
  }
  virtual const unsigned char* GetRawState(long hndl, long& bytes) const {
    return 0;
  }
  virtual long FirstHandle() const {
    return -1;
  }
  virtual long NextHandle(long hndl) const {
    return -1;
  }
  virtual int CompareHH(long h1, long h2) const {
    return 0;
  }
  virtual int CompareHF(long hndl, int size, const int* state) const {
    return 0;
  }
  virtual unsigned long Hash(long hndl, int bits) const {
    return 0;
  }
  virtual int NumEncodingMethods() const {
    return 0;
  }
  virtual const char* EncodingMethod(int m) const {
    return 0;
  }
  virtual long ReportEncodingCount(int m) const {
    return 0;
  }
};



// ******************************************************************
// *                                                                *
// *                       main_coll  class                         *
// *                                                                *
// ******************************************************************

/** Conceptually, an array of explicitly-stored states.
    In reality, states are stored in a compressed format, as follows,
    on a stream of bits.

    We request integers of various sizes 
    (1-bit, 4-bit, 8-bit, 16-bit, etc) 
    and pull them off the stream. 
     
    Every state starts on a byte boundary.  Thus, the only wasted space 
    is at the end of a state, where we may have a few unused bits before
    the next state starts on a byte boundary.

    The first byte of a state's storage tells us how it is stored.  Actually, 
    we are pulling off small integers here also!

    bits 7,6:  (i.e., first we read a 2-bit integer...)  
        - 0: truncated full
        - 1: sparse 
        - 2: 
        - 3: 
     
    bits 5,4,3: 
        - 0 : 0 <= \#places < 16,             four bits used 
        - 1 : 16 <= \#places < 256,           one byte used 
        - 2 : 256 <= \#places < 65536,        two bytes used 
        - 3 : 65536 <= \#places < 16777216,   three bytes used 
        - 4 : 1677216 <= \#places < 2^32,     four bytes used 

    bits 2,1,0: 
        - 0 : 0 <= maxtokens < 2,             one bit used 
        - 1 : 2 <= maxtokens < 4,             two bits used 
        - 2 : 2 <= maxtokens < 16,            four bits used 
        - 3 : 16 <= maxtokens < 256,          one byte used 
        - 4 : 256 <= maxtokens < 65536,       two bytes used 
        - 5 : 65536 <= maxtokens < 16777216,  three bytes used 
        - 6 : 16777216 <= maxtokens < 2^32,   four bytes used 

    If the state sizes are stored, then it is read off next,
    using the number of bytes according to the size of \#places.

    Then we pull off integers of appropriate sizes (e.g., if we know that
    16 <= \#places < 256, we'll get 8-bits for a place\#) according to the
    following storage schemes:

      Truncated full storage: 
        \#places 
        \#places entries of (\#tokens), of appropriate size.

      Sparse storage: 
        \#nonzero entries (size dictated by \#places) 
        \#nz pairs of (place\#, \#tokens) with each of appropriate size. 
        Note if maxtokens = 1 then we don't need to store \#tokens! 

    Whenever we add a state, we figure out which storage technique uses the 
    least memory, and that's the one we use.  In case of a tie, we favor 
    truncated full storage, then sparse, then run-length (because I think 
    that's the order in terms of speed). 

*/
class main_coll : public coll_base {
  /// Should we store the sizes?
  bool store_sizes;
  /** Index to handle mapping, if desired.
      Invariant:
      map[0] always equals 0.
      map[h+1] always points to the start of the state after h.
  */
  long* map;
  /// Current size of map array (will expand as necessary)
  long mapsize;
  /// Handle of first state
  long firsthandle;
  /// Handle of next state to be added
  long lasthandle;   

  /// Stats: counts the number of full encodings
  long full_count;
  /// Stats: counts the number of sparse encodings
  long sparse_count;

protected:
  static const char ENCODING_FULL = 0;
  static const char ENCODING_SPARSE = 1;

  static const int placelimit[];
  static const char placebits[];
  static const int placedim;

  static const int tokenlimit[];
  static const char tokenbits[];
  static const int tokendim;

protected:
  inline int Bits2Bytes(int numbits) const { return (numbits+7)/8; }

  // Helper for NextHandle, GetRawState
  long NextRawHandle(long rawh) const;

public:
  main_coll(bool use_indices, bool use_sizes);
  virtual ~main_coll();

  virtual bool StateSizesAreStored() const;
  virtual bool StateHandlesAreIndexes() const;
  virtual void Clear();
  virtual long AddState(const int* state, int size);
  virtual bool PopLast(long hndl);
  virtual long GetStateKnown(long hndl, int* state, int size) const;
  virtual int GetStateUnknown(long hndl, int* state, int size) const;
  virtual const unsigned char* GetRawState(long hndl, long &bytes) const;
  virtual long FirstHandle() const;
  virtual long NextHandle(long hndl) const;
  virtual int CompareHH(long h1, long h2) const;
  virtual int CompareHF(long hndl, int size, const int* state) const;
  virtual unsigned long Hash(long hndl, int bits) const;
  virtual long* RemoveIndexHandles();
  virtual int NumEncodingMethods() const;
  virtual const char* EncodingMethod(int m) const;
  virtual long ReportEncodingCount(int m) const;
  virtual long ReportMemTotal() const;

  inline void DumpState(FILE* s, long h) const {
    if (map) DumpMemory(s, map[h], map[h+1]);
    else {
      long nexth = NextHandle(h);
      if (nexth <= 0)  DumpMemory(s, h, lasthandle);
      else    DumpMemory(s, h, nexth);
    }
  }
};



#endif
