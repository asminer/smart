
/*
    Tests for period computation
*/

#include <iostream>
#include "mclib.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

typedef struct { long from; long to; } edge;

// ==============================> Graph <==============================

/*
  State 0: transient, goes to each recurrent class.
  States 1..5   : absorbing
  States 6,7    : Class 1, period 1
  States 8,9    : Class 2, period 2
  States 10..12 : Class 3, period 3
  States 13..27 : Class 4, period 2 (loop length 6, loop length 10)
*/
const edge graph[] = {
  {0, 1},
  {0, 2},
  {0, 3},
  {0, 4},
  {0, 5},
  {0, 6},
  {0, 8},
  {0, 10},
  {0, 13},
  // Absorbing; some with self loops
  {1, 1},
  {3, 3},
  {5, 5},
  // Class 1
  {6, 6},
  {6, 7},
  {7, 6},
  // Class 2
  {8, 9},
  {9, 8},
  // Class 3
  {10, 11},
  {11, 12},
  {12, 10},
  // Class 4
  {13, 14},
  {14, 15},
  {15, 16},
  {16, 17},
  {17, 18},
  {18, 19},
  {19, 20},
  {20, 21},
  {21, 22},
  {22, 13}, {22, 23},
  {23, 24},
  {24, 25},
  {25, 26},
  {26, 27},
  {27, 22},
  // End 
  {-1, -1}
};

const long num_nodes = 28;

const long periods[] = {0, 1, 1, 2, 3, 2, -1};


// =======================================================================

dynamic_summable<double>* buildGraph()
{
  dynamic_summable<double>* G = new dynamic_summable<double>(true, true);
  G->addNodes(num_nodes);

  for (long i=0; graph[i].from >= 0; i++) {
    G->addEdge(graph[i].from, graph[i].to, 1.0);
#ifdef VERBOSE
    cout << "\t" << graph[i].from << " -> " << graph[i].to << " (rate 1.0)\n";
#endif
  }

  return G;
}

class my_timer : public timer_hook {
  public:
    my_timer(bool activ) {
      active = activ;
    }
    virtual ~my_timer() {
    }
    virtual void start(const char* w) {
      if (active) {
        cout << "    " << w << " ...";
        cout.flush();
      }
    }
    virtual void stop() {
      if (active) {
        cout << "\n";
        cout.flush();
      }
    }

  private:
    bool active;
};

int main()
{
#ifdef VERBOSE
  cout << "Building Markov chain\n";
  my_timer T(true);
#else
  my_timer T(false);
#endif
  dynamic_summable<double>* G = buildGraph();
  abstract_classifier* SCCs = G->determineSCCs(0, 1, true, &T);
  static_classifier C;
  node_renumberer* R = SCCs->buildRenumbererAndStatic(C);

#ifdef VERBOSE
  if (0==R) {
    cout << "Null renumbering:\n";
  } else {
    cout << "Got renumbering:\n";
    for (long i=0; i<num_nodes; i++) {
      cout << "    " << i << " -> " << R->new_number(i) << "\n";
    }
  }
  
  cout << "Got static classifier:\n";
  cout << "    #classes: " << C.getNumClasses() << "\n";
  for (long c=0; c<C.getNumClasses(); c++) {
    cout << "    class " << c << ": ";
    if (C.firstNodeOfClass(c) <= C.lastNodeOfClass(c)) {
      cout << " [" << C.firstNodeOfClass(c) << ".." << C.lastNodeOfClass(c) << "]";
    } else {
      cout << " []";
    }
    cout << " size " << C.sizeOfClass(c) << "\n";
  }
#endif

  Markov_chain MC(true, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;

  try {
    for (long c=0; periods[c]>=0; c++) {
      cout << "Computing period for class " << c << "\n";
      long mcp = MC.computePeriodOfClass(c);
      cout << "    computed " << mcp << "\n";
      cout << "    expected " << periods[c] << "\n";
      if (periods[c] != mcp) return 1;
    }
  }
  catch (GraphLib::error e) {
    cout << "    Caught graph library error: ";
    cout << e.getString() << "\n";
    return 1;
  }

  return 0;
}
