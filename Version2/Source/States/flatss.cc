
// $Id$

#include "states.hh"

/** @name states.cc
    @type File
    @args \ 

  Working on a new and efficient way to store states.
    
 */

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

const unsigned int twoexp[32] = 
	{0x0001, 0x0002, 0x0004, 0x0008, 
	 0x0010, 0x0020, 0x0040, 0x0080,
	 0x0100, 0x0200, 0x0400, 0x0800,
	 0x1000, 0x2000, 0x4000, 0x8000};

const char* EncodingMethod[4] = 
	{ "deleted state", "sparse", "inverse sparse", "full" };

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
  unsigned char chunk = x / twoexp[bits - (bitptr+1)];
  x %= twoexp[bits - (bitptr+1)];
  mem[byteptr] |= chunk;
  bits -= (bitptr+1);
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
      chunk = x / twoexp[bits];
      x %= twoexp[bits];
      mem[byteptr] = chunk;
    } else {
      mem[byteptr] = x;
    }
    byteptr++;
  }
}

void state_array::PrintBits(int start, int stop)
{
  int i, b;
  for (i=start; i<stop; i++) {
    OutLog->getStream() << "|";
    for (b=7; b>=0; b--) {
      CHECK_RANGE(0, i, memsize);
      if (mem[i] & frommask[b] & tomask[b]) 
	OutLog->getStream() << "1"; else OutLog->getStream() << "0";
    }
  }
}

void state_array::EnlargeMem(int newsize)
{
  if (newsize<memsize) return;
  int newmemsize = memsize;
  while (newsize>=newmemsize) {
    if (newmemsize<16777216)  // less than 16Mb, double size
      newmemsize *= 2;
    else  // greater than 16Mb, add 16Mb increments 
      newmemsize += 16777216;
  }
  unsigned char *newmem = (unsigned char*) realloc(mem, newmemsize);
  if (NULL==newmem) {
    cerr << "Error, memory exceeded in array of states\n";
    exit(0);
  }
  mem = newmem;
  memsize = newmemsize;
}

state_array::state_array(int bsize, bool useindices)
{
  MEM_ALLOC(states_hh, sizeof(state_array), "state_array");

  // start with 256 bytes, in case we're a small set
  mem = (unsigned char*) malloc(256);  
  memsize = 256;

  if (useindices) map = new heaparray<int>(bsize, 2);
  else map = NULL;
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
  MEM_FREE(states_hh, sizeof(state_array), "state_array");

  delete map;
  free(mem); mem = NULL;
}
 
