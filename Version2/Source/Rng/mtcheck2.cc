
// $Id$

//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "rng.h"

void smart_exit()
{
}

Rng stream1(7309259);
Rng stream2(7309259);

shared_matrix A(MT_STATE_SIZE);

void JumpA()
{
  mt_state tmp;
  A.vector_multiply(tmp.statevec, stream1.GetState()->statevec);
  int i;
  for (i=0; i<MT_STATE_SIZE; i++)
    stream1.GetState()->statevec[i] = tmp.statevec[i];
}

void JumpB(long long n)
{
  Rng tmp;
  for (long long i=0; i<n; i++) {
    tmp.JumpStream(stream2);
    SWAP(tmp, stream2);
  }
}

bool CheckSequence(int d)
{
  int i;
  for (i=0; i<d; i++) {
    unsigned long x1 = stream1.lrand();
    unsigned long x2 = stream2.lrand();
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
  if (argc < 5) {
    Output << "Checks a jump multiplier against the stream jumper\n";
    Output << "Usage: " << argv[0] << " (jump file) (jump length) dist reps\n";
    Output << "where:\n";
    Output << "\t(jump file) is the multiplier matrix file\n";
    Output << "\t(jump length) is the number of times to jump the stream\n";
    Output << "\t(dist) is the distance to check for matches\n";
    Output << "\t(reps) is the number of replications to perform\n";
    return 0;
  }

  FILE*in = fopen(argv[1], "r");
  Input.SwitchInput(in);
  A.read(Input);

  long long JUMP = atoll(argv[2]);
  if (JUMP<1) {
    Output << "Bad jump length: " << JUMP << "\n";
    return 0;
  }

  int DISTANCE = atoi(argv[3]);
  if (DISTANCE<1) {
    Output << "Bad distance: " << DISTANCE << "\n";
    return 0;
  }

  int REPS = atoi(argv[4]);
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
    Output << "\tUsing mt jump:\n";
    Output.flush();
    JumpB(JUMP);
    Output << "\tComparing:\n";
    Output.flush();

    if (CheckSequence(DISTANCE)) continue;
    break;
  }
  return 0;
}
