
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "../Base/streams.cc"

// This is just wrong...

#include <iostream>

#include "bitmatrix.h"

// #include "mtwist.h"

// fancy stuff

#include "../memmgr.h"
// #include "../splay.h"

//#define DEBUG_MATRIX

//#define COMPARE_STREAMS
#define DUMP_MATRIX

#define FEEDBACK

Manager <bitmatrix> matrix_pile(16);

int hashsizes[15] = { 	0, 251, 503, 1013, 2027, 4057, 8117, 16249, 32503, 
			65011, 130027, 260081, 520193, 1040387, 2147483647 };

class myhash {
  struct node {
    bitmatrix *data;
    node *next;
  };
  int size_index;
  int count;
  int maxchain;
  node** table;
  Manager <node> *node_pile;
protected:
  void Resize(int delta);
public:
  myhash();
  ~myhash();
  bitmatrix* Find(bitmatrix *);
  bitmatrix* Insert(bitmatrix *x);
  bool Remove(bitmatrix *);
  inline int Entries() const { return count; }
  inline int Size() const { return hashsizes[size_index]; }
  inline int MaxChain() const { return maxchain; }
};

myhash::myhash()
{
  size_index = 1;
  table = (node **) malloc(sizeof(void*) * Size());
  int i;
  for (i=0; i<Size(); i++) table[i] = NULL;
  count = 0;
  maxchain = 0;
  node_pile = new Manager <node> (16);
}

myhash::~myhash()
{
  delete node_pile;
  free(table);
}

void myhash::Resize(int delta)
{
  // build huge list
  node* list = NULL;
  int i = 0;
  for (i=0; i<Size(); i++) {
    while (table[i]) {
      node* link = table[i]->next;
      table[i]->next = list;
      list = table[i];
      table[i] = link;
    }
    i++;
  }
  // hash table = one huge list
  size_index += delta;
  table = (node **) realloc(table, sizeof(void*) * hashsizes[size_index]);
  for (i=0; i<Size(); i++) table[i] = NULL;
  // put them back into the list
  while (list) {
    int h = list->data->Signature(Size());
    node* link = list->next;
    list->next = table[h];
    table[h] = list;
    list = link;
  }
  maxchain = 0;
}

bitmatrix* myhash::Find(bitmatrix *m)
{
  int h = m->Signature(Size());
  node *ptr;
  node *prev = NULL;
  for (ptr = table[h]; ptr; ptr=ptr->next) {
    if (0 == Compare(ptr->data, m)) {
      if (prev) {  // not at front, make it so
	prev->next = ptr->next;
	ptr->next = table[h];
	table[h] = ptr;
      }
      return ptr->data;
    }
  } // for
  return NULL;
}

bitmatrix* myhash::Insert(bitmatrix *m)
{
  if (count > hashsizes[size_index+1]) Resize(1);
  int h = m->Signature(Size());
  node *ptr;
  node *prev = NULL;
  int time = 0;
  for (ptr = table[h]; ptr; ptr=ptr->next) {
    time++;
    if (0 == Compare(ptr->data, m)) {
      if (prev) {  // not at front, make it so
	prev->next = ptr->next;
	ptr->next = table[h];
	table[h] = ptr;
      }
      return ptr->data;
    }
    prev = ptr;
  } // for
  // not there, insert it
  count++;
  time++;
  maxchain = MAX(maxchain, time);
  ptr = node_pile->NewObject();
  ptr->data = m;
  ptr->next = table[h];
  table[h] = ptr;
  return m;
}

bool myhash::Remove(bitmatrix *m)
{
  // if (count < hashsizes[size_index-1]) Resize(-1);
  int h = m->Signature(Size());
  node *ptr;
  node *prev = NULL;
  for (ptr = table[h]; ptr; ptr=ptr->next) {
    if (ptr->data == m) {
      if (prev) {  
	prev->next = ptr->next;
      } else {
	table[h] = ptr->next;
      }
      count--;
      node_pile->FreeObject(ptr);
      return true;
    }
    prev = ptr;
  } // for
  // not there
  return false;
}

void smart_exit()
{
}

myhash UniqueTable;

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

// const int N = 7;
// const int M = 4;

struct matrix {
  bitmatrix*** ptrs;
  matrix();
  ~matrix();
  void zero();
  void null();
  void show(OutputStream &s);
#ifdef COMPARE_STREAMS
  void vmult(mt_state *x, mt_state *y);
#endif
  void CompactMatrix();
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

void matrix::null()
{
  int i,j;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) ptrs[i][j] = NULL;
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
      s.flush();
      ptrs[i][j]->show(s);
      ptrs[i][j]->flag = true;
    }
}

