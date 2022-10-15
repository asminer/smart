
/*
    Tests for vanishing chains
*/

#include <iostream>
#include "mclib.h"

#define VERBOSE

using namespace GraphLib;
using namespace std;
using namespace MCLib;

typedef struct { long from; long to; double rate; } edge;

// ==============================> Graph 1 <==============================

/*
    t0 -> v0 -> t1
*/

const edge tt1[] = {
  {1, 1, 1},
  {-1, -1, -1}
};

const edge tv1[] = {
  {0, 0, 1}, 
  {-1, -1, -1}
};

const edge vv1[] = {
  {-1, -1, -1}
};

const edge vt1[] = {
  {0, 1, 1},
  {-1, -1, -1}
};

const edge answer1[] = {
  {0, 1, 1},
  {1, 1, 1},
  {-1, -1, -1}
};

const double tinit1[] = { 1, 0 };
const double vinit1[] = { 0 };

const double init1[] = { 1, 0 };

bool discrete1 = true;
const long num_tan1 = 2;
const long num_van1 = 1;

// ==============================> Graph 2 <==============================

/*
           n
    t0 -> v0 -> t1
          |
          t2
*/

const edge tt2[] = {
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

const edge tv2[] = {
  {0, 0, 1},
  {-1, -1, -1}
};

const edge vv2[] = {
  {0, 0, 1},
  {-1, -1, -1}
};

const edge vt2[] = {
  {0, 1, 1},
  {0, 2, 1},
  {-1, -1, -1}
};

const edge answer2[] = {
  {0, 1, 0.5},
  {0, 2, 0.5},
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

bool discrete2 = true;
const long num_tan2 = 3;
const long num_van2 = 1;

const double tinit2[] = { 0, 0, 0};
const double vinit2[] = { 1 };
const double init2[] =  { 0, 0.5, 0.5};

// ==============================> Graph 3 <==============================

/*
    t0 -> v0 -> v1 -> t1
*/

const edge tt3[] = {
  {1, 1, 1},
  {-1, -1, -1}
};

const edge tv3[] = {
  {0, 0, 1},
  {-1, -1, -1}
};

const edge vv3[] = {
  {0, 1, 1},
  {-1, -1, -1}
};

const edge vt3[] = {
  {1, 1, 1},
  {-1, -1, -1}
};

const edge answer3[] = {
  {0, 1, 1},
  {1, 1, 1},
  {-1, -1, -1}
};

bool discrete3 = true;
const long num_tan3 = 2;
const long num_van3 = 2;

const double tinit3[] = { 3, 1 };
const double vinit3[] = { 1, 0 };
const double init3[] =  { 0.6, 0.4 };


// ==============================> Graph 4 <==============================

/*
    t0 -> v0 -> v1
    |     |     |
    v     v     v
    t1    v2 -> t2
  
*/

const edge tt4[] = {
  {0, 1, 0.5},
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

const edge tv4[] = {
  {0, 0, 0.5},
  {-1, -1, -1}
};

const edge vv4[] = {
  {0, 1, 0.5},
  {0, 2, 0.5},
  {-1, -1, -1}
};

const edge vt4[] = {
  {1, 2, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

const edge answer4[] = {
  {0, 1, 0.5},
  {0, 2, 0.5},
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

bool discrete4 = true;
const long num_tan4 = 3;
const long num_van4 = 3;


const double tinit4[] = { 1, 1, 1 };
const double vinit4[] = { 1, 0, 0 };
const double init4[] =  { 0.25, 0.25, 0.5 };


// ==============================> Graph 5 <==============================

/*
    t0 -> v0 -> v1
    |     ^     |
    v     |     v
    t1    v3 <- v2 -> t2
  
*/

const edge tt5[] = {
  {0, 1, 0.5},
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

const edge tv5[] = {
  {0, 0, 0.5},
  {-1, -1, -1}
};

const edge vv5[] = {
  {0, 1, 1},
  {1, 2, 1},
  {2, 3, 0.5},
  {3, 0, 1},
  {-1, -1, -1}
};

const edge vt5[] = {
  {2, 2, 0.5},
  {-1, -1, -1}
};

const edge answer5[] = {
  {0, 1, 0.5},
  {0, 2, 0.5},
  {1, 1, 1},
  {2, 2, 1},
  {-1, -1, -1}
};

bool discrete5 = true;
const long num_tan5 = 3;
const long num_van5 = 4;


const double tinit5[] = { 0, 0, 0 };
const double vinit5[] = { 0, 0, 1, 0 };
const double init5[] =  { 0, 0, 1 };

// ==============================> Graph 6 <==============================

/*
    t0 -> v0 -> v1  -> t2
    |     ^     |  /
    v     |     v /
    t1 -> v3 <- v2 --> t3
  
*/

const edge tt6[] = {
  {0, 1, 0.5},
  {2, 2, 1},
  {3, 3, 1},
  {-1, -1, -1}
};

const edge tv6[] = {
  {0, 0, 0.5},
  {1, 3, 1},
  {-1, -1, -1}
};

const edge vv6[] = {
  {0, 1, 1},
  {1, 2, 1},
  {2, 3, 1.0},  // 1/6
  {3, 0, 0.5},
  {3, 3, 0.5},
  {-1, -1, -1}
};

const edge vt6[] = {
  {2, 2, 3.0},  // 3/6
  {2, 3, 2.0},  // 2/6
  {-1, -1, -1}
};

const edge answer6[] = {
  {0, 1, 0.5},
  {0, 2, 0.3},
  {0, 3, 0.2},
  {1, 2, 0.6},
  {1, 3, 0.4},
  {2, 2, 1},
  {3, 3, 1},
  {-1, -1, -1}
};

bool discrete6 = true;
const long num_tan6 = 4;
const long num_van6 = 4;

const double tinit6[] = { 1, 0, 0, 1 };
const double vinit6[] = { 2, 0, 0, 0 };
const double init6[] =  { 0.25, 0, 0.6/2, 0.25 + 0.4/2 };


// ==============================> Graph 7 <==============================

/*
            
    t0 -> v0 -> t1
     \    |
      \   v
       -> t2
*/

const edge tt7[] = {
  {0, 2, 1.5},
  {-1, -1, -1}
};

const edge tv7[] = {
  {0, 0, 3},
  {-1, -1, -1}
};

const edge vv7[] = {
  {-1, -1, -1}
};

const edge vt7[] = {
  {0, 1, 2.0},
  {0, 2, 1.0},
  {-1, -1, -1}
};

const edge answer7[] = {
  {0, 1, 2.0},
  {0, 2, 2.5},
  {-1, -1, -1}
};

bool discrete7 = false;
const long num_tan7 = 3;
const long num_van7 = 1;

const double tinit7[] = { 1, 0, 0};
const double vinit7[] = { 0 };
const double init7[] =  { 1, 0, 0};

// ==============================> Graph 8 <==============================

const edge tt8[] = {
  {-1, -1, -1}
};

const edge tv8[] = {
  {0, 3, 12.0},
  {-1, -1, -1}
};

const edge vv8[] = {
  {0, 1, 2.0},  {0, 2, 2.0},
  {1, 0, 1.0},
  {2, 0, 3.0},
  {3, 4, 2.0},
  {4, 5, 1.0},
  {5, 4, 1.0},  {5, 6, 1.0},
  {6, 5, 1.0},  {6, 7, 1.0},
  {7, 8, 5.0},
  {8, 8, 10.0},
  {-1, -1, -1}
};

const edge vt8[] = {
  {1, 0, 1.0},
  {2, 1, 3.0},
  {3, 1, 1.0},
  {7, 2, 3.0},
  {8, 3, 1.0},
  {-1, -1, -1}
};

const edge answer8[] = {
  {0, 1, 4.0},
  {0, 2, 3.0},
  {0, 3, 5.0},
  {-1, -1, -1}
};

bool discrete8 = false;
const long num_tan8 = 4;
const long num_van8 = 9;

const double tinit8[] = { 0, 0, 0, 0};
const double vinit8[] = { 1, 0, 0, 0, 0, 0, 0, 0, 0}; 
const double init8[] =  { 0.5, 0.5, 0, 0};

// ==============================> Graph 9 <==============================

const edge tt9[] = {
  {-1, -1, -1}
};

const edge tv9[] = {
  {0, 0, 4.2},
  {-1, -1, -1}
};

const edge vv9[] = {
  {-1, -1, -1}
};

const edge vt9[] = {
  {-1, -1, -1}
};

bool discrete9 = false;
const long num_tan9 = 1;
const long num_van9 = 1;

// ==============================> Graph 10 <=============================

const edge tt10[] = {
  {0, 0, 0.5},
  {1, 1, 1.0},
  {-1, -1, -1}
};

const edge tv10[] = {
  {0, 0, 0.5},
  {-1, -1, -1}
};

const edge vv10[] = {
  {0, 1, 1},
  {1, 0, 0.25}, {1, 2, 0.5},
  {2, 1, 1}, {2, 3, 2},
  {3, 4, 1},
  {4, 3, 1},
  {-1, -1, -1}
};

const edge vt10[] = {
  {1, 1, 0.25},
  {-1, -1, -1}
};

bool discrete10 = true;
const long num_tan10 = 2;
const long num_van10 = 5;

// =======================================================================

class show_graph : public GraphLib::BF_graph_traversal {
  public:
    show_graph(long _num_nodes) {
      num_nodes = _num_nodes;
      current = 0;
    }

    virtual ~show_graph() { 
    }

    void reset() {
      current = 0;
    }

    virtual bool hasNodesToExplore() {
      return current < num_nodes;
    }

    virtual long getNextToExplore() {
      return current++;
    }

    virtual bool visit(long src, long dest, const void* wt) {
      cout << "\t" << src << " : " << dest << " : ";
      const double* rate = (const double*) wt;
      cout << *rate << "\n";
      return false;
    }

  private:
    long num_nodes;
    long current;
};

// =======================================================================

inline double absdiff(double targ, double val)
{
  double d = targ - val;
  if (d<0) d*=-1;
  return d;
}

inline double reldiff(double targ, double val)
{
  double d = absdiff(targ, val);
  if (targ) d /= targ;
  return d;
}

// =======================================================================

void showVector(const char* name, const LS_Vector &V)
{
  cout << name << ": ";
  if (V.index) {
    // Sparse
    cout << "(";
    for (long z=0; z<V.size; z++) {
      if (z) cout << ", ";
      cout << V.index[z] << ":";
      if (V.d_value)  cout << V.d_value[z];
      else            cout << V.f_value[z];
    }
    cout << ")\n";
  } else {
    // Truncated full
    cout << "[";
    for (long i=0; i<V.size; i++) {
      if (i) cout << ", ";
      if (V.d_value)  cout << V.d_value[i];
      else            cout << V.f_value[i];
    }
    cout << "]\n";
  }
}

// =======================================================================

void showVector(const char* name, const double v[], long size)
{
  cout << name << ": [" << v[0];
  for (long i=1; i<size; i++) {
    cout << ", " << v[i];
  }
  cout << "]\n";
}

// =======================================================================

bool vectorsEqual(const LS_Vector &V, const double v[], long size)
{
  const double precision = 1e-6;

  if (V.index) {
    // sparse
    long z=0;
    long nexti = (V.size>z) ? V.index[z] : -1;
    for (long i=0; i<size; i++) {
      if (i==nexti) {
        double Vv = V.d_value ? V.d_value[z] : V.f_value[z];
        if (reldiff(Vv, v[i]) > precision) return false;
        z++;
        nexti = (V.size>z) ? V.index[z] : -1;
      } else {
        if (reldiff(0, v[i]) > precision) return false;
      }
    }
    return true;
  }

  for (long i=0; i<V.size; i++) {
    double Vv = V.d_value ? V.d_value[i] : V.f_value[i];
    if (reldiff(Vv, v[i]) > precision) return false;
  }
  for (long i=V.size; i<size; i++) {
    if (reldiff(0, v[i]) > precision) return false;
  }
  return true;
}

// =======================================================================

bool graphsEqual(const static_graph &G, const static_graph &H)
{
  const double precision = 1e-6;

  if (G.getNumNodes() != H.getNumNodes()) return false;
  if (G.getNumEdges() != H.getNumEdges()) return false;
  if (G.isByRows() != H.isByRows()) return false;

  for (long i=0; i<=G.getNumNodes(); i++) {
    if (G.RowPointer(i) != H.RowPointer(i)) return false;
  }
  for (long i=0; i<G.getNumEdges(); i++) {
    if (G.ColumnIndex(i) != H.ColumnIndex(i)) return false;
    double g = *((const double*) G.Label(i));
    double h = *((const double*) H.Label(i));
    if (reldiff(g, h) > precision) return false;
  }
  return true;
}

// =======================================================================

bool checkVanishing(const char* name, bool discrete, long nt, long nv, 
  const edge tt[], const edge tv[], const edge vv[], const edge vt[], 
  const edge answer[], 
  const double tinit[], const double vinit[], const double init[])
{
  cout << name << "\n";

  //
  // Build the vanishing chain
  //
  cout << "    Building\n";
  vanishing_chain VC(discrete, nt, nv);

  //
  // Add edges
  //

  for (long i=0; tt[i].from >= 0; i++) {
    VC.addTTedge(tt[i].from, tt[i].to, tt[i].rate);
  }
  for (long i=0; tv[i].from >= 0; i++) {
    VC.addTVedge(tv[i].from, tv[i].to, tv[i].rate);
  }
  for (long i=0; vv[i].from >= 0; i++) {
    VC.addVVedge(vv[i].from, vv[i].to, vv[i].rate);
  }
  for (long i=0; vt[i].from >= 0; i++) {
    VC.addVTedge(vt[i].from, vt[i].to, vt[i].rate);
  }

  //
  // Initial weights
  //

  for (long i=0; i<nt; i++) if (tinit[i]) {
    VC.addInitialTangible(i, tinit[i]);
  }
  for (long i=0; i<nv; i++) if (vinit[i]) {
    VC.addInitialVanishing(i, vinit[i]);
  }


  //
  // Eliminate the vanishing states
  //
  cout << "    Eliminating\n";
  LS_Options opt;
  opt.method = LS_Gauss_Seidel;
  opt.precision = 1e-8;
  // set these?
  VC.eliminateVanishing(opt);
  // Build static eliminated graph
  static_graph elimgraph;
  VC.TT().exportToStatic(elimgraph, 0);
  // Build initial distribution as floats and doubles
  LS_Vector elim_init_d;
  LS_Vector elim_init_f;
  VC.buildInitialVector(false, elim_init_d);
  VC.buildInitialVector(true,  elim_init_f);

#ifdef VERBOSE
  show_graph S(nt);
  cout << "    Built eliminated graph:\n";
  elimgraph.traverse(S);
  showVector("    Initial distro (doubles) ", elim_init_d);
  showVector("    Initial distro (floats ) ", elim_init_f);
  showVector("    Initial distro (expected)", init, nt);
#endif

  //
  // Compare distributions with answer
  //
  bool okd = vectorsEqual(elim_init_d, init, nt);
  bool okf = vectorsEqual(elim_init_f, init, nt);
  if (!okd || !okf) {
    cout << "    Initial distributions are different!\n";
#ifndef VERBOSE
    showVector("    Initial distro (doubles) ", elim_init_d);
    showVector("    Initial distro (floats ) ", elim_init_f);
    showVector("    Initial distro (expected)", init, nt);
#endif
  } else {
    cout << "    Initial distributions match\n";
  }
  delete[] elim_init_d.index;
  delete[] elim_init_d.d_value;
  delete[] elim_init_f.index;
  delete[] elim_init_f.f_value;
  if (!okd || !okf) return false;

  //
  // Build the answer graph
  //
  dynamic_summable<double> ansdynamic(discrete, true);
  ansdynamic.addNodes(nt);
  for (long i=0; answer[i].from >= 0; i++) {
    ansdynamic.addEdge(answer[i].from, answer[i].to, answer[i].rate);
  }
  static_graph ansgraph;
  ansdynamic.exportToStatic(ansgraph, 0);
#ifdef VERBOSE
  S.reset();
  cout << "    Expected answer graph:\n";
  ansgraph.traverse(S);
#endif

  //
  // Compare with answer
  //
  cout << "    Comparing\n";
  if (graphsEqual(elimgraph, ansgraph)) {
    cout << "    Graphs match\n";
    cout << "OK!\n";
    return true;
  }

  cout << "    Graphs are different!\n";
#ifndef VERBOSE
  show_graph S(nt);
  cout << "    Built eliminated graph:\n";
  elimgraph.traverse(S);
  S.reset();
  cout << "    Expected answer graph:\n";
  ansgraph.traverse(S);
#endif

  return false;
}


// =======================================================================

bool detectVanLoop(const char* name, bool discrete, long nt, long nv, 
  const edge tt[], const edge tv[], const edge vv[], const edge vt[])
{
  cout << name << "\n";

  //
  // Build the vanishing chain
  //
  cout << "    Building\n";
  vanishing_chain VC(discrete, nt, nv);

  //
  // Add edges
  //

  for (long i=0; tt[i].from >= 0; i++) {
    VC.addTTedge(tt[i].from, tt[i].to, tt[i].rate);
  }
  for (long i=0; tv[i].from >= 0; i++) {
    VC.addTVedge(tv[i].from, tv[i].to, tv[i].rate);
  }
  for (long i=0; vv[i].from >= 0; i++) {
    VC.addVVedge(vv[i].from, vv[i].to, vv[i].rate);
  }
  for (long i=0; vt[i].from >= 0; i++) {
    VC.addVTedge(vt[i].from, vt[i].to, vt[i].rate);
  }

  //
  // Eliminate the vanishing states
  //
  cout << "    Eliminating\n";
  LS_Options opt;

  try {
    VC.eliminateVanishing(opt);
    cout << "    Failed to detect loop\n";
    return false;
  }
  catch (MCLib::error e) {
    if (MCLib::error::Loop_Of_Vanishing == e.getCode()) {
      cout << "    Detected loop\n";
      cout << "OK!\n";
      return true;
    }

    cout << "Caught MCLib error " << e.getString() << "\n";
    return false;
  }

}


// =======================================================================

int main()
{
  cout.precision(8);

  //
  // Discrete tests
  //
  if (!checkVanishing("Test 1", discrete1, num_tan1, num_van1, tt1, tv1, vv1, 
      vt1, answer1, tinit1, vinit1, init1)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 2", discrete2, num_tan2, num_van2, tt2, tv2, vv2, 
      vt2, answer2, tinit2, vinit2, init2)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 3", discrete3, num_tan3, num_van3, tt3, tv3, vv3, 
      vt3, answer3, tinit3, vinit3, init3)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 4", discrete4, num_tan4, num_van4, tt4, tv4, vv4, 
      vt4, answer4, tinit4, vinit4, init4)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 5", discrete5, num_tan5, num_van5, tt5, tv5, vv5, 
      vt5, answer5, tinit5, vinit5, init5)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 6", discrete6, num_tan6, num_van6, tt6, tv6, vv6, 
      vt6, answer6, tinit6, vinit6, init6)) 
  {
    return 1;
  }

  //
  // Continuous tests
  //
  if (!checkVanishing("Test 7", discrete7, num_tan7, num_van7, tt7, tv7, vv7, 
      vt7, answer7, tinit7, vinit7, init7)) 
  {
    return 1;
  }
  if (!checkVanishing("Test 8", discrete8, num_tan8, num_van8, tt8, tv8, vv8, 
      vt8, answer8, tinit8, vinit8, init8)) 
  {
    return 1;
  }

  //
  // Graphs that cannot escape (some) vanishing states
  //
  if (!detectVanLoop("Test 9", discrete9, num_tan9, num_van9, tt9, tv9, vv9, vt9))
  {
    return 1;
  }
  if (!detectVanLoop("Test 10", discrete10, num_tan10, num_van10, tt10, tv10, vv10, vt10))
  {
    return 1;
  }

  return 0;
}