int state_array::AddState(discrete_state *s)
{
  int answer;
  if (map) {
    (*map)[numstates] = lasthandle;
    answer = numstates;
  } else {
    answer = lasthandle;
  }
  int np = s->Size();
  int i;
  int nnz, nnM, Mfv, maxtok;
#ifdef SHORT_STATES
  for (i = 0; i <= USHRT_MAX; i++) {
    histogram[i] = 0;
  }
#else
  for (i=0; i<256; i++) histogram[i] = 0;
#endif
  for (i = 0; i < np; i++) {
    histogram[s->Get(i)]++;
  }
  maxtok = 0;
  Mfv = 0;
#ifdef SHORT_STATES
  for (i = 1; i <= USHRT_MAX; i++) {
#else
  for (i=1; i<256; i++) {
#endif
    if (histogram[i]>histogram[Mfv]) Mfv = i;
    if (histogram[i]) maxtok = i;
  }
  nnz = np - histogram[0];
  nnM = np - histogram[Mfv];
  
  char npbits;
  char npselect;
  if (np<=16) {
    npselect = 0;
    npbits = 4; 
  }
  else if (np<=256) {
    npselect = 1;
    npbits = 8;
  }
  else if (np<=65536) {
    npselect = 2;
    npbits = 16;
  }
  else if (np<=16777216) {
    npselect = 3;
    npbits = 24;
  }
  else {
    // I can't imagine having more than 16 million places, but ok...
    npselect = 4;
    npbits = 32;
  }

  char tkbits;
  char tkselect;
  if (maxtok<2) 
    tkselect = 0; 
  else if (maxtok < 4) 
    tkselect = 1; 
#ifdef SHORT_STATES
  else if (maxtok < 16) 
    tkselect = 2; 
  else if (maxtok < 256) 
    tkselect = 3; 
  else 
    tkselect = 4; 
#else
  else if (maxtok<16) tkselect = 2; 
  else tkselect = 3; 
#endif

  tkbits = twoexp[tkselect];

  // count sparse bits required
  int sparse_bits = npbits + nnz*npbits;
  if (tkbits>1) sparse_bits += nnz*tkbits;
  int sparse_bytes = sparse_bits / 8;
  if (sparse_bits%8) sparse_bytes++;

  // count inverse sparse bits required
  int invsp_bits = npbits + nnM*npbits;
  if (tkbits>1) invsp_bits += (1+nnM)*tkbits;
  int invsp_bytes = invsp_bits / 8;
  if (invsp_bits%8) invsp_bytes++;

  // count full bits required
  int full_bits = npbits + np*tkbits;
  int full_bytes = full_bits / 8;
  if (full_bits%8) full_bytes++;

  // Determine best encoding
  int min = MIN(MIN(sparse_bytes, full_bytes), invsp_bytes);
  char encoding;
  if (min==full_bytes) 
  	encoding = 3; 
  else if (min==sparse_bytes)
    	encoding = 1;
  else  
    	encoding = 2;

  // Make room for this state
  EnlargeMem(lasthandle + 1+min);

  // here we go...
  byteptr = lasthandle;
  bitptr = 7;


  // write the state code
  WriteInt(2, encoding);
  WriteInt(3, npselect);
#ifdef SHORT_STATES
  WriteInt(3, tkselect);
#else
  WriteInt(2, tkselect);
  WriteInt(1, 0);		// extra unused bit
#endif

  encodecount[encoding]++;

  switch (encoding) {
    case 1: 
      // *****************	Sparse Encoding 
      WriteInt(npbits, nnz);
      for (i=0; i<np; i++) {
        int tk = s->Get(i);
        if (tk) {
          WriteInt(npbits, i);
          if (tkbits>1) WriteInt(tkbits, tk);
        }
      }
      break;

    case 2: 
      // *****************	Inverse-Sparse Encoding 
      if (tkbits>1) WriteInt(tkbits, Mfv);
      WriteInt(npbits, nnM);
      for (i=0; i<np; i++) {
        int tk = s->Get(i);
        if (tk != Mfv) {
          WriteInt(npbits, i);
          if (tkbits>1) WriteInt(tkbits, tk);
        }
      }
      break;

    case 3:
      // *****************	Full Encoding 
      WriteInt(npbits, (np-1));
      for (i=0; i<np; i++) 
	WriteInt(tkbits, s->Get(i));
    break;
  }
  
  // Done writing state, cleanup
  if (map) (*map)[numstates] = lasthandle;
  if (firsthandle<0) firsthandle = lasthandle;
  numstates++;
  if (bitptr<7) byteptr++;
  lasthandle = byteptr;

#ifdef  DEBUG_COMPACT
  OutLog->getStream() << "Added state #" << numstates-1 << " : " << s << "\n";
  if (map)
    OutLog->getStream() << "handle: " << (*map)[numstates-1] << "\n";
  else
    OutLog->getStream() << "handle: " << answer << "\n";
  OutLog->getStream() << "nexthandle: " << lasthandle << "\n";
  OutLog->getStream() << "#places bits: " << (int) npbits;
  OutLog->getStream() << "\t#tokens bits: " << (int) tkbits << "\n";
  OutLog->getStream() << "Used " << EncodingMethod[encoding] << " storage\n";
  switch (encoding) {
    case 1: 
      OutLog->getStream() << "#nonzeroes: " << nnz << "\n"; 
      break;
    case 2:
      OutLog->getStream() << "Mfv: " << Mfv << "\n";
      OutLog->getStream() << "#not-Mfvs: " << nnM << "\n";
      break;
    case 3:
      OutLog->getStream() << "#places: " << np << "\n";
      break;
  }
  OutLog->getStream() << "Encoding:\n";
  PrintBits(answer, lasthandle);
  OutLog->getStream() << "\n";
#endif
  
  return answer;
}

bool state_array::GetState(int Handel, discrete_state *s)
{
  int h;
  if (Handel<0) return false;
  if (map) {
    if (Handel>=numstates) return false;
    h = (*map)[Handel];
  } else {
    if (Handel>=lasthandle) return false;
    h = Handel;
  }
  int tk, np, nnz;
  int encselect;
  int npselect;
  char npbits;
  int tkselect;
  char tkbits;
  int i;
  int Mfv;
#ifndef SHORT_STATES
  int dummy;
#endif
  
  // here we go...
  byteptr = h;
  bitptr = 7;
  // read the state code
  ReadInt(2, encselect);
  ReadInt(3, npselect); 	// #places selector
#ifdef SHORT_STATES
  ReadInt(3, tkselect);  	// maxtokens selector
#else
  ReadInt(2, tkselect);  	// maxtokens selector
  ReadInt(1, dummy);		// extra unused bit
#endif

  // Figure out the numbers of bits
  switch(npselect) {
    case 0: npbits = 4; break;
    case 1: npbits = 8; break;
    case 2: npbits = 16; break;
    case 3: npbits = 24; break;
    case 4: npbits = 32; break;
    default:
      return false;  // shouldn't get here
  }

  tkbits = twoexp[tkselect];
  
  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      s->Fill(0);
      ReadInt(npbits, nnz);
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
	if (tkbits>1) {
	  ReadInt(tkbits, tk);
	  s->Set(np, tk);
	} else 
	  s->Set(np, 1);
      }
      break;
      
    case 2: 
      // *****************	Inverse-Sparse Encoding 
      if (tkbits>1) ReadInt(tkbits, Mfv);
      else Mfv = 1;
      s->Fill(Mfv);
      ReadInt(npbits, nnz);
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
	if (tkbits>1) {
	  ReadInt(tkbits, tk);
	  s->Set(np, tk);
	} else 
	  s->Set(np, 0);
      }
      break;
      
    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      for (i=0; i<np; i++) {
	ReadInt(tkbits, tk);
	s->Set(i, tk);
      }
    break;

    default:
      return false;  // shouldn't get here
  }

