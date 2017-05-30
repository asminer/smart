
/*
    Interactive graph testing application.
*/

#include <stdio.h>
#include <stdlib.h>

#include "intset.h"
#include "graphlib.h"

using namespace GraphLib;

// #define USE_OLD_INTERFACE


//======================================================================================================================================================
#ifdef USE_OLD_INTERFACE

class counter : public generic_graph::element_visitor {
  long& count;
public:
  counter(long& c) : count(c) { }
  virtual bool visit(long from, long to, void* wt) { count++; return false; }
};

class row_visit : public generic_graph::element_visitor {
public:
  row_visit() { }
  virtual bool visit(long from, long to, void* wt) {
    printf("\t\tTo state %ld\n", to);
    return false;
  }
};

void addGraphNode(digraph* g)
{
  try {
    g->addNode();
  }
  catch (GraphLib::error e) {
    printf("Couldn't add graph node: %s\n", e.getString());
    return;
  }
  printf("Added graph node %ld\n", g->getNumNodes()-1);
}

void addGraphEdge(digraph* g)
{
  long hfrom, hto;
  scanf("%ld", &hfrom);
  scanf("%ld", &hto);
  bool dupedge;
  try {
    dupedge = g->addEdge(hfrom, hto);
  }
  catch (GraphLib::error e) {
    printf("Couldn't add graph edge: %s\n", e.getString());
    return;
  }
  if (dupedge) 
    printf("Added duplicate edge from %ld to %ld\n", hfrom, hto);
  else
    printf("Added edge from %ld to %ld\n", hfrom, hto);
}

void finishGraph(digraph* g)
{
  digraph::finish_options o;
  try {
    g->finish(o);
  }
  catch (GraphLib::error e) {
    printf("Couldn't finish graph: %s\n", e.getString());
    return;
  }
  printf("Finished graph\n");
}

void unfinishGraph(digraph* g)
{
  try {
    g->unfinish();
  }
  catch (GraphLib::error e) {
    printf("Couldn't unfinish graph: %s\n", e.getString());
    return;
  } 
  printf("Unfinished graph\n");
}

void reachableGraph(digraph* g)
{
  long hfrom;
  scanf("%ld", &hfrom);
  intset rs(g->getNumNodes());
  rs.removeAll();
  if (g->getReachable(hfrom, rs) < 0) {
    printf("Not enough memory\n");
    return;
  }
  printf("{");
  long z = rs.getSmallestAfter(-1);
  if (z>=0) {
    printf("%ld", z);
    for(;;) {
      z = rs.getSmallestAfter(z);
      if (z<0) break;
      printf(", %ld", z);
    }
  }
  printf("}\n");
}

void showGraph(digraph* g)
{
  printf("Current graph:\n");
  long count;
  counter foo(count);
  row_visit bar;
  for (int n=0; n<g->getNumNodes(); n++) {
    count = 0;
    g->traverseFrom(n, foo);
    if (count <= 0) continue;
    printf("\tFrom state %d:\n", n);
    g->traverseFrom(n, bar);
  } // for n
}

//======================================================================================================================================================
#else

void addGraphNode(dynamic_digraph* g)
{
  try {
    g->addNode();
  }
  catch (GraphLib::error e) {
    printf("Couldn't add graph node: %s\n", e.getString());
    return;
  }
  printf("Added graph node %ld\n", g->getNumNodes()-1);
}

void addGraphEdge(dynamic_digraph* g)
{
  long hfrom, hto;
  scanf("%ld", &hfrom);
  scanf("%ld", &hto);
  bool dupedge;
  try {
    dupedge = g->addEdge(hfrom, hto);
  }
  catch (GraphLib::error e) {
    printf("Couldn't add graph edge: %s\n", e.getString());
    return;
  }
  if (dupedge) 
    printf("Added duplicate edge from %ld to %ld\n", hfrom, hto);
  else
    printf("Added edge from %ld to %ld\n", hfrom, hto);
}

void finishGraph(dynamic_digraph* g)
{
  printf("New interface: no finishing\n");
}

void unfinishGraph(dynamic_digraph* g)
{
  printf("New interface: no unfinishing\n");
}

void reachableGraph(dynamic_digraph* g)
{
  long hfrom;
  scanf("%ld", &hfrom);
  intset rs(g->getNumNodes());
  rs.removeAll();
  if (g->getReachable(hfrom, rs) < 0) {
    printf("Not enough memory\n");
    return;
  }
  printf("{");
  long z = rs.getSmallestAfter(-1);
  if (z>=0) {
    printf("%ld", z);
    for(;;) {
      z = rs.getSmallestAfter(z);
      if (z<0) break;
      printf(", %ld", z);
    }
  }
  printf("}\n");
}

void showGraph(dynamic_digraph* g)
{
  printf("Current graph:\n");
  long count;
  counter foo(count);
  row_visit bar;
  for (int n=0; n<g->getNumNodes(); n++) {
    count = 0;
    g->traverseFrom(n, foo);
    if (count <= 0) continue;
    printf("\tFrom state %d:\n", n);
    g->traverseFrom(n, bar);
  } // for n
}

//======================================================================================================================================================
#endif

void showMenu()
{
  printf("Interactive graph testing\n\n");
  printf("\t?: print this menu\n");
  printf("\tA: add graph node\n"); 
  printf("\tE: <from> <to> add graph edge\n"); 
  printf("\tF: finish graph\n"); 
  printf("\tU: unfinish graph\n"); 
  printf("\tR: <source> show nodes reachable from source\n");
  printf("\tS: show graph\n");
  printf("\n\tQ: quit\n");
}


int main()
{
  puts(GraphLib::Version());
  showMenu();

  digraph* g = new digraph(true);
  if (0==g) {
    printf("Got null graph\n");
    return 0;
  }

  char c;
  do {
    c = fgetc(stdin);
    switch (c) {
        case '?':
          showMenu();
          break;

        case 'a':
        case 'A':
          addGraphNode(g);
          break;

        case 'e':
        case 'E':
          addGraphEdge(g);
          break;

        case 'f':
        case 'F':
          finishGraph(g);
          break;

        case 'u':
        case 'U':
          unfinishGraph(g);
          break;

        case 'r':
        case 'R':
          reachableGraph(g);
          break;

        case 's':
        case 'S':
          showGraph(g);
          break;

        case EOF:
        case 'q':
        case 'Q':
          c = 'q';
          break;

        default:
          c = ' ';
    }
  } while (c != 'q');

  printf("Done!\n");
  delete g;
  return 0;
}
