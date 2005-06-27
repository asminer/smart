
// $Id$

#include "flatss.h"

#include "../Base/memtrack.h"

/** @name states.cc
    @type File
    @args \ 

  Working on a new and efficient way to store states.
    
 */

// #define DEBUG
// #define DEBUG_COMPACT
// #define PRINT_BITS_BINARY

//@{ 

// ==================================================================
// ||                                                              ||
// ||                     state_array  methods                     ||
// ||                                                              ||
// ==================================================================

const unsigned char frommask[8] = 
	{0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x7F, 0xFF};

const unsigned char tomask[8] = 
	{0xFF, 0xFE, 0xFC, 0xF8, 0xF0, 0xE0, 0xC0, 0x80};

const unsigned long int twoexp[32] = {
	0x00000001, 0x00000002, 0x00000004, 0x00000008, 
	0x00000010, 0x00000020, 0x00000040, 0x00000080,
	0x00000100, 0x00000200, 0x00000400, 0x00000800,
	0x00001000, 0x00002000, 0x00004000, 0x00008000,
	0x00010000, 0x00020000, 0x00040000, 0x00080000,
	0x00100000, 0x00200000, 0x00400000, 0x00800000,
	0x01000000, 0x02000000, 0x04000000, 0x08000000,
	0x10000000, 0x20000000, 0x40000000, 0x80000000
};

const char* EncodingMethod[4] = 
	{ "deleted state", "sparse", "runlength", "full" };

const char LIST_BIT = 0;
const char RUN_BIT = 1;

const unsigned placelimit[] = { 16, 256, 65536, 16777216, 0xffffffff };
const char placebits[] = { 4, 8, 16, 24, 32 };

const unsigned tokenlimit[] = { 2, 4, 16, 256, 65536, 16777216, 0xffffffff };
const char tokenbits[] = { 1, 2, 4, 8, 16, 24, 32 };

const int MAX_MEM_ADD = 65536;	// 64kb
const int MAX_STATE_ADD = 2048; 

void state_array::ReadInt(char bits, int &x)
{
  int toptr = bitptr-bits+1;
  if (toptr>=0) {
    // Fairly easy case: we don't cross a byte boundary
    CHECK_RANGE(0, byteptr, memsize);
    x = mem[byteptr] & frommask[bitptr] & tomask[toptr];
    if (toptr) {
      // we have to shift everything
      x>>=toptr;
      bitptr = toptr-1;
    } else {
      // we don't have to shift
      bitptr = 7;
      byteptr++;
    }
    return;
  }
  
  // Most general case: we cross a byte boundary.
  // First: get the rest of the bits from this byte.
  CHECK_RANGE(0, byteptr, memsize);
  x = mem[byteptr] & frommask[bitptr];
  bits -= (bitptr+1);
  bitptr = 7;
  byteptr++;
  // Get the rest of the bits
  while (bits) {
    CHECK_RANGE(0, byteptr, memsize);
    if (bits<8) {
      // We only have part of a byte left
      x<<=bits;
      toptr = bitptr-bits+1;
      x += ( mem[byteptr] & tomask[toptr] ) >> toptr;
      bitptr = toptr-1;
      return;
    }
    // we need to consume an entire byte
    x *= 256; 
    x += mem[byteptr];
    byteptr++;
    bits-=8;
  }
}

