
// $Id$

/*
  Sparse bitvector test.
*/

#include "vectlib.h"

#include <iostream>

sparse_bitvector* set;

using namespace std;

class mytraverse : public bitvector_traverse {
  bool visited;
public:
  mytraverse() : bitvector_traverse() { visited = false; }
  virtual void Visit(long index) {
    if (visited) cout << ", ";
    visited = true;
    cout << index;
  }
};

void AddElements()
{
  cout << "Enter a series of elements, terminated by -1\n";
  int x;
  for (;;) {
    cin >> x;
    if (x<0) break;
    set->SetElement(x);
  }
  cout << "The set is now: {";
  mytraverse foo;
  set->Traverse(&foo);
  cout << "}\n";
}

void RemoveElements()
{
  cout << "Enter a series of elements, terminated by -1\n";
  int x;
  for (;;) {
    cin >> x;
    if (x<0) break;
    set->ClearElement(x);
  }
  cout << "The set is now: {";
  mytraverse foo;
  set->Traverse(&foo);
  cout << "}\n";
}

int main()
{
  cout << SV_LibraryVersion() << "\n";
  set = SV_CreateSparseBitvector(0, 0);
  if (0==set) {
    cout << "Couldn't create sparse bitvector\n";
    return 1;
  }
  char cmd;
  for (;;) {
    cout << "What would you like to do?\n";
    cout << "\ta: Add elements to the set\n";
    cout << "\tr: Remove elements from the set\n";
    cout << "\ts: Convert to static representation\n";
    cout << "\td: Convert to dynamic representation\n";
    cout << "\tq: Quit\n";
    cin >> cmd;
    switch (cmd) {
      case 'a':
      case 'A':
        AddElements();
        break;

      case 'r':
      case 'R':
        RemoveElements();
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
