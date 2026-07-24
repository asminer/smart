/*
    Tests for "reverse transient" computation
*/

#include <iostream>
#include <string.h>
#include <vector>
#include <cstdint>
#include <cstdio>
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
// ==============================> Graph 02 <==============================
const edge graph02[]={
  {0,1,5},
  {0,2,2},
  {0,3,3},
  {1,4,1},
  {2,4,1},
  {3,5,1},
  {4,4,1},
  {5,5,1},
  // {1,4,1},
  // {2,4,1},
  // {3,5,1},
  // {4,4,1},
  // {5,5,1},
  //END
  {-1,-1,-1}
  };
const long num_nodes2 = 6;
const double p2[] ={0,0,0,0,1,0} ;
const double q2[] ={0,0,0,0,1,0} ;
const double pt2[]={ 0.7142857614,0,0,0,0,0};

const double reward2[]={0,1,0,1,0,0};

// ==============================> Graph 03 <==============================
const edge graph03[]={
  {0,1,1},
  {1,2,1},
  {2,3,1},
  {3,4,1},
  {4,4,1},
  //END
  {-1,-1,-1}
  };
const long num_nodes3 = 5;
const double p3[] ={0,0,0,0,1} ;
const double q3[] ={0,0,0,0,1} ;
/* Goal within 4 steps; accumulate reward for first 3 steps only. */
const double pt3[]={ 2, 0, 0, 0, 0};
/* Same line, T=4 reward steps: same result here because goal is reached at step 4. */
const double pt3_T4[]={ 2, 0, 0, 0, 0};
/* Gold for reverse_accumulated_reward_unbounded_time on graph03. */
const double pt3_acc_t3[]={ 0, 2, 1, 1, 0 };
const double pt3_acc_t4[]={ 2, 2, 1, 1, 0 };

const double reward3[]={0,1,0,1,0};
/* Gold for conditional_accumulated_reward_timestep (cond_acc_reward / templ_dtmc_accumulated_reward_timestep). */
const double pt3_timestep_t4[] = { 2, 0, 0, 0, 0 };
//=============================> Graph 04 <==============================
const edge graph04[]={
  {0,1,1},
  {1,2,1},
  {2,3,1},
  {3,4,1},
  {3,5,1},
  {4,4,1},
  {5,5,1},
  //END
  {-1,-1,-1}
  };
const long num_nodes4 = 6;
const double p4[] ={0,0,0,0,1,0} ;
const double q4[] ={0,0,0,0,1,0} ;
const double pt4[]={ 2, 0, 0, 0, 0, 0};

const double reward4[]={0,1,0,1,0,0};
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

// ----------------------------------------------------------------------
// Independent reference for conditional_accumulated_reward_unbounded_time:
// diag/off split matches mcbuilder DTMCs: non-absorbing = class 0, absorbing = 1.
// First backward step uses only cross-class (off) transitions; later steps use P.
// ----------------------------------------------------------------------
static void build_P_from_edges(const edge graph[], long n, std::vector<double>& P)
{
  P.assign((size_t)n * (size_t)n, 0.0);
  std::vector<double> rowsum((size_t)n, 0.0);
  for (long k = 0; graph[k].from >= 0; k++) {
    long i = graph[k].from;
    long j = graph[k].to;
    double w = graph[k].rate;
    rowsum[(size_t)i] += w;
    P[(size_t)i * (size_t)n + (size_t)j] += w;
  }
  for (long i = 0; i < n; i++) {
    if (rowsum[(size_t)i] <= 0.0) {
      P[(size_t)i * (size_t)n + (size_t)i] = 1.0;
    } else {
      double rs = rowsum[(size_t)i];
      for (long j = 0; j < n; j++) {
        P[(size_t)i * (size_t)n + (size_t)j] /= rs;
      }
    }
  }
}

// Class ids consistent with mcbuilder DTMC split:
// all non-absorbing states share one class; all absorbing states another.
static void build_coarse_dtmc_classes(long n, const std::vector<double>& P,
  std::vector<long>& cid)
{
  cid.resize((size_t)n);
  const long TRANSIENT_CLASS = 0;
  const long ABSORBING_CLASS = 1;
  for (long i = 0; i < n; i++) {
    bool absorbing = true;
    for (long j = 0; j < n; j++) {
      if (P[(size_t)i * (size_t)n + (size_t)j] > 1e-15 && j != i) {
        absorbing = false;
        break;
      }
    }
    cid[(size_t)i] = absorbing ? ABSORBING_CLASS : TRANSIENT_CLASS;
  }
}