void state_array::WriteInt(char bits, int x)
{
  // clear out the rest of the bits of this byte
  CHECK_RANGE(0, byteptr, memsize);
  if (bitptr<7) {
    mem[byteptr] &= tomask[bitptr+1];
  } else {
    mem[byteptr] = 0;
  }

  int toptr = bitptr-bits+1;
  if (toptr>=0) {
    // Fairly easy case: we don't cross a byte boundary
    if (toptr) {
      // shift x
      x <<= toptr;
      mem[byteptr] |= x;
      bitptr = toptr-1;
    } else {
      mem[byteptr] |= x;
      bitptr = 7;
      byteptr++;
    }
    return;
  }

  // If we're here, we cross a byte boundary.
  
  // First: fill the rest of the bits from this byte.
  bits -= (bitptr+1);
  unsigned char chunk = x / twoexp[int(bits)];
  x %= twoexp[int(bits)];
  mem[byteptr] |= chunk;
  bitptr = 7;
  byteptr++;
  // Do the rest of the bits
  while (bits) {
    CHECK_RANGE(0, byteptr, memsize);
    mem[byteptr] = 0;
    if (bits<8) {
      // We only have part of a byte left
      x<<=(8-bits);
      mem[byteptr] = x;
      bitptr = 7-bits;
      return;
    }
    // we write an entire byte
    bits -= 8;
    if (bits) {
      chunk = x / twoexp[int(bits)];
      x %= twoexp[int(bits)];
      mem[byteptr] = chunk;
    } else {
      mem[byteptr] = x;
    }
    byteptr++;
  }
}

void state_array::PrintBits(OutputStream &s, int start, int stop) const
{
#ifdef PRINT_BITS_BINARY
  int i, b;
  for (i=start; i<stop; i++) {
    s << "|";
    for (b=7; b>=0; b--) {
      CHECK_RANGE(0, i, memsize);
      if (mem[i] & frommask[b] & tomask[b]) 
	s << "1"; else s << "0";
    }
  }
#else
  s.Put("[");
  s.PutArray(mem+start, stop-start);
  s.Put("]");
#endif
}

void state_array::EnlargeMem(int newsize)
{
  if (newsize<memsize) return;
  int newmemsize = memsize;
  while (newsize>=newmemsize) {
    if (newmemsize<MAX_MEM_ADD)  
      newmemsize *= 2;
    else  
      newmemsize += MAX_MEM_ADD;
  }
  mem = (unsigned char*) realloc(mem, newmemsize);
  if (NULL==mem) OutOfMemoryError("state array resize");
  memsize = newmemsize;
}

state_array::state_array(bool useindices)
{
  // start with 256 bytes, in case we're a small set
  mem = (unsigned char*) malloc(256);  
  if (NULL==mem) OutOfMemoryError("new state array");
  memsize = 256;

  if (useindices) {
    mapsize = 256;
    map = (int*) malloc(mapsize * sizeof(int));
    if (NULL==map) OutOfMemoryError("new state array");
    map[0] = 0; // initialize "first" handle
  } else {
    mapsize = 0;
    map = NULL;
  }
  bitptr = 7;
  byteptr = 0;
  numstates = 0;
  lasthandle = 0;
  firsthandle = -1;
  // stats
  encodecount[0] = encodecount[1] = encodecount[2] = encodecount[3] = 0;
}

state_array::~state_array()
{
  free(map);
  free(mem); mem = NULL;
}


// Helper function for counting bits for runlength encoding.
// This one will bail out if we exceed minbits.
// Assumes a non-binary state
inline int RunlengthBits(char npbits, char tkbits, const state &s, 
				int minbits, int &items)
{
  // state is not binary
  int bits = npbits; // first we store number of runs+lists
  int i = 0;
  int np = s.Size();
  items = 0;
  while (i<np) {
    if (bits >= minbits) return bits+1;  // no point continuing
    if (i+1 >= np) {
     	// one last state var, treat as a list of length 1
	items++;
	bits+= (1+npbits+tkbits);
        i++;
        continue;
    }
    if (s.Read(i).ivalue == s.Read(i+1).ivalue) {
 	// Start of a RUN
	items++;
	bits++;
	int j;
	for (j=i+1; j<np && s.Read(i).ivalue == s.Read(j).ivalue; j++) { } 
	// j is one past the end of the run
	bits+=(npbits+tkbits);
 	i = j;
     	continue;
    }
    // Must be a LIST
    bits++;
    items++;
    if (i+2 >= np) {
     	// two last state vars, treat as a list of length 2
	bits += npbits + 2*tkbits;
        i+=2;
        continue;
    }
    int j;
    // find end of list
    for (j=i+1; j<np; j++) {
      if (j+2 >= np) {
	j=np;
	break;
      } 
      if (s.Read(j).ivalue != s.Read(j+1).ivalue) continue;
      if (s.Read(j).ivalue == s.Read(j+2).ivalue) break;  // run of 3
    } // for j
    // j is one past the end of the list
    bits += npbits + (j-i)*tkbits;
  } // while i < np
  return bits;
}

