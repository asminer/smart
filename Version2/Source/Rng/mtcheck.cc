
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "../Base/streams.cc"

#include "bitmatrix.h"

#include "mtwist.h"

// fancy stuff

#include "../memmgr.h"
#include "../splay.h"

//#define DEBUG_MATRIX

#define COMPARE_STREAMS
//#define DUMP_MATRIX

Manager <bitmatrix> matrix_pile(16);

class mysplay {
  SplayWrap <bitmatrix> splaywrapper;
  PtrSplay::node *root;
  int node_count;
  Manager <PtrSplay::node> *node_pile;
public:
  mysplay();
  ~mysplay();
  bitmatrix* Insert(bitmatrix *);
  int Nodes() { return node_count; }
};


mysplay::mysplay()
{
  root = NULL;
  node_count = 0;
  node_pile = new Manager <PtrSplay::node> (16);
}

mysplay::~mysplay()
{
  delete node_pile;
}

bitmatrix* mysplay::Insert(bitmatrix *m)
{
  int foo = splaywrapper.Splay(root, m);
  if (0==foo) {
    // found!
    return (bitmatrix*)root->data;
  }
  PtrSplay::node *x = node_pile->NewObject();
  x->data = m;
  if (foo>0) {
      // root > x
      x->right = root; 
      if (root) {
        x->left = root->left;
        root->left = NULL;
      } else {
	x->left = NULL;
      }
  } else {
      // root < x
      x->left = root; 
      if (root) {
        x->right = root->right;
        root->right = NULL;
      } else {
	x->right = NULL;
      }
  }
  root = x;
  node_count++;
  return m;
}

void smart_exit()
{
}

mysplay UniqueTable;

bitmatrix *ZERO;
bitmatrix *IDENTITY;
bitmatrix *L;
bitmatrix *U;
bitmatrix *C = NULL;

void showbm(OutputStream &s, bitmatrix *b)
{
  if (b==ZERO) {
    s << "0";
    return;
  }
  if (b==IDENTITY) {
    s << "I";
    return;
  }
  if (b==L) {
    s << "L";
    return;
  }
  if (b==U) {
    s << "U";
    return;
  }
  void *x = b;
  int foo = (int)x;
  s << foo;
}


const int N = 624;
const int M = 397;

struct matrix {
  bitmatrix*** ptrs;
  matrix();
  ~matrix();
  void zero();
  void show(OutputStream &s);
  void vmult(mt_state *x, mt_state *y);
};

matrix::matrix()
{
  ptrs = new bitmatrix**[N];
  int i,j;
  for (i=0; i<N; i++) {
    ptrs[i] = new bitmatrix*[N];
  }
}

matrix::~matrix()
{
  int i;
  for (i=0; i<N; i++) delete[] ptrs[i];
  delete[] ptrs;
}

void matrix::zero()
{
  int i,j;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) ptrs[i][j] = ZERO;
  }
}

void matrix::show(OutputStream &s)
{
  int i,j;
  for (i=0; i<N; i++) {
    s << "[";
    for (j=0; j<N; j++) {
      if (j) s << ", ";
      showbm(s, ptrs[i][j]);
      ptrs[i][j]->flag = false;
    }
    s << "]\n";
    s.flush();
  }
  ZERO->flag = true;
  IDENTITY->flag = true;
  for (i=0; i<N; i++) 
    for (j=0; j<N; j++) {
      if (ptrs[i][j]->flag) continue;
      s << "Submatrix ";
      showbm(s, ptrs[i][j]);
      s << "\n";
      ptrs[i][j]->show(s);
      ptrs[i][j]->flag = true;
    }
}

// x = y * thismatrix
void matrix::vmult(mt_state *x, mt_state *y)
{
  int i,j;
  for (i=0; i<N; i++) x->statevec[i] = 0;
  for (i=0; i<N; i++) 
    for (j=0; j<N; j++) 
      if (ptrs[i][j] != ZERO) {
        x->statevec[j] ^= ptrs[i][j]->vm_mult(y->statevec[i]);
      }
}

bitmatrix* MyMultiply(bitmatrix *b, bitmatrix *c)
{
  if ((b==ZERO) || (c==ZERO)) return ZERO;
  if (b==IDENTITY) return c;
  if (c==IDENTITY) return b;

#ifdef DEBUG_MULT
  Output << "Multiplying ";
  showbm(Output, b);
  Output << " and ";
  showbm(Output, c);
  Output << ": ";
#endif


  bitmatrix *a = matrix_pile.NewObject();
  mm_mult(a, b, c);
  bitmatrix *d = UniqueTable.Insert(a);

  if (a!=d) {
    // duplicate, recycle a
    matrix_pile.FreeObject(a);
  }

#ifdef DEBUG_MULT
  Output << " got ";
  showbm(Output, d);
  Output << "\n";
#endif
  return d;
}

