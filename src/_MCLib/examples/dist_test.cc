
/*
    Tests for discrete "tta" distributions.
*/

#include <iostream>
#include <math.h>

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
const long accept1 = 1;

const double init1[] = {0.25, 0.25, 0.25, 0.25, 0};
const double distro1[] = {0, 0.25, 0.25, 0.25, 0.25, -1};
const double infty1 = 0;


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
const long accept2 = 1;

const double init2[] = {1, 0, 0, 0, 0};
const double distro2[] = {0, 0.25, 0.25, 0.25, 0.25, -1};
const double infty2 = 0;


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
const long accept3 = 1;

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
const double infty3 = 0;


// ==============================> Graph 4 <==============================

/*
  0, with prob 1/4
  1, with prob 1/4
  2, with prob 1/4
  infinity, with prob 1/4
*/
const edge graph4[] = {
  {0,1,1},
  {1,2,1},
  // End 
  {-1, -1, -1}
};

const long num_nodes4 = 4;
const long accept4 = -2;

const double init4[] = {0.25, 0.25, 0.25, 0.25};
const double distro4[] = {0.25, 0.25, 0.25, -1};
const double infty4 = 0.25;


// ==============================> Graph 5 <==============================

/*
  Expo(2).
*/
const edge graph5[] = {
  {0, 1, 2.0},
  {-1, -1, -1}
};

const long num_nodes5 = 2;
const long accept5 = 1;

const double init5[] = {1, 0};
const double zero5 = 0;
const double infty5 = 0;

double pdf5(double t) {
  return 2.0 * exp(-2.0*t);
}

const double dts5[] = { 1.6, 0.8, 0.4, 0.2, 0.1, 0.08, 0.04, 0.02, 0.01, 0};

// =======================================================================

void show_distro(const char* name, double (*pdf)(double), 
  double dt, long stop, const double zero, const double infty)
{
  cout << "  " << name << ": " << zero; 
  for (long i=1; i<stop; i++) {
    cout << ", " << pdf(i*dt);
  }
  if (infty) {
    cout << ", 0, 0, ...; " << infty << ".";
  }
  cout << "\n";
}

// =======================================================================

void show_distro(const char* name, const double* x, const double infty)
{
  cout << "  " << name << ": " << x[0];
  for (long i=1; x[i]>=0; i++) cout << ", " << x[i];
  if (infty) {
    cout << ", 0, 0, ...; " << infty << ".";
  }
  cout << "\n";
}

// =======================================================================

void show_distro(const char* name, const discrete_pdf &x)
{
  cout << "  " << name << ": " << x.f(0);
  for (long i=1; i<=x.right_trunc(); i++) {
    if (i!=x.left_trunc()) cout << ", "; else cout << "; ";
    cout << x.f(i);
  }
  if (x.f_infinity()) {
    cout << ", 0, 0, ...; " << x.f_infinity() << ".";
  }
  cout << "\n";
}

// =======================================================================

inline double reldiff(double targ, double val)
{
  double d = targ - val;
  if (d<0) d*=-1;
  if (targ) d /= targ;
  return d;
}

// =======================================================================

double diff_distro(const double* A, double inftyA, const discrete_pdf &B)
{
  double rel_diff = reldiff(inftyA, B.f_infinity());

  for (long i=0; A[i]>=0; i++) {
    double d = reldiff(A[i], B.f(i));
    if (d > rel_diff) rel_diff = d;
  }
  return rel_diff;
}

// =======================================================================

double diff_distro(double (*pdf)(double), double zeroA, double inftyA, const discrete_pdf &B, double dt)
{
  double rel_diff = reldiff(zeroA, B.f(0));
  double d = reldiff(inftyA, B.f_infinity());
  if (d > rel_diff) rel_diff = d;

  for (long i=1; i<=B.right_trunc(); i++) {
    d = reldiff(pdf(i*dt), B.f(i));
    if (d > rel_diff) rel_diff = d;
  }
  return rel_diff;
}

