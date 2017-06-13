
/*
    Tests for period computation
*/

#include <iostream>
#include "mclib.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

typedef struct { long from; long to; double rate; } edge;

// ==============================> Graph 1 <==============================

/*
  Absorbing chain from lecture notes.
    States 0,1,2 are transient.
    States 3,4,5 are absorbing.
*/
const edge graph1[] = {
  {0, 1, 2},  {0, 3, 1},
  {1, 2, 3},  {1, 4, 1},
  {2, 0, 4},  {2, 5, 1},
  // End 
  {-1, -1, -1}
};

const bool discrete1 = true;
const long num_nodes1 = 6;

const double init1[] = {1.0/3.0, 0, 1.0/3.0, 1.0/3.0, 0, 0};
const double tta1[] =  {1, 2.0/3.0, 5.0/6.0,    2.0/3.0, 1.0/6.0, 1.0/6.0};

// =======================================================================

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

bool run_test(const char* name, const bool discrete, const edge graph[], 
  const long num_nodes, const double init[], const double tta[])
{
  cout << "Testing ";
  if (discrete) cout << "DTMC "; else cout << "CTMC ";
  cout << name << "\n";
#ifdef VERBOSE
  my_timer T(true);
#else
  my_timer T(false);
#endif

  
  //
  // Build graph from list of edges
  //
  dynamic_summable<double>* G = new dynamic_summable<double>(true, true);
  G->addNodes(num_nodes);

  for (long i=0; graph[i].from >= 0; i++) {
    G->addEdge(graph[i].from, graph[i].to, graph[i].rate);
#ifdef VERBOSE
    cout << "\t" << graph[i].from << " -> " << graph[i].to;
    cout << " rate " << graph[i].rate << "\n";
#endif
  }

  //
  // Construct Markov chain
  //
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

  Markov_chain MC(discrete, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;

  //
  // Set up initial vector
  //
  LS_Vector p0;
  p0.size = num_nodes;
  p0.index = 0;
  p0.d_value = init;
  p0.f_value = 0;

  //
  // Set up options
  //
  LS_Options opt;
//  opt.debug = true;
  opt.method = LS_Gauss_Seidel;

  LS_Output out;

  //
  // Solve TTA (for now...)
  //
  double* sol = new double[num_nodes];

  try {
    MC.computeFirstRecurrentProbs(p0, sol, opt, out);
  }
  catch (GraphLib::error e) {
    cout << "    Caught graph library error: ";
    cout << e.getString() << "\n";
    return false;
  }

#ifdef VERBOSE
  cout << "Linear solver output:\n";
  cout << "    number of iterations: " << out.num_iters << "\n";
  cout << "    relaxation: " << out.relaxation << "\n";
  cout << "    precision: " << out.precision << "\n";
  cout << "    status: ";
  switch (out.status) {
    case LS_Success:          cout << "LS_Success\n";           break;
    case LS_Wrong_Format:     cout << "LS_Wrong_Format\n";      break;
    case LS_Illegal_Method:   cout << "LS_Illegal_Method\n";    break;
    case LS_Out_Of_Memory:    cout << "LS_Out_Of_Memory\n";     break;
    case LS_No_Convergence:   cout << "LS_No_Convergence\n";    break;
    case LS_Not_Implemented:  cout << "LS_Not_Implemented\n";   break;
  }
#endif

  cout << "  Got solution vector: [" << sol[0];
  for (long i=1; i<num_nodes; i++) cout << ", " << sol[i];
  cout << "]\n";
  cout << "  Expected     vector: [" << tta[0];
  for (long i=1; i<num_nodes; i++) cout << ", " << tta[i];
  cout << "]\n";
  double rel_diff = 0.0;
  for (long i=0; i<num_nodes; i++) {
    double d = tta[i] - sol[i];
    if (tta[i]) d /= tta[i];
    if (d > rel_diff) rel_diff = d;
  }
  cout << "  Relative difference: " << rel_diff;

  bool ok = rel_diff < 1e-5;

  if (ok) cout << " (OK)\n";
  else    cout << " too large!\n";

  return ok;
}


int main()
{
  if (!run_test("abcdef", discrete1, graph1, num_nodes1, init1, tta1)) {
    return 1;
  }

  return 0;
}


