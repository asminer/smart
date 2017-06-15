
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

const double init1[] = {1, 0, 0}; // doesn't matter
const double pinfinity1[] =  {2.0/5.0, 1.0/5.0, 2.0/5.0};

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

const bool discrete2 = true;
const long num_nodes2 = 6;

const double init2[] = {0.7, 0.3, 0, 0, 0, 0};
const double pinfinity2[] =  {0, 0, 0, 0, 4067.0/6480.0, 2413.0/6480.0};

// ==============================> Graph 3 <==============================

/*
  Simple two-state CTMC.
*/
const edge graph3[] = {
  {0, 1, 0.75},
  {1, 0, 1},
  // End 
  {-1, -1, -1}
};

const bool discrete3 = false;
const long num_nodes3 = 2;

const double init3[] = {1, 0};
const double pinfinity3[] =  { 1.0/1.75, 0.75/1.75};

// ==============================> Graph 4 <==============================

/*
  Land of oz with selector state at beginning and absorbing state.
*/
const edge graph4[] = {
  // Go to "Oz" with probability 5/8
  {0, 2, 5},  {0, 1, 3},
  // Land of oz portion
  {2, 2, 2},  {2, 3, 1},  {2, 4, 1},
  {3, 2, 1},              {3, 4, 1},
  {4, 2, 1},  {4, 3, 1},  {4, 4, 2},
  // End 
  {-1, -1, -1}
};

const bool discrete4 = true;
const long num_nodes4 = 5;

const double init4[] = {1, 0, 0, 0, 0}; 
const double pinfinity4[] =  {0,  3.0/8.0,   2.0/8.0, 1.0/8.0, 2.0/8.0};

const double init4a[] = {0, 1, 0, 0, 0}; 
const double pinfinity4a[] =  {0,  1,   0, 0, 0};

const double init4b[] = {0, 0, 1, 0, 0}; 
const double pinfinity4b[] =  {0,  0,   2.0/5.0, 1.0/5.0, 2.0/5.0};

// ==============================> Graph 4 <==============================

/*
  FMS N=1 CTMC.
*/
const edge graph5[] = {
	{0, 1, 0.333333}, {0, 2, 0.333333}, {0, 3, 0.333333},
	{1, 4, 0.2}, {1, 5, 0.05}, {1, 6, 0.5}, {1, 7, 0.5},
	{2, 6, 0.5}, {2, 8, 0.0666667}, {2, 9, 0.1}, {2, 10, 0.5},
	{3, 7, 0.5}, {3, 10, 0.5}, {3, 11, 0.5},
	{4, 0, 0.0166667}, {4, 12, 0.5}, {4, 13, 0.5},
	{5, 14, 0.5}, {5, 15, 0.5},
	{6, 12, 0.2}, {6, 14, 0.05}, {6, 16, 0.0666667}, {6, 17, 0.1}, {6, 18, 1},
	{7, 13, 0.2}, {7, 15, 0.05}, {7, 18, 1}, {7, 19, 0.5},
	{8, 16, 0.5}, {8, 20, 0.5},
	{9, 0, 0.0166667}, {9, 17, 0.5}, {9, 21, 0.5},
	{10, 18, 1}, {10, 20, 0.0666667}, {10, 21, 0.1},
	{11, 0, 0.0166667}, {11, 19, 0.5}, {11, 22, 0.5},
	{12, 2, 0.0166667}, {12, 23, 0.0666667}, {12, 24, 0.1}, {12, 25, 1},
	{13, 3, 0.0166667}, {13, 25, 1}, {13, 26, 0.5},
	{14, 27, 0.0666667}, {14, 28, 0.1}, {14, 29, 1},
	{15, 29, 1}, {15, 30, 0.5},
	{16, 23, 0.2}, {16, 27, 0.05}, {16, 31, 1},
	{17, 1, 0.0166667}, {17, 24, 0.2}, {17, 28, 0.05}, {17, 32, 1},
	{18, 25, 0.2}, {18, 29, 0.05}, {18, 31, 0.0666667}, {18, 32, 0.1},
	{19, 1, 0.0166667}, {19, 26, 0.2}, {19, 30, 0.05}, {19, 33, 1},
	{20, 31, 1}, {20, 34, 0.5},
	{21, 3, 0.0166667}, {21, 32, 1}, {21, 35, 0.5},
	{22, 2, 0.0166667}, {22, 33, 1}, {22, 34, 0.0666667}, {22, 35, 0.1},
	{23, 8, 0.0166667}, {23, 36, 1},
	{24, 4, 0.0166667}, {24, 9, 0.0166667}, {24, 37, 1},
	{25, 10, 0.0166667}, {25, 36, 0.0666667}, {25, 37, 0.1},
	{26, 4, 0.0166667}, {26, 11, 0.0166667}, {26, 38, 1},
	{27, 39, 0.5}, {27, 40, 0.5},
	{28, 5, 0.0166667}, {28, 41, 1},
	{29, 40, 0.0666667}, {29, 41, 0.1},
	{30, 5, 0.0166667}, {30, 42, 1},
	{31, 36, 0.2}, {31, 40, 0.05}, {31, 43, 0.5},
	{32, 7, 0.0166667}, {32, 37, 0.2}, {32, 41, 0.05}, {32, 44, 0.5},
	{33, 6, 0.0166667}, {33, 38, 0.2}, {33, 42, 0.05}, {33, 43, 0.0666667}, {33, 44, 0.1},
	{34, 8, 0.0166667}, {34, 43, 1},
	{35, 9, 0.0166667}, {35, 11, 0.0166667}, {35, 44, 1},
	{36, 20, 0.0166667}, {36, 45, 0.5},
	{37, 13, 0.0166667}, {37, 21, 0.0166667}, {37, 46, 0.5},
	{38, 12, 0.0166667}, {38, 22, 0.0166667}, {38, 45, 0.0666667}, {38, 46, 0.1},
	{39, 47, 1}, {39, 48, 1},
	{40, 48, 1}, {40, 49, 0.5},
	{41, 15, 0.0166667}, {41, 50, 0.5},
	{42, 14, 0.0166667}, {42, 49, 0.0666667}, {42, 50, 0.1},
	{43, 16, 0.0166667}, {43, 45, 0.2}, {43, 49, 0.05},
	{44, 17, 0.0166667}, {44, 19, 0.0166667}, {44, 46, 0.2}, {44, 50, 0.05},
	{45, 23, 0.0166667}, {45, 34, 0.0166667},
	{46, 24, 0.0166667}, {46, 26, 0.0166667}, {46, 35, 0.0166667},
	{47, 0, 0.0166667}, {47, 51, 1},
	{48, 51, 1}, {48, 52, 0.5},
	{49, 27, 0.0166667}, {49, 52, 1},
	{50, 28, 0.0166667}, {50, 30, 0.0166667},
	{51, 3, 0.0166667}, {51, 53, 0.5},
	{52, 39, 0.0166667}, {52, 53, 1},
	{53, 11, 0.0166667}, {53, 47, 0.0166667},
  {-1, -1, -1}
};

