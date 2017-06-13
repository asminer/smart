
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

// ==============================> Graph 2 <==============================

/*
  Absorbing chain from lecture notes, converted to CTMC.
    States 0,1,2 are transient.
    States 3,4,5 are absorbing.
*/
const edge graph2[] = {
  {0, 1, 2},  {0, 3, 1},    // Same as DTMC except row multiplied by 3
  {1, 2, 3},  {1, 4, 1},    // Same as DTMC except row multiplied by 4
  {2, 0, 4},  {2, 5, 1},    // Same as DTMC except row multiplied by 5
  // End 
  {-1, -1, -1}
};

const bool discrete2 = false;
const long num_nodes2 = 6;

const double init2[] = {1.0/3.0, 0, 1.0/3.0, 1.0/3.0, 0, 0};
const double tta2[] =  {1.0/3.0, 1.0/6.0, 1.0/6.0,    2.0/3.0, 1.0/6.0, 1.0/6.0};
// Adjust tta times: divide first by 3, second by 4, third by 5.

// ==============================> Graph 3 <==============================

/*
  Absorbing chain from lecture notes, converted to CTMC.
    States 0,1,2 are transient.
    States 3,4,5 are absorbing.
*/
const edge graph3[] = {
  {0, 1, 2.0/3.0},  {0, 3, 1.0/3.0},    // Same as DTMC 
  {1, 2, 1.0/4.0},  {1, 4, 1.0/12.0},   // Same as DTMC except row divided by 3
  {2, 0, 2.0/15.0}, {2, 5, 1.0/30.0},   // Same as DTMC except row divided by 6
  // End 
  {-1, -1, -1}
};

const bool discrete3 = false;
const long num_nodes3 = 6;

const double init3[] = {1.0/3.0, 0, 1.0/3.0, 1.0/3.0, 0, 0};
const double tta3[] =  {1.0, 2.0, 5.0,    2.0/3.0, 1.0/6.0, 1.0/6.0};
// Adjust tta times: first ok, multiply second by 3, multiply third by 6

// ==============================> Graph 4 <==============================

/*
  Absorbing chain from lecture notes, converted to CTMC.
    States 0,1,2 are transient.
    States 3,4,5 are absorbing.
*/
const edge graph4[] = {
  //
  // Use DTMC and multiply probabilities by 15
  //
  {0, 1, 10},         {0, 3, 5},
  {1, 2, 45.0/4.0},   {1, 4, 15.0/4.0},  
  {2, 0, 12},         {2, 5, 3},
  // End 
  {-1, -1, -1}
};

const bool discrete4 = false;
const long num_nodes4 = 6;

const double init4[] = {1.0/3.0, 0, 1.0/3.0, 1.0/3.0, 0, 0};
const double tta4[] =  {1.0/15.0, 2.0/45.0, 1.0/18.0,    2.0/3.0, 1.0/6.0, 1.0/6.0};
// Adjust tta times: divide all by 15

// ==============================> Graph 5 <==============================

/*
  The "university" DTMC.
    States 0,1,2,3 are transient.
    States 4,5 are absorbing.
*/
const edge graph5[] = {
  {0, 0, 1},  // repeat fr prob 1/10
  {0, 5, 1},  // flunk out fr prob 1/10
  {0, 1, 8},  // pass fr prob 8/10
  {1, 1, 2},  // repeat soph prob 2/10
  {1, 5, 1},  // flunk out soph prob 1/10
  {1, 2, 7},  // pass soph prob 7/10
  {2, 2, 2},  // repeat jr prob 2/10
  {2, 5, 1},  // flunk out jr prob 1/10
  {2, 3, 7},  // pass jr prob 7/10
  {3, 3, 1},  // repeat sr prob 1/10
  {3, 5, 1},  // flunk out sr prob 1/10
  {3, 4, 8},  // pass sr prob 8/10
  // End 
  {-1, -1, -1}
};

const bool discrete5 = true;
const long num_nodes5 = 6;

const double init5[] = {0.7, 0.3, 0, 0, 0, 0};
const double tta5[] =  {7.0/9.0, 83.0/72.0, 581.0/576.0, 4067.0/5184.0,     4067.0/6480.0, 2413.0/6480.0};

// ==============================> Graph 6 <==============================

/*
  The "university" DTMC, with all rates divided by 9.
    States 0,1,2,3 are transient.
    States 4,5 are absorbing.
*/
const edge graph6[] = {
  {0, 0, 1.0/90.0},  // repeat fr prob 1/10
  {0, 5, 1.0/90.0},  // flunk out fr prob 1/10
  {0, 1, 8.0/90.0},  // pass fr prob 8/10
  {1, 1, 2.0/90.0},  // repeat soph prob 2/10
  {1, 5, 1.0/90.0},  // flunk out soph prob 1/10
  {1, 2, 7.0/90.0},  // pass soph prob 7/10
  {2, 2, 2.0/90.0},  // repeat jr prob 2/10
  {2, 5, 1.0/90.0},  // flunk out jr prob 1/10
  {2, 3, 7.0/90.0},  // pass jr prob 7/10
  {3, 3, 1.0/90.0},  // repeat sr prob 1/10
  {3, 5, 1.0/90.0},  // flunk out sr prob 1/10
  {3, 4, 8.0/90.0},  // pass sr prob 8/10
  // End 
  {-1, -1, -1}
};

const bool discrete6 = false;
const long num_nodes6 = 6;

const double init6[] = {0.7, 0.3, 0, 0, 0, 0};
const double tta6[] =  {7.0, 83.0/8.0, 581.0/64.0, 4067.0/576.0,     4067.0/6480.0, 2413.0/6480.0};

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
  if (!run_test("abcdef dtmc", discrete1, graph1, num_nodes1, init1, tta1)) {
    return 1;
  }
  if (!run_test("abcdef ctmc1", discrete2, graph2, num_nodes2, init2, tta2)) {
    return 1;
  }
  if (!run_test("abcdef ctmc2", discrete3, graph3, num_nodes3, init3, tta3)) {
    return 1;
  }
  if (!run_test("abcdef ctmc3", discrete4, graph4, num_nodes4, init4, tta4)) {
    return 1;
  }
  if (!run_test("univ dtmc", discrete5, graph5, num_nodes5, init5, tta5)) {
    return 1;
  }
  if (!run_test("univ ctmc", discrete6, graph6, num_nodes6, init6, tta6)) {
    return 1;
  }

  return 0;
}


