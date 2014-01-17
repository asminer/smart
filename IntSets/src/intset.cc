
// $Id$

#include <stdio.h>
#include "intset.h"

#include "revision.h"
#include <stdlib.h>
#include <string.h>

// #define DEBUG_ALLOC




#ifdef INTSET_DEVELOPMENT_CODE
#include <assert.h>
#define DCASSERT(X) assert(X)
#else
#define DCASSERT(X)
#endif

template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }

char* intset::version = 0;

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 1;

// ======================================================================

class intset::bitvector {
  long size;
  unsigned long *data;
  long links;
  unsigned long tail_mask;
protected:
  static const int bitwidth = sizeof(long) * 8;
  static const int bitwidthm1 = bitwidth-1;
  static const unsigned long msbit = 1L << bitwidthm1;
  inline static long Bits2Words(long bits) { 
    return (bits>0) ? (((bits-1)/bitwidth)+1) : 0; 
  }
  inline static long Bits2Bytes(long bits) {
    return (bits>0) ? ((bits-1)/8 + 1) : 0;
  }
public:
  bitvector(long s); 
protected:
  ~bitvector();
  void SetSize(long s);
public:
  inline long NumBits() const { return size; }
  inline long NumWords() const { return Bits2Words(NumBits()); }
  inline long NumBytes() const { return Bits2Bytes(NumBits()); }
  inline const unsigned long* read() const { return data; }
  inline unsigned long* write() { 
    DCASSERT(1==links);
    return data; 
  }
  inline void Link() { links++; }
  inline void Unlink() { links--; if (0==links) delete this; }
  inline long numLinks() const { return links; }
  bitvector* Modify();
  void Resize(long ns); 
  void UnsetAll();
  void SetAll();
  void Set(long n);
  void Unset(long n);
  bool IsSet(long n) const;

  // Like set, but returns true if the bit was changed.
  bool SetBit_Changed(long n);

  // Like unset, but returns true if the bit was changed.
  bool UnsetBit_Changed(long n);

  /// Set all bits from b1 to b2, inclusive
  void SetRange(long b1, long b2);

  /// Unset all bits from b1 to b2, inclusive
  void UnsetRange(long b1, long b2);

  /// Find first bit set after n
  long FirstSetAfter(long n) const;

  /// Find first bit unset after n
  long FirstUnsetAfter(long n) const;

  /// This = This \cap B
  void IntersectWith(const bitvector &B);

  /// This = This \cup B
  void UnionWith(const bitvector &B);

  /// This = This \ B
  void DifferenceWith(const bitvector &B);

  /// This = A \cap B
  void Intersect(const bitvector &A, const bitvector &B);

  /// This = A \cup B
  void Union(const bitvector &A, const bitvector &B);

  /// This = A \ B
  void Difference(const bitvector &A, const bitvector &B);

  /// does This == B?
  bool Equals(const bitvector &B) const;

  /// does This == ~B?
  bool EqualsComplement(const bitvector &B) const;

  /// does This \ B = 0?
  bool EmptyDifference(const bitvector &B) const;

  /// does This \cap B = 0?
  bool EmptyIntersect(const bitvector &B) const;

  /// does This \cup B = 1?
  bool FullUnion(const bitvector &B) const;

  /// Is This the empty set?
  bool Empty() const;

#ifdef INTSET_DEVELOPMENT_CODE
  void dump(FILE* s) const;
#endif
};


// ======================================================================

intset::bitvector::bitvector(long s) 
{ 
  data = 0;
  size = 0;
  if (s>0) Resize(s);
  else     SetSize(0);
  links = 1;
#ifdef DEBUG_ALLOC
  fprintf(stderr, "alloc bitvector\n");
#endif
}

intset::bitvector::~bitvector() 
{ 
  Resize(0);
#ifdef DEBUG_ALLOC
  fprintf(stderr, "free  bitvector\n");
#endif
}

void intset::bitvector::SetSize(long ns)
{
  size = ns;
  int d = bitwidth - ns % bitwidth;
  tail_mask = ~0;
  if (d < bitwidth) tail_mask <<= d;
  // printf("size: %ld shift %d bitmask: %lx\n", ns, d, tail_mask);
}


void intset::bitvector::Resize(long ns) 
{
  long words = Bits2Words(ns);
  long oldwords = NumWords();
  if (words != oldwords) {
    data = (unsigned long*) realloc(data, words*sizeof(long));
    if (words>0) data[words-1] = 0;
  } 
  SetSize(ns);
}


inline intset::bitvector* intset::bitvector::Modify() 
{
  if (1==links) return this;
  bitvector* foo = new bitvector(size);
  memcpy(foo->data, data, NumWords() * sizeof(long));
  return foo;
}