const bool discrete5 = false;
const long num_nodes5 = 54;

const double init5[] = {1, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,};
const double pinfinity5[] =  {
  8.80289789903105e-05,
  4.12047186504873e-05,
  4.79976446062514e-05,
  7.84355735594038e-05,
  7.225484795164e-05,
  6.13936132538748e-05,
  9.92147120577963e-05,
  3.77638741763752e-05,
  0.000270183469490893,
  6.72591275804355e-05,
  8.97068609000315e-05,
  0.0026029196409182,
  0.000273364089623626,
  7.36815611807847e-05,
  0.000194098284399786,
  6.3417893228892e-05,
  0.000564502572866163,
  0.000160890413809861,
  0.000544045047476306,
  0.00116888306133098,
  0.000273880030456362,
  7.29695069907923e-05,
  0.00132589032581936,
  0.00796316126298326,
  0.00185439422772452,
  0.00248647915743128,
  0.00205868734416382,
  8.25763448868046e-05,
  0.0017491623576754,
  0.00170831020617031,
  0.00181083325864178,
  0.00116620309964201,
  0.000375997062040278,
  0.00575716878464422,
  0.00805582481734845,
  0.00196041934393682,
  0.0161848384905256,
  0.00408420246353817,
  0.0160505981496407,
  4.15125967315215e-05,
  0.000142323827772277,
  0.00375250276146759,
  0.0114474055762991,
  0.0338352627808454,
  0.0096145908856935,
  0.477884431694763,
  0.111401363944213,
  0.00253928970917687,
  0.000122557621691344,
  0.00248467451526003,
  0.105051440056555,
  0.00515196216677018,
  0.00250421631347454,
  0.152405623328974
};


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
  const long num_nodes, const double init[], const double ss[])
{
  cout << "Testing ";
  if (discrete) cout << "DTMC "; else cout << "CTMC ";
  cout << name << "\n";

  Markov_chain* MCd = build_double(discrete, graph, num_nodes);  
  Markov_chain* MCf = build_float(discrete, graph, num_nodes);  

  //
  // Set up initial vector
  //
  LS_Vector p0;
  p0.size = num_nodes;
  p0.index = 0;
  p0.d_value = init;
  p0.f_value = 0;

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
  // Solve Infinite time probabilities
  //
  double* sold = new double[num_nodes];
  double* solf = new double[num_nodes];

  try {
    MCd->computeInfinityDistribution(p0, sold, opt, outd);
    MCf->computeInfinityDistribution(p0, solf, opt, outf);
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
  show_vector("Expected     vector", ss, num_nodes);

  double diff_d = diff_vector(ss, sold, num_nodes);
  double diff_f = diff_vector(ss, solf, num_nodes);

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
  if (!run_test("Oz dtmc", discrete1, graph1, num_nodes1, init1, pinfinity1)) {
    return 1;
  }
  if (!run_test("University dtmc", discrete2, graph2, num_nodes2, init2, pinfinity2)) {
    return 1;
  }
  if (!run_test("2-state ctmc", discrete3, graph3, num_nodes3, init3, pinfinity3)) {
    return 1;
  }
  if (!run_test("Reducible dtmc", discrete4, graph4, num_nodes4, init4, pinfinity4)) {
    return 1;
  }
  if (!run_test("Reducible dtmc", discrete4, graph4, num_nodes4, init4a, pinfinity4a)) {
    return 1;
  }
  if (!run_test("Reducible dtmc", discrete4, graph4, num_nodes4, init4b, pinfinity4b)) {
    return 1;
  }
  if (!run_test("FMS N=1 ctmc", discrete5, graph5, num_nodes5, init5, pinfinity5)) {
    return 1;
  }
  return 0;
}


