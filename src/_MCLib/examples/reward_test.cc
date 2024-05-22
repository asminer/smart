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

// ==============================> Graph 00 <==============================
/*
Total 5 states, 0..3 transient states, 4 absorbing state1
*/
const edge graph00[]={
  {0,1,1},
  {1,2,1},
  {2,3,1},
  {3,3,1},
  {3,4,1},
  {4,4,1},
  //END
  {-1,-1,-1}
  };
const long num_nodes00 = 5;
const double final00[] ={0,0,0,0,1} ;
const double t1[]={0, 0, 0, 2.25, 0};
const double ex_t2_t4[]={0, 2, 1, 0, 0}; 

const double reward_0[]={5,2.5,4.5,1.5,0};
// ==============================> Graph 01 <==============================
/*
Total 4 states, 0..2 transient states, 3 absorbing state1
*/
const edge graph01[]={
  {0,1,1},
  {0,2,1},
  {1,1,1},
  {1,3,1},
  {2,2,1},
  {2,3,1},
  {3,3,1},
  //END
  {-1,-1,-1}
  };
const long num_nodes = 4;
const double p[] ={0,0,0,1} ;
const double q[] ={0,0,0,1} ;
const double pt[]={ 0, 0.5, 0.5, 0};
//const double ex_t2_t4[]={0, 2, 1, 0, 0}; 

const double reward[]={0,1,1,0};
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
bool reverse_accumulated_reward_unbounded_test(const char* name, const edge graph[], 
  const long num_nodes, const double init[], const long time, const double reward[], const double pt[])
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
  double* r = new double[num_nodes];
  memcpy(r, reward, num_nodes*sizeof(double));

  try {
    MCd->reverse_accumulated_reward_unbounded(time, sold,r, opt);
    MCf->reverse_accumulated_reward_unbounded(time, solf,r, opt);
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
bool cond_accumulated_reward_unbounded_test(const char* name, const edge graph[], 
  const long num_nodes, const double init[], const long time, const double reward[], const double pt[],const double q[])
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
  double* r = new double[num_nodes];
  memcpy(r, reward, num_nodes*sizeof(double));
  double* Q=new double[num_nodes];
  memcpy(Q,q,num_nodes*sizeof(double));

  try {
    MCd->conditional_accumulated_reward_unbounded(time, sold,r, Q,opt);
    MCf->conditional_accumulated_reward_unbounded(time, solf,r, Q,opt);
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

int main(){

    //bool test = reverse_accumulated_reward_unbounded_test("test", graph00,num_nodes00,final00,15,reward_0,t1);
    //show_vector("MCd solution vector", sold, num_nodes);
    bool test_c = cond_accumulated_reward_unbounded_test("test_cond", graph01,num_nodes,p,3,reward,pt,q);
    //bool test = reverse_accumulated_reward_unbounded_test("test", graph01,num_nodes,p,35,reward,pt);
    return 0;
}