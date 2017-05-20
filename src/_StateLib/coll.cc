
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "coll.h"

#define UINT32  unsigned int


// #define DEBUG

// #define SLOW_COMPARE_HF

const int MAX_MEM_ADD = 65536;  // 64kb
const int MAX_STATE_ADD = 2048; 


/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }
/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }



// ******************************************************************
// *                      coll_base  methods                        *
// ******************************************************************


coll_base::coll_base() : state_coll()
{
  // start with 256 bytes, in case we're a small set
  mem = (unsigned char*) malloc(256);  
  if (NULL==mem) {
    memsize = 0;
  } else {
    memsize = 256;
  }
}

coll_base::~coll_base()
{
  free(mem);
  mem = NULL;
}

void coll_base::EnlargeMem(long newsize)
{
  if (newsize < 0) {
    throw StateLib::error(StateLib::error::NoMemory);
  }
  if (newsize<memsize)   return;   // easy success
  long newmemsize = memsize ? memsize : MAX_MEM_ADD;
  while (newsize>=newmemsize) {
    if (newmemsize<MAX_MEM_ADD)  
      newmemsize *= 2;
    else  
      newmemsize += MAX_MEM_ADD;
    if (newmemsize <= 0) throw StateLib::error(StateLib::error::NoMemory);
  }
  unsigned char* nmem = (unsigned char*) realloc(mem, newmemsize);
  if (0==nmem) {
    throw StateLib::error(StateLib::error::NoMemory);
  }
  mem = nmem;
  memsize = newmemsize;
}


void coll_base::ReadInt(bitstream &s, char bits, int &x) const
{
  CHECK_RANGE(0, s.byteptr, memsize);
  // read a fraction of a byte
  if (bits <= s.bitptr) {
    x = mem[s.byteptr] & ~(0xFE << s.bitptr);
    s.bitptr -= bits;
    x >>= (1+s.bitptr);
    return;
  }

  // (1) Read the remaining fraction of the first byte.
  x = mem[s.byteptr] & ~(0xFE << s.bitptr);
  bits -= s.bitptr+1;
  s.bitptr = 7;
  s.byteptr++;

  // (2) Read entire bytes for the "middle portion"
  while (bits > 7) {
    x <<= 8;
    x += mem[s.byteptr];
    s.byteptr++;
    bits-=8;
  } // while bits
  if (0==bits) return;

  // (3) Read the fraction of the last byte
  x <<= bits;
  int shift = 8-bits;
  x += (mem[s.byteptr] & (0xFF << shift)) >> shift;
  s.bitptr -= bits;
}


void coll_base::WriteInt(bitstream &s, char bits, int x)
{
  CHECK_RANGE(0, s.byteptr, memsize);
  // can we just stick to the first byte?
  if (bits <= s.bitptr) {
    x <<= (s.bitptr+1-bits);
    if (7 == s.bitptr)  mem[s.byteptr] = x;
    else                mem[s.byteptr] |= x;
    s.bitptr -= bits;
    return;
  }

  // (1) Write the remaining fraction of the first byte
  bits -= (s.bitptr+1);
  if (7 == s.bitptr)  mem[s.byteptr] = (x >> bits);
  else                mem[s.byteptr] |= (x >> bits);
  s.bitptr = 7;
  s.byteptr++;
//  unsigned int mask = 0xFFFFFFFF >> (32-bits);
//  x &= mask;

  // (2) Write entire bytes for the "middle portion"
  while (bits > 7) {
    bits -= 8;
    mem[s.byteptr] = (x >> bits);
//    mask >>= 8;
//    x &= mask;
    s.byteptr++;
  }
  if (0==bits) return;

  // (3) Write the fraction of the last byte
  mem[s.byteptr] = x << (8-bits);
  s.bitptr -= bits;
}

 
void coll_base::DumpMemory(FILE* s, long starth, long stoph) const
{
  for (long h=starth; h<stoph; h++) {
    fprintf(s, "%hhx ", mem[h]);
  }
}



// ******************************************************************
// *                      main_coll  methods                        *
// ******************************************************************

const int main_coll::placelimit[] = 
  { 15, 255, 65535, 16777215, 0x7fffffff };

