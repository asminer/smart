
#ifndef BITMATRIX_H
#define BITMATRIX_H

unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };


const int N = 7;

class bigmatrix {
protected
  unsigned int** row;
public:
  bigmatrix();
  ~bigmatrix();

  inline void column_dot_product(int cw, unsigned int *vect, unsigned int &a) {
    for (int i=N-1; i>=0; i--) 
      for (register int b=31; b>=0; b--) {
        if (vect[i] & mask[b]) a ^= row[i*32+b][cw];
      }
  }
  inline void zero() {
    for (int i=N-1; i>=0; i--) row[0][i] = 0;
    for (int i=32*N-1; i; i--)
      memcpy(row[i], row[0], N * sizeof(int));
  }
};


struct bitmatrix {
  unsigned int row[32];
  bool flag;
  int count;

  inline unsigned int vm_mult(unsigned int v) {
    unsigned int answer = 0;
    for (int b=0; b<32; b++)
      if (v & mask[b]) answer ^= row[b];
    return answer;
  }

  inline void show(OutputStream &s) {
    for (int r=0; r<32; r++) {
      s << '[';
      for (int c=0; c<32; c++) if (row[r] & mask[c]) s << '1'; else s << '0';
      s << "]\n";
    }
  }

  inline void zero() {
    for (int r=0; r<32; r++) row[r] = 0;
  }

  inline int Signature(int prime) {
    // return ((row[31] % prime) * 256 + row[29]) % prime;
    return ((row[31] % prime) * 256 + row[0]) % prime;
  }
};

/// a = b * c
inline void mm_mult(bitmatrix *a, bitmatrix *b, bitmatrix *c)
{
  for (int i=0; i<32; i++) a->row[i] = c->vm_mult(b->row[i]);
}

/// a = b + c
inline void mm_add(bitmatrix *a, bitmatrix *b, bitmatrix *c)
{
  for (int i=0; i<32; i++) a->row[i] = b->row[i] ^ c->row[i];
}

/// a += c
inline void mm_acc(bitmatrix *a, bitmatrix *c)
{
  for (int i=0; i<32; i++) a->row[i] ^= c->row[i];
}

inline int Compare(bitmatrix *a, bitmatrix *b)
{
  for (int i=0; i<32; i++) {
    if (a->row[i] < b->row[i]) return -1;
    if (a->row[i] > b->row[i]) return 1;
  }
  return 0;
}

#endif