inline void intset::bitvector::UnsetAll() 
{
  DCASSERT(1==links);
  memset(data, 0, NumWords() * sizeof(long));
}


inline void intset::bitvector::SetAll() 
{
  DCASSERT(1==links);
  memset(data, 0xFF, NumWords() * sizeof(long));
  data[NumWords()-1] = tail_mask;
}


inline void intset::bitvector::Set(long n) 
{
  DCASSERT(n>=0);
  DCASSERT(n<size);
  DCASSERT(1==links);
  data[n/bitwidth] |= (msbit >> n%bitwidth); // set bit n
}


inline void intset::bitvector::Unset(long n) 
{
  DCASSERT(n>=0);
  DCASSERT(n<size);
  DCASSERT(1==links);
  data[n/bitwidth] &= ~(msbit >> n%bitwidth); // clear bit n      
}


inline bool intset::bitvector::IsSet(long n) const 
{
  DCASSERT(n>=0);
  DCASSERT(n<size);
  return (data[n/bitwidth] & (msbit >> n%bitwidth)) > 0; 
}


inline bool intset::bitvector::SetBit_Changed(long n) 
{
  DCASSERT(n>=0);
  DCASSERT(n<size);
  DCASSERT(1==links);
  unsigned long tweak = (msbit >> n%bitwidth);
  unsigned long* dataword = data + n/bitwidth;
  // long word = n/bitwidth;
  if (dataword[0] & tweak) return false;  // already set
  dataword[0] |= tweak; // set bit
  return true; 
}


inline bool intset::bitvector::UnsetBit_Changed(long n) 
{
  DCASSERT(n>=0);
  DCASSERT(n<size);
  DCASSERT(1==links);
  unsigned long tweak = (msbit >> n%bitwidth);
  unsigned long* dataword = data + n/bitwidth;
  // long word = n/bitwidth;
  if (!(dataword[0] & tweak)) return false;  // already unset
  dataword[0] &= ~tweak; // clear bit
  return true; 
}


inline void intset::bitvector::SetRange(long b1, long b2) 
{
  DCASSERT(b1>=0);
  DCASSERT(b1<size);
  DCASSERT(b1>=b1);
  DCASSERT(b2<size);
  // do the stray bits up to the word boundary
  while ((b1 <= b2) && (b1 % bitwidth > 0)) Set(b1++);
  // do words at a time
  while (b1+bitwidthm1 <= b2) {
    data[b1/bitwidth] = ~0; 
    b1 += bitwidth;
  }
  // do stray bits at the end
  while (b1 <= b2) Set(b1++);
}


