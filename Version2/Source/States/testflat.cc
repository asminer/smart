
#include "../Base/streams.h"
#include "stateheap.h"
#include "flatss.h"

void smart_exit()
{
}

/*  
    This function reads states until EOF then prints them back.
    Useful for testing the encodings themselves.
*/
int main_unlimited()
{
  Output.Activate();
  Output << "Testing state array\nEnter state size:\n";
  Output.flush();
  int np;
  Input.Get(np);

  state s;
  AllocState(s, np);

  state_array pile_of_states(false);
  // state_array pile_of_states(true);
  bool aok = true;
  while (aok) {
    Output << "Enter state (length " << np << "):\n";
    Output.flush();
    int j;
    for (j=0; j<np; j++) {
      s[j].Clear();
      bool aok = Input.Get(s[j].ivalue);
      if (!aok) break;
    }
    if (Input.Eof()) {
      Output << "EOF\n";
      Output.flush();
      break;
    }
    if (aok) pile_of_states.AddState(s);
  }

  Output << "\n\nRetrieving states...\n";
  int h = pile_of_states.FirstHandle();
  while (h < pile_of_states.MaxHandle()) {
    Output << "Trying state with handle " << h << "\n";
    Output.flush();
    pile_of_states.GetState(h, s);
    h = pile_of_states.NextHandle(h);
    if (h<0) break;
  }

  FreeState(s);
  pile_of_states.Report(Output);
  return 0;
}

int main()
{
  state_array pile_of_states(true);
  Output.Activate();
  Output << "Testing state array Compare\nEnter first state size:\n";
  Output.flush();
  int np1;
  Input.Get(np1);
  state s1;
  AllocState(s1, np1);
  Output << "Enter state (length " << np1 << "):\n";
  Output.flush();
  int j;
  for (j=0; j<np1; j++) {
      s1[j].Clear();
      Input.Get(s1[j].ivalue);
  }
  pile_of_states.AddState(s1);
  Output << "Enter second state size:\n";
  Output.flush();
  int np2;
  Input.Get(np2);
  state s2;
  AllocState(s2, np2);
  Output << "Enter state (length " << np2 << "):\n";
  Output.flush();
  for (j=0; j<np2; j++) {
      s2[j].Clear();
      Input.Get(s2[j].ivalue);
  }
  pile_of_states.AddState(s2);

  Output << "Comparing states:\n";
  int cmp = pile_of_states.Compare(0,1);
  Output << "Got compare value: " << cmp << "\n";

  return 0;
}
