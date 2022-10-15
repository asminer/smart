
/*
  Sparse bitvector test.
*/

#include "vectlib.h"

#include <iostream>

sparse_intvector* set;

using namespace std;

class mytraverse : public intvector_traverse {
  bool visited;
public:
  mytraverse() : intvector_traverse() { visited = false; }
  virtual void Visit(long index, long value) {
    if (visited) cout << ", ";
    visited = true;
    cout << index << ":" << value;
  }
};

void AddElements(long delta)
{
  cout << "Enter a series of elements, terminated by -1\n";
  int x;
  for (;;) {
    cin >> x;
    if (x<0) break;
    set->ChangeElement(x, delta);
  }
  cout << "The set is now: {";
  mytraverse foo;
  set->Traverse(&foo);
  cout << "}\n";
}


int main()
{
  cout << SV_LibraryVersion() << "\n";
  set = SV_CreateSparseIntvector(0, 0);
  if (0==set) {
    cout << "Couldn't create sparse intvector\n";
    return 1;
  }
  char cmd;
  for (;;) {
    cout << "What would you like to do?\n";
    cout << "\ta: Add 1 to elements in the set\n";
    cout << "\tr: Remove 1 from elements in the set\n";
    cout << "\ts: Convert to static representation\n";
    cout << "\td: Convert to dynamic representation\n";
    cout << "\tq: Quit\n";
    cin >> cmd;
    switch (cmd) {
      case 'a':
      case 'A':
        AddElements(1);
        break;

      case 'r':
      case 'R':
        AddElements(-1);
        break;

      case 's':
      case 'S':
        set->ConvertToStatic(false);
        break;

      case 'd':
      case 'D':
        set->ConvertToDynamic();
        break;
    }
    if ('q' == cmd) break;
    if ('Q' == cmd) break;
  } // infinite loop
  cout << "Final set: {";
  mytraverse foo;
  set->Traverse(&foo);
  cout << "}\n";
  delete set;
  cout << "That's all, folks\n";
  return 0;
}
