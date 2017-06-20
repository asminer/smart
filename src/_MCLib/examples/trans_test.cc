
/*
    Tests for "steady state" computation
*/

#include <iostream>
#include "mcbuilder.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

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
const long   time1_a = 1;
const double p1_a[] =  {0.5, 0, 0.5};
const long   time1_b = 2;
const double p1_b[] =  {3.0/8.0, 2.0/8.0, 3.0/8.0};
const long   time1_c = 3;
const double p1_c[] =  {13.0 / 32.0, 6.0/32.0, 13.0/32.0};
const long   time1_d = 1000000;
const double p1_d[] =  {2.0/5.0, 1.0/5.0, 2.0/5.0};


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

const double p2_0[] = {1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0}; 
const double p2_1[] = {0, 1.0/2.0, 1.0/6.0, 1.0/3.0, 0};
const double p2_2[] = {0, 1.0/2.0, 0, 1.0/6.0, 1.0/3.0};
const double p2_3[] = {0, 1.0/2.0, 1.0/3.0, 0, 1.0/6.0};

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
  memcpy(sold, init, num_nodes*sizeof(double));
  double* solf = new double[num_nodes];
  memcpy(solf, init, num_nodes*sizeof(double));

  try {
    MCd->computeTransient(time, sold, opt);
    MCf->computeTransient(time, solf, opt);
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
  memcpy(sold, init, num_nodes*sizeof(double));
  double* solf = new double[num_nodes];
  memcpy(solf, init, num_nodes*sizeof(double));

  try {
    MCd->computeTransient(time, sold, opt);
    MCf->computeTransient(time, solf, opt);
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
  if (!run_dtmc_test("Oz t1", graph1, num_nodes1, init1, time1_a, p1_a)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t2", graph1, num_nodes1, init1, time1_b, p1_b)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t3", graph1, num_nodes1, init1, time1_c, p1_c)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t1000000", graph1, num_nodes1, init1, time1_d, p1_d)) {
    return 1;
  }

  if (!run_dtmc_test("Periodic t1", graph2, num_nodes2, p2_0, 1, p2_1)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t2", graph2, num_nodes2, p2_0, 2, p2_2)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t3", graph2, num_nodes2, p2_0, 3, p2_3)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t4", graph2, num_nodes2, p2_0, 4, p2_1)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t5", graph2, num_nodes2, p2_0, 5, p2_2)) {
    return 1;
  }
  if (!run_dtmc_test("Periodic t6", graph2, num_nodes2, p2_0, 6, p2_3)) {
    return 1;
  }

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

  return 0;
}


