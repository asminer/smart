
// $Id$

/*
    Used to test the sparsevect class(es).  
    This file is not part of smart or any libraries!
*/

#include <iostream>

#include "../defines.h"
#include "sparsevect.h"

using namespace std;

void OutOfMemoryError(char const *)
{
  exit(0);
}

void dumpvector(const sparse_bitvector &x)
{
  cout << "{";
  int i;
  for (i=0; i<x.NumNonzeroes(); i++)  {
    if (i) cout << ", ";
    cout << x.GetNthNonzeroIndex(i);
  }
  cout << "}\n";
}

int main()
{
  sparse_bitvector foo(2);
  cout << "Enter integers to add to set (-1 to quit)\n";
  int N;
  while (1) {
    cin >> N;
    if (N<0) break;
    foo.SortedAppend(N);
    cout << "Set: ";
    dumpvector(foo);
  }
  cout << "Later...\n";
  return 0;
}
