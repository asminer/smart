
/* Test of multiple stream stuff */

#include "mtwist.h"

#include <iostream>

using namespace std;

unsigned long seeds[MT_STATE_SIZE];

const int KNUTH_MULTIPLIER_OLD = 69069;

unsigned long firstseed;

inline long knuth(long seed)
{
  return (KNUTH_MULTIPLIER_OLD * seed) & 0xffffffff;
}

void Init()
{
  int i;
  unsigned long seed = firstseed;
  for (i=0; i<MT_STATE_SIZE; i++) {
    seeds[i] = seed;
    seed = knuth(seed);
  }
}

void InitNext()
{
  int i;
  for (i=0; i<MT_STATE_SIZE-1; i++) seeds[i] = seeds[i+1];
  seeds[MT_STATE_SIZE-1] = knuth(seeds[MT_STATE_SIZE-2]);
}

void Dump()
{
  cout << "[";
  int i;
  for (i=0; i<MT_STATE_SIZE; i++) {
    if (i) cout << ", ";
    cout << mt_default_state.statevec[i];
  }
  cout << "]\n\n";
}

bool IsSequence()
{
  int i;
  unsigned long sd = mt_default_state.statevec[MT_STATE_SIZE-1];
  for (i=MT_STATE_SIZE-2; i >= 0; i--) {
    sd = knuth(sd);
    if (mt_default_state.statevec[i] != sd) return false;
  }

  return true;
}

unsigned long MAXDIST;

bool Test()
{
  unsigned long i;
  for (i=MAXDIST; i; i--) {
    mts_refresh(&mt_default_state);
    if (IsSequence()) {
      cout << "Starting with sequence generated from " << seeds[0] << "\n";
      cout << "  we reach the sequence generated from ";
      cout << mt_default_state.statevec[MT_STATE_SIZE-1] << "\n";
      cout << "  in " << MAXDIST-i << "+1 steps" << endl;
      return true;
    }
  }
  return false;
}

int main()
{
  cout << "Enter starting seed thingy\n";
  cin >> firstseed;
  cout << "Enter max distance to check\n";
  cin >> MAXDIST;

  if (!IsSequence()) {
    cout << "Not a sequence?\n";
    return 0;
  }

  unsigned long count = 0;
  unsigned long nextprint = 1;
  Init();
  do {
    if (Test()) return 0; 
    count++;
    if (count >= nextprint) {
      cout << count << " seeds checked" << endl;
      nextprint *= 2;
    }
    InitNext();
  } while (seeds[0] != firstseed);

  cout << "\n\n" << count << " seeds checked, all pass!\n";
  return 0;
}

