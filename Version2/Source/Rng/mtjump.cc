
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "../Base/timers.h"

//#define USE_SHARED

void smart_exit()
{
}


const unsigned int A = 0x9908b0df;
const int N = 624;
const int M = 397;



int main()
{
  Output << "Jump to 2^ ???\n";
  Output.flush();
  int expo;
  Input.Get(expo);

  timer compute;
  int i;

#ifdef USE_SHARED
  shared_matrix *answer;
  shared_matrix *tmp;
  answer = new shared_matrix(N);
  answer->MakeB(M, A);
  tmp = new shared_matrix(N);
#else
  shared_matrix B(N);
  B.MakeB(M, A);
  fullmatrix *answer;
  fullmatrix *tmp;
  answer = new fullmatrix(N);
  answer->FillFrom(B);
  tmp = new fullmatrix(N);
#endif
  
  for (i=1; i<=expo; i++) {
    compute.Start();
    int nnz = tmp->Multiply(answer, answer);
    SWAP(tmp, answer);
    compute.Stop();
    Output << "Computed B^(2^" << i << ")\n";
    Output << "\titeration took " << compute << "\n";
    Output << "\tmultiplier: ";
#ifdef USE_SHARED
    Output << answer->Distinct() << " distinct / ";
#endif
    Output << nnz << " nonzeroes\n";
#ifdef USE_SHARED
    MatrixStats();
#else
    Output.flush();
#endif
  }
  
  Output << "That's all for now\n";
  Output.flush();
  return 0;
}
