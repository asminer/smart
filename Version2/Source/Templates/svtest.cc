
// $Id$

/*
    Used to test the sparsevect class(es).  
    This file is not part of smart or any libraries!
*/

#include <iostream>

#include "../defines.h"
#include "sparsevect.h"

using namespace std;

typedef sparse_vector <int> intvector;

void OutOfMemoryError(char const *)
{
  exit(0);
}

void dumpvector(const intvector &x)
{
  cout << "{";
  int i;
  for (i=0; i<x.NumNonzeroes(); i++)  {
    if (i) cout << ", ";
    cout << x.index[i];
    if (x.value[i]>1) cout << " (" << x.value[i] << ")";
  }
  cout << "}\n";
}

int main()
{
  intvector foo(2);
  cout << "Enter integers to add to multiset (-1 to quit)\n";
  int N;
  while (1) {
    cin >> N;
    if (N<0) break;
    int i = foo.BinarySearchIndex(N);
    if (i<0) {
      foo.SortedAppend(N, 1);
    } else {
      foo.value[i]++;
    }
    cout << "Set: ";
    dumpvector(foo);
  }
  cout << "Later...\n";
  return 0;
}