void state_array::RunlengthEncode(char npbits, char tkbits, const state &s)
{
  // state is not binary
  int i = 0;
  int np = s.Size();
  while (i<np) {
    if (i+1 >= np) {
     	// one last state var, treat as a list of length 1
	WriteInt(1, LIST_BIT);
	WriteInt(npbits, 0);  // length-1
	WriteInt(tkbits, s.Read(i).ivalue);
        i++;
        continue;
    }
    if (s.Read(i).ivalue == s.Read(i+1).ivalue) {
 	// Start of a RUN
	WriteInt(1, RUN_BIT); 
	int j;
	for (j=i+1; j<np && s.Read(i).ivalue == s.Read(j).ivalue; j++) { } 
	// j is one past the end of the run
  	WriteInt(npbits, (j-i)-1);    	// run length-1
	WriteInt(tkbits, s.Read(i).ivalue);	// run value
 	i = j;
     	continue;
    }
    // Must be a LIST
    WriteInt(1, LIST_BIT);	
    if (i+2 >= np) {
     	// two last state vars, treat as a list of length 2
	WriteInt(npbits, 1);  // length-1
	WriteInt(tkbits, s.Read(i).ivalue);
	WriteInt(tkbits, s.Read(i+1).ivalue);
        i+=2;
        continue;
    }
    int j;
    // find end of list
    for (j=i+1; j<np; j++) {
      if (j+2 >= np) {
	j=np;
	break;
      } 
      if (s.Read(j).ivalue != s.Read(j+1).ivalue) continue;
      if (s.Read(j).ivalue == s.Read(j+2).ivalue) break;  // run of 3
    } // for j
    // j is one past the end of the list
    WriteInt(npbits, (j-i)-1);	// list length-1
    for (; i<j; i++)
      WriteInt(tkbits, s.Read(i).ivalue);
  } // while i < np
}

// Helper function for counting bits for runlength encoding.
// This one will bail out if we exceed minbits.
// The state is binary (0 and 1 state vars only)
inline int RunlengthBitsBinary(char npbits, const state &s, 
					int minbits, int& items)
{
  // special case: state is binary
  int bits = 1+npbits; // "store" number of runs+lists, and initial state var
  int i = 0;
  int np = s.Size();
  items = 0;
  while (i<np) {
    if (bits >= minbits) return bits+1;  // no point continuing
    if (i+1 >= np) {
     	// one last state var, treat as a flip of length 1
	items++;
	bits += 1+npbits;
        i++;
        continue;
    }
    if (s.Read(i).ivalue == s.Read(i+1).ivalue) {
 	// Start of a RUN
	items++;
	bits++;
	int j;
	for (j=i+1; j<np && s.Read(i).ivalue == s.Read(j).ivalue; j++) { } 
	// j is one past the end of the run
	bits += npbits;
 	i = j;
     	continue;
    }
    // Must be a LIST, in this case, a sequence of FLIPS
    bits++;
    items++;
    int j;
    // find end of list
    for (j=i+1; j<np; j++) {
      if (j+1 >= np) {
	j=np;
	break;
      } 
      if (s.Read(j).ivalue == s.Read(j+1).ivalue) break;
    } // for j
    // j is one past the end of the list
    bits += npbits;
    i = j;
  } // while i<np
  return bits;
}
 
