
// $Id$

/* Test of multiple stream stuff */

#include "../Base/timers.h"
#include "multstrm.h"

const unsigned int A = 0x9908b0df;
const int N = 624;
const int M = 397;

void smart_exit()
{
}


int main(int argc, char** argv)
{
  if (argc<4) {
    Output << "\tComputes A^(2^N) for matrix A\n";
    Output << "\tMatrix A is read from a file.  Output is to a file.\n\n";
    Output << "\tUsage: " << argv[0] << " <infile> N <outfile>\n\n";
    return 0;
  }

  FILE* in = fopen(argv[1], "r");
  int iters = atoi(argv[2]);
  FILE* out = fopen(argv[3], "w");

  if (iters<1) {
    Output << "Illegal number of iterations: " << iters << "\n";
    return 0;
  }

  Input.SwitchInput(in);
  Verbose.SwitchDisplay(out);
  Verbose.Activate();

  shared_matrix *answer = new shared_matrix(N);
  answer->read(Input);
  Output << "Read matrix: " << answer->Distinct() << " distinct\n";
  Output.flush();
  shared_matrix *tmp = new shared_matrix(N);
  for (int i=1; i<=iters; i++) {
    timer compute;
    compute.Start();
    int nnz = tmp->Multiply(answer, answer);
    SWAP(tmp, answer);
    compute.Stop();
    Output << "Computed A^(2^" << i << ")\n";
    Output << "\titeration took " << compute.User_Seconds() << " secs\n";
    Output << "\tmultiplier: ";
    Output << answer->Distinct() << " distinct / ";
    Output << nnz << " nonzeroes\n";
    MatrixStats();
  }
  
  answer->write(Verbose);
  Verbose.flush();

  Output << "Done\n";

  return 0;
}
