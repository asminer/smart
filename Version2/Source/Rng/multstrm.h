
#ifndef MULTSTRM_H
#define MULTSTRM_H

unsigned int mask[32] = { 0x80000000, 0x40000000, 0x20000000, 0x10000000,
			  0x08000000, 0x04000000, 0x02000000, 0x01000000,

			  0x00800000, 0x00400000, 0x00200000, 0x00100000,
			  0x00080000, 0x00040000, 0x00020000, 0x00010000,

			  0x00008000, 0x00004000, 0x00002000, 0x00001000,
			  0x00000800, 0x00000400, 0x00000200, 0x00000100,

			  0x00000080, 0x00000040, 0x00000020, 0x00000010,
			  0x00000008, 0x00000004, 0x00000002, 0x00000001 };


struct bigmatrix {
  int N;
  unsigned int** row;
  bigmatrix(int n);
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
  void Show(OutputStream &s);
};

/// a = b * c
inline void mm_mult(bigmatrix *a, bigmatrix *b, bigmatrix *c)
{
  for (int i=32*N-1; i>=0; i--)
    for (int j=N-1; j>=0; j--) {
      a->row[i][j] = 0;
      c->column_dot_product(j, b->row[i]);
    }
}

