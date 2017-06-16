
/*
    Tests for "steady state" computation
*/

#include <iostream>
#include "mclib.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

typedef struct { long from; long to; double rate; } edge;

// ==============================> Graph 0 <==============================

/*
  Trivial one state DTMC
*/
const edge graph0[] = {
  // End 
  {-1, -1, -1}
};

const long num_nodes0 = 1;

const double init0[] = {1};

const double acc0_a[] = {1};
const double acc0_b[] = {2};
const double acc0_c[] = {4};
const double acc0_d[] = {8};

// ==============================> Graph 1 <==============================

/*
  Land of oz.
*/
const edge graph1[] = {
  {0, 0, 2},  {0, 1, 1},  {0, 2, 1},
  {1, 0, 1},              {1, 2, 1},
  {2, 0, 1},  {2, 1, 1},  {2, 2, 2},
  // End 
  {-1, -1, -1}
};

const long num_nodes1 = 3;

const double init1[] = {0, 1, 0}; 

const double acc1_a[] = {0, 1, 0};    
const double acc1_b[] = {0.5, 1, 0.5};
const double acc1_c[] = {7.0/8.0, 10.0/8.0, 7.0/8.0};
const double acc1_d[] = {41.0/32.0, 46.0/32.0, 41.0/32.0};



// ==============================> Graph 2 <==============================

/*
  A fun DTMC with period 3.
*/
const edge graph2[] = {
  {0, 1, 1},  {0, 2, 1},
  {2, 3, 1},
  {3, 4, 1},
  {4, 2, 1},
  // End 
  {-1, -1, -1}
};

const long num_nodes2 = 5;

const double init2[] = {1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0}; 

const double acc2_a[] = {1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0}; 
const double acc2_b[] = {1.0/3.0, 5.0/6.0, 1.0/2.0, 1.0/3.0, 0}; 
const double acc2_c[] = {1.0/3.0, 4.0/3.0, 1.0/2.0, 1.0/2.0, 1.0/3.0};
const double acc2_d[] = {1.0/3.0, 11.0/6.0, 5.0/6.0, 1.0/2.0, 1.0/2.0};

// ==============================> Graph 3 <==============================

/*
  Transient analysis CTMC from lecture notes
*/
const edge graph3[] = {
  {0, 1, 0.6},
  {1, 2, 0.6},
  {2, 1, 0.6},
  // End 
  {-1, -1, -1}
};

const long num_nodes3 = 3;

const double p3_0[] = {1, 0, 0};
const double p3_10[] = {0.0024787522, 0.49999693, 0.49752432};

// ==============================> Graph 4 <==============================

/*
  Land of oz as a CTMC.
*/
const edge graph4[] = {
  {0, 0, 0.5},  {0, 1, 0.25}, {0, 2, 0.25},
  {1, 0, 0.5},                {1, 2, 0.5},
  {2, 0, 0.25}, {2, 1, 0.25}, {2, 2, 0.5},
  // End 
  {-1, -1, -1}
};

const long num_nodes4 = 3;

const double p4_0[] = {0, 1, 0}; 
const double p4_1[] = {0.28539808, 0.42920384, 0.28539808};
const double p4_2[] = {0.36716600, 0.26566800, 0.36716600};
const double p4_4[] = {0.39730482, 0.20539036, 0.39730482};
const double p4_8[] = {0.39998184, 0.20003632, 0.39998184};


const double p4_ss[] = {2.0/5.0, 1.0/5.0, 2.0/5.0};


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

// =======================================================================