#ifdef COMPARE_STREAMS
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
#endif

void matrix::CompactMatrix()
{
  int i,j;
  for (i=0; i<N; i++)
    for (j=0; j<N; j++) {
      if (NULL == ptrs[i][j])
	ptrs[i][j] = ZERO;
      else {
	bitmatrix *d = UniqueTable.Insert(ptrs[i][j]);
	if (d != ptrs[i][j]) {
	  matrix_pile.FreeObject(ptrs[i][j]);
	  ptrs[i][j] = d;
	}
      }
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
  a->count = 0;
  mm_mult(a, b, c);

#ifdef DEBUG_MULT
  Output << " got ";
  showbm(Output, a);
  Output << "\n";
#endif
  return a;
}

// A = B * C
// it is ok if B==C, but A must be unique
void Multiply(matrix *A, matrix *B, matrix *C)
{
  int i,j,k;
  for (i=0; i<N; i++) {
    for (j=0; j<N; j++) {
      // compute A[i][j]
      if (NULL==A->ptrs[i][j]) {
        A->ptrs[i][j] = matrix_pile.NewObject();
	A->ptrs[i][j]->count = 1;
      } else if (A->ptrs[i][j]->count>1) {
	A->ptrs[i][j]->count--;  // detach
        A->ptrs[i][j] = matrix_pile.NewObject();
	A->ptrs[i][j]->count = 1;
      } else {
	// remove
	UniqueTable.Remove(A->ptrs[i][j]);
      }
      A->ptrs[i][j]->zero();
      for (k=0; k<N; k++) {
	bitmatrix* term = MyMultiply(B->ptrs[i][k], C->ptrs[k][j]);
	if (term != ZERO) {
	  mm_acc(A->ptrs[i][j], term);	
	  if (0==term->count) {
	    // temporary, trash it
	    matrix_pile.FreeObject(term);
	  }
	}
      }
      // find duplicates
      bitmatrix *d = UniqueTable.Insert(A->ptrs[i][j]);
      if (d != A->ptrs[i][j]) {
	d->count++;
	matrix_pile.FreeObject(A->ptrs[i][j]);
	A->ptrs[i][j] = d;
      }
    }
#ifdef FEEDBACK
    if (i % 8 == 0) std::cerr << ".";
#endif
  } // for i
#ifdef FEEDBACK
  std::cerr << "\n";
#endif
}

matrix *B;

void Init()
{
  int i;
  ZERO = matrix_pile.NewObject();
  ZERO->count = 1;
  for (i=0; i<32; i++) ZERO->row[i] = 0;

  IDENTITY = matrix_pile.NewObject();
  IDENTITY->count = 1;
  for (i=0; i<32; i++) IDENTITY->row[i] = mask[i]; 

  L = matrix_pile.NewObject();
  L->count = 1;
  L->row[0] = 0;
  for (i=1; i<31; i++) L->row[i] = mask[i+1];
  L->row[31] = 0x9908b0df;  // A from MT19937

  U = matrix_pile.NewObject();
  U->count = 1;
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
  int JUMPDISTANCE;
  Output << "Enter exponent for jump distance\n";
  Output.flush();
  Input.Get(JUMPDISTANCE);
  Init();

  /*
  mt_state foo; 
  mt_seed32(7309259);
  mts_seed32(&foo, 7309259);

  mt_state bar;

  mt_state* current = &foo;
  mt_state* next = &bar;
  */

  if (0==JUMPDISTANCE) {
    B->show(Output);
    Output.flush();
    return 0;
  }

  Output << "Computing B matrix\n";
  Output.flush();
  /*
  const int CYCLES = 1;
  int POWER = N*CYCLES;
  matrix *thing = Braised(POWER); // two cycles
  */
  matrix *tmp = new matrix();
  tmp->null();
  matrix *jump = new matrix();
  jump->null();
  Multiply(jump, B, B);
  int expo = 1;
  for (;expo<JUMPDISTANCE; expo++) {
    Output << "Computed B^(2^" << expo << ")\t\t";
    Output << UniqueTable.Entries() << " subs\t";
    Output << UniqueTable.Size() << " table\t";
    Output << UniqueTable.MaxChain() << " chain\n";
    Output.flush();
    Multiply(tmp, jump, jump);
    SWAP(jump, tmp);
  }
  Output << "Computed B^(2^" << expo << ")\t\t";
  Output << UniqueTable.Entries() << " submatrices\t\t";
  Output << UniqueTable.Size() << " table\n";
  Output.flush();
#ifdef DUMP_MATRIX
  Output << "Matrix is:\n";
  jump->show(Output);
#endif

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
  Output.flush();
  return 0;
}