const char main_coll::placebits[] = 
  { 4, 8, 16, 24, 32 };

const int main_coll::placedim = 5;  // dimension of above two arrays

const int main_coll::tokenlimit[] = 
  { 1, 3, 15, 255, 65535, 16777215, 0x7fffffff };

const char main_coll::tokenbits[] = 
  { 1, 2, 4, 8, 16, 24, 32 };

const int main_coll::tokendim = 7;  // dimension of above two arrays

main_coll::main_coll(bool use_indices, bool use_sizes) : coll_base()
{
  store_sizes = use_sizes;
  if (use_indices) {
    mapsize = 256;
    map = (long*) malloc(mapsize * sizeof(long));
    if (NULL==map) {
      mapsize = 0;
    } else {
      map[0] = 0;
    }
  } else {
    mapsize = 0;
    map = NULL;
  }
  lasthandle = 0;
  firsthandle = -1;
  // stats here
  full_count = sparse_count = 0;
}


main_coll::~main_coll()
{
  free(map);
}

bool main_coll::StateSizesAreStored() const
{
  return store_sizes;
}

bool main_coll::StateHandlesAreIndexes() const
{
  return mapsize>0;
}

void main_coll::Clear()
{
  lasthandle = 0;
  firsthandle = -1;
  numstates = 0;
}

long main_coll::AddState(const int* s, int np)
{
  long answer;
  if (firsthandle<0) firsthandle = lasthandle;
  if (map) {
    if (numstates+1>=mapsize) {
      // Enlarge map array
      if (0==mapsize)                     mapsize = MAX_STATE_ADD;
      else if (mapsize > MAX_STATE_ADD)   mapsize += MAX_STATE_ADD;
      else                                mapsize *= 2;
      map = (long*) realloc(map, mapsize * sizeof(long));
      if (0==map) {
        throw StateLib::error(StateLib::error::NoMemory);
      }
    }
    answer = numstates;
    map[answer] = lasthandle;
  } else {
    answer = lasthandle;
  }
  // 
  // Scan state for max #tokens and number of nonzeroes 
  //
  int i;
  DCASSERT(np);        // can we have 0-length states?
  int maxval = 0;
  int nnz = 0;
  int lastnz = 0;
  for (i = 0; i < np; i++) {
    if (s[i]) {
      maxval = MAX(maxval, s[i]);
      nnz++;
      lastnz = i;
    }
  }
  //  
  //  Determine number of bits to use for "places"
  //
  int max_index = (store_sizes) ? np : lastnz;
  char npselect;
  for (npselect=0; npselect<placedim; npselect++) 
    if (max_index <= placelimit[int(npselect)]) break;
  DCASSERT(npselect<8); // 3 bits available
  char npbits = placebits[int(npselect)];

  //  
  //  Determine number of bits to use for "tokens"
  //
  char tkselect;
  for (tkselect=0; tkselect<tokendim; tkselect++) 
    if (maxval <= tokenlimit[int(tkselect)]) break;
  DCASSERT(tkselect<8); // 3 bits available
  char tkbits = tokenbits[int(tkselect)];

  //
  // Count bits/bytes required total for full storage
  //
  int full_bits = npbits + (1+lastnz)*tkbits;
  if (store_sizes) full_bits += npbits;  

  //
  // Count bits/bytes required total for sparse storage
  //
  int sparse_bits = (1+nnz) * npbits;
  if (tkbits>1) sparse_bits += nnz*tkbits;
  if (store_sizes) sparse_bits += npbits;

  //
  // Determine best encoding with tie preference to truncated full.
  // Also, make room for the state.
  //
  long   full_bytes = Bits2Bytes(full_bits);
  long sparse_bytes = Bits2Bytes(sparse_bits);
  
  char encoding;
  if (sparse_bytes < full_bytes) {
    encoding = ENCODING_SPARSE;
    EnlargeMem(lasthandle + 1 + sparse_bytes);
  } else {
    encoding = ENCODING_FULL;
    EnlargeMem(lasthandle + 1 + full_bytes);
  }
 
  //
  // Initialize bitstream
  // 
  bitstream bs(lasthandle);

  // 
  // Write the state code
  //
  WriteInt(bs, 2, encoding);
  WriteInt(bs, 3, npselect);
  WriteInt(bs, 3, tkselect);

  //
  // Write the size (if necessary)
  //
  if (store_sizes) WriteInt(bs, npbits, np);

  // 
  // Write the state using the appropriate encoding
  //
  switch (encoding) {
    case ENCODING_FULL:
      // *****************  Truncated Full Encoding 
      WriteInt(bs, npbits, lastnz);
      for (i=0; i<=lastnz; i++) {
        WriteInt(bs, tkbits, s[i]);
      }
      full_count++;
    break;

    case ENCODING_SPARSE: 
      // *****************  Sparse Encoding 
      WriteInt(bs, npbits, nnz);
      for (i=0; i<=lastnz; i++) {
        if (s[i]) {
          WriteInt(bs, npbits, i);
          if (tkbits>1) WriteInt(bs, tkbits, s[i]);
        }
      }
      sparse_count++;
    break;

    default:
      throw StateLib::error(StateLib::error::Internal);
  }
  
#ifdef DEBUG
  printf("Encoded state as: ");
  DumpMemory(lasthandle, bs.NextByte());
  printf("\n");
#endif
  // 
  // Remember where to start for the next state
  // 
  lasthandle = bs.NextByte();
  numstates++;
  if (map) map[numstates] = lasthandle;
  return answer;
}


