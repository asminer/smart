
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "mtwist.h"

void smart_exit()
{
}

void Braised(int n, shared_matrix* B, shared_matrix *tmp, shared_matrix *answer)
{
  int nnz; 
  if (2==n) {
    nnz = answer->Multiply(B, B);
  } else if (n%2 == 0) {
    // even, square it
    Braised(n/2, B, answer, tmp);
    nnz = answer->Multiply(tmp, tmp);
  } else {
    // odd
    Braised(n-1, B, answer, tmp);
    nnz = answer->Multiply(B, tmp);
  }
  Output << "Computed B^" << n << "\t\t\t";
  Output << nnz << " nonzeroes\n";
  Output.flush();
}

const unsigned int A = 0x9908b0df;
const int N = 624;
const int M = 397;
//const int N = 7;
//const int M = 4;

int CYCLES = 1;

void MakeMatrix(shared_matrix &answer)
{
  Output << "Initializing...\n";
  Output.flush();

  shared_matrix B(N);
  shared_matrix tmp(N);

  B.MakeB(M, A);
  Output << "Computed B\n";
  Output.flush();

  Braised(N*CYCLES, &B, &tmp, &answer);

  MatrixStats();
}

int main()
{
  Output << "How many refresh cycles?\n";
  Output.flush();

  Input.Get(CYCLES);
  shared_matrix refresh(N);
  MakeMatrix(refresh);

  Output << "Comparing against MT19937\n";
  Output.flush();
  int c;
  int rep;

  mt_state mine1, mine2;
  mts_seed32(&mine1, 7309259);
  mt_seed32(7309259);

  mt_state* current = &mine1;
  mt_state* next = &mine2;
 
  for (rep=0; rep<1000; rep++) {
    // advance seeds CYCLES times
    // us:
    refresh.vector_multiply(next->statevec, current->statevec);
    SWAP(next, current);
    // them:
    for (c=0; c<CYCLES; c++) mts_refresh(&mt_default_state);

    // compare vectors
    int i;
    for (i=0; i<N; i++) {
      if (current->statevec[i] != mt_default_state.statevec[i]) {
        Output << "Replication " << rep << "\n";
        Output << "Difference at position " << i << ":\n";
	Output << "Us  : " << (int) current->statevec[i] << "\n";
	Output << "Them: " << (int) mt_default_state.statevec[i] << "\n";
	Output.flush();
	return 0;
      }
    } // for i
  }
  
  Output << "That's all for now\n";
  Output.flush();
  return 0;
}
