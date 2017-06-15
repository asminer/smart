
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

const bool discrete1 = true;
const long num_nodes1 = 3;

const double init1[] = {0, 1, 0}; 
const long   time1_a = 1;
const double p1_a[] =  {0.5, 0, 0.5};
const long   time1_b = 2;
const double p1_b[] =  {3.0/8.0, 2.0/8.0, 3.0/8.0};
const long   time1_c = 3;
const double p1_c[] =  {13.0 / 32.0, 6.0/32.0, 13.0/32.0};


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

const bool discrete2 = true;
const long num_nodes2 = 5;

const double p2_0[] = {1.0/3.0, 1.0/3.0, 1.0/3.0, 0, 0}; 
const double p2_1[] = {0, 1.0/2.0, 1.0/6.0, 1.0/3.0, 0};
const double p2_2[] = {0, 1.0/2.0, 0, 1.0/6.0, 1.0/3.0};
const double p2_3[] = {0, 1.0/2.0, 1.0/3.0, 0, 1.0/6.0};

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

bool run_test(const char* name, const bool discrete, const edge graph[], 
  const long num_nodes, const double init[], const long time, const double pt[])
{
  cout << "Testing ";
  if (discrete) cout << "DTMC "; else cout << "CTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(discrete, graph, num_nodes);  
  Markov_chain* MCf = build_float(discrete, graph, num_nodes);  

  //
  // Set up options
  //
  Markov_chain::DTMC_transient_options opt;

  //
  // Solve Infinite time probabilities
  //
  double* sold = new double[num_nodes];
  memcpy(sold, init, num_nodes*sizeof(double));
  double* solf = new double[num_nodes];
  memcpy(solf, init, num_nodes*sizeof(double));

  try {
    if (discrete) {
      MCd->computeTransient(time, sold, opt);
      MCf->computeTransient(time, solf, opt);
    } else {
      double real_time = time;
      MCd->computeTransient(real_time, sold, opt);
      MCf->computeTransient(real_time, solf, opt);
    }
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


  // Cleanup
  delete MCd;
  delete MCf;
  delete[] sold;
  delete[] solf;

  return okd && okf;
}

// =======================================================================

int main()
{
  if (!run_test("Oz dtmc t1", discrete1, graph1, num_nodes1, init1, time1_a, p1_a)) {
    return 1;
  }
  if (!run_test("Oz dtmc t2", discrete1, graph1, num_nodes1, init1, time1_b, p1_b)) {
    return 1;
  }
  if (!run_test("Oz dtmc t3", discrete1, graph1, num_nodes1, init1, time1_c, p1_c)) {
    return 1;
  }

  if (!run_test("Periodic dtmc t1", discrete2, graph2, num_nodes2, p2_0, 1, p2_1)) {
    return 1;
  }
  if (!run_test("Periodic dtmc t2", discrete2, graph2, num_nodes2, p2_0, 2, p2_2)) {
    return 1;
  }
  if (!run_test("Periodic dtmc t3", discrete2, graph2, num_nodes2, p2_0, 3, p2_3)) {
    return 1;
  }
  if (!run_test("Periodic dtmc t4", discrete2, graph2, num_nodes2, p2_0, 4, p2_1)) {
    return 1;
  }
  if (!run_test("Periodic dtmc t5", discrete2, graph2, num_nodes2, p2_0, 5, p2_2)) {
    return 1;
  }
  if (!run_test("Periodic dtmc t6", discrete2, graph2, num_nodes2, p2_0, 6, p2_3)) {
    return 1;
  }
  return 0;
}