bool main_coll::PopLast(long hndl)
{
  if (hndl < 0) return false;
  long h;
  if (map) {
    if (hndl >= numstates) return false;
    h = map[hndl];
  } else {
    if (hndl >= lasthandle) return false;
    h = hndl;
  }

  // decrement encoding count...
  bitstream bs(h);
  int encselect;
  ReadInt(bs, 2, encselect);
  switch(encselect) {
    case ENCODING_FULL:
        full_count--;
        break;

    case ENCODING_SPARSE: 
        sparse_count--;
        break;

    default:
        return false;  // bad handle?
  } // switch

  // remove the state
  lasthandle = h;
  numstates--;
  if (map) {
    DCASSERT(map[numstates] == lasthandle);
  }
  return true;
}

long main_coll::GetStateKnown(long hndl, int* s, int size) const
{
  long h;
  if (hndl<0) throw StateLib::error(StateLib::error::BadHandle);
  if (map) {
    if (hndl >= numstates) throw StateLib::error(StateLib::error::BadHandle);
    h = map[hndl];
  } else {
    if (hndl >= lasthandle) throw StateLib::error(StateLib::error::BadHandle);
    h = hndl;
  }
  bitstream bs(h);

  // 
  // Read the state code
  //
  int encselect;
  int npselect;
  int tkselect;
  ReadInt(bs, 2, encselect);
  ReadInt(bs, 3, npselect);
  ReadInt(bs, 3, tkselect);
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Read the size, if necessary
  // 
  if (store_sizes) {
    int actualsize;
    ReadInt(bs, npbits, actualsize);
    if (actualsize != size) return -1;  // size mismatch
  }

  // 
  // Decode the state
  //
  int np, nnz;
  int i, z, stop;

  switch(encselect) {
    case ENCODING_FULL:
      // *****************  Full Encoding 
      ReadInt(bs, npbits, np);
      np++;
      stop = MIN(np, size);
      for (i=0; i<stop; i++) {
        ReadInt(bs, tkbits, s[i]);
      }
      // size < np, advance thingy
      // for (; i<np; i++)  bs.Advance(tkbits);
      bs.Advance(tkbits * (np-i));
      // np < size, fill with zeroes
      for (; i<size; i++) s[i] = 0;
    break;

    case ENCODING_SPARSE: 
      // *****************  Sparse Encoding 
      ReadInt(bs, npbits, nnz);
      i = 0;
      for (z=0; z<nnz; z++) {
        ReadInt(bs, npbits, np);
        // fill in zeroes before np
        stop = MIN(np, size);
        for (; i<stop; i++) s[i] = 0;
        if (tkbits>1) {
          if (np < size) ReadInt(bs, tkbits, s[np]);
          else           bs.Advance(tkbits);
        } else {
          if (np < size) s[np] = 1;
        }
        i = np+1;
      } // for z
      for (; i<size; i++) s[i] = 0;
    break;

    default:
      return -2;

  }; // switch

  if (map) {
    hndl++;
    if (hndl == numstates) return 0;
    return hndl;
  } 
  return bs.NextByte();
}


