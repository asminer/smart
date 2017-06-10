
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
  virtual bool visit(long from, long to, void* wt) { 
    count++; 
    return false; 
  }
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

class forwd_reachable : public BF_with_queue {
  public:
    forwd_reachable(long init, intset &_reachable);
    virtual ~forwd_reachable();

    virtual bool visit(long src, long dest, const void*);

  private:
    intset &reachable;
};

// ----------

forwd_reachable::forwd_reachable(long init, intset &_reachable)
 : BF_with_queue(_reachable.getSize()), reachable(_reachable)
 {
   queuePush(init);
   reachable.removeAll();
   reachable.addElement(init);
 }

forwd_reachable::~forwd_reachable()
{
}

bool forwd_reachable::visit(long, long dest, const void*)
{
  if (!reachable.contains(dest)) {
    reachable.addElement(dest);
    queuePush(dest);
  }
  return false;
}

// ----------

class show_traverse : public BF_graph_traversal {
    bool show;
    long& count;
    long init;
  public:
    show_traverse(bool sh, long& c);
    virtual bool hasNodesToExplore();
    virtual long getNextToExplore();
    virtual bool visit(long, long to, const void*);

    void reset(long new_init);
};

// ----------

show_traverse::show_traverse(bool sh, long& c)
 : count(c)
{
  show = sh;
  reset(-1);
}

bool show_traverse::hasNodesToExplore()
{
  return init >= 0;
}

long show_traverse::getNextToExplore()
{
  long next = init;
  init = -1;
  return next;
}

bool show_traverse::visit(long, long to, const void*)
{
  if (show) printf("\t\tTo state %ld\n", to);
  count++;
  return false;
}

void show_traverse::reset(long new_init)
{
  init = new_init;
  count = 0;
}

// ----------


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

  forwd_reachable ft(hfrom, rs); 
  g->traverse(ft);

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
  show_traverse counter(false, count);
  show_traverse shower(true, count);
  for (int n=0; n<g->getNumNodes(); n++) {
    counter.reset(n);
    g->traverse(counter);
    if (count <= 0) continue;
    shower.reset(n);
    printf("\tFrom state %d:\n", n);
    g->traverse(shower);
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

#ifdef USE_OLD_INTERFACE
  digraph* g = new digraph(true);
#else
  dynamic_digraph* g = new dynamic_digraph(true);
#endif
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
