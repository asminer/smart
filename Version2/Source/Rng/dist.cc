
/* Test of multiple stream stuff */

#include "mtwist.h"

#include <signal.h>
#include <iostream>

using namespace std;

unsigned long seeds[MT_STATE_SIZE];

const int KNUTH_MULTIPLIER_OLD = 69069;

unsigned long firstseed;

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

void Init()
{
  int i;
  unsigned long seed = firstseed;
  for (i=0; i<MT_STATE_SIZE; i++) {
    seeds[i] = seed;
    seed = knuth(seed);
  }
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

int main()
{
  signal(SIGTERM, catchterm);
  cout << "Enter seed to check: ";
  cin >> firstseed;
  cout << firstseed << "\n";
  Init();
  long long distance = 0;
  long long nextprint = 1024;
  while (1) {
    distance++;
    mts_refresh(&mt_default_state);
    if (IsSequence()) {
      cout << "Starting with sequence generated from " << seeds[0] << "\n";
      cout << "  we reach the sequence generated from ";
      cout << mt_default_state.statevec[MT_STATE_SIZE-1] << "\n";
      cout << "  in " << distance << " steps!" << endl;
      return 0;
    }
    if (distance<=0) {
      cout << "Distance overflow!\n";
      distance--;
      break;
    }
    if (distance==nextprint) {
      cout << "  > " << distance << endl;
      nextprint *= 2;
    }
    if (stop_tests) break;
  }
  cout << "Distance is greater than " << distance << endl;
  return 0;
}

