
// $Id$

//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "../Base/timers.h"

void smart_exit()
{
}

timer compute;

void Braised(int n, shared_matrix* B, shared_matrix *tmp, shared_matrix *answer)
{
  int nnz; 
  if (2==n) {
    compute.Start();
    nnz = answer->Multiply(B, B);
  } else if (n%2 == 0) {
    // even, square it
    Braised(n/2, B, answer, tmp);
    compute.Start();
    nnz = answer->Multiply(tmp, tmp);
  } else {
    // odd
    Braised(n-1, B, answer, tmp);
    compute.Start();
    nnz = answer->Multiply(B, tmp);
  }
  compute.Stop();
  Output << "Computed B^" << n << "\t\t\t";
  Output << answer->Distinct() << " distinct / ";
  Output << nnz << " nzs\n";
  Output << "\titeration took " << compute.User_Seconds() << " secs\n";
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

  if (CYCLES < 2) {
    answer.MakeB(M, A);
    return;
  }

  shared_matrix B(N);
  shared_matrix tmp(N);

  B.MakeB(M, A);
  Output << "Computed B\n";
  Output.flush();

  Braised(CYCLES, &B, &tmp, &answer);

  MatrixStats();
}


int main(int argc, char** argv)
{
  if (argc<3) {
    Output << "\tComputes B^N, B is the one-step MT19937 matrix\n";
    Output << "\tUsage: " << argv[0] << " N <outfile>\n\n";
    return 0;
  }
  
  CYCLES = atoi(argv[1]);
  if (CYCLES < 1) {
    Output << "Illegal N: " << CYCLES << "\n";
    return 0;
  }

  timer compute;
  compute.Start();
  
  shared_matrix refresh(N);
  MakeMatrix(refresh);

  FILE* out = fopen(argv[2], "w");
  Verbose.SwitchDisplay(out);
  Verbose.Activate();
  refresh.write(Verbose);
  Verbose.flush();
  
  compute.Stop();
  Output << "Total time: " << compute << "\n";
  return 0;
}
