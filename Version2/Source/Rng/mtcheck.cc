
//
//  Code to manipulate and test the boolean matrix for MT19937.
//

#include "multstrm.h"
#include "mtwist.h"

#define USE_SHARED

void smart_exit()
{
}

mt_state state1;
mt_state state2;

shared_matrix A(MT_STATE_SIZE);

void Synchronize()
{
  mts_refresh(&state1);
  mts_refresh(&state2);
  int i;
  for (i=0; i<MT_STATE_SIZE; i++) state2.statevec[i] = state1.statevec[i];
}

void JumpA()
{
  mt_state tmp;
  A.vector_multiply(tmp.statevec, state1.statevec);
  int i;
  for (i=0; i<MT_STATE_SIZE; i++) state1.statevec[i] = tmp.statevec[i]; 
}

void JumpB(int n)
{
  int i;
  for (i=0; i<n; i++) mts_lrand(&state2);
}

bool CheckSequence(int d)
{
  int i;
  for (i=0; i<d; i++) {
    unsigned long x1 = mts_lrand(&state1);
    unsigned long x2 = mts_lrand(&state2); 
    if (x1 != x2) {
      Output << "\n\nDIFFERENCE at position " << i << "\n";
      Output << "  We get " << x1 << "\n";
      Output << "They get " << x2 << "\n";
      return false;
    }
  }
  return true;
}

int atoi(const char* a)
{
  int i = 0;
  while (a[0] != 0) {
    i += (a[0] - '0');
    a++;
    if (a[0]) i*= 10;
    if (i<0) return i; // overflow
  }
  return i;
}



int main(int argc, char** argv)
{
  if (argc < 5) {
    Output << "Checks a jump multiplier against the actual stream\n";
    Output << "Usage: " << argv[0] << " (jump file) (jump length) dist reps\n";
    Output << "where:\n";
    Output << "\t(jump file) is the multiplier matrix file\n";
    Output << "\t(jump length) is the jump length in rng sequence\n";
    Output << "\t(dist) is the distance to check for matches\n";
    Output << "\t(reps) is the number of replications to perform\n";
    return 0;
  }

  FILE*in = fopen(argv[1], "r");
  Input.SwitchInput(in);
  A.read(Input);

  int JUMP = atoi(argv[2]);
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

  mts_seed32(&state1, 7309259);
  mts_seed32(&state2, 7309259);

  int i;
  for (i=0; i<REPS; i++) {
    Output << "Replication " << i << "\n";
    Synchronize();
    Output << "\tUsing multiplier:\n";
    Output.flush();
    JumpA();
    Output << "\tUsing mt:\n";
    Output.flush();
    JumpB(JUMP);
    Output << "\tComparing:\n";
    Output.flush();

    if (CheckSequence(DISTANCE)) continue;
    break;
  }
  return 0;
}
