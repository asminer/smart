
// $Id$

/** Computes the distance between a
    starting seed (using the Knuth rng)
    and any other seed generated using Knuth's rng.
  */

#include "mtwist.h"

#include <signal.h>
#include <iostream>

using namespace std;

const int KNUTH_MULTIPLIER_OLD = 69069;

bool stop_tests = false;

void catchterm(int dealy)
{
  cout << "Caught signal " << dealy << ", will terminate" << endl;
  stop_tests = true;
}

inline long knuth(long seed)
{
  return (KNUTH_MULTIPLIER_OLD * seed) & 0xffffffff;
}

void Init(long seed)
{
  mt_default_state.statevec[MT_STATE_SIZE - 1] = seed & 0xffffffff;
  for (int i = MT_STATE_SIZE - 2;  i >= 0;  i--)
      mt_default_state.statevec[i] = knuth(mt_default_state.statevec[i+1]);

  mt_default_state.stateptr = MT_STATE_SIZE;
  mts_mark_initialized(&mt_default_state);
  mts_refresh(&mt_default_state);
}

unsigned long NextRaw()
{
  if (mt_default_state.stateptr <= 0)  mts_refresh(&mt_default_state);
  unsigned long x = mt_default_state.statevec[--mt_default_state.stateptr];
  return x;
}

int main()
{
  signal(SIGTERM, catchterm);
  cout << "Enter seed to check: ";
  long firstseed;
  cin >> firstseed;
  cout << firstseed << "\n";
  Init(firstseed);
  long long distance = 0;
  int run = 0;
  int maxrun = 0;
  unsigned long lastx = NextRaw();
  unsigned long thisx;
  while (maxrun < 623) {
    distance++;
    thisx = NextRaw();
    if (knuth(lastx) == thisx) {
      run++;
    } else {
      if (run > maxrun) {
	maxrun = run;
	cout << "Max run is " << maxrun << "\t\tdistance " << distance << endl;
      }
      run = 0;
    }
    lastx = thisx;
    if (stop_tests) break;
  }
  cout << distance << " prngs checked " << endl;
  return 0;
}