#ifdef  DEBUG_COMPACT
  OutLog->getStream() << "Decoded state successfully?\n";
  OutLog->getStream() << "Handle: " << h << "\n";
  OutLog->getStream() << "Encoding:\n";
  if (bitptr<7) byteptr++;
  PrintBits(h, byteptr);
  OutLog->getStream() << "\n#places bits: " << (int) npbits;
  OutLog->getStream() << "\t#tokens bits: " << (int) tkbits << "\n";
  OutLog->getStream() << "Used " << EncodingMethod[encselect] << " storage\n";
  switch (encselect) {
    case 1: 
      OutLog->getStream() << "#nonzeroes: " << nnz << "\n"; 
      break;
    case 2:
      OutLog->getStream() << "Mfv: " << Mfv << "\n";
      OutLog->getStream() << "#not-Mfvs: " << nnz << "\n";
      break;
    case 3:
      OutLog->getStream() << "#places: " << np << "\n";
      break;
  }
  OutLog->getStream() << "Got state " << s << "\n";
#endif

  return true;
}

int state_array::Compare(int Handel, discrete_state *s)
{
  int h;
  if (Handel<0) return false;
  if (map) {
    if (Handel>=numstates) return false;
    h = (*map)[Handel];
  } else {
    if (Handel>=lasthandle) return false;
    h = Handel;
  }
  int tk, np, nnz, Mfv;
  int encselect;
  int npselect;
  char npbits;
  int tkselect;
  char tkbits;
  int i, j;
#ifndef SHORT_STATES
  int dummy;
#endif
  
  // here we go...
  byteptr = h;
  bitptr = 7;
  // read the state code
  ReadInt(2, encselect);
  ReadInt(3, npselect); 	// #places selector
#ifdef SHORT_STATES
  ReadInt(3, tkselect);  	// maxtokens selector
#else
  ReadInt(2, tkselect);  	// maxtokens selector
  ReadInt(1, dummy);		// extra unused bit
#endif

  // Figure out the numbers of bits
  switch(npselect) {
    case 0: npbits = 4; break;
    case 1: npbits = 8; break;
    case 2: npbits = 16; break;
    case 3: npbits = 24; break;
    case 4: npbits = 32; break;
    default:
      return -2;  // shouldn't get here
  }

  tkbits = twoexp[tkselect];

  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      ReadInt(npbits, nnz);
      j=0;
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
	if (tkbits>1) ReadInt(tkbits, tk);
	else tk = 1;
	
	// check the gap
	while (j<np) {
	  if (s->Get(j)) return -1;  // encoded state is missing 
	  j++;
	}
	if (s->Get(np)>tk) return -1;
	if (s->Get(np)<tk) return 1;
	j = np+1;
      }
      // check the end gap
      while (j<s->Size()) {
	if (s->Get(j)) return -1; 
	j++;
      }
      // we're equal
      return 0;

    case 2: 
      // *****************	Inverse-Sparse Encoding 
      if (tkbits>1) ReadInt(tkbits, Mfv);
      else Mfv = 1;
      ReadInt(npbits, nnz);
      j=0;
      for (i=0; i<nnz; i++) {
        ReadInt(npbits, np);
	if (tkbits>1) ReadInt(tkbits, tk);
	else tk = 0;
	
	// check the gap
	while (j<np) {
	  if (s->Get(j)>Mfv) return -1;
	  if (s->Get(j)<Mfv) return 1;
	  j++;
	}
	if (s->Get(np)>tk) return -1;
	if (s->Get(np)<tk) return 1;
	j = np+1;
      }
      // check the end gap
      while (j<s->Size()) {
	if (s->Get(j)>Mfv) return -1;
	if (s->Get(j)<Mfv) return 1;
	j++;
      }
      // we're equal
      return 0;

    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      for (i=0; i<np; i++) {
	ReadInt(tkbits, tk);
	if (s->Get(i)>tk) return -1;
	if (s->Get(i)<tk) return 1;
      }
      return 0;

    default:
      return -2;  // shouldn't get here
  }
}

