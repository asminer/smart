
//
//  Code to check our jump multiplication
//

#include "../Base/streams.h"
#include "rng.h"

const long JUMP = 33554432;

void smart_exit()
{
}

Rng strm1(7309259ul);
Rng strm2(7309259ul);

void JumpA()
{
  strm1.JumpStream(strm2);
}

void JumpB()
{
  long long i;
  for (i=0; i<JUMP; i++) strm2.lrand();
}

bool CheckSequence(int d)
{
  int i;
  for (i=0; i<d; i++) {
    unsigned long x1 = strm1.lrand();
    unsigned long x2 = strm2.lrand(); 
    if (x1 != x2) {
      Output << "\n\nDIFFERENCE at position " << i << "\n";
      Output << "  We get " << x1 << "\n";
      Output << "They get " << x2 << "\n";
      return false;
    }
  }
  return true;
}



int main(int argc, char** argv)
{
  if (argc < 3) {
    Output << "Checks a jump multiplier against the actual stream\n";
    Output << "Usage: " << argv[0] << " dist reps\n";
    Output << "where:\n";
    Output << "\t(dist) is the distance to check for matches\n";
    Output << "\t(reps) is the number of replications to perform\n";
    return 0;
  }

  int DISTANCE = atoi(argv[1]);
  if (DISTANCE<1) {
    Output << "Bad distance: " << DISTANCE << "\n";
    return 0;
  }

  int REPS = atoi(argv[2]);
  if (REPS<1) {
    Output << "Bad #reps: " << REPS << "\n";
    return 0;
  }

  int i;
  for (i=0; i<REPS; i++) {
    Output << "Replication " << i << "\n";
    Output << "\tUsing multiplier:\n";
    Output.flush();
    JumpA();
    Output << "\tUsing mt:\n";
    Output.flush();
    JumpB();
    Output << "\tComparing:\n";
    Output.flush();

    if (CheckSequence(DISTANCE)) continue;
    break;
  }
  return 0;
}
