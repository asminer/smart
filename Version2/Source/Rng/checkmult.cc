
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
    Output << "\tChecks that A == B * C for matrices A, B, C\n";
    Output << "\tUsage: " << argv[0] << " <Bmatrix> <Cmatrix> <Amatrix>\n\n";
    return 0;
  }

  FILE* inB = fopen(argv[1], "r");
  FILE* inC = fopen(argv[2], "r");
  FILE* inA = fopen(argv[3], "r");

  Input.SwitchInput(inB);
  shared_matrix B(N);
  B.read(Input);

  Input.SwitchInput(inC);
  shared_matrix C(N);
  C.read(Input);
 
  Input.SwitchInput(inA);
  shared_matrix A(N);
  A.read(Input);

  bool ok = A.CheckMultiply(&B, &C);

  if (!ok) {
    Output << "Incorrect!\n";
  } else {
    Output << "correct\n";
  }
  
  return 0;
}
