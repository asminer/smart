
// $Id$

/*
    Used to test the listarray class(es).  
    This file is not part of smart or any libraries!
*/


#include "../defines.h"
#include "../Base/streams.h"
#include "listarray.h"

void smart_exit()
{
  exit(0);
}

int main()
{
  listarray <char> foo;
  Output << "Enter number of lists in graph\n";
  Output.flush();
  int N, i;
  if (!Input.Get(N)) return 0;
  if (N<=0) return 0;
  for (i=0; i<N; i++) foo.NewList();

  Output << "Enter (character) data to add to each list (i c)\n";
  Output.flush();

  while (1) {
    if (!Input.Get(i)) break;
    if (i<0) break;

    char c=' ';
    do {
      if (!Input.Get(c)) break;
    } while (c==' ');
    if (c==' ') break;

    foo.AddItem(i, c);
  }

  Output << "Done adding edges\n";
  Output << "Dynamic graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  Output << "Converting to static\n";
  Output.flush();
  foo.ConvertToStatic();
  Output << "Static graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  Output << "The end\n";
  return 0;
}
