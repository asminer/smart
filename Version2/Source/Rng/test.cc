
/* Test of multiple stream stuff */

#include "mtwist.h"

#include <iostream>

using namespace std;

unsigned long seeds[MT_STATE_SIZE];

void Init(unsigned long nz)
{
  int i;
  nz += MT_STATE_SIZE;
  for (i=0; i<MT_STATE_SIZE; i++) {
    nz--;
    seeds[i] = nz;
  }
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
  for (i=1; i<MT_STATE_SIZE; i++) 
    if (mt_default_state.statevec[i-1]+1 != mt_default_state.statevec[i]) return false;

  return true;
}

int main()
{
  int foo;
  cout << "Enter starting seed thingy\n";
  cin >> foo;
  Init(foo);
  mt_seedfull(seeds);
  Dump();

  if (!IsSequence()) {
    cout << "Not a sequence\n";
    return 0;
  }
  int i;
  for (i=1; i<1000; i++) {
    mts_refresh(&mt_default_state);
    if (IsSequence()) {
      cout << "Hit next sequence!  First is " << mt_default_state.statevec[0] << endl;
      cout << "Distance is " << i << endl;
      return 0;
    }
  }
  cout << "No sequence with distance less than " << i << endl;
  return 0;
}

