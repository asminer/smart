
// $Id$

//
//  Code to write a C header file from a boolean matrix.
//

#include "multstrm.h"


const int N = 624;

int smart_exit()
{
}

int main(int argc, char** argv)
{
  if (argc<3) {
    Output << "\tConverts a matrix file to a C header file.\n";
    Output << "\tUsage: " << argv[0] << " <matrixfile> <outfile>\n\n";
    return 0;
  }
  

  FILE* inB = fopen(argv[1], "r");
  FILE* out = fopen(argv[2], "w");

  Input.SwitchInput(inB);
  shared_matrix B(N);
  B.read(Input);

  Verbose.SwitchDisplay(out);
  Verbose.Activate();

  Verbose << "/*\n\n\t This file generated automatically by makeJump\n";
  Verbose << "\tbased on jump multiplier " << argv[1] << "\n\n*/\n\n";

  Verbose << "#ifndef JUMP_MATRIX\n#define JUMP_MATRIX\n\n";
  
  B.writeC(Verbose);

  Verbose << "\n\n#endif\n";
  
  Verbose.flush();
  
  return 0;
}
