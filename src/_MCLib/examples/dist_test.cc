
/*
    Tests for discrete "tta" distributions.
*/

#include <iostream>

#include "mcbuilder.h"

// #define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

// ==============================> Graph 1 <==============================

/*
  Equilikely 1,4 DTMC.
    States 0-3: transient
    State 4: absorbing accepting.
*/
const edge graph1[] = {
  {0,1,1},
  {1,2,1},
  {2,3,1},
  {3,4,1},
  // End 
  {-1, -1, -1}
};

const long num_nodes1 = 5;

const double init1[] = {0.25, 0.25, 0.25, 0.25, 0};
const double distro1[] = {0, 0.25, 0.25, 0.25, 0.25, -1};


// ==============================> Graph 2 <==============================

/*
  Equilikely 1,4 DTMC.
    States 0-3: transient
    State 4: absorbing accepting.
*/
const edge graph2[] = {
  {0,4,1},  {0,1,3},
  {1,4,1},  {1,2,2},
  {2,4,1},  {2,3,1},
  {3,4,1},
  // End 
  {-1, -1, -1}
};

const long num_nodes2 = 5;

const double init2[] = {1, 0, 0, 0, 0};
const double distro2[] = {0, 0.25, 0.25, 0.25, 0.25, -1};


// ==============================> Graph 3 <==============================

/*
  Geometric 1/4 DTMC
*/
const edge graph3[] = {
  {0,0,1},
  {0,1,3},
  // End 
  {-1, -1, -1}
};

const long num_nodes3 = 2;

const double init3[] = {1, 0};
const double distro3[] = {0, 
  3.0/4.0, 
  3.0/16.0, 
  3.0/64.0, 
  3.0/256.0,
  3.0/1024.0,
  3.0/4096.0,
  3.0/16384.0,
  3.0/65536.0, 
  3.0/262144.0,
  3.0/1048576.0,
-1};


// =======================================================================

void show_distro(const char* name, const double* x)
{
  cout << "  " << name << ": " << x[0];
  for (long i=1; x[i]>=0; i++) cout << ", " << x[i];
  cout << "\n";
}

// =======================================================================

void show_distro(const char* name, discrete_pdf &x)
{
  cout << "  " << name << ": " << x.f(0);
  for (long i=1; i<=x.right_trunc(); i++) cout << ", " << x.f(i);
  cout << "\n";
}

// =======================================================================

double diff_distro(const double* A, discrete_pdf &B)
{
  double rel_diff = 0;
  for (long i=0; A[i]>=0; i++) {
    double d = A[i] - B.f(i);
    if (d<0) d*=-1;
    if (A[i]) d /= A[i];
    if (d > rel_diff) rel_diff = d;
  }
  return rel_diff;
}

// =======================================================================

bool run_dtmc_test(const char* name, const edge graph[], 
  const long num_nodes, const double init[], const double distro[])
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
  // Set up p0
  //
  LS_Vector p0;
  p0.index = 0;
  p0.size = num_nodes;
  p0.d_value = init;
  p0.f_value = 0;

  //
  // Set up options
  //
  Markov_chain::DTMC_distribution_options opt;
  opt.setMaxSize(1000);

  //
  // Compute distributions
  //
  discrete_pdf distd, distf;

  try {
    MCd->computeDiscreteDistTTA(opt, p0, 1, distd);
    MCf->computeDiscreteDistTTA(opt, p0, 1, distf);
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

  show_distro("MCd distribution", distd);
  show_distro("MCf distribution", distf);
  show_distro("Expected  distro", distro);

  double diff_d = diff_distro(distro, distd);
  double diff_f = diff_distro(distro, distf);

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

  return okd && okf;
}

// =======================================================================

int main()
{
  cout.precision(8);

  //
  // DTMC tests
  //
  if (!run_dtmc_test("Equilikely 1,4 1", graph1, num_nodes1, init1, distro1)) {
    return 1;
  }
  if (!run_dtmc_test("Equilikely 1,4 2", graph2, num_nodes2, init2, distro2)) {
    return 1;
  }
  if (!run_dtmc_test("Geometric 1/4", graph3, num_nodes3, init3, distro3)) {
    return 1;
  }


  return 0;
}
