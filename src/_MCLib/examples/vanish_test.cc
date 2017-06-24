
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

bool discrete1 = true;
const long num_tan1 = 2;
const long num_van1 = 1;

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
  const edge answer[])
{
  cout << name << "\n";

  //
  // Build the vanishing chain
  //
  cout << "    Building\n";
  vanishing_chain VC(discrete, nt, nv);

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
  // set these?
  VC.eliminateVanishing(opt);
  // Build static eliminated graph
  static_graph elimgraph;
  VC.TT().exportToStatic(elimgraph, 0);

#ifdef VERBOSE
  show_graph S(nt);
  cout << "    Built eliminated graph:\n";
  elimgraph.traverse(S);
#endif

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
    cout << "OK!\n";
    return true;
  }

  cout << "Graphs are different!\n";
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

int main()
{
  if (!checkVanishing("Test 1", discrete1, num_tan1, num_van1, tt1, tv1, vv1, vt1, answer1)) {
    return 1;
  }
  return 0;
}

