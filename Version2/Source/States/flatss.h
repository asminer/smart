
// $Id$

#ifndef FLATSS_H
#define FLATSS_H

#include "../Base/streams.h"
#include "stateheap.h" 


/** @name flatss.h
    @type File
    @args \ 

  Class for storing a collection of compressed states.

  This has just been re-written big time.  We finally replaced the klunky
  old interface of states acting upon a heap of bytes.  We now have a
  more generic, more user-friendly interface with the notion of a
  collection of states.  The interface allows the states to be stored in 
  strange and complex ways (for compression) without bothering the outside 
  world.

*/

//@{


// ******************************************************************
// *                                                                *
// *                      state_array  class                        *
// *                                                                *
// ******************************************************************

/**  
     Conceptually, an array of explicitly-stored states.
     In reality, states are stored in a compressed format, as follows.

     We have a stream of \emph{bits} used to store states.  
     We then request integers of various sizes 
     (1-bit, 4-bit, 8-bit, 16-bit, etc) 
     and pull them off the stream.  Gets kind of ugly when we're mid-byte and 
     then want to consume a 4-byte integer, but we're talking bit-counting 
     memory conservation here.
     
     To make life a tiny bit simpler (no pun intended), every state starts 
     on a byte boundary.  Thus, the only wasted space is at the end of 
     a state, where we may have a few unused bits before the next state 
     starts on a byte boundary.

     The first byte of a state's storage tells us how it is stored.  Actually, 
     we are pulling off small integers here also!

     bits 7,6:  (i.e., first we read a 2-bit integer...)  \\
	0: not in use, deleted state  \\
	1: sparse \\
	2: runlength encoding \\
	3: full  
     
     bits 5,4,3: \\
     	0 : 0 < \#places <= 16,  		 four bits used \\
     	1 : 16 < \#places <= 256, 		 one byte used \\
     	2 : 256 < \#places <= 65536,  		 two bytes used \\
     	3 : 65536 < \#places <= 16777216,	 three bytes used \\
     	4 : 1677216 < \#places <= 2^32, 	 four bytes used \\

     bits 2,1,0: \\
     	0 : 0 <= maxtokens < 2,		 	one bit used \\
	1 : 2 <= maxtokens < 4,		 	two bits used \\
     	2 : 2 <= maxtokens < 16,	 	four bits used \\
     	3 : 16 <= maxtokens < 256,	 	one byte used \\
     	4 : 256 <= maxtokens < 65536,	 	two bytes used \\
     	5 : 65536 <= maxtokens < 16777216, 	three bytes used \\
     	6 : 16777216 <= maxtokens < 2^32, 	four bytes used \\

     Then we pull off integers of appropriate sizes (e.g., if we know that
     16 <= \#places < 256, we'll get 8-bits for a place\#) according to the
     following storage schemes:

     Sparse storage: \\
     	\#nonzero entries (size dictated by \#places) \\
      	\#nz pairs of (place\#, \#tokens) with each of appropriate size. \\
      	Note if maxtokens = 1 then we don't need to store \#tokens! 

     Runlength encoding: \\
        \#entries total (uses \#places bits) \\
	Bit indicating if the next entry is a RUN or a LIST \\
	run/list length, of size #places bits \\
	if RUN, then a single value (the value of the next count state vars) \\
	if LIST, then count values are listed	
	Note: if maxtokens = 1 then the value is ALWAYS implied

     Full storage: \\
      	\#places \\
      	\#places entries of (\#tokens), of appropriate size.

     Whenever we add a state, we figure out which storage technique uses the 
     least memory, and that's the one we use.  In case of a tie, we favor 
     full storage, then sparse, then run-length (because I think that's the 
     order in terms of speed). 

 */
class state_array {
  /// Memory space for actual states
  unsigned char *mem;
  int memsize;
  
  // Used by WriteInt and ReadInt
  int byteptr;
  int bitptr;

  // stats
  int encodecount[4];

protected:
  /// Index to handle mapping, if desired
  int* map;
  /// Current size of map array (will expand as necessary)
  int mapsize;
  /// Number of inserted states
  int numstates;  
  /// Handle of first state
  int firsthandle;
  /// Handle of next state to be added
  int lasthandle;   