static void ref_hit_probs(long n, const std::vector<double>& P,
  const double* q_goal, double* h)
{
  std::vector<double> y((size_t)n, 0.0);
  for (long i = 0; i < n; i++) {
    h[i] = (q_goal[i] != 0.0) ? 1.0 : 0.0;
  }
  for (int it = 0; it < 256; it++) {
    for (long i = 0; i < n; i++) {
      double s = 0.0;
      for (long j = 0; j < n; j++) {
        s += P[(size_t)i * (size_t)n + (size_t)j] * h[j];
      }
      y[(size_t)i] = s;
    }
    double delta = 0.0;
    for (long i = 0; i < n; i++) {
      if (q_goal[i] != 0.0) {
        h[i] = 1.0;
      } else {
        double ni = y[(size_t)i];
        if (ni < 0.0) ni = 0.0;
        if (ni > 1.0) ni = 1.0;
        double d = ni - h[i];
        if (d < 0.0) d = -d;
        if (d > delta) delta = d;
        h[i] = ni;
      }
    }
    if (delta < 1e-14) break;
  }
}

static void ref_backward_step(long n, int mult, const std::vector<double>& P,
  const std::vector<long>& cid, const double* newQ, double* acc)
{
  for (long i = 0; i < n; i++) acc[i] = 0.0;
  if (mult == 1) {
    for (long i = 0; i < n; i++) {
      for (long j = 0; j < n; j++) {
        if (cid[(size_t)i] != cid[(size_t)j]) {
          acc[i] += P[(size_t)i * (size_t)n + (size_t)j] * newQ[j];
        }
      }
    }
  } else {
    for (long i = 0; i < n; i++) {
      double s = 0.0;
      for (long j = 0; j < n; j++) {
        s += P[(size_t)i * (size_t)n + (size_t)j] * newQ[j];
      }
      acc[i] = s;
    }
  }
}

static void ref_cond_accumulated_reward_unbounded_time(long n, int t_prob, int T_reward,
  std::vector<double>& P, const double* goal, const double* reward, const double* q_avoid,
  double* p_out)
{
  const int reward_steps = std::max(0, std::min(t_prob, T_reward));
  std::vector<double> D((size_t)n), U((size_t)n, 0.0), Dnext((size_t)n), Unext((size_t)n), goal0((size_t)n);

  for (long i = 0; i < n; i++) {
    D[(size_t)i] = goal[i];
    goal0[(size_t)i] = goal[i];
  }

  for (int step = 0; step < t_prob; step++) {
    for (long i = 0; i < n; i++) {
      double sD = 0.0;
      double sU = 0.0;
      for (long j = 0; j < n; j++) {
        const double pij = P[(size_t)i * (size_t)n + (size_t)j];
        sD += pij * D[(size_t)j];
        sU += pij * U[(size_t)j];
      }
      Dnext[(size_t)i] = sD;
      Unext[(size_t)i] = sU;
    }

    for (long i = 0; i < n; i++) {
      if (q_avoid[i] != 0.0) {
        Dnext[(size_t)i] = 0.0;
        Unext[(size_t)i] = 0.0;
      } else if (step < reward_steps) {
        double add = reward[i] * Dnext[(size_t)i];
        if (step == 0 && goal0[(size_t)i] != 0.0) {
          add = 0.0;
        }
        Unext[(size_t)i] += add;
      }
    }

    D.swap(Dnext);
    U.swap(Unext);
  }

  for (long i = 0; i < n; i++) {
    const double den = D[(size_t)i];
    p_out[i] = (den != 0.0) ? (U[(size_t)i] / den) : 0.0;
  }
}

