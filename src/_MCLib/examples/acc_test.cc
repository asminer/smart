
/*
    Tests for "steady state" computation
*/

#include <iostream>

#include "mcbuilder.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

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
  The "university" DTMC.
    States 0,1,2,3 are transient.
    States 4,5 are absorbing.
*/
const edge graph3[] = {
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

const long num_nodes3 = 6;

const double init3[] = {0.7, 0.3, 0, 0, 0, 0};
const double acc3[] =  {7.0/9.0, 83.0/72.0, 581.0/576.0, 4067.0/5184.0,     60.653043, 36.623192};

// ==============================> Graph 4 <==============================

/*
  Simple two-state CTMC with rate lambda = 2.
*/
const edge graph4[] = {
  {0, 1, 2.0},
  // End 
  {-1, -1, -1}
};

const long num_nodes4 = 2;

const double init4[] = {1, 0};

// 
// accumulated up to time t vector should be 
//      [ 1/2 - 1/2 exp(-2t),  *]
// where * is such that the vector elements sum to t.
//

// time 0
const double acc4_a[] = {0, 0};

// time 1/2
const double acc4_b[] = { 0.3160602794142788, 0.1839397205857212};
// time 1
const double acc4_c[] = { 0.4323323583816937, 0.5676676416183063};
// time 2
const double acc4_d[] = { 0.4908421805556329, 1.5091578194443671};
// time 50
const double acc4_e[] = { 0.5, 49.5 };


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
#ifdef VERBOSE
  const bool verbose = true;
#else
  const bool verbose = false;
#endif

  cout << "Testing DTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(true, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(true, graph, num_nodes, verbose);

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

bool run_ctmc_test(const char* name, const double q, const edge graph[], 
  const long num_nodes, const double init[], const double time, const double pt[])
{
#ifdef VERBOSE
  const bool verbose = true;
#else
  const bool verbose = false;
#endif

  cout << "Testing CTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(false, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(false, graph, num_nodes, verbose);

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


  if (!run_dtmc_test("University t100", graph3, num_nodes3, init3, 100, acc3)) {
    return 1;
  }

  //
  // CTMC tests
  //

  if (!run_ctmc_test("2-state t0", 2, graph4, num_nodes4, init4, 0, acc4_a)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t0.5", 2, graph4, num_nodes4, init4, 0.5, acc4_b)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t1", 2, graph4, num_nodes4, init4, 1, acc4_c)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t1", 3, graph4, num_nodes4, init4, 1, acc4_c)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t2", 2, graph4, num_nodes4, init4, 2, acc4_d)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t2", 3, graph4, num_nodes4, init4, 2, acc4_d)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t50", 2, graph4, num_nodes4, init4, 50, acc4_e)) {
    return 1;
  }
  if (!run_ctmc_test("2-state t50", 3, graph4, num_nodes4, init4, 50, acc4_e)) {
    return 1;
  }

  return 0;
}