// =======================================================================

bool run_dtmc_test(const char* name, const edge graph[], 
  const long num_nodes, const long accept, const double init[], 
  const double distro[], const double infty)
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
  double prec_d, prec_f;

  try {
    MCd->computeDiscreteDistTTA(opt, p0, accept, distd);
    prec_d = opt.precision;

    MCf->computeDiscreteDistTTA(opt, p0, accept, distf);
    prec_f = opt.precision;
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
  cout << "    precision: " << prec_d << "\n";
  show_distro("MCf distribution", distf);
  cout << "    precision: " << prec_f << "\n";
  show_distro("Expected  distro", distro, infty);

  double diff_d = diff_distro(distro, infty, distd);
  double diff_f = diff_distro(distro, infty, distf);

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

bool run_ctmc_test(const char* name, const edge graph[], 
  const long num_nodes, const long accept, const double init[], 
  const double q, const double dt,
  double (*pdf)(double), const double zero, const double infty)
{
#ifdef VERBOSE
  const bool verbose = true;
#else
  const bool verbose = false;
#endif

  cout << "Testing CTMC ";
  cout << name << " with dt=" << dt << "\n";

  Markov_chain* MCd = build_double(false, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(false, graph, num_nodes, verbose);

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
  Markov_chain::CTMC_distribution_options opt;
  opt.setMaxSize(1000);
  opt.q = q;

  //
  // Compute distributions
  //
  discrete_pdf distd, distf;
  double prec_d, prec_f;

  try {
    MCd->computeContinuousDistTTA(opt, p0, accept, dt, distd);
    prec_d = opt.precision;

    MCf->computeContinuousDistTTA(opt, p0, accept, dt, distf);
    prec_f = opt.precision;
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

  double diff_d = diff_distro(pdf, zero, infty, distd, dt);
  cout << "  MCd:\n";
  cout << "    prec: " << prec_d << "\n";
  cout << "    size: " << distd.right_trunc() << "\n";
  cout << "    relative difference: " << diff_d;

  if (diff_d < 1e-5) {
    cout << " (OK)\n";
  }
  else {
    cout << " too large!\n";
    show_distro("MCd distribution", distd);
    show_distro("Expected  distro", pdf, dt, distd.right_trunc()+1, zero, infty);
    return false;
  }

  //

  double diff_f = diff_distro(pdf, zero, infty, distf, dt);
  cout << "  MCf:\n";
  cout << "    prec: " << prec_f << "\n";
  cout << "    size: " << distf.right_trunc() << "\n";
  cout << "    relative difference: " << diff_f;

  if (diff_f < 1e-5) {
    cout << " (OK)\n";
  }
  else {
    cout << " too large!\n";
    show_distro("MCf distribution", distf);
    show_distro("Expected  distro", pdf, dt, distf.right_trunc()+1, zero, infty);
    return false;
  }

  return true;
}

// =======================================================================

int main()
{
  cout.precision(8);

  //
  // DTMC tests
  //
  if (!run_dtmc_test("Equilikely 1,4 1", graph1, num_nodes1, accept1, init1, distro1, infty1)) {
    return 1;
  }
  if (!run_dtmc_test("Equilikely 1,4 2", graph2, num_nodes2, accept2, init2, distro2, infty2)) {
    return 1;
  }
  if (!run_dtmc_test("Geometric 1/4", graph3, num_nodes3, accept3, init3, distro3, infty3)) {
    return 1;
  }
  if (!run_dtmc_test("0, 1, 2, or infinity", graph4, num_nodes4, accept4, init4, distro4, infty4)) {
    return 1;
  }

  //
  // CTMC tests
  //
  for (long i=0; dts5[i]; i++) {
    if (!run_ctmc_test("Expo(2)", graph5, num_nodes5, accept5, init5, 2, dts5[i], pdf5, zero5, infty5)) {
      return 1;
    }
  }


  return 0;
}