void state_array::RunlengthEncodeBinary(char npbits, const state &s)
{
  // special case: state is binary
  int i = 0;
  int np = s.Size();
  // write the first bit value
  WriteInt(1, s.Read(0).ivalue);
  while (i<np) {
    if (i+1 >= np) {
     	// one last state var, treat as a flip of length 1
	WriteInt(1, LIST_BIT);
	WriteInt(npbits, 0);  // list length-1
        i++;
        continue;
    }
    if (s.Read(i).ivalue == s.Read(i+1).ivalue) {
 	// Start of a RUN
	WriteInt(1, RUN_BIT); 
	int j;
	for (j=i+1; j<np && s.Read(i).ivalue == s.Read(j).ivalue; j++) { } 
	// j is one past the end of the run
  	WriteInt(npbits, (j-i)-1);    	// run length-1
 	i = j;
     	continue;
    }
    // Must be a LIST, in this case, a sequence of FLIPS
    WriteInt(1, LIST_BIT);	
    int j;
    // find end of list
    for (j=i+1; j<np; j++) {
      if (j+1 >= np) {
	j=np;
	break;
      } 
      if (s.Read(j).ivalue == s.Read(j+1).ivalue) break;
    } // for j
    // j is one past the end of the list
    WriteInt(npbits, (j-i)-1);	// list length-1 
    i = j;
  } // while i<np
}
 
int state_array::AddState(const state &s)
{
  int answer;
  if (firsthandle<0) firsthandle = lasthandle;
  if (map) {
    if (numstates+1>=mapsize) {
      // Enlarge map array
      mapsize = MIN(2*mapsize, mapsize+MAX_STATE_ADD);
      DCASSERT(mapsize>=0);
      map = (int*) realloc(map, mapsize * sizeof(int));
      if (NULL==map) OutOfMemoryError("too many states");
    }
    answer = numstates;
  } else {
    answer = lasthandle;
  }
  // 
  // Scan state for max #tokens and number of nonzeroes 
  //
  unsigned i;
  unsigned np = s.Size();
  DCASSERT(np);        // can we have 0-length states?
  unsigned maxval;
  int nnz;
  nnz = maxval = 0;
  for (i = 0; i < np; i++) {
    // verify the state vars are legal
    DCASSERT(s.Read(i).isNormal());
    unsigned si = s.Read(i).ivalue;
    maxval = MAX(maxval, si);
    if (si) nnz++;
  }
  //  
  //  Determine number of bits to use for "places"
  //
  char npselect;
  for (npselect=0; npselect<5; npselect++) 
    if (np <= placelimit[int(npselect)]) break;
  DCASSERT(npselect<5);
  char npbits = placebits[int(npselect)];

  //  
  //  Determine number of bits to use for "tokens"
  //
  char tkselect;
  for (tkselect=0; tkselect<7; tkselect++) 
    if (maxval < tokenlimit[int(tkselect)]) break;
  DCASSERT(tkselect<7);
  char tkbits = tokenbits[int(tkselect)];

  //
  // Count bits/bytes required total for full storage
  //
  int full_bytes = Bits2Bytes(npbits + np*tkbits);

  //
  // Count bits/bytes required total for sparse storage
  //
  int sparse_bits = (1+nnz) * npbits;
  if (tkbits>1) sparse_bits += nnz*tkbits;
  int sparse_bytes = Bits2Bytes(sparse_bits);

  // if runlength requires more than this, don't use it
  int minbits = (MIN(sparse_bytes, full_bytes)-1)*8;

  //
  // Scan and count bits required total for runlength storage
  //
  int items;
  int runlength_bits;
  if (maxval == 1) // binary states
    runlength_bits = RunlengthBitsBinary(npbits, s, minbits, items);
  else
    runlength_bits = RunlengthBits(npbits, tkbits, s, minbits, items);
  int runlength_bytes = Bits2Bytes(runlength_bits);

  //
  // Determine best encoding with tie preference to full / sparse
  //
  int min = MIN(MIN(sparse_bytes, full_bytes), runlength_bytes);
  char encoding;
  if (min==full_bytes) 
  	encoding = 3; 
  else if (min==sparse_bytes)
    	encoding = 1;
  else  
    	encoding = 2;
  encodecount[int(encoding)]++;

  //
  // Make room for this state
  //
  EnlargeMem(lasthandle + 1+min);
 
  //
  // Initialize bitstream
  // 
  byteptr = lasthandle;
  bitptr = 7;

  // 
  // Write the state code
  //
  WriteInt(2, encoding);
  WriteInt(3, npselect);
  WriteInt(3, tkselect);

  // 
  // Write the state using the appropriate encoding
  //
  switch (encoding) {
    case 1: 
      // *****************	Sparse Encoding 
      WriteInt(npbits, nnz);
      for (i=0; i<np; i++) {
        int tk = s.Read(i).ivalue;
        if (tk) {
          WriteInt(npbits, i);
          if (tkbits>1) WriteInt(tkbits, tk);
        }
      }
      break;

    case 2: 
      // *****************	Runlength Encoding 
      WriteInt(npbits, items);
      if (tkbits>1) RunlengthEncode(npbits, tkbits, s);
      else RunlengthEncodeBinary(npbits, s);
      break;

    case 3:
      // *****************	Full Encoding 
      WriteInt(npbits, (np-1));
      for (i=0; i<np; i++) 
	WriteInt(tkbits, s.Read(i).ivalue);
    break;

    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad state encoding?\n";
	Internal.Stop();
  }
  
  // 
  // Get bitstream ready for next time
  // 
  if (bitptr<7) byteptr++;
  lasthandle = byteptr;
  numstates++;
  map[numstates] = lasthandle;  // keeps track of this state "size"

#ifdef  DEBUG_COMPACT
  Output << "Added state #" << numstates-1 << " : " << s << "\n";
  int Handel = (map) ? map[numstates-1] : answer;
  Output << "\thandle: " << Handel << "\n";
  Output << "\tnexthandle: " << lasthandle << "\n";
  Output << "\t#places bits: " << (int) npbits;
  Output << "\t#tokens bits: " << (int) tkbits << "\n";
  Output << "\tUsed " << EncodingMethod[encoding] << " storage\n";
/*
  switch (encoding) {
    case 1: 
      Output << "#nonzeroes: " << nnz << "\n"; 
      break;
    case 2:
      Output << "#Runs: " << runs << "\n";
      Output << "#Lists: " << lists << "\n";
      Output << "List length: " << listlengths << "\n";
      break;
    case 3:
      Output << "#places: " << (int) np << "\n";
      break;
  }
*/
  Output << "Encoding:\n";
  PrintBits(Output, Handel, lasthandle);
  Output << "\n";
  Output.flush();
#endif
  
  return answer;
}