int state_array::NextHandle(int h)
{
  if (h<0) return -1;
  if (map) {
    // handles are indices
    if (h>=numstates-1) return -1;
    return h+1;
  }
  if (h>=lasthandle) return -1;
  int np, nnz, Mfv;
  int encselect;
  int npselect;
  char npbits;
  int tkselect;
  char tkbits;
#ifndef SHORT_STATES
  int dummy;
#endif
  int numbits;
  
  // here we go...
  byteptr = h;
  bitptr = 7;
  // read the state code
  ReadInt(2, encselect);
  ReadInt(3, npselect); 	// #places selector
#ifdef SHORT_STATES
  ReadInt(3, tkselect);  	// maxtokens selector
#else
  ReadInt(2, tkselect);  	// maxtokens selector
  ReadInt(1, dummy);		// extra unused bit
#endif

  // Figure out the numbers of bits
  switch(npselect) {
    case 0: npbits = 4; break;
    case 1: npbits = 8; break;
    case 2: npbits = 16; break;
    case 3: npbits = 24; break;
    case 4: npbits = 32; break;
    default:
      return -1;  // shouldn't get here
  }

  tkbits = twoexp[tkselect];

  switch(encselect) {
    case 1: 
      // *****************	Sparse Encoding 
      ReadInt(npbits, nnz);
      if (tkbits==1) tkbits=0;  // #tk not used in this case
      numbits = 8+npbits+ nnz*(npbits + tkbits);
      break;

    case 2: 
      // *****************	Inverse-Sparse Encoding 
      if (tkbits==1) tkbits=0;  // #tk not used in this case
      if (tkbits) ReadInt(tkbits, Mfv);
      ReadInt(npbits, nnz);
      numbits = 8+npbits+tkbits+ nnz*(npbits + tkbits);
      break;

    case 3:
      // *****************	Full Encoding 
      ReadInt(npbits, np);
      np++;
      numbits = 8+npbits+ np*tkbits;
      break;

    default:
      return -1;  // shouldn't get here
  }
  int answer;
  bool extrabits = (numbits % 8);
  answer = h+numbits / 8;
  if (extrabits) answer++;

#ifdef  DEBUG_COMPACT
  OutLog->getStream() << "Determined next handle ok?\n";
  OutLog->getStream() << "Handle: " << h << "\n";
  OutLog->getStream() << "Encoding:\n";
  PrintBits(h, answer);
  OutLog->getStream() << "\n#places bits: " << (int)npbits;
  OutLog->getStream() << "\t#tokens bits: " << (int)tkbits << "\n";
  OutLog->getStream() << "Next handle: " << answer << "\n";
#endif

  return answer;
}

void state_array::Report()
{
  OutLog->getStream() << "State array report\n";
  OutLog->getStream() << "  " << numstates << " states inserted\n";
  int i;
  for (i=0; i<4; i++) if (encodecount[i])
    OutLog->getStream() <<"  "<< encodecount[i] <<" "<< EncodingMethod[i] << " encodings\n";
  OutLog->getStream() << "State array " << memsize << " bytes\n";
  OutLog->getStream() << " states use " << lasthandle << " bytes\n";
  if (map) 
    OutLog->getStream() << "Index handles require " << numstates*sizeof(int) << " bytes\n";
}

int state_array::MemUsed()
{
  int answer = memsize;
  if (map) answer += numstates * sizeof(int);
  return answer;
}

void state_array::Write(ostream &s)
{
  ASSERT(mem);
  s.write((char*)&lasthandle, sizeof(lasthandle));
  s.write((char*)mem, lasthandle);
  s.write((char*)encodecount, 4*sizeof(int));
  bool usemap = (map!=NULL);
  s.write((char*)&usemap, sizeof(bool));
  if (map) map->Write(s);
  s.write((char*)&firsthandle, sizeof(firsthandle));
  s.write((char*)&numstates, sizeof(numstates));
}

void state_array::Read(istream &s)
{
  s.read((char*)&lasthandle, sizeof(lasthandle));
  EnlargeMem(lasthandle);
  s.read((char*)mem, lasthandle);
  s.read((char*)encodecount, 4*sizeof(int));
  bool usemap;
  s.read((char*)&usemap, sizeof(bool));
  ASSERT((map!=NULL)==usemap);
  if (map) map->Read(s);
  s.read((char*)&firsthandle, sizeof(firsthandle));
  s.read((char*)&numstates, sizeof(numstates));
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

//@}