// A = B * C
// it is ok if B==C, but A must be unique
void Multiply(matrix *A, matrix *B, matrix *C)
{
  int i,j,k;
  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      // compute A[i][j]
      bitmatrix* acc = matrix_pile.NewObject();
      acc->zero();
      for (k=0; k<N; k++) {
	bitmatrix* term = MyMultiply(B->ptrs[i][k], C->ptrs[k][j]);
	if (term != ZERO) mm_acc(acc, term);	
      }

      bitmatrix* d = UniqueTable.Insert(acc);
      if (acc!=d) matrix_pile.FreeObject(acc);
      A->ptrs[i][j] = d;
    }
}

matrix *B;

void Init()
{
  int i;
  ZERO = matrix_pile.NewObject();
  for (i=0; i<32; i++) ZERO->row[i] = 0;

  IDENTITY = matrix_pile.NewObject();
  for (i=0; i<32; i++) IDENTITY->row[i] = mask[i]; 

  L = matrix_pile.NewObject();
  L->row[0] = 0;
  for (i=1; i<31; i++) L->row[i] = mask[i+1];
  L->row[31] = 0x9908b0df;  // A from MT19937

  U = matrix_pile.NewObject();
  U->row[0] = mask[1];
  for (i=1; i<32; i++) U->row[i] = 0;

  UniqueTable.Insert(ZERO);
  UniqueTable.Insert(IDENTITY);
  UniqueTable.Insert(L);
  UniqueTable.Insert(U);

  B = new matrix();
  B->zero();
  for (i=1; i<N; i++)
    B->ptrs[i-1][i] = IDENTITY;
  B->ptrs[N-M-1][0] = IDENTITY;
  B->ptrs[N-2][0] = L;
  B->ptrs[N-1][0] = U;

#ifdef DEBUG_MATRIX
  Output << "Zero matrix:\n";
  ZERO->show(Output);
  Output << "Identity matrix:\n";
  IDENTITY->show(Output);
  Output << "L matrix:\n";
  L->show(Output);
  Output << "U matrix:\n";
  U->show(Output);

  Output << "B matrix:\n";
  B->show(Output);
#endif 

}

matrix* Braised(int n)
{
  matrix *answer;
  if (2==n) {
    answer = new matrix;
    Multiply(answer, B, B);
  } else if (n%2 == 0) {
    // even, square it
    matrix *tmp = Braised(n/2);
    answer = new matrix;
    Multiply(answer, tmp, tmp);
    delete tmp;
  } else {
    // odd
    matrix *tmp = Braised(n-1);
    answer = new matrix();
    Multiply(answer, B, tmp);
    delete tmp;
  }
  Output << "Computed B^" << n << "\n";
  Output.flush();
  return answer;
}

int main()
{
  Init();

  mt_state foo; 
  mt_seed32(7309259);
  mts_seed32(&foo, 7309259);

  mt_state bar;

  mt_state* current = &foo;
  mt_state* next = &bar;

  Output << "Computing B matrix\n";
  Output.flush();
  const int CYCLES = 1;
  int POWER = N*CYCLES;
  matrix *thing = Braised(POWER); // two cycles

#ifdef COMPARE_STREAMS
  Output << "Comparing generators\n";
  Output.flush();

  int j;
  int i;
  int c;
  for (j=0; j<1000; j++) {
    for (c=0; c<CYCLES; c++) {
      mts_refresh(&mt_default_state);
    }
    thing->vmult(next, current);
    SWAP(next, current);
    for (i=N-1; i>=0; i--) {
      int a = mt_default_state.statevec[i];
      int b = current->statevec[i];
      if (a!=b) {
        Output.Put(j, 5);
        Output.Put(i, 5);
        Output.Pad(5);
        Output.Put(a, 14);
        Output.Pad(5);
        Output.Put(b, 14);
        Output.Pad(5);
        Output << "\n";
        Output.flush();
      }
    }
  }
  Output << "Streams matched up to " << j << " cycles\n";
  Output << "Done\n";
#endif
  Output << "Peak of " << UniqueTable.Nodes() << " unique bit matrix\n";
#ifdef DUMP_MATRIX
  Output << "Matrix B^" << POWER << " =\n";
  thing->show(Output);
#endif
  Output.flush();
  return 0;
}