bool state_array::GetState(int Handel, state &s)
{
  //
  //  Find starting point of the state represented by Handel
  //
  int h;
  if (Handel<0) return false;  // Impossible, bail out
  if (map) {
    if (Handel>=numstates) return false;
    h = map[Handel];
  } else {
    if (Handel>=lasthandle) return false;
    h = Handel;
  }
  
  // 
  // Initialize bitstream and read state encoding method
  // 
  byteptr = h;
  bitptr = 7;
  int encselect;
  ReadInt(2, encselect);
  int npselect;
  ReadInt(3, npselect); 	// #places selector
  int tkselect;
  ReadInt(3, tkselect);  	// maxtokens selector

  // Figure out the numbers of bits used
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Decode the state based on the encoding method
  //
  int tk, np, nnz;
  int i, j, type;
  // stats for debugging
#ifdef DEBUG_COMPACT
  int lists, runs, listlength;
  lists = runs = listlength = 0;
#endif

  for (i=s.Size()-1; i>=0; i--) s[i].Clear();

  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      for (i=s.Size()-1; i>=0; i--) s[i].ivalue = 0; 
      ReadInt(npbits, nnz);
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
	if (tkbits>1) {
	  ReadInt(tkbits, tk);
	  s[np].ivalue = tk;
	} else 
	  s[np].ivalue = 1;
      }
      break;
      
    case 2: 
      // *****************	Runlength Encoding 
      ReadInt(npbits, nnz); 
      j=0;
      if (1==tkbits) ReadInt(1, tk);
      for (i=0; i<nnz; i++) {
        ReadInt(1, type);
        if (RUN_BIT == type) {
	  // RUN
#ifdef DEBUG_COMPACT
	  runs++;
#endif
          ReadInt(npbits, np);
	  np++;
	  if (tkbits>1) ReadInt(tkbits, tk);
          for(; np; np--) 
	    s[j++].ivalue = tk;
	  // flip bit
	  tk = !tk;
        } else {
	  // LIST
	  ReadInt(npbits, np);
	  np++;
#ifdef DEBUG_COMPACT
	  lists++;
	  listlength+=np;
#endif
	  for(; np; np--) {
	    if (tkbits>1) ReadInt(tkbits, tk);
	    s[j++].ivalue = tk;
	    if (tkbits<=1) tk = !tk;
          }
        }
      } // for i
      break;
      
    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      for (i=0; i<np; i++) {
	ReadInt(tkbits, tk);
	s[i].ivalue = tk;
      }
    break;

    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad state encoding?\n";
	Internal.Stop();
      	return false;  // shouldn't get here
  }