int main_coll::GetStateUnknown(long hndl, int* s, int size) const
{
  if (!store_sizes) return -1;
  long h;
  if (hndl<0) throw StateLib::error(StateLib::error::BadHandle);
  if (map) {
    if (hndl >= numstates) throw StateLib::error(StateLib::error::BadHandle);
    h = map[hndl];
  } else {
    if (hndl >= lasthandle) throw StateLib::error(StateLib::error::BadHandle);
    h = hndl;
  }
  bitstream bs(h);

  // 
  // Read the state code
  //
  int encselect;
  int npselect;
  int tkselect;
  ReadInt(bs, 2, encselect);
  ReadInt(bs, 3, npselect);
  ReadInt(bs, 3, tkselect);
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Read the size
  // 
  int actualsize;
  ReadInt(bs, npbits, actualsize);
  if (size < 1) return actualsize;  // no point decoding...

  // 
  // Decode the state
  //
  int np, nnz;
  int i, z, stop;

  switch(encselect) {
    case ENCODING_FULL:
      // *****************  Full Encoding 
      ReadInt(bs, npbits, np);
      stop = MIN(np+1, size);
      for (i=0; i<stop; i++) {
        ReadInt(bs, tkbits, s[i]);
      }
      stop = MIN(actualsize, size);
      for (; i<stop; i++) s[i] = 0;
    break;

    case ENCODING_SPARSE: 
      // *****************  Sparse Encoding 
      ReadInt(bs, npbits, nnz);
      i = 0;
      for (z=0; z<nnz; z++) {
        ReadInt(bs, npbits, np);
        // fill in zeroes before np
        stop = MIN(np, size);
        for (; i<stop; i++) s[i] = 0;
        if (np >= size) return actualsize;
        if (tkbits>1) {
          ReadInt(bs, tkbits, s[np]);
        } else {
          s[np] = 1;
        }
        i = np+1;
      }
      stop = MIN(actualsize, size);
      for (; i<stop; i++) s[i] = 0;
    break;

    default:
      return -2;

  };

  return actualsize;
}


const unsigned char* main_coll::GetRawState(long hndl, long &bytes) const
{
  long h;
  if (hndl<0) return 0;
  if (map) {
    if (hndl >= numstates) return 0;
    h = map[hndl];
  } else {
    if (hndl >= lasthandle) return 0;
    h = hndl;
  }

  bytes = NextRawHandle(h);
  bytes -= h;
  
  return GetPtr(h);
}

long main_coll::FirstHandle() const 
{
  return map ? 0 : firsthandle;
}

long main_coll::NextHandle(long hndl) const
{
  if (map) {
    // handles are indices
    if (hndl>=numstates) return -1;
    return hndl+1; 
  }
  if (hndl>=lasthandle) return -1;
  return NextRawHandle(hndl);
}

long main_coll::NextRawHandle(long hndl) const
{
  // 
  // Take care of easy cases first
  //
  if (hndl>=lasthandle) return -1;
  
  bitstream bs(hndl);

  // 
  // Read state encoding method
  // 
  int encselect;
  int npselect;
  int tkselect;
  ReadInt(bs, 2, encselect);
  ReadInt(bs, 3, npselect);
  ReadInt(bs, 3, tkselect);

  // Figure out the numbers of bits used
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Skip the actual size, if necessary.
  //
  if (store_sizes) {
    bs.Advance(npbits);
  }

  //
  // Determine state length (#bits), reading only as much as necessary
  //
  int np, nnz;
  // stats for debugging
  switch(encselect) {
    case ENCODING_FULL:
      // *****************  Full Encoding 
      ReadInt(bs, npbits, np);
      np++;
      bs.Advance(np * tkbits);
    break;

    case ENCODING_SPARSE: 
      // *****************  Sparse Encoding 
      ReadInt(bs, npbits, nnz);
      if (tkbits==1) tkbits=0;  // #tokens never part of state
      bs.Advance(nnz * (npbits + tkbits));
    break;

    default:
        return -2;  // shouldn't get here
  } // switch

  return bs.NextByte();
}