static void ref_reverse_accumulated_reward_unbounded_time(long n, int t,
  std::vector<double>& P, const double* goal, const double* reward, double* p_out)
{
  std::vector<double> D((size_t)n), U((size_t)n, 0.0), Dnext((size_t)n), Unext((size_t)n), goal0((size_t)n);

  for (long i = 0; i < n; i++) {
    D[(size_t)i] = goal[i];
    goal0[(size_t)i] = goal[i];
  }

  for (int step = 0; step < t; step++) {
    for (long i = 0; i < n; i++) {
      double sD = 0.0;
      double sU = 0.0;
      for (long j = 0; j < n; j++) {
        const double pij = P[(size_t)i * (size_t)n + (size_t)j];
        sD += pij * D[(size_t)j];
        sU += pij * U[(size_t)j];
      }
      Dnext[(size_t)i] = sD;
      Unext[(size_t)i] = sU;
    }

    for (long i = 0; i < n; i++) {
      double add = 0.0;
      if (Dnext[(size_t)i] > 0.0) {
        add = reward[i];
      }
      if (step == 0 && goal0[(size_t)i] != 0.0) {
        add = 0.0;
      }
      Unext[(size_t)i] += add;
    }

    D.swap(Dnext);
    U.swap(Unext);
  }

  for (long i = 0; i < n; i++) {
    p_out[i] = U[(size_t)i];
  }
}

//=======================================================================