#ifdef  DEBUG_COMPACT
  Output << "Decoded state successfully?\n";
  Output << "Handle: " << h << "\n";
  Output << "Encoding:\n";
  if (bitptr<7) byteptr++;
  PrintBits(Output, h, byteptr);
  Output << "\n#places bits: " << (int) npbits;
  Output << "\t#tokens bits: " << (int) tkbits << "\n";
  Output << "Used " << EncodingMethod[encselect] << " storage\n";
  switch (encselect) {
    case 1: 
      Output << "#nonzeroes: " << nnz << "\n"; 
      break;
    case 2:
      Output << "#Runs: " << runs << "\n";
      Output << "#Lists: " << lists << "\n";
      Output << "List length: " << listlength << "\n";
      break;
    case 3:
      Output << "#places: " << np << "\n";
      break;
  }
  Output << "Got state " << s << "\n";
  Output.flush();
#endif

  return true;
}


int state_array::NextHandle(int h)
{
  // 
  // Take care of easy cases first
  //
  if (map) {
    // handles are indices
    if (h>=numstates) return -1;
    return h+1; 
  }
  if (h>=lasthandle) return -1;
  
  // 
  // Initialize bitstream and read state encoding method
  // 
  byteptr = h;
  bitptr = 7;
  int encselect;
  ReadInt(2, encselect);
  int npselect;
  ReadInt(3, npselect); 	// #places selector
  int tkselect;
  ReadInt(3, tkselect);  	// maxtokens selector

  // Figure out the numbers of bits used
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Determine state length (#bits), reading only as much as necessary
  //
  int np, nnz;
  int i, j, type;
  // stats for debugging
#ifdef DEBUG_COMPACT
  int lists, runs, listlength;
  lists = runs = listlength = 0;
#endif
  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      ReadInt(npbits, nnz);
      if (tkbits==1) tkbits=0;  // #tokens never part of state
      SkipInt(nnz * (npbits + tkbits));
      break;

    case 2: 
      // *****************	Runlength Encoding 
      ReadInt(npbits, nnz); 
      j=0;
      if (tkbits==1) {
	tkbits=0;    // #tokens never part of state...
	SkipInt(1);  // except here
      }
      for (i=0; i<nnz; i++) {
        ReadInt(1, type);
        if (RUN_BIT == type) {
	  // RUN
          SkipInt(npbits + tkbits);
        } else {
	  // LIST
	  ReadInt(npbits, np);
	  SkipInt(tkbits * np);
        }
      } // for i
      break;

    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      SkipInt(np * tkbits);
    break;

    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad state encoding?\n";
	Internal.Stop();
      	return -1;  // shouldn't get here
  } // switch

  // 
  // Close off bitstream
  // 
  if (bitptr<7) byteptr++;

#ifdef  DEBUG_COMPACT
  Output << "Determined next handle ok?\n";
  Output << "Handle: " << h << "\n";
  Output << "Encoding:\n";
  PrintBits(Output, h, byteptr);
  Output << "\n#places bits: " << (int)npbits;
  Output << "\t#tokens bits: " << (int)tkbits << "\n";
  Output << "Next handle: " << byteptr << "\n";
#endif

  return byteptr;
}