Markov_chain* build_double(const bool discrete, const edge graph[], 
  const long num_nodes)
{
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

  Markov_chain* MC = new Markov_chain(discrete, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;
  return MC;
}

// =======================================================================

Markov_chain* build_float(const bool discrete, const edge graph[], 
  const long num_nodes)
{
#ifdef VERBOSE
  my_timer T(true);
#else
  my_timer T(false);
#endif

  //
  // Build graph from list of edges
  //
  dynamic_summable<float>* G = new dynamic_summable<float>(true, true);
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

  Markov_chain* MC = new Markov_chain(discrete, *G, C, &T);
  delete R;
  delete SCCs;
  delete G;
  return MC;
}

// =======================================================================

void show_vector(const char* name, const double* x, long size)
{
  cout << "  " << name << ": [" << x[0];
  for (long i=1; i<size; i++) cout << ", " << x[i];
  cout << "]\n";
}

// =======================================================================

double diff_vector(const double* A, const double* B, long size)
{
  double rel_diff = 0;
  for (long i=0; i<size; i++) {
    double d = A[i] - B[i];
    if (d<0) d*=-1;
    if (A[i]) d /= A[i];
    if (d > rel_diff) rel_diff = d;
  }
  return rel_diff;
}

// =======================================================================

bool run_dtmc_test(const char* name, const edge graph[], 
  const long num_nodes, const double init[], const long time, const double pt[])
{
  cout << "Testing DTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(true, graph, num_nodes);  
  Markov_chain* MCf = build_float(true, graph, num_nodes);  

  //
  // Set up options
  //
  Markov_chain::DTMC_transient_options opt;

  //
  // Solve finite time probabilities
  //
  double* sold = new double[num_nodes];
  double* solf = new double[num_nodes];

  try {
    MCd->accumulate(time, init, sold, opt);
    MCf->accumulate(time, init, solf, opt);
  }
  catch (GraphLib::error e) {
    cout << "    Caught graph library error: ";
    cout << e.getString() << "\n";
    return false;
  }
  catch (MCLib::error e) {
    cout << "    Caught Markov chain library error: ";
    cout << e.getString() << "\n";
    return false;
  }

  //
  // Check results
  //

  if (opt.multiplications < time) {
    cout << "Performed " << opt.multiplications;
    cout << " multiplications instead of " << time << "\n";
  }
  show_vector("MCd solution vector", sold, num_nodes);
  show_vector("MCf solution vector", solf, num_nodes);
  show_vector("Expected     vector", pt, num_nodes);

  double diff_d = diff_vector(pt, sold, num_nodes);
  double diff_f = diff_vector(pt, solf, num_nodes);

  cout << "  MCd relative difference: " << diff_d;

  bool okd = diff_d < 1e-5;

  if (okd)  cout << " (OK)\n";
  else      cout << " too large!\n";

  cout << "  MCf relative difference: " << diff_f;

  bool okf = diff_f < 1e-5;

  if (okf)  cout << " (OK)\n";
  else      cout << " too large!\n";

  //
  // Cleanup
  //
  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;

  return okd && okf;
}

// =======================================================================

/*
bool run_ctmc_test(const char* name, const double q, const edge graph[], 
  const long num_nodes, const double init[], const double time, const double pt[])
{
  cout << "Testing CTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(false, graph, num_nodes);  
  Markov_chain* MCf = build_float(false, graph, num_nodes);  

  //
  // Set up options
  //
  Markov_chain::CTMC_transient_options opt;
  opt.q = q;

  //
  // Solve finite time probabilities
  //
  double* sold = new double[num_nodes];
  double* solf = new double[num_nodes];

  try {
    MCd->accumulate(time, init, sold, opt);
    MCf->accumulate(time, init, solf, opt);
  }
  catch (GraphLib::error e) {
    cout << "    Caught graph library error: ";
    cout << e.getString() << "\n";
    return false;
  }
  catch (MCLib::error e) {
    cout << "    Caught Markov chain library error: ";
    cout << e.getString() << "\n";
    return false;
  }

  //
  // Check results
  //

  cout << "Poisson right truncation point: " << opt.poisson_right << "\n";
  cout << "Performed " << opt.multiplications << " multiplications\n";

  show_vector("MCd solution vector", sold, num_nodes);
  show_vector("MCf solution vector", solf, num_nodes);
  show_vector("Expected     vector", pt, num_nodes);

  double diff_d = diff_vector(pt, sold, num_nodes);
  double diff_f = diff_vector(pt, solf, num_nodes);

  cout << "  MCd relative difference: " << diff_d;

  bool okd = diff_d < 1e-5;

  if (okd)  cout << " (OK)\n";
  else      cout << " too large!\n";

  cout << "  MCf relative difference: " << diff_f;

  bool okf = diff_f < 1e-5;

  if (okf)  cout << " (OK)\n";
  else      cout << " too large!\n";

  //
  // Cleanup
  //
  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;

  return okd && okf;
}
*/

// =======================================================================

int main()
{
  cout.precision(8);

  //
  // DTMC tests
  //
  if (!run_dtmc_test("1-state t0", graph0, num_nodes0, init0, 0, acc0_a)) {
    return 1;
  }
  if (!run_dtmc_test("1-state t1", graph0, num_nodes0, init0, 1, acc0_b)) {
    return 1;
  }
  if (!run_dtmc_test("1-state t3", graph0, num_nodes0, init0, 3, acc0_c)) {
    return 1;
  }
  if (!run_dtmc_test("1-state t7", graph0, num_nodes0, init0, 7, acc0_d)) {
    return 1;
  }

  if (!run_dtmc_test("Oz t0", graph1, num_nodes1, init1, 0, acc1_a)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t1", graph1, num_nodes1, init1, 1, acc1_b)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t2", graph1, num_nodes1, init1, 2, acc1_c)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t3", graph1, num_nodes1, init1, 3, acc1_d)) {
    return 1;
  }

  if (!run_dtmc_test("Periodic t0", graph2, num_nodes2, init2, 0, acc2_a)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t1", graph2, num_nodes2, init2, 1, acc2_b)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t2", graph2, num_nodes2, init2, 2, acc2_c)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t3", graph2, num_nodes2, init2, 3, acc2_d)) {
    return 1;
  }
  /*

  //
  // CTMC tests
  //
  if (!run_ctmc_test("Transient", 0.6, graph3, num_nodes3, p3_0, 10, p3_10)) {
    return 1;
  }
  if (!run_ctmc_test("Transient", 0.8, graph3, num_nodes3, p3_0, 10, p3_10)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t1", 0, graph4, num_nodes4, p4_0, 1, p4_1)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t2", 0, graph4, num_nodes4, p4_0, 2, p4_2)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t4", 0, graph4, num_nodes4, p4_0, 4, p4_4)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t8", 0, graph4, num_nodes4, p4_0, 8, p4_8)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t100", 0, graph4, num_nodes4, p4_0, 100, p4_ss)) {
    return 1;
  }
  */

  return 0;
}


