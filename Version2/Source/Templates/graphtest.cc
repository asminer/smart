
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
  labeled_digraph <char> foo;
  Output << "Enter number of nodes in graph\n";
  Output.flush();
  int N, i, j;
  if (!Input.Get(N)) return 0;
  if (N<=0) return 0;
  foo.ResizeNodes(N);
  foo.ResizeEdges(8);
  for (i=0; i<N; i++) foo.AddNode();

  Output << "Enter (character-labeled) edges (i j c)\n";
  Output.flush();

  while (1) {
    if (!Input.Get(i)) break;
    if (i<0) break;

    if (!Input.Get(j)) break;
    if (j<0) break;

    char c=' ';
    do {
      if (!Input.Get(c)) break;
    } while (c==' ');
    if (c==' ') break;

    foo.AddEdge(i, j, c);
    //foo.AddEdgeInOrder(i, j);
  }

  Output << "Done adding edges\n";
  Output << "Dynamic graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  /*
  Output << "Transposing\n";
  Output.flush();
  foo.Transpose();
  Output << "Dynamic graph:\n";
  for (i=0; i<N; i++) foo.ShowNodeList(Output, i);
  */
  Output << "The end\n";
  return 0;
}