int state_array::Hash(int handle, int M) const 
{
    DCASSERT(map);
    CHECK_RANGE(0, handle, numstates);
    int start = map[handle];
    int end = map[handle+1];
    if (end - start > 4) end--;   // last byte is only partly full
    if (end - start > 4) start++; // first byte is encoding method
    DCASSERT(end - start >= 2);
    unsigned int first4;
    if (end - start < int(sizeof(unsigned int))) {
      first4 = 0;
      for (; start < end; start++) {
        first4 *= 256;
        first4 += mem[start];
      }
    } else {
      first4 = *(unsigned int*)(mem+start);
      start += sizeof(unsigned int);
    }
    // Look at remaining state
    int answer = first4 % M;
    for (int i=start; i<end; i++) {
      answer += ( (answer * 256) + mem[i] );
      answer %= M;
    }
#ifdef DEBUG_COMPACT
    Output << "State " << handle << " rep: [";
    Output.PutArray(mem+map[handle], map[handle+1] - map[handle]);
    Output << "]\nHash value (M=" << M << "): " << answer << "\n";
    Output.flush();
#endif
    return answer;
}


int state_array::Compare(int h1, int h2) const
{
  DCASSERT(map);
  // compare by length first, then in case of a tie,
  // check the actual encodings.
  int length1 = map[h1+1] - map[h1];
  int length2 = map[h2+1] - map[h2];
  int foo = length1 - length2;
  if (foo!=0) return foo;
  // same size, check bytes
  const char* ptr1 = (char *) mem + map[h1];
  const char* ptr2 = (char *) mem + map[h2];
#ifdef DEBUG_COMPACT
  int compare = memcmp(ptr1, ptr2, length1);
  Output << "Comparing handle " << h1 << " and " << h2 << "\n";
  Output << "Encoding of " << h1 << ": ";
  PrintBits(Output, map[h1], map[h1+1]);
  Output << "\nEncoding of " << h2 << ": ";
  PrintBits(Output, map[h2], map[h2+1]);
  Output << "\nGot: " << compare << "\n";
  Output.flush();
#endif
  // doesn't get much faster than this:
  return memcmp(ptr1, ptr2, length1); 
  // don't use strncmp, because if ptr1 and ptr2 contain '0', comparison stops!
}

