
// $Id$

#ifndef FLATSS_H
#define FLATSS_H

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
	2: inverse-sparse \\
	3: full  
     
     bits 5,4,3: \\
     	0 : 0 < \#places <= 16,  		 half-byte can be used \\
     	1 : 16 < \#places <= 256, 		 one byte can be used \\
     	2 : 256 < \#places <= 65536,  		 two bytes can be used \\
     	3 : 65536 < \#places <= 16777216,	 three bytes can be used \\
     	4 : 65536 < \#places <= 2^32, 		 four bytes used 

     bits 2,1,0: \\
     	0 : 0 <= maxtokens < 2,		 one bit can be used \\
	1 : 2 <= maxtokens < 4,		 two bits can be used \\
     	2 : 2 <= maxtokens < 16,	 four bits can be used \\
     	3 : 16 <= maxtokens < 256,	 one byte used \\
     	4 : 256 <= maxtokens < 65536,	 two bytes used \\

     Then we pull off integers of appropriate sizes (e.g., if we know that
     16 <= \#places < 256, we'll get 8-bits for a place\#) according to the
     following storage schemes:

     Sparse storage: \\
     	\#nonzero entries (size dictated by \#places) \\
      	\#nz pairs of (place\#, \#tokens) with each of appropriate size. \\
      	Note if maxtokens = 1 then we don't need to store \#tokens! 

     Inverse sparse: \\
        Mfv : Most frequent value (size dictated by \#tokens) \\
	\#values not equal to most frequent value \\
	\#values pairs of (place\#, \#tokens) with each of appropriate size.\\
	If maxtokens = 1, then we must have Mfv=1, so it is not stored, nor
	is #tokens for each pair.

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

  /// Used by inverse-sparse encoding to determine Mfv.
#ifdef SHORT_STATES
  int histogram[USHRT_MAX+1];
#else
  int histogram[256];
#endif

protected:
  /// Index to handle mapping, if desired
  heaparray <int> *map;
  int firsthandle;
  int lasthandle;   
  int numstates;  

  /** The workhorse of the whole thing.
      So it better be efficient!
      Pulls off the next \emph{bits} bits from the "bit stream",
      stores the integer result in \emph{x}.
   */
  void ReadInt(char bits, int &x); 

  /** The opposite of ReadInt.
      Writes the integer \emph{x} onto the "bit stream", using
      \emph{bits} bits.
   */
  void WriteInt(char bits, int x);

  /// For debugging.
  void PrintBits(int start, int stop);

  /** Enlarge the memory array, if necessary. */
  void EnlargeMem(int newsize);

public:
  state_array(int bsize, bool useindices);
  ~state_array();

  inline bool UsesIndexHandles() { return (map!=NULL); }
  inline int NumStates() { return numstates; }
  
  int AddState(const state &s);
  bool GetState(int h, discrete_state *s);
  int Compare(int h, discrete_state *s);

  // void Write(ostream &f);

  inline int MaxHandle() { return map ? numstates : lasthandle; }

  inline int FirstHandle() { return map ? 0 : firsthandle; }
  
  int NextHandle(int h); 

  void Report();
  int MemUsed();

  /** Write the array to a stream (not human readable) */
  void Write(ostream &);

  /** Read the array from a stream */
  void Read(istream &);

  // Clear out old states but keep memory allocated.
  void Clear();
};

//@}

#endif