  /** The workhorse of the whole thing.
      So it better be efficient!
      Pulls off the next \emph{bits} bits from the "bit stream",
      stores the integer result in \emph{x}.
   */
  void ReadInt(char bits, int &x); 

  /** Skips the next \emph{bits} bits from the "bit stream".
      I.e., like ReadInt except we don't care about the value.
  */
  inline void SkipInt(char bits) {
    byteptr += bits / 8;
    bitptr -= bits % 8;
    if (bitptr < 0) {
      bitptr += 8;
      byteptr++;
    }
    CHECK_RANGE(bitptr, 0, 7);
  };

  /** The opposite of ReadInt.
      Writes the integer \emph{x} onto the "bit stream", using
      \emph{bits} bits.
   */
  void WriteInt(char bits, int x);

  /// For debugging.
  void PrintBits(OutputStream &s, int start, int stop) const;

  /** Enlarge the memory array, if necessary. */
  void EnlargeMem(int newsize);

  // Helpers for AddState
  void RunlengthEncode(char npbits, char tkbits, const state &s);
  void RunlengthEncodeBinary(char npbits, const state &s);

  inline int Bits2Bytes(int numbits) const { return (numbits+7)/8; }
public:
  state_array(bool useindices);
  ~state_array();

  inline bool UsesIndexHandles() const { return (map!=NULL); }
  inline int NumStates() const { return numstates; }

  inline int MaxHandle() const { return map ? numstates : lasthandle; }
  inline int FirstHandle() const { return map ? 0 : firsthandle; }
  
  int AddState(const state &s);
  /// Can only be used with map array.
  inline void PopLast() {
    DCASSERT(map);
    DCASSERT(numstates);
    lasthandle = map[--numstates];
    int encoding = (mem[map[numstates]] & 0xC0) / 64;
    CHECK_RANGE(0, encoding, 4);
    encodecount[encoding]--;
  }

  bool GetState(int h, state &s);

  int NextHandle(int h); 

  /**  Compares the encodings of two states.
       Note: this is done by comparing the encodings of the states
       without unpacking.  For speed, this requires a map array (index 
       handles) to find the "end" of the encoding.
       @param h1	First state
       @param h2	Second state
       @return	An integer with the same sign as 
		 (encoding of state h1) - (encoding of state h2)
       This would be used for example with a "dictionary" data structure
       to keep track of unique states, something like:
       1) add new state
       2) check if it exists already
       3) if so, call PopLast
  */
  int Compare(int h1, int h2) const;

  /**  Compare an encoded state with a full state.
       The encoded state is unpacked "on the fly",
       only as much as necessary.
       @param h1	First state (handle)
       @param s2	Second state (full)	
       @return  An integer with the same sign as
		(unpacked state h1) - (s2)
       Could also be used with a dictionary structure to keep track
       of unique states.  Differences with the above "Compare":
       * the second state doesn't have to be added yet 
       * can be used without a map array (non-index handles)
  */
  int Compare(int h1, const state& s2);

  void Report(OutputStream &r);
  int MemUsed();

  // Clear out old states but keep memory allocated.
  void Clear();
};



// ******************************************************************
// *                                                                *
// *                         flatss class                           *
// *                                                                *
// ******************************************************************

/** Final structure for flat reachability sets.

    All we need is the state array itself, to "get" states,
    and an ordering array, to "find" states.
    
    We assume that states are "indexed" in the state array,
    e.g., the third state has handle 3.
*/
class flatss {
  /// States are stored here in discovery order.
  state_array *states;
  /// Order of states according to state_array::Compare.
  int* order;
public:
  flatss(state_array *sa, int *o);
  ~flatss();

  inline int NumStates() const {
    DCASSERT(states);
    return states->NumStates();
  }

  inline bool GetState(int h, state &s) { 
    DCASSERT(states);
    return states->GetState(h, s);
  }

  inline int FindState(const state &s) {
    DCASSERT(states);
    int a = binsearch(states->AddState(s));
    states->PopLast();
    return a;
  }

protected:
  int binsearch(int h);
};


//@}

#endif