bool cond_accumulated_reward_timestep_test(const char* name, const edge graph[], 
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
    MCd->conditional_accumulated_reward_timestep(time,sold,r,Q,opt);
    MCf->conditional_accumulated_reward_timestep(time, solf,r, Q,opt);
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

// ******************************************************************
bool conditional_accumulated_reward_unbounded_time_test(const char* name, const edge graph[], 
  const long num_nodes, const double init[], const long time, const long T, const double reward[], const double pt[],const double q[])
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
    MCd->conditional_accumulated_reward_unbounded_time(time, T,sold,r, Q,opt);
    MCf->conditional_accumulated_reward_unbounded_time(time, T,solf,r, Q,opt);
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

// ----------------------------------------------------------------------
// Quiet regression: multiple DTMCs for backward timed conditional reward.
// On failure prints the case name and vectors; on success prints one summary line.
// ----------------------------------------------------------------------
static bool quiet_cond_accumulated_reward_unbounded_time_case(const char* case_name,
  const edge graph[], long num_nodes, const double init[], long time, long T,
  const double reward[], const double q_mask[], const double expect[])
{
  const double tol = 1e-5;
  bool verbose = false;
  Markov_chain* MCd = build_double(true, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(true, graph, num_nodes, verbose);
  Markov_chain::DTMC_transient_options opt;

  double* sold = new double[num_nodes];
  memcpy(sold, init, num_nodes * sizeof(double));
  double* solf = new double[num_nodes];
  memcpy(solf, init, num_nodes * sizeof(double));
  double* r = new double[num_nodes];
  memcpy(r, reward, num_nodes * sizeof(double));
  double* Q = new double[num_nodes];
  memcpy(Q, q_mask, num_nodes * sizeof(double));

  bool ok = true;
  try {
    MCd->conditional_accumulated_reward_unbounded_time(time, T, sold, r, Q, opt);
    MCf->conditional_accumulated_reward_unbounded_time(time, T, solf, r, Q, opt);
  }
  catch (GraphLib::error e) {
    cerr << "  [" << case_name << "] graph error: " << e.getString() << "\n";
    ok = false;
  }
  catch (MCLib::error e) {
    cerr << "  [" << case_name << "] mclib error: " << e.getString() << "\n";
    ok = false;
  }

  if (ok) {
    double dd = diff_vector(expect, sold, num_nodes);
    double df = diff_vector(expect, solf, num_nodes);
    if (dd >= tol || df >= tol) {
      ok = false;
      cerr << "  FAIL " << case_name << " (tol " << tol << ")\n";
      show_vector("    MCd", sold, num_nodes);
      show_vector("    MCf", solf, num_nodes);
      show_vector("    exp", expect, num_nodes);
    }
  }

  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;
  delete[] r;
  delete[] Q;
  return ok;
}

static bool quiet_reverse_accumulated_reward_unbounded_time_case(const char* case_name,
  const edge graph[], long num_nodes, const double init[], long time,
  const double reward[], const double expect[])
{
  const double tol = 1e-5;
  bool verbose = false;
  Markov_chain* MCd = build_double(true, graph, num_nodes, verbose);
  Markov_chain* MCf = build_float(true, graph, num_nodes, verbose);
  Markov_chain::DTMC_transient_options opt;

  double* sold = new double[num_nodes];
  memcpy(sold, init, num_nodes * sizeof(double));
  double* solf = new double[num_nodes];
  memcpy(solf, init, num_nodes * sizeof(double));
  double* r = new double[num_nodes];
  memcpy(r, reward, num_nodes * sizeof(double));

  bool ok = true;
  try {
    MCd->reverse_accumulated_reward_unbounded_time(time, sold, r, opt);
    MCf->reverse_accumulated_reward_unbounded_time(time, solf, r, opt);
  }
  catch (GraphLib::error e) {
    cerr << "  [" << case_name << "] graph error: " << e.getString() << "\n";
    ok = false;
  }
  catch (MCLib::error e) {
    cerr << "  [" << case_name << "] mclib error: " << e.getString() << "\n";
    ok = false;
  }

  if (ok) {
    double dd = diff_vector(expect, sold, num_nodes);
    double df = diff_vector(expect, solf, num_nodes);
    if (dd >= tol || df >= tol) {
      ok = false;
      cerr << "  FAIL " << case_name << " (tol " << tol << ")\n";
      show_vector("    MCd", sold, num_nodes);
      show_vector("    MCf", solf, num_nodes);
      show_vector("    exp", expect, num_nodes);
    }
  }

  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;
  delete[] r;
  return ok;
}

static bool quiet_library_matches_dense_ref(const char* case_name,
  const edge graph[], long num_nodes, const double init[], long time, long T,
  const double reward[], const double q_mask[])
{
  const double tol_d = 1e-5;
  const double tol_f = 2e-4;
  std::vector<double> P;
  build_P_from_edges(graph, num_nodes, P);
  std::vector<double> expect((size_t)num_nodes);
  ref_cond_accumulated_reward_unbounded_time(
    num_nodes, (int)time, (int)T, P, init, reward, q_mask, expect.data());

  Markov_chain* MCd = build_double(true, graph, num_nodes, false);
  Markov_chain* MCf = build_float(true, graph, num_nodes, false);
  Markov_chain::DTMC_transient_options opt;
  std::vector<double> r(reward, reward + num_nodes);
  std::vector<double> qv(q_mask, q_mask + num_nodes);
  std::vector<double> sold((size_t)num_nodes, 0.0);
  std::vector<double> solf((size_t)num_nodes, 0.0);
  for (long i = 0; i < num_nodes; i++) {
    sold[(size_t)i] = init[i];
    solf[(size_t)i] = init[i];
  }

  bool ok = true;
  try {
    MCd->conditional_accumulated_reward_unbounded_time(
      (int)time, (int)T, sold.data(), r.data(), qv.data(), opt);
    MCf->conditional_accumulated_reward_unbounded_time(
      (int)time, (int)T, solf.data(), r.data(), qv.data(), opt);
  }
  catch (GraphLib::error e) {
    cerr << "  [" << case_name << "] graph error: " << e.getString() << "\n";
    ok = false;
  }
  catch (MCLib::error e) {
    cerr << "  [" << case_name << "] mclib error: " << e.getString() << "\n";
    ok = false;
  }

  if (ok) {
    if (diff_vector(expect.data(), sold.data(), num_nodes) >= tol_d ||
        diff_vector(expect.data(), solf.data(), num_nodes) >= tol_f) {
      ok = false;
      cerr << "  FAIL dense-ref " << case_name << "\n";
      show_vector("    ref", expect.data(), num_nodes);
      show_vector("    MCd", sold.data(), num_nodes);
      show_vector("    MCf", solf.data(), num_nodes);
    }
  }
  delete MCd;
  delete MCf;
  return ok;
}

static bool quiet_reverse_library_matches_dense_ref(const char* case_name,
  const edge graph[], long num_nodes, const double init[], long time,
  const double reward[])
{
  const double tol_d = 1e-5;
  const double tol_f = 2e-4;
  std::vector<double> P;
  build_P_from_edges(graph, num_nodes, P);
  std::vector<double> expect((size_t)num_nodes);
  ref_reverse_accumulated_reward_unbounded_time(
    num_nodes, (int)time, P, init, reward, expect.data());

  Markov_chain* MCd = build_double(true, graph, num_nodes, false);
  Markov_chain* MCf = build_float(true, graph, num_nodes, false);
  Markov_chain::DTMC_transient_options opt;
  std::vector<double> r(reward, reward + num_nodes);
  std::vector<double> sold((size_t)num_nodes, 0.0);
  std::vector<double> solf((size_t)num_nodes, 0.0);
  for (long i = 0; i < num_nodes; i++) {
    sold[(size_t)i] = init[i];
    solf[(size_t)i] = init[i];
  }

  bool ok = true;
  try {
    MCd->reverse_accumulated_reward_unbounded_time(
      (int)time, sold.data(), r.data(), opt);
    MCf->reverse_accumulated_reward_unbounded_time(
      (int)time, solf.data(), r.data(), opt);
  }
  catch (GraphLib::error e) {
    cerr << "  [" << case_name << "] graph error: " << e.getString() << "\n";
    ok = false;
  }
  catch (MCLib::error e) {
    cerr << "  [" << case_name << "] mclib error: " << e.getString() << "\n";
    ok = false;
  }

  if (ok) {
    if (diff_vector(expect.data(), sold.data(), num_nodes) >= tol_d ||
        diff_vector(expect.data(), solf.data(), num_nodes) >= tol_f) {
      ok = false;
      cerr << "  FAIL dense-ref " << case_name << "\n";
      show_vector("    ref", expect.data(), num_nodes);
      show_vector("    MCd", sold.data(), num_nodes);
      show_vector("    MCf", solf.data(), num_nodes);
    }
  }
  delete MCd;
  delete MCf;
  return ok;
}

static bool run_backward_timed_random_vs_ref()
{
  const int trials = 50;
  int nfail = 0;
  uint64_t seed = 0xC0FFEEu;
  auto rnd32 = [&]() -> uint32_t {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return (uint32_t)(seed >> 32);
  };

  for (int trial = 0; trial < trials; trial++) {
    long n = 4 + (long)(rnd32() % 5);
    long goal = n - 1;
    std::vector<edge> edges;
    for (long i = 0; i < goal; i++) {
      double w = 1.0 + (double)(rnd32() % 5);
      edges.push_back({i, i + 1, w});
    }
    edges.push_back({goal, goal, 1.0});
    int nex = 2 + (int)(rnd32() % 5);
    for (int e = 0; e < nex; e++) {
      long i = (long)(rnd32() % (uint32_t)(n - 1));
      long j = (long)(rnd32() % (uint32_t)n);
      if (i == j) continue;
      double w = 1.0 + (double)(rnd32() % 5);
      edges.push_back({i, j, w});
    }
    edges.push_back({-1, -1, -1});

    std::vector<double> reward((size_t)n, 0.0);
    std::vector<double> qmask((size_t)n, 0.0);
    for (long i = 0; i < n; i++) {
      reward[(size_t)i] = (double)(rnd32() % 4);
    }
    qmask[(size_t)goal] = 1.0;

    long t = 2 + (long)(rnd32() % 4);
    long T = 2 + (long)(rnd32() % 4);

    char name[80];
    std::snprintf(name, sizeof(name), "rand_t%ld_T%ld_n%ld_%d", t, T, n, trial);

    if (!quiet_library_matches_dense_ref(name, edges.data(), n, qmask.data(), t, T,
          reward.data(), qmask.data())) {
      nfail++;
    }
  }

  if (nfail == 0) {
    cout << "backward_timed_cond: " << trials
         << " random DTMCs OK vs dense ref (MCd+MCf)\n";
    return true;
  }
  cerr << "backward_timed_cond: " << nfail << " random case(s) failed vs dense ref\n";
  return false;
}

static bool run_backward_timed_cond_regression()
{
  int nfail = 0;
  int nrun = 0;

  if (!quiet_cond_accumulated_reward_unbounded_time_case(
        "graph03_line_t4_T3", graph03, num_nodes3, p3, 4, 3, reward3, q3, pt3)) {
    nfail++;
  }
  nrun++;

  if (!quiet_cond_accumulated_reward_unbounded_time_case(
        "graph03_line_t4_T4", graph03, num_nodes3, p3, 4, 4, reward3, q3, pt3_T4)) {
    nfail++;
  }
  nrun++;

  if (!quiet_cond_accumulated_reward_unbounded_time_case(
        "graph04_fork_t4_T4", graph04, num_nodes4, p4, 4, 4, reward4, q4, pt4)) {
    nfail++;
  }
  nrun++;

  if (!quiet_library_matches_dense_ref(
        "dense_graph03_t4_T3", graph03, num_nodes3, p3, 4, 3, reward3, q3)) {
    nfail++;
  }
  if (!quiet_library_matches_dense_ref(
        "dense_graph03_t4_T4", graph03, num_nodes3, p3, 4, 4, reward3, q3)) {
    nfail++;
  }
  if (!quiet_library_matches_dense_ref(
        "dense_graph04_t4_T4", graph04, num_nodes4, p4, 4, 4, reward4, q4)) {
    nfail++;
  }

  if (nfail == 0) {
    cout << "backward_timed_cond: " << nrun << " gold + 3 dense-ref checks OK (MCd + MCf)\n";
    return true;
  }
  cerr << "backward_timed_cond: " << nfail << " case(s) failed\n";
  return false;
}

static bool run_backward_timed_acc_random_vs_ref()
{
  const int trials = 30;
  int nfail = 0;
  uint64_t seed = 0xA11CEu;
  auto rnd32 = [&]() -> uint32_t {
    seed = seed * 6364136223846793005ull + 1442695040888963407ull;
    return (uint32_t)(seed >> 32);
  };

  for (int trial = 0; trial < trials; trial++) {
    long n = 4 + (long)(rnd32() % 5);
    long goal = n - 1;
    std::vector<edge> edges;
    for (long i = 0; i < goal; i++) {
      double w = 1.0 + (double)(rnd32() % 5);
      edges.push_back({i, i + 1, w});
    }
    edges.push_back({goal, goal, 1.0});
    int nex = 2 + (int)(rnd32() % 5);
    for (int e = 0; e < nex; e++) {
      long i = (long)(rnd32() % (uint32_t)(n - 1));
      long j = (long)(rnd32() % (uint32_t)n);
      if (i == j) continue;
      double w = 1.0 + (double)(rnd32() % 5);
      edges.push_back({i, j, w});
    }
    edges.push_back({-1, -1, -1});

    std::vector<double> reward((size_t)n, 0.0);
    std::vector<double> goal_mask((size_t)n, 0.0);
    for (long i = 0; i < n; i++) {
      reward[(size_t)i] = (double)(rnd32() % 4);
    }
    goal_mask[(size_t)goal] = 1.0;

    long t = 2 + (long)(rnd32() % 4);
    char name[80];
    std::snprintf(name, sizeof(name), "rand_acc_t%ld_n%ld_%d", t, n, trial);

    if (!quiet_reverse_library_matches_dense_ref(
          name, edges.data(), n, goal_mask.data(), t, reward.data())) {
      nfail++;
    }
  }

  if (nfail == 0) {
    cout << "backward_timed_acc: " << trials
         << " random DTMCs OK vs dense ref (MCd+MCf)\n";
    return true;
  }
  cerr << "backward_timed_acc: " << nfail << " random case(s) failed vs dense ref\n";
  return false;
}

static bool run_backward_timed_acc_regression()
{
  int nfail = 0;
  int nrun = 0;

  if (!quiet_reverse_accumulated_reward_unbounded_time_case(
        "graph03_acc_t3", graph03, num_nodes3, q3, 3, reward3, pt3_acc_t3)) {
    nfail++;
  }
  nrun++;

  if (!quiet_reverse_accumulated_reward_unbounded_time_case(
        "graph03_acc_t4", graph03, num_nodes3, q3, 4, reward3, pt3_acc_t4)) {
    nfail++;
  }
  nrun++;

  if (!quiet_reverse_library_matches_dense_ref(
        "dense_graph03_acc_t3", graph03, num_nodes3, q3, 3, reward3)) {
    nfail++;
  }
  if (!quiet_reverse_library_matches_dense_ref(
        "dense_graph03_acc_t4", graph03, num_nodes3, q3, 4, reward3)) {
    nfail++;
  }
  if (!quiet_reverse_library_matches_dense_ref(
        "dense_graph04_acc_t4", graph04, num_nodes4, q4, 4, reward4)) {
    nfail++;
  }

  if (nfail == 0) {
    cout << "backward_timed_acc: " << nrun << " gold + 3 dense-ref checks OK (MCd + MCf)\n";
    return true;
  }
  cerr << "backward_timed_acc: " << nfail << " case(s) failed\n";
  return false;
}


// =======================================================================

int main(){
  if (!run_backward_timed_cond_regression())
    return 1;
  if (!run_backward_timed_random_vs_ref())
    return 1;
  if (!run_backward_timed_acc_regression())
    return 1;
  if (!run_backward_timed_acc_random_vs_ref())
    return 1;
  if (!cond_accumulated_reward_timestep_test(
        "graph03_cond_accumulated_reward_timestep_t4", graph03, num_nodes3, p3, 4, reward3,
        pt3_timestep_t4, q3))
    return 1;
  return 0;
}