int state_array::Compare(int hndl1, const state& s2)
{
  //
  //  Check handle
  //
  int h1;
  DCASSERT(hndl1>=0);
  if (map) {
    DCASSERT(hndl1<numstates);
    h1 = map[hndl1];
  } else {
    DCASSERT(hndl1<lasthandle);
    h1 = hndl1;
  }

  // 
  // Initialize bitstream and read state encoding method
  // 
  byteptr = h1;
  bitptr = 7;
  int encselect;
  ReadInt(2, encselect);
  int npselect;
  ReadInt(3, npselect); 	// #places selector
  int tkselect;
  ReadInt(3, tkselect);  	// maxtokens selector

  // Figure out the numbers of bits used
  char npbits = placebits[npselect];
  char tkbits = tokenbits[tkselect];

  //
  // Decode the state based on the encoding method
  //
  int tk, np, nnz;
  int i, j, type, cmp;
  // stats for debugging
  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      j = 0;  // scans s2
      ReadInt(npbits, nnz);
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
        // next nonzero is at np, compare with s2
        for (;j<np; j++) if (s2.Read(j).ivalue) return -1;
	if (tkbits>1) {
	  ReadInt(tkbits, tk);
          cmp = (tk - s2.Read(np).ivalue);
	} else {
          cmp = 1 - s2.Read(np).ivalue;
        }
 	if (cmp) return cmp;
        j = np+1;
      }
      break;
      
    case 2: 
      // *****************	Runlength Encoding 
      ReadInt(npbits, nnz); 
      j=0;
      if (1==tkbits) ReadInt(1, tk);
      for (i=0; i<nnz; i++) {
        ReadInt(1, type);
        if (RUN_BIT == type) {
	  // RUN
          ReadInt(npbits, np);
	  if (tkbits>1) ReadInt(tkbits, tk);
          for(; np; np--) {
	    cmp = tk - s2.Read(j++).ivalue;
	    if (cmp) return cmp;
          } // for np
	  // flip bit
	  tk = !tk;
        } else {
	  // LIST
	  ReadInt(npbits, np);
	  for(; np; np--) {
	    if (tkbits>1) ReadInt(tkbits, tk);
	    cmp = tk - s2.Read(j++).ivalue;
	    if (cmp) return cmp;
	    tk = !tk;  // faster to just do it every time
          }
        }
      } // for i
      break;
      
    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      for (i=0; i<np; i++) {
	ReadInt(tkbits, tk);
	cmp = tk - s2.Read(i).ivalue;
	if (cmp) return cmp;
      }
    break;

    default:
	Internal.Start(__FILE__, __LINE__);
	Internal << "Bad state encoding?\n";
	Internal.Stop();
      	return false;  // shouldn't get here
  }

  // 
  // Still here? must be equal
  //
  return 0;
}

void state_array::Report(OutputStream &R)
{
  R << "State array report\n";
  R << "\t  " << numstates << " states inserted\n";
  int i;
  for (i=0; i<4; i++) if (encodecount[i])
    R <<"\t  "<< encodecount[i] <<" "<< EncodingMethod[i] << " encodings\n";
  R << "\tState array allocated " << memsize << " bytes\n";
  R << "\tState array using     " << lasthandle << " bytes\n";
  if (map) {
    R << "\tIndex map allocated " << int(mapsize*sizeof(int)) << " bytes\n";
    R << "\tIndex map using     " << int(numstates*sizeof(int)) << " bytes\n";
  }
}

int state_array::MemUsed()
{
  int answer = memsize;
  if (map) answer += numstates * sizeof(int);
  return answer;
}


void state_array::Clear()
{
  bitptr = 7;
  byteptr = 0;
  numstates = 0;
  lasthandle = 0;
  firsthandle = -1;
  // stats
  encodecount[0] = encodecount[1] = encodecount[2] = encodecount[3] = 0;
}


// ==================================================================
// ||                                                              ||
// ||                        flatss methods                        ||
// ||                                                              ||
// ==================================================================

flatss::flatss(state_array *ta, int *to, state_array *va, int *vo)
{
  ALLOC("flatss", sizeof(flatss));
  t_states = ta;
  t_order = to;
  v_states = va;
  v_order = vo;
#ifdef DEVELOPMENT_CODE
  DCASSERT(t_states);
  DCASSERT(t_states->UsesIndexHandles());
  if (v_states)
    DCASSERT(v_states->UsesIndexHandles());
#endif
#ifdef DEBUG
  Output << "Tangible state array: [";
  Output.PutArray(t_order, t_states->NumStates());
  Output << "]\n";
  Output.flush();
#endif
}

flatss::~flatss()
{
  FREE("flatss", sizeof(flatss));
  delete t_states;
  delete[] t_order;
  delete v_states;
  delete[] v_order;
}

int flatss::binsearch(state_array* s, int* ord, int h)
{
  int low = 0;
  int high = s->NumStates()-1;  
  // note: there is one extra state, the one we are looking for
  while (low<high) {
    int mid = (low+high)/2;
    int cmp = s->Compare(ord[mid], h);
    if (0==cmp) return ord[mid];
    if (cmp<0) low = mid+1;
    else high = mid;
  }
  return -1; // not found
}


//@}

