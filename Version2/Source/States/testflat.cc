
#include "../Base/streams.h"
#include "stateheap.h"
#include "flatss.h"

void smart_exit()
{
}

int main()
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
