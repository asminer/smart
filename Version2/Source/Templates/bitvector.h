
// $Id$

#ifndef BITVECTOR_H
#define BITVECTOR_H

const unsigned int bitvector_mask[32] = 
	{ 0x80000000, 0x40000000, 0x20000000, 0x10000000,
	  0x08000000, 0x04000000, 0x02000000, 0x01000000,
	  0x00800000, 0x00400000, 0x00200000, 0x00100000,
	  0x00080000, 0x00040000, 0x00020000, 0x00010000,
	  0x00008000, 0x00004000, 0x00002000, 0x00001000,
	  0x00000800, 0x00000400, 0x00000200, 0x00000100,
	  0x00000080, 0x00000040, 0x00000020, 0x00000010,
	  0x00000008, 0x00000004, 0x00000002, 0x00000001 };

const unsigned int bitvector_notm[32] = 
	{ 0x7FFFFFFF, 0xBFFFFFFF, 0xDFFFFFFF, 0xEFFFFFFF,
	  0xF7FFFFFF, 0xFBFFFFFF, 0xFDFFFFFF, 0xFEFFFFFF,
	  0xFF7FFFFF, 0xFFBFFFFF, 0xFFDFFFFF, 0xFFEFFFFF,
	  0xFFF7FFFF, 0xFFFBFFFF, 0xFFFDFFFF, 0xFFFEFFFF,
	  0xFFFF7FFF, 0xFFFFBFFF, 0xFFFFDFFF, 0xFFFFEFFF,
	  0xFFFFF7FF, 0xFFFFFBFF, 0xFFFFFDFF, 0xFFFFFEFF,
	  0xFFFFFF7F, 0xFFFFFFBF, 0xFFFFFFDF, 0xFFFFFFEF,
	  0xFFFFFFF7, 0xFFFFFFFB, 0xFFFFFFFD, 0xFFFFFFFE };

/**   An array of bits.

      Use this class if you KNOW you have an array of bits.
*/  

class bitvector {
  int size;
  unsigned int *data;
public:
  bitvector(int s) { 
    data = NULL;
    size = 0;
    Resize(s);
  }
  ~bitvector() { 
    Resize(0);
  }
  void Resize(int ns) {
    int words = (ns>0) ? (((ns-1)/32)+1) : 0;
    int oldwords = (size>0) ? (((size-1)/32)+1) : 0;
    if (words != oldwords) {
      unsigned int *foo = (unsigned int*) realloc(data, words*sizeof(unsigned int));
      if (words && (NULL==foo)) OutOfMemoryError("bitvector resize");
      data = foo;
    } 
    size = ns;
  }
  inline int Size() const { return size; }
  inline int MemUsed() const { return (size>0) ? 4*(((size-1)/32)+1) : 0; }
  inline void Set(int n) {
    CHECK_RANGE(0, n, size);
    data[n/32] |= bitvector_mask[n%32]; // set bit n
  }
  inline void Unset(int n) {
    CHECK_RANGE(0, n, size);
    data[n/32] &= bitvector_notm[n%32]; // clear bit n      
  }
  inline bool IsSet(int n) const {
    CHECK_RANGE(0, n, size);
    return (data[n/32] & bitvector_mask[n%32]) > 0;  
  }
  inline void UnsetAll() {
    int i;
    for(i=(size-1)/32; i>=0; i--) data[i] = 0;
  }
  inline void SetAll() {
    int i;
    for(i=(size-1)/32; i>=0; i--) data[i] = 0xFFFFFFFF;
  }
  /// Unset all bits from b1 to b2, inclusive
  inline void UnsetRange(int b1, int b2) {
    CHECK_RANGE(0, b1, size);
    CHECK_RANGE(b1, b2, size);
    // do the stray bits up to the word boundary
    while ((b1 <= b2) && (b1 % 32 > 0)) Unset(b1++);
    // do words at a time
    while (b1+31 <= b2) {
      data[b1/32] = 0; 
      b1 += 32;
    }
    // do stray bits at the end
    while (b1 <= b2) Unset(b1++);
  }
  /// Set all bits from b1 to b2, inclusive
  inline void SetRange(int b1, int b2) {
    CHECK_RANGE(0, b1, size);
    CHECK_RANGE(b1, b2, size);
    // do the stray bits up to the word boundary
    while ((b1 <= b2) && (b1 % 32 > 0)) Set(b1++);
    // do words at a time
    while (b1+31 <= b2) {
      data[b1/32] = 0xFFFFFFFF; 
      b1 += 32;
    }
    // do stray bits at the end
    while (b1 <= b2) Set(b1++);
  }
};

#endif

