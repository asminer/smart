
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "../Base/timers.h"

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
  Output << "Jump to 2^ ???\n";
  Output.flush();
  int expo;
  Input.Get(expo);

  timer compute;
  int i;

  shared_matrix *answer;
  shared_matrix *tmp;

  answer = new shared_matrix(N);
  answer->MakeB(M, A);
  tmp = new shared_matrix(N);
  
  for (i=1; i<=expo; i++) {
    compute.Start();
    int nnz = tmp->Multiply(answer, answer);
    SWAP(tmp, answer);
    compute.Stop();
    Output << "Computed B^(2^" << i << ")\n";
    Output << "\titeration took " << compute << "\n";
    Output << "\tmultiplier: " << answer->Distinct() << " distinct / ";
    Output << nnz << " nonzeroes\n";
    MatrixStats();
  }
  
  Output << "That's all for now\n";
  Output.flush();
  return 0;
}