int main_coll::CompareHH(long h1, long h2) const
{
  if (NULL==map) return 0;
  if ((h1>=numstates) || (h2>=numstates)) return 0; 
  if ((h1<0) || (h2<0)) return 0; 

  // compare by length first, then in case of a tie,
  // check the actual encodings.
  int length1 = map[h1+1] - map[h1];
  int length2 = map[h2+1] - map[h2];
  int foo = length1 - length2;
  if (foo!=0) return foo;
  // same size, check bytes
  // doesn't get much faster than this:
  int cmp = memcmp(GetPtr(map[h1]), GetPtr(map[h2]), length1); 
  // don't use strncmp, because if ptr1 and ptr2 contain '0', comparison stops!
  return cmp;
}


int main_coll::CompareHF(long hndl, int size, const int* state) const
{
#ifdef SLOW_COMPARE_HF
  int* hstate = new int[size];
  GetStateKnown(hndl, hstate, size); 
  int cmp = memcmp(hstate, state, size*sizeof(int));
  delete[] hstate;
  return cmp;
#else
  long h;
  if (hndl<0) throw StateLib::error(StateLib::error::BadHandle);
  if (map) {
    if (hndl >= numstates) throw StateLib::error(StateLib::error::BadHandle);
    h = map[hndl];
  } else {
    if (hndl >= lasthandle) throw StateLib::error(StateLib::error::BadHandle);
    h = hndl;
  }
  bitstream bs(h);

  // 
  // Read the state code
  //
  int encselect;
  int npselect;
  int tkselect;
  ReadInt(bs, 2, encselect);
  ReadInt(bs, 3, npselect);
  ReadInt(bs, 3, tkselect);
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Read the size, if necessary
  // 
  int actualsize = -1;
  if (store_sizes) {
    ReadInt(bs, npbits, actualsize);
  }

  // 
  // Decode the state, compare with the other
  //
  int tk=0;
  int np=0, nnz;
  int i, z, stop;

  switch(encselect) {
    case ENCODING_FULL:
      // *****************  Full Encoding 
      ReadInt(bs, npbits, np);
      np++;
      stop = MIN(np, size);
      for (i=0; i<stop; i++) {
        ReadInt(bs, tkbits, tk);
        if (tk != state[i]) return (tk - state[i]);
      }
      if (np>size) return 1;
      // still here? we must have np <= size.
      for (; i<size; i++) if (state[i]) return -1;
      // we match up to size.
      // If the actual size is known, then compare those;
      // otherwise, consider us equal.
      if (store_sizes) return (actualsize - size);
      return 0;
    break;

    case ENCODING_SPARSE: 
      // *****************  Sparse Encoding 
      ReadInt(bs, npbits, nnz);
      i = 0;
      for (z=0; z<nnz; z++) {
        ReadInt(bs, npbits, np);
        // compare with zeroes before np
        stop = MIN(np, size);
        for (; i<stop; i++) {
          if (state[i]) return -1;
        }
        if (np+1>size) return 1;
        if (tkbits>1) {
          ReadInt(bs, tkbits, tk);
        } else {
          tk = 1;
        }
        if (tk != state[np]) return (tk - state[np]);
        i = np+1;
      } // for z
      if (np>size) return 1;
      for (; i<size; i++) if (state[i]) return -1;
      // we match up to size.
      // If the actual size is known, then compare those;
      // otherwise, consider us equal.
      if (store_sizes) return (actualsize - size);
      return 0;
    break;

  };

  // Still here? Error!
  throw StateLib::error(StateLib::error::Internal);
#endif
}

#define rot(x,k) (((x)<<(k)) | ((x)>>(32-(k))))

inline void mix(UINT32 &a, UINT32 &b, UINT32 &c)
{
  a -= c;  a ^= rot(c, 4);  c += b; 
  b -= a;  b ^= rot(a, 6);  a += c; 
  c -= b;  c ^= rot(b, 8);  b += a; 
  a -= c;  a ^= rot(c,16);  c += b; 
  b -= a;  b ^= rot(a,19);  a += c; 
  c -= b;  c ^= rot(b, 4);  b += a; 
}

