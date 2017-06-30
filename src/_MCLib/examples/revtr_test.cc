
/*
    Tests for "reverse transient" computation
*/

#include <iostream>
#include <string.h>
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

const double final1[] = {0, 1, 0}; 
const double e1_a[] = {0.25, 0, 0.25};
const double e1_b[] = {3.0/16.0, 1.0/4.0, 3.0/16.0};
const double e1_c[] = {13.0/64.0, 3.0/16.0, 13.0/64.0};
const double e1_d[] = {1.0/5.0, 1.0/5.0, 1.0/5.0};


// ==============================> Graph 2 <==============================

/*
  The "university" DTMC.
    States 0,1,2,3 are transient.
    States 4,5 are absorbing.
*/
const edge graph2[] = {
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

const long num_nodes2 = 6;

const double final2[] = {0, 0, 0, 0, 1, 0};
const double e2_a[] = {0, 0, 0, 0.8, 1, 0};
const double e2_b[] = {0, 0, 0.56, 0.88, 1, 0};
const double e2_c[] = {0, 0.392, 0.728, 0.888, 1, 0};
const double e2_d[] = {0.3136, 0.588, 0.7672, 0.8888, 1, 0};
const double e2_e[] = {0.50176, 0.65464, 0.7756, 0.88888, 1, 0};
const double e2_f[] = {49.0/81.0, 49.0/72.0, 7.0/9.0, 8.0/9.0, 1, 0};

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

const double final3a[] = {1, 0, 0};
const double e3_a10[] = {0.0024787522, 0, 0};
const double final3b[] = {0, 1, 0};
const double e3_b10[] = {0.49999693, 0.50000307, 0.49999693};


// ==============================> Graph 4 <==============================

/*
  Land of oz, as a ctmc.
*/
const edge graph4[] = {
  {0, 0, 0.5},    {0, 1, 0.25},   {0, 2, 0.25},
  {1, 0, 0.5},                    {1, 2, 0.5},
  {2, 0, 0.25},   {2, 1, 0.25},   {2, 2, 0.5},
  // End 
  {-1, -1, -1}
};

const long num_nodes4 = 3;

const double final4[] = {0, 1, 0}; 
const double e4_a[] = {0.14269904, 0.42920384, 0.14269904};
const double e4_b[] = {0.183583, 0.265668, 0.183583};
const double e4_c[] = {0.19865241, 0.20539036, 0.19865241};
const double e4_d[] = {1.0/5.0, 1.0/5.0, 1.0/5.0};

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
    MCd->reverseTransient(time, sold, opt);
    MCf->reverseTransient(time, solf, opt);
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
    MCd->reverseTransient(time, sold, opt);
    MCf->reverseTransient(time, solf, opt);
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
  if (!run_dtmc_test("Oz t1", graph1, num_nodes1, final1, 1, e1_a)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t2", graph1, num_nodes1, final1, 2, e1_b)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t3", graph1, num_nodes1, final1, 3, e1_c)) {
    return 1;
  }
  if (!run_dtmc_test("Oz t1000000", graph1, num_nodes1, final1, 1000000, e1_d)) {
    return 1;
  }

  if (!run_dtmc_test("University t1", graph2, num_nodes2, final2, 1, e2_a)) {
    return 1;
  }
  if (!run_dtmc_test("University t2", graph2, num_nodes2, final2, 2, e2_b)) {
    return 1;
  }
  if (!run_dtmc_test("University t3", graph2, num_nodes2, final2, 3, e2_c)) {
    return 1;
  }
  if (!run_dtmc_test("University t4", graph2, num_nodes2, final2, 4, e2_d)) {
    return 1;
  }
  if (!run_dtmc_test("University t5", graph2, num_nodes2, final2, 5, e2_e)) {
    return 1;
  }
  if (!run_dtmc_test("University t1000000", graph2, num_nodes2, final2, 1000000, e2_f)) {
    return 1;
  }


  //
  // CTMC tests
  //
  if (!run_ctmc_test("Transient", 0.6, graph3, num_nodes3, final3a, 10, e3_a10)) {
    return 1;
  }
  if (!run_ctmc_test("Transient", 0.8, graph3, num_nodes3, final3a, 10, e3_a10)) {
    return 1;
  }
  if (!run_ctmc_test("Transient", 0.6, graph3, num_nodes3, final3b, 10, e3_b10)) {
    return 1;
  }
  if (!run_ctmc_test("Transient", 0.8, graph3, num_nodes3, final3b, 10, e3_b10)) {
    return 1;
  }

  if (!run_ctmc_test("Oz t1", 0, graph4, num_nodes4, final4, 1, e4_a)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t2", 0, graph4, num_nodes4, final4, 2, e4_b)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t4", 0, graph4, num_nodes4, final4, 4, e4_c)) {
    return 1;
  }
  if (!run_ctmc_test("Oz t1000", 0, graph4, num_nodes4, final4, 1000, e4_d)) {
    return 1;
  }

  return 0;
}


