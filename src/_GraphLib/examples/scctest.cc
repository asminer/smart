
/*
    Tests for SCC computation.
*/

#include <iostream>
#include "graphlib.h"

#define VERBOSE

using namespace GraphLib;
using namespace std;

typedef struct { long from; long to; } edge;

// ==============================> Graph 1 <==============================

const long nodes1 = 1;

const edge graph1[] = { {0,0}, {-1,-1} };

const long scc1_nn[] = { 0 };
const long scc1_n7[] = { 7 };

// ==============================> Graph 2 <==============================

const long nodes2 = 10;

const edge graph2[] = {
  {0, 1}, 
  {0, 2}, 
  {0, 3}, 
  {1, 7}, 
  {1, 9}, 
  {2, 6}, 
  {3, 4}, 
  {4, 4}, 
  {4, 5}, 
  {5, 4}, 
  {7, 6}, 
  {8, 7}, 
  {6, 8}, 
  {-1, -1}
};

const long scc2_nn[] = { 0, 1, 2, 3, 4, 4, 5, 5, 5, 6 };
const long scc2_n4[] = { 0, 1, 2, 3, 5, 5, 6, 6, 6, 4 };
const long scc2_n9[] = { 0, 1, 2, 3, 4, 4, 5, 5, 5, 9 };
const long scc2_0n[] = { 0, 0, 0, 0, 1, 1, 2, 2, 2, 3 };
const long scc2_2n[] = { 2, 2, 2, 2, 0, 0, 1, 1, 1, 3 };
const long scc2_12[] = { 1, 1, 1, 1, 0, 0, 3, 3, 3, 2 };
const long scc2_21[] = { 2, 2, 2, 2, 0, 0, 3, 3, 3, 1 };
const long scc2_01[] = { 0, 0, 0, 0, 2, 2, 3, 3, 3, 1 };

// ==============================> Graph 3 <==============================

const long nodes3 = 26;

const edge graph3[] = {
  {0,   1},
  {0,   9},
  {1,   2},
  {2,   3},
  {2,   5},
  {3,   2},
  {3,   4},
  {3,  12},
  {4,   3},
  {4,   5},
  {5,   6},
  {6,  15},
  {7,   6},
  {8,   7},
  {10,  1},
  {10, 19},
  {11, 10},
  {11, 20},
  {12, 13},
  {13, 12},
  {13, 13},
  {14,  5},
  {15, 14},
  {15, 16},
  {16, 17},
  {17,  8},
  {18, 19},
  {19, 20},
  {20, 21},
  {21, 22},
  {22, 19},
  {22, 23},
  {23, 24},
  {24, 25},
  {25, 22},
  {-1, -1}
};

const long scc3_nn[] = { 0, 1, 2, 2, 2, 3, 3, 3, 3, 4, 5, 6, 7, 7, 3, 3, 3, 3, 8, 9, 9, 9, 9, 9, 9, 9};
const long scc3_n0[] = { 1, 2, 3, 3, 3, 4, 4, 4, 4, 0, 5, 6, 7, 7, 4, 4, 4, 4, 8, 9, 9, 9, 9, 9, 9, 9};
const long scc3_0n[] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 0, 0, 3, 3, 1, 1, 1, 1, 0, 4, 4, 4, 4, 4, 4, 4};
const long scc3_00[] = { 0, 0, 0, 0, 0, 1, 1, 1, 1, 0, 0, 0, 2, 2, 1, 1, 1, 1, 0, 3, 3, 3, 3, 3, 3, 3};
const long scc3_01[] = { 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 0, 0, 3, 3, 2, 2, 2, 2, 0, 4, 4, 4, 4, 4, 4, 4};
const long scc3_10[] = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 0, 1, 1, 3, 3, 2, 2, 2, 2, 1, 4, 4, 4, 4, 4, 4, 4};


// =======================================================================


dynamic_graph* buildGraph(long nodes, const edge E[]) 
{
  dynamic_digraph* G = new dynamic_digraph(true);
  G->addNodes(nodes);

  for (long i=0; E[i].from >= 0; i++) {
    G->addEdge(E[i].from, E[i].to);
#ifdef VERBOSE
    cout << "\t" << E[i].from << " -> " << E[i].to << "\n";
#endif
  }

  return G;
}

bool testGraph(const char* name, long nodes, const edge E[], long nonterm, 
  long sink, const long answer[])
{
#ifdef VERBOSE
  cout << "Building " << name  << "; " << nodes << " nodes with edges:\n";
#endif
  dynamic_graph* G = buildGraph(nodes, E);

  abstract_classifier* SCCs = G->determineSCCs(nonterm, sink, true, 0);

#ifdef VERBOSE
  if (0==SCCs) {
    cout << "Got Null SCCs\n";
  } else {
    cout << "Got SCCs: ";
    for (long i=0; i<nodes; i++) {
      cout << SCCs->classOfNode(i) << ", ";
    }
    cout << "\n";
  }
#endif

  // check it
  bool pass = true;

  if (0==SCCs) {
    delete G;
    return false;
  }

  for (long i=0; i<nodes; i++) {
    if (answer[i] != SCCs->classOfNode(i)) {
      cerr << "SCC mismatch in " << name << "\n";
      pass = false;
      break;
    }
  }

  delete G;
  delete SCCs;
  return pass;
}

int main()
{
  if (!testGraph("graph1 test1", nodes1, graph1, -1, -1, scc1_nn)) return 1;
  if (!testGraph("graph1 test2", nodes1, graph1, -1, 7, scc1_n7)) return 1;
  if (!testGraph("graph1 test3", nodes1, graph1, 3, -1, scc1_nn)) return 1;
  if (!testGraph("graph1 test4", nodes1, graph1, 3, 7, scc1_n7)) return 1;

  if (!testGraph("graph2 test1", nodes2, graph2, -1, -1, scc2_nn)) return 1;
  if (!testGraph("graph2 test2", nodes2, graph2, -1, 4, scc2_n4)) return 1;
  if (!testGraph("graph2 test3", nodes2, graph2, -1, 9, scc2_n9)) return 1;
  if (!testGraph("graph2 test4", nodes2, graph2, 0, -1, scc2_0n)) return 1;
  if (!testGraph("graph2 test4", nodes2, graph2, 0, -1, scc2_0n)) return 1;
  if (!testGraph("graph2 test5", nodes2, graph2, 2, -1, scc2_2n)) return 1;
  if (!testGraph("graph2 test6", nodes2, graph2, 0, 1, scc2_01)) return 1;
  if (!testGraph("graph2 test7", nodes2, graph2, 1, 2, scc2_12)) return 1;
  if (!testGraph("graph2 test8", nodes2, graph2, 2, 1, scc2_21)) return 1;

  if (!testGraph("graph3 test1", nodes3, graph3, -1, -1, scc3_nn)) return 1;
  if (!testGraph("graph3 test2", nodes3, graph3, -1, 0, scc3_n0)) return 1;
  if (!testGraph("graph3 test3", nodes3, graph3, 0, -1, scc3_0n)) return 1;
  if (!testGraph("graph3 test4", nodes3, graph3, 0, 0, scc3_00)) return 1;
  if (!testGraph("graph3 test5", nodes3, graph3, 0, 1, scc3_01)) return 1;
  if (!testGraph("graph3 test6", nodes3, graph3, 1, 0, scc3_10)) return 1;

  return 0;
}