inline void final(UINT32 &a, UINT32 &b, UINT32 &c)
{
  c ^= b; c -= rot(b,14);
  a ^= c; a -= rot(c,11);
  b ^= a; b -= rot(a,25);
  c ^= b; c -= rot(b,16);
  a ^= c; a -= rot(c, 4); 
  b ^= a; b -= rot(a,14);
  c ^= b; c -= rot(b,24);
}

unsigned long main_coll::Hash(long hndl, int bits) const
/*
  Hash function based on lookup3.c, by Bob Jenkins, May 2006, Public Domain.
  See http://burtleburtle.net/bob/c/lookup3.c
*/
{
  if (0==map) return 0;
  if ((hndl>=numstates) || (hndl<0)) return 0;
  if (0==bits) return 0;

  long length = map[hndl+1] - map[hndl];
  const unsigned char* k = GetPtr(map[hndl]);
  bool needs_final = true;

  UINT32 a, b, c; /* internal state */
  a = b = c = 0xdeadbeef;

  /*--------------- all but the last block: affect some 32 bits of (a,b,c) */
  while (length > 12) {
    a += k[0];
    a += ((UINT32)k[1])<<8;
    a += ((UINT32)k[2])<<16;
    a += ((UINT32)k[3])<<24;
    b += k[4];
    b += ((UINT32)k[5])<<8;
    b += ((UINT32)k[6])<<16;
    b += ((UINT32)k[7])<<24;
    c += k[8];
    c += ((UINT32)k[9])<<8; 
    c += ((UINT32)k[10])<<16; 
    c += ((UINT32)k[11])<<24; 
    mix(a, b, c); 
    length -= 12; 
    k += 12; 
  }

  /*-------------------------------- last block: affect all 32 bits of (c) */ 
  switch(length) {                 /* all the case statements fall through */
    case 12:  c+=((UINT32)k[11])<<24; 
    case 11:  c+=((UINT32)k[10])<<16; 
    case 10:  c+=((UINT32)k[9])<<8; 
    case 9 :  c+=k[8]; 
    case 8 :  b+=((UINT32)k[7])<<24; 
    case 7 :  b+=((UINT32)k[6])<<16; 
    case 6 :  b+=((UINT32)k[5])<<8; 
    case 5 :  b+=k[4]; 
    case 4 :  a+=((UINT32)k[3])<<24; 
    case 3 :  a+=((UINT32)k[2])<<16; 
    case 2 :  a+=((UINT32)k[1])<<8; 
    case 1 :  a+=k[0]; 
              break; 
    case 0 :  needs_final = false;  /* zero length strings require no mixing */ 
  } 

  if (needs_final) final(a, b, c);

  // answer is b:c, rightmost bits.
  if (bits < 32) {
    unsigned int mask = (UINT32)1 << bits;
    mask -= 1;
    return c & mask;
  }
  if (bits == 32) return c;
  unsigned int mask = (UINT32)1 << (bits-32);
  mask -= 1;
  unsigned long upper = b & mask;
  return (upper << 32) + c;
}

long* main_coll::RemoveIndexHandles()
{
  long* answer;
  if (mapsize > numstates) {
    answer = (long*) realloc(map, numstates * sizeof(long));
  } else {
    answer = map;
  }
  map = 0;
  mapsize = 0;
  return answer;
}

int main_coll::NumEncodingMethods() const
{
  return 2;  // full and sparse
}

const char* main_coll::EncodingMethod(int m) const
{
  switch (m) {
    case ENCODING_FULL:   return "Truncated Full";
    case ENCODING_SPARSE: return "Sparse";
  };
  return NULL;
}

long main_coll::ReportEncodingCount(int m) const
{
  switch (m) {
    case ENCODING_FULL:   return full_count;
    case ENCODING_SPARSE: return sparse_count;
  };
  return 0;  
}

long main_coll::ReportMemTotal() const 
{
  long answer = 0;
  if (map) answer += mapsize * sizeof(long);
  answer += lasthandle;
  return answer;
}


