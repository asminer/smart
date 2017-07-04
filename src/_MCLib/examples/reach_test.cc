
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
  The "university" DTMC.
    States 0,1,2,3 are transient.
    States 4,5 are absorbing.
*/
const edge graph1[] = {
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

const bool discrete1 = true;
const long num_nodes1 = 6;

const long target1[] = {4, -1};
const double reaches1[] =  {49.0/81.0, 49.0/72.0, 7.0/9.0, 8.0/9.0, 1, 0};



// =======================================================================

void show_LS_output(const char* name, const LS_Output &out)
{
#ifdef VERBOSE
  cout << name << " linear solver output:\n";
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
}

// =======================================================================

void show_set(const char* name, const intset &x)
{
  cout << "  " << name << ": {";
  bool comma = false;
  for (long i=x.getSmallestAfter(-1); i>=0; 
            i=x.getSmallestAfter(i))
  {
    if (comma) cout << ", ";
    comma = true;
    cout << i;
  }
  cout << "}\n";
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

bool run_test(const char* name, const bool discrete, const edge graph[], 
  const long num_nodes, const long target[], const double reaches[])
{
#ifdef VERBOSE
  const bool verbose = true;
#else
  const bool verbose = false;
#endif

  cout << "Testing ";
  if (discrete) cout << "DTMC "; else cout << "CTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(discrete, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(discrete, graph, num_nodes, verbose);

  //
  // Set up target set
  //
  intset tset(num_nodes);
  tset.removeAll();
  for (long i=0; target[i]>=0; i++) {
    tset.addElement(target[i]);
  }
#ifdef VERBOSE
  show_set("target set", tset);
#endif

  //
  // Set up options
  //
  LS_Options opt;
//  opt.debug = true;
  opt.method = LS_Gauss_Seidel;

  //
  // Catch LS outputs
  //
  LS_Output outd;
  LS_Output outf;

  //
  // Solve probability to reach target
  //
  double* sold = new double[num_nodes];
  double* solf = new double[num_nodes];
  double* aux = new double[num_nodes];

  try {
    MCd->computeProbsToReach(tset, sold, aux, opt, outd);
    MCf->computeProbsToReach(tset, solf, 0, opt, outf);   // gotta test no aux vector
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

  show_LS_output("double Markov chain", outd);
  show_LS_output("float  Markov chain", outf);

  show_vector("MCd solution vector", sold, num_nodes);
  show_vector("MCf solution vector", solf, num_nodes);
  show_vector("Expected     vector", reaches, num_nodes);

  double diff_d = diff_vector(reaches, sold, num_nodes);
  double diff_f = diff_vector(reaches, solf, num_nodes);

  cout << "  MCd relative difference: " << diff_d;

  bool okd = diff_d < 1e-5;

  if (okd)  cout << " (OK)\n";
  else      cout << " too large!\n";

  cout << "  MCf relative difference: " << diff_f;

  bool okf = diff_f < 1e-5;

  if (okf)  cout << " (OK)\n";
  else      cout << " too large!\n";


  // Cleanup
  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;
  delete[] aux;

  return okd && okf;
}

// =======================================================================

int main()
{
  if (!run_test("University dtmc", discrete1, graph1, num_nodes1, target1, reaches1)) {
    return 1;
  }
  return 0;
}