inline void intset::bitvector::UnsetRange(long b1, long b2) 
{
  DCASSERT(b1>=0);
  DCASSERT(b1<size);
  DCASSERT(b1>=b1);
  DCASSERT(b2<size);
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


inline long intset::bitvector::FirstSetAfter(long n) const 
{
  unsigned long* dptr;
  if (n<0) {
    dptr = data-1;
    n = -1;
  } else {
    dptr = data + n/bitwidth;
  }
  while (1) {
    n++;
    DCASSERT(n>=0);
    if (n>=size) return -1;
    long nmbw = n % bitwidth;
    if (nmbw) {
      if (dptr[0] & (msbit >> nmbw)) return n;
      continue;
    }
    // next word
    dptr++;
    while (0==dptr[0]) {
      n += bitwidth;
      dptr++;
      if (n>=size) return -1;
    }
    if (dptr[0] & msbit) return n;
  } // while 1
}


inline long intset::bitvector::FirstUnsetAfter(long n) const 
{
  unsigned long* dptr;
  if (n<0) {
    dptr = data-1;
    n = -1;
  } else {
    dptr = data + n/bitwidth;
  }
  while (1) {
    n++;
    DCASSERT(n>=0);
    if (n>=size) return -1;
    long nmbw = n % bitwidth;
    if (nmbw) {
      if (0==(dptr[0] & (msbit >> nmbw))) return n;
      continue;
    }
    // next word
    dptr++;
    while (0==~dptr[0]) {
      n += bitwidth;
      dptr++;
      if (n>=size) return -1;
    }
    if (0==(dptr[0] & msbit)) return n;
  } // while 1
}


inline void intset::bitvector::IntersectWith(const bitvector &B) 
{
  DCASSERT(1==links);
  if (size > B.size) 
    UnsetRange(B.size, size-1);
  for (long w = MIN(NumWords(), B.NumWords())-1; w>=0; w--)
    data[w] &= B.data[w];
}


inline void intset::bitvector::UnionWith(const bitvector &B) 
{
  DCASSERT(1==links);
  if (B.size > size) {
    long oldsize = size;
    Resize(B.size); 
    UnsetRange(oldsize, size-1);
  }
  for (long w = MIN(NumWords(), B.NumWords())-1; w>=0; w--)
    data[w] |= B.data[w];
}


inline void intset::bitvector::DifferenceWith(const bitvector &B) 
{
  DCASSERT(1==links);
  for (long w = MIN(NumWords(), B.NumWords())-1; w>=0; w--)
    data[w] &= ~B.data[w];
}


inline void intset::bitvector
::Intersect(const bitvector &A, const bitvector &B)
{
  DCASSERT(1==links);
  Resize(MIN(A.size, B.size));
  long w = NumWords()-1;
  for (long w = NumWords()-1; w>=0; w--)
    data[w] = A.data[w] & B.data[w];
}


inline void intset::bitvector::Union(const bitvector &A, const bitvector &B) 
{
  DCASSERT(1==links);
  Resize(MAX(A.size, B.size));
  for (long w=A.NumWords()-1; w>=B.NumWords(); w--)
    data[w] = A.data[w];
  for (long w=B.NumWords()-1; w>=A.NumWords(); w--)
    data[w] = B.data[w];
  for (long w = MIN(A.NumWords(), B.NumWords())-1; w>=0; w--)
    data[w] = A.data[w] | B.data[w];
}


inline void intset::bitvector
::Difference(const bitvector &A, const bitvector &B) 
{
  DCASSERT(1==links);
  Resize(A.size);
  long w;
  for (w=A.NumWords()-1; w>=B.NumWords(); w--)
    data[w] = A.data[w];
  for ( ; w>=0; w--)
    data[w] = A.data[w] & (~B.data[w]);
}


inline bool intset::bitvector::Equals(const bitvector &B) const 
{
  // check overlap
  unsigned long common = MIN(NumWords(), B.NumWords());
  if (memcmp(data, B.data, common*sizeof(long))!=0) return false;
  // make sure remaining B words, if any, are zero
  for (long w = NumWords(); w<B.NumWords(); w++) 
    if (B.data[w]) return false;
  // make sure Remaining words, if any, are zero
  for (long w = B.NumWords(); w<NumWords(); w++)
    if (data[w]) return false; 
  return true;
} 


inline bool intset::bitvector::EqualsComplement(const bitvector &B) const 
{
  // check overlap
  for (long w = MIN(NumWords(), B.NumWords())-2; w>=0; w--) 
    if (data[w] != ~B.data[w]) return false;
  // check the last word, igoring bits past size
  long w = MIN(NumWords(), B.NumWords())-1;
  if (NumWords() >= B.NumWords()) {
    if ( data[w] != ((~B.data[w]) & B.tail_mask) ) return false; 
    return FirstUnsetAfter(B.size-1) < 0;
  } 
  // NumWords() < B.NumWords()
  if ( data[w] != ~B.data[w] ) return false; 
  return B.FirstUnsetAfter(size-1) < 0;
} 


inline bool intset::bitvector::EmptyDifference(const bitvector &B) const 
{
  for (long w = B.NumWords(); w<NumWords(); w++)
    if (data[w]) return false; 
  for (long w = MIN(NumWords(), B.NumWords())-1; w>=0; w--)
    if (data[w] & ~B.data[w]) return false;
  return true;
}


inline bool intset::bitvector::EmptyIntersect(const bitvector &B) const 
{
  for (long w = NumWords(); w<B.NumWords(); w++)
    if (B.data[w]) return false;
  for (long w = B.NumWords(); w<NumWords(); w++)
    if (data[w]) return false; 
  for (long w = MIN(NumWords(), B.NumWords())-1; w>=0; w--)
    if (data[w] & B.data[w]) return false;
  return true;
}


inline bool intset::bitvector::FullUnion(const bitvector &B) const 
{
  // check overlap
  for (long w = MIN(NumWords(), B.NumWords())-2; w>=0; w--) 
    if (~(data[w] | B.data[w])) return false;
  long w = MIN(NumWords(), B.NumWords())-1;
  if (NumWords() == B.NumWords()) {
    return ((data[w] | B.data[w]) == (tail_mask | B.tail_mask));
  }
  if (~(data[w] | B.data[w])) return false;
  // check tails
  if (NumWords() < B.NumWords())
    return B.FirstUnsetAfter(size-1) < 0;
  else
    return FirstUnsetAfter(B.size-1) < 0;
}

inline bool intset::bitvector::Empty() const
{
  for (long w = NumWords()-1; w>=0; w--)
    if (data[w]) return false;
  return true;
}


#ifdef INTSET_DEVELOPMENT_CODE
void intset::bitvector::dump(FILE* strm) const
{
  long bitno = 0;
  for (long w = 0; w<NumWords(); w++) {
    fprintf(strm, " ");
    for (long n = 0; n<bitwidth; n++) {
      if (data[w] & msbit >> n) fputc('1', strm); else fputc('0', strm);
      bitno++;
      if (bitno == size) fprintf(strm, ":");
    }
  }
}
#endif

// ======================================================================

intset::intset(long N)
{
  size = N;
  flip = false;
  data = new bitvector(N);
}

intset::intset(const intset &x)
{
  DCASSERT(x.data);
  data = x.data;
  data->Link();
  flip = x.flip;
  size = x.size;
}

intset::~intset()
{
  if (data) data->Unlink();
}

void intset::resetSize(long N)
{
  DCASSERT(data);
  data = data->Modify();
  data->Resize(N);
  size = N;
}

const char* intset::getVersion()
{
  if (0==version) {
    version = new char[80];
    snprintf(version, 80, "Compact integer set library, version %d.%d.%d",
      MAJOR_VERSION, MINOR_VERSION, REVISION_NUMBER);
  }
  return version;
}

long intset::cardinality() const
{
  DCASSERT(data);
  long c = 0;
  long n = -1;
  while ((n=data->FirstSetAfter(n)) >= 0) c++;
  if (flip)   return size - c;
  else        return c;
}

bool intset::isEmpty() const
{
  DCASSERT(data);
  return data->Empty();
}

void intset::addElement(long n)
{
  DCASSERT(data);
  if (n<0 || n>=size) return;
  data = data->Modify();
  if (flip)   data->Unset(n);
  else        data->Set(n);
}

void intset::removeElement(long n)
{
  DCASSERT(data);
  if (n<0 || n>=size) return;
  data = data->Modify();
  if (flip)   data->Set(n);
  else        data->Unset(n);
}

void intset::addRange(long a, long b)
{
  DCASSERT(data);
  data = data->Modify();
  if (flip)   data->UnsetRange(a, b);
  else        data->SetRange(a, b);
}

void intset::removeRange(long a, long b)
{
  DCASSERT(data);
  data = data->Modify();
  if (flip)   data->SetRange(a, b);
  else        data->UnsetRange(a, b);
}

void intset::addAll()
{
  DCASSERT(data);
  data = data->Modify();
  data->SetAll();
  flip = false;
}

void intset::removeAll()
{
  DCASSERT(data);
  data = data->Modify();
  data->UnsetAll();
  flip = false;
}

bool intset::contains(long n) const
{
  DCASSERT(data);
  if (n<0 || n>=size) return 0;
  return (flip != data->IsSet(n));
}

bool intset::testAndAdd(long n)
{
  DCASSERT(data);
  if (n<0 || n>=size) return 0;
  data = data->Modify();
  if (flip)   return !data->UnsetBit_Changed(n);
  else        return !data->SetBit_Changed(n);
}

bool intset::testAndRemove(long n)
{
  DCASSERT(data);
  if (n<0 || n>=size) return 0;
  data = data->Modify();
  if (flip)   return !data->SetBit_Changed(n);
  else        return !data->UnsetBit_Changed(n);
}

long intset::getSmallestAfter(long n) const
{
  DCASSERT(data);
  if (n<0)   n = -1;
  if (flip)   return data->FirstUnsetAfter(n);
  else        return data->FirstSetAfter(n);
}

void intset::assignFrom(const intset &x)
{
  if (&x == this) return;
  data = data->Modify();
  if (size != x.size) {
    data->Resize(x.size);
    size = x.size;
  }
  flip = x.flip;
  memcpy(data->write(), x.data->read(), data->NumWords() * sizeof(long));
}

void intset::operator=(const intset &x)
{
  if (&x == this) return;
  data->Unlink();
  data = x.data;
  DCASSERT(data);
  data->Link();
  flip = x.flip;
  size = x.size;
}

void intset::operator+=(const intset &x)
{
  DCASSERT(data);
  DCASSERT(x.data);
  data = data->Modify();
  if (flip)
    if (x.flip) {
      // ~this + ~x = ~ (this*x)
      data->IntersectWith(*x.data);
    } else {
      // ~this + x = ~ (this-x)
      data->DifferenceWith(*x.data);
    }
  else 
    if (x.flip) {
      // this + ~x = ~ (x-this);
      data->Difference(*x.data, *data);     
      flip = true;
    } else {
      // this + x
      data->UnionWith(*x.data);
    }
}

void intset::operator*=(const intset &x)
{
  DCASSERT(data);
  DCASSERT(x.data);
  data = data->Modify();

  if (flip)
    if (x.flip) {
      // ~this * ~x = ~ (this + x)
      data->UnionWith(*x.data);
    } else {
      // ~this * x = x - this
      data->Difference(*x.data, *data);
      flip = false;
    }
  else
    if (x.flip) {
      // this * ~x = this - x;
      data->DifferenceWith(*x.data);
    } else {
      // this * x
      data->IntersectWith(*x.data);
    }
}

void intset::operator-=(const intset &x)
{
  DCASSERT(data);
  DCASSERT(x.data);
  data = data->Modify();

  if (flip)
    if (x.flip) {
      // ~this - ~x = x - this
      data->Difference(*x.data, *data);
      flip = false;
    } else {
      // ~this - x = ~ (this + x)
      data->UnionWith(*x.data);
    }
  else
    if (x.flip) {
      // this - ~x = this * x;
      data->IntersectWith(*x.data);
    } else {
      // this - x
      data->DifferenceWith(*x.data);
    }
}

// friends here

bool operator==(const intset &x, const intset &y)
{
  DCASSERT(x.data);
  DCASSERT(y.data);
  if (x.flip == y.flip)   return x.data->Equals(*y.data);
  else                    return x.data->EqualsComplement(*y.data);
}

bool operator<=(const intset &x, const intset &y)
{
  DCASSERT(x.data);
  DCASSERT(y.data);
  if (x.flip)
    if (y.flip)
      return y.data->EmptyDifference(*x.data);
    else
      return x.data->FullUnion(*y.data);
  else
    if (y.flip)
      return x.data->EmptyIntersect(*y.data);
    else
      return x.data->EmptyDifference(*y.data);
}

intset operator+ (const intset &x, const intset &y)
{
  DCASSERT(x.data);
  DCASSERT(y.data);
  intset answer(MAX(x.size, y.size));
  DCASSERT(answer.data);
  if (x.flip)
    if (y.flip) {
      // ~x + ~y = ~ (x*y)
      answer.data->Intersect(*x.data, *y.data);
      answer.flip = true;
    } else {
      // ~x + y = ~ (x-y)
      answer.data->Difference(*x.data, *y.data);
      answer.flip = true;
    }
  else 
    if (y.flip) {
      // x + ~y = ~ (y-x);
      answer.data->Difference(*y.data, *x.data);
      answer.flip = true;
    } else {
      // x+y
      answer.data->Union(*x.data, *y.data);
      answer.flip = false;
    }

  answer.size = answer.data->NumBits();
  return answer;
}

intset operator* (const intset &x, const intset &y)
{
  DCASSERT(x.data);
  DCASSERT(y.data);
  intset answer(0);
  DCASSERT(answer.data);
  if (x.flip)
    if (y.flip) {
      // ~x * ~y = ~ (x+y)
      answer.data->Union(*x.data, *y.data);
      answer.flip = true;
    } else {
      // ~x * y = y-x
      answer.data->Difference(*y.data, *x.data);
      answer.flip = false;
    }
  else 
    if (y.flip) {
      // x * ~y = x-y;
      answer.data->Difference(*x.data, *y.data);
      answer.flip = false;
    } else {
      // x*y
      answer.data->Intersect(*x.data, *y.data);
      answer.flip = false;
    }

  answer.size = answer.data->NumBits();
  return answer;
}

intset operator- (const intset &x, const intset &y)
{
  DCASSERT(x.data);
  DCASSERT(y.data);
  intset answer(0);
  DCASSERT(answer.data);
  if (x.flip)
    if (y.flip) {
      // ~x - ~y = y-x
      answer.data->Difference(*y.data, *x.data);
      answer.flip = false;
    } else {
      // ~x - y = ~(x+y)
      answer.data->Union(*x.data, *y.data);
      answer.flip = true;
    }
  else 
    if (y.flip) {
      // x - ~y = x*y;
      answer.data->Intersect(*x.data, *y.data);
      answer.flip = false;
    } else {
      // x-y
      answer.data->Difference(*x.data, *y.data);
      answer.flip = false;
    }
  
  answer.size = answer.data->NumBits();
  return answer;
}

intset operator! (const intset &x)
{
  intset answer(x);
  answer.flip = !answer.flip;
  return answer;
}

#ifdef INTSET_DEVELOPMENT_CODE
void intset::dump(FILE* strm)
{
  if (flip) fprintf(strm, "~");
  else      fprintf(strm, " ");
  data->dump(strm);
}

#endif

