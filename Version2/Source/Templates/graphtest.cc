
// $Id$

/*
    Used to test the graph class(es).  
    This file is not part of smart or any libraries!
*/


#include "../defines.h"
#include "../Base/streams.h"
#include "graphs.cc"

void smart_exit()
{
  exit(0);
}

void OutOfMemoryError(char const *)
{
  exit(0);
}

int main()
{
  digraph foo;
  Output << "Enter number of nodes in graph (-1 to quit)\n";
  Output.flush();
  int N, i, j;
  Input.Get(N);
  if (N<=0) return 0;
  foo.ResizeNodes(N);
  foo.ResizeEdges(8);
  for (i=0; i<N; i++) foo.AddNode();

  Output << "Enter edges (i j), -1 to quit\n";
  Output.flush();

  while (1) {
    Input.Get(i);
    Input.Get(j);

    if (i<0) break;
    if (j<0) break;

    //foo.AddEdge(i, j);
    foo.AddEdgeInOrder(i, j);
  }

  Output << "Done adding edges\n";
  Output << "Dynamic graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  Output << "Converting to static\n";
  Output.flush();
  foo.ConvertToStatic();
  Output << "Static graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);

  Output << "Converting back to dynamic\n";
  foo.ConvertToDynamic();
  Output << "Dynamic graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  Output << "The end\n";
  return 0;
}
