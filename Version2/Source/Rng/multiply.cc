
// $Id$

/* Test of multiple stream stuff */

#include "../Base/streams.h"
#include "../Base/timers.h"
#include "multstrm.h"

void smart_exit()
{
}

const int N = 624;

int main(int argc, char** argv)
{
  if (argc<4) {
    Output << "\tComputes A = B * C for matrices A, B, C\n";
    Output << "\tUsage: " << argv[0] << " <Bmatrix> <Cmatrix> <Amatrix>\n\n";
    return 0;
  }

  FILE* inB = fopen(argv[1], "r");
  FILE* inC = fopen(argv[2], "r");
  FILE* out = fopen(argv[3], "w");

  timer compute;
  compute.Start();

  Input.SwitchInput(inB);
  shared_matrix B(N);
  B.read(Input);

  Input.SwitchInput(inC);
  shared_matrix C(N);
  C.read(Input);

  Verbose.SwitchDisplay(out);
  Verbose.Activate();
  shared_matrix A(N);
  int nnz = A.Multiply(&B, &C);
  A.write(Verbose);
  Verbose.flush();

  compute.Stop();
  Output << "Multiplication took " << compute.User_Seconds() << " secs\n";
  Output << "\tmatrix A: ";
  Output << A.Distinct() << " distinct / ";
  Output << nnz << " nonzeroes\n";
  MatrixStats();

  return 0;
}
