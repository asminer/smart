
// $Id$

#ifndef MULTSTRM_H
#define MULTSTRM_H

#include "../Base/api.h"
#include "../Templates/memmgr.h"

const unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };

/// Bottom-level submatrix
struct bitmatrix {
  unsigned long row[32];
  int ptrcount;
  int cachecount;
  int flag;

  inline void vm_mult(unsigned long v, unsigned long &answer) {
    if (0==v) return;
    // for (int b=0; b<32; b++) 
    for (int b=31; b>=0; b--)
      if (row[b])
        if (v & mask[b]) 
	  answer ^= row[b];
  }

  void show(OutputStream &s) {
    s << "Submatrix " << flag;
    s << "\n\t" << ptrcount << " ptrs\t" << cachecount << " cache\n";
    for (int r=0; r<32; r++) {
      s << '[';
      for (int c=0; c<32; c++) if (row[r] & mask[c]) s << '1'; else s << '0';
      s << "]\n";
    }
    s.flush();
  }

  void write(OutputStream &s) {
    for (int r=0; r<32; r++) { s << row[r] << " "; }
  }

  void writeC(OutputStream &s) {
    s << "submatrix b" << flag << " = { ";
    for (int r=0; r<32; r++) {
      if (r) s << ", ";
      s.PutHex(row[r]);
    }
    s << " };\n";
    s.flush();
  }

  bool read(InputStream &s) {
    for (int r=0; r<32; r++) if (!s.Get(row[r])) return false;
    return true;
  }

  inline void zero() {
    for (int r=31; r>=0; r--) row[r] = 0;
  }

  inline void FillFrom(bitmatrix *x) {
    if (NULL==x) 
      zero();
    else
      for (int r=0; r<32; r++) row[r] = x->row[r];
  }

  inline bool is_zero() {
    for (int r=31; r>=0; r--) if (row[r]) return false;
    return true;
  }

  inline unsigned long Signature(long prime) {
    // return (row[31] ^ row[15] ^ row[0]) % prime;
    return ((((row[31] % prime) * 256 + row[15]) % prime) * 256 + row[0]) % prime;
  }
};

/// a = b * c
inline void mm_mult(bitmatrix *a, bitmatrix *b, bitmatrix *c)
{
  for (int i=0; i<32; i++) {
    a->row[i] = 0;
    c->vm_mult(b->row[i], a->row[i]);
  }
}


/// a += c
inline void mm_acc(bitmatrix *a, bitmatrix *c)
{
  for (int i=0; i<32; i++) a->row[i] ^= c->row[i];
}




/// middle level in 3-level mxd
struct midmatrix {
  bitmatrix* mx[16][16];
  int flag;

  unsigned long Signature(long prime) {
    unsigned long d1 = mx[15][15] ? mx[15][15]->Signature(prime) : 0;
    unsigned long d2 = mx[8][7] ? mx[8][7]->Signature(prime) : 0;
    unsigned long d3 = mx[0][0] ? mx[0][0]->Signature(prime) : 0;
    return (((d1 * 256 + d2) % prime) * 256 + d3) % prime;
  }

  void ClearFlags();
  void writeCbelow(OutputStream &s, int &botcnt);
  void writeC(OutputStream &s);
};





/// A 2-level Mxd, essentially ;^)
class shared_matrix {
  int N;
  bitmatrix*** ptrs;
  bool initialized;
public:
  shared_matrix(int n);
  ~shared_matrix();
  void MakeB(int M, unsigned int A);
  void MakeBN(int M, unsigned int A);
  inline int Size() const { return N; }
  // Human readable
  void show(OutputStream &s);
  // Less so.
  void write(OutputStream &s);
  void writeC(OutputStream &s);
  void read(InputStream &s);
  int Distinct();
  /// conversion to 3-level
  void FillMiddle(int rs, int cs, midmatrix *m) {
    int i, j;
    for (i=15; i>=0; i--)
      for (j=15; j>=0; j--)
	m->mx[i][j] = ptrs[rs+i][cs+j];
  }
  
  /// x = y * this;
  inline void vector_multiply(unsigned long *x, unsigned long *y) {
    int i,j;
    for (i=N-1; i>=0; i--) x[i] = 0;
    for (i=N-1; i>=0; i--)
      for (j=N-1; j>=0; j--)
	if (ptrs[i][j])
	  ptrs[i][j]->vm_mult(y[i], x[j]);
  }

  /// this = b * c, returns #nonzeroes
  int Multiply(shared_matrix *b, shared_matrix *c);

  /** For debugging.
      Returns true if this == b * c
  */
  bool CheckMultiply(shared_matrix *b, shared_matrix *c);

  int CheckShift(shared_matrix *a);
  
protected:
  inline void SetPtr(int i, int j, bitmatrix* m) {
    if (ptrs[i][j]) {
      DCASSERT(ptrs[i][j]->ptrcount>0);
      ptrs[i][j]->ptrcount--;
    }
    ptrs[i][j] = m;
    if (m) m->ptrcount++;
  }
  void ColCpy(int dest, int src);

};


/// Top level in 3-level mxd
class topmatrix {
  int N;
  midmatrix*** ptrs;
public:
  topmatrix(shared_matrix *foo);
  ~topmatrix();
  void writeC(OutputStream &s);
  int midcount;
  int botcount;
};


void InitMatrix();

void MatrixStats();

void GarbageCollect();

#endif

