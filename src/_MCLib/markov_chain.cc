
/**
  Implementation of Markov_chain class.
*/

#include "mclib.h"

// #define DEBUG_CONSTRUCTOR
// #define DEBUG_PERIOD

#ifdef DEBUG_PERIOD
  #define USES_IOSTREAM
#endif

#ifdef DEBUG_CONSTRUCTOR
  #define USES_IOSTREAM
#endif

#ifdef USES_IOSTREAM
#include <iostream>

using namespace std;

template <class TYPE>
void showArray(const TYPE* A, long size)
{
  cout << "[";
  cout << A[0];
  for (long i=1; i<size; i++) cout << ", " << A[i];
  cout << "]\n";
}

void showGraph(GraphLib::static_graph &G)
{
  cout << "    #nodes: " << G.getNumNodes() << "\n";
  cout << "    #edges: " << G.getNumEdges() << "\n";
  cout << "    row pointer: ";
  showArray(G.RowPointer(), G.getNumNodes()+1);
  cout << "    column index: ";
  showArray(G.ColumnIndex(), G.getNumEdges());
  if (G.EdgeBytes() == sizeof(double)) {
    cout << "    value (doubles): ";
    showArray( (const double*) G.Labels(), G.getNumEdges());
  }
  if (G.EdgeBytes() == sizeof(float)) {
    cout << "    value (floats): ";
    showArray( (const float*) G.Labels(), G.getNumEdges());
  }
  if (G.EdgeBytes() == 0) {
    cout << "    no values\n";
  }
}

#endif  // #ifdef USES_IOSTREAM

template <class TYPE>
inline void zeroArray(TYPE* A, long size)
{
  for (long i=0; i<size; i++) A[i] = 0;
}

void float_graph_rowsums(const GraphLib::static_graph &G, double* rowsums)
{
  for (long s=0; s<G.getNumNodes(); s++) {
    double sum = 0;
    for (long e=G.RowPointer(s); e<G.RowPointer(s+1); e++) {
      sum += *((float*) G.Label(e));
    } // for e
    rowsums[s] += sum;
  } // for s
}
  
void double_graph_rowsums(const GraphLib::static_graph &G, double* rowsums)
{
  for (long s=0; s<G.getNumNodes(); s++) {
    double sum = 0;
    for (long e=G.RowPointer(s); e<G.RowPointer(s+1); e++) {
      sum += *((double*) G.Label(e));
    } // for e
    rowsums[s] += sum;
  } // for s
}

template <class REAL>
inline void graphToMatrix(const GraphLib::static_graph &G, LS_CCS_Matrix<REAL> &M)
{
  DCASSERT(G.EdgeBytes() == sizeof(REAL));

  M.start = 0;
  M.stop = G.getNumNodes();
  M.size = G.getNumNodes();

  M.val = (const REAL*) G.Labels();
  M.row_ind = G.ColumnIndex();
  M.col_ptr = G.RowPointer();

  M.one_over_diag = 0;  // We'll do this by hand later
}
  
template <class REAL>
inline void graphToMatrix(const GraphLib::static_graph &G, LS_CRS_Matrix<REAL> &M)
{
  DCASSERT(G.EdgeBytes() == sizeof(REAL));

  M.start = 0;
  M.stop = G.getNumNodes();
  M.size = G.getNumNodes();

  M.val = (const REAL*) G.Labels();
  M.col_ind = G.ColumnIndex();
  M.row_ptr = G.RowPointer();

  M.one_over_diag = 0;  // We'll do this by hand later
}
  
// ======================================================================
// |                                                                    |
// |                        Markov_chain methods                        |
// |                                                                    |
// ======================================================================

MCLib::Markov_chain::Markov_chain(bool discrete, 
  GraphLib::dynamic_summable<double> &G, 
  const GraphLib::static_classifier &TSCCinfo,
  GraphLib::timer_hook *sw) : stateClass(TSCCinfo)
{
#ifdef DEBUG_CONSTRUCTOR
  cout << "Inside Markov_chain (double) constructor.\n";
#endif

  is_discrete = discrete;
  double_graphs = true;

  //
  // Allocate row sum array
  //
  double* rowsums = new double[G.getNumNodes()];

  //
  // If we're a DTMC, normalize rows
  //
  if (discrete) {
    zeroArray(rowsums, G.getNumNodes());
    G.addRowSums(rowsums);
    G.divideRows(rowsums);
  }

  //
  // Common stuff (to double/float graphs) here
  //
  finish_construction(rowsums, G, sw);

#ifdef DEBUG_CONSTRUCTOR
  cout << "Exiting Markov_chain (double) constructor.\n";
#endif
}

// ******************************************************************

MCLib::Markov_chain::Markov_chain(bool discrete, 
  GraphLib::dynamic_summable<float> &G, 
  const GraphLib::static_classifier &TSCCinfo,
  GraphLib::timer_hook *sw) : stateClass(TSCCinfo)
{
#ifdef DEBUG_CONSTRUCTOR
  cout << "Inside Markov_chain (float) constructor.\n";
#endif

  is_discrete = discrete;
  double_graphs = false;

  //
  // Allocate row sum array
  //
  double* rowsums = new double[G.getNumNodes()];

  //
  // If we're a DTMC, normalize rows
  //
  if (discrete) {
    zeroArray(rowsums, G.getNumNodes());
    G.addRowSums(rowsums);
    G.divideRows(rowsums);
  }

  //
  // Common stuff (to double/float graphs) here
  //
  finish_construction(rowsums, G, sw);

#ifdef DEBUG_CONSTRUCTOR
  cout << "Exiting Markov_chain (float) constructor.\n";
#endif
}

// ******************************************************************

MCLib::Markov_chain::~Markov_chain()
{
  delete[] one_over_rowsums_d;
  delete[] one_over_rowsums_f;
}

// ******************************************************************

void MCLib::Markov_chain::finish_construction(double* rowsums, 
  GraphLib::dynamic_graph &G, GraphLib::timer_hook *sw)
{
  //
  // Build subgraphs and remove any self loops
  //

  if (G.isByRows()) {
    G.splitAndExport(stateClass, false, G_byrows_diag, G_byrows_off, sw);
    G_bycols_diag.transposeFrom(G_byrows_diag);
    G_bycols_off.transposeFrom(G_byrows_off);
  } else {
    G.splitAndExport(stateClass, false, G_bycols_diag, G_bycols_off, sw);
    G_byrows_diag.transposeFrom(G_bycols_diag);
    G_byrows_off.transposeFrom(G_bycols_off);
  }

  //
  // Determine rowsums from split graphs
  //
  zeroArray(rowsums, G.getNumNodes());
  if (double_graphs) {
    double_graph_rowsums(G_byrows_diag, rowsums);
    double_graph_rowsums(G_byrows_off, rowsums);
  } else {
    float_graph_rowsums(G_byrows_diag, rowsums);
    float_graph_rowsums(G_byrows_off, rowsums);
  }


  //
  // Uniformization constant
  //
  if (isDiscrete()) {
    // 
    // DTMCs: set uniformization constant to 1
    // (TBD - should we do the same as CTMCs?)
    //
    uniformization_const = 1;
  } else {
    //
    // CTMCs: determine uniformization constant from rowsums
    //
    uniformization_const = rowsums[0];
    for (long i=1; i<G.getNumNodes(); i++) {
      if (rowsums[i] > uniformization_const) {
        uniformization_const = rowsums[i];
      }
    }
  }

  //
  // Sanity checks
  //
  DCASSERT(G_byrows_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(G_byrows_off.getNumNodes() == G.getNumNodes());
  DCASSERT(G_bycols_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(G_bycols_off.getNumNodes() == G.getNumNodes());

#ifdef DEBUG_CONSTRUCTOR
  cout << "  G_byrows_diag:\n";
  showGraph(G_byrows_diag);
  cout << "  G_byrows_off:\n";
  showGraph(G_byrows_off);
  cout << "  G_bycols_diag:\n";
  showGraph(G_bycols_diag);
  cout << "  G_bycols_off:\n";
  showGraph(G_bycols_off);
  cout << "  rowsums:\n";
  cout << "    ";
  showArray(rowsums, getNumStates());
#endif

  //
  // Build one_over_rowsums_d array, used for linear solvers
  //
  if (double_graphs) {
    one_over_rowsums_f = 0;
    one_over_rowsums_d = new double[getNumStates()];
    for (long i=0; i<G_byrows_diag.getNumNodes(); i++) {
      one_over_rowsums_d[i] = rowsums[i] ? (1.0/rowsums[i]) : 0.0;
    }
  } else {
    one_over_rowsums_d = 0;
    one_over_rowsums_f = new float[getNumStates()];
    for (long i=0; i<G_byrows_diag.getNumNodes(); i++) {
      one_over_rowsums_f[i] = rowsums[i] ? (1.0/rowsums[i]) : 0.0;
    }
  }

  // TBD - for CSL, we will need one_over_colsums arrays, right?  :(

  //
  // Cleanup rowsums
  //
  delete[] rowsums;

#ifdef DEBUG_CONSTRUCTOR
  if (one_over_rowsums_f) {
    cout << "  one_over_rowsums_f:\n";
    cout << "    ";
    showArray(one_over_rowsums_f, getNumStates());
  }
  if (one_over_rowsums_d) {
    cout << "  one_over_rowsums_d:\n";
    cout << "    ";
    showArray(one_over_rowsums_d, getNumStates());
  }
#endif
}

// ******************************************************************

size_t MCLib::Markov_chain::getMemTotal() const
{
  size_t mem = 
    stateClass.getMemTotal() +
    G_byrows_diag.getMemTotal() +
    G_byrows_off.getMemTotal() +
    G_bycols_diag.getMemTotal() +
    G_bycols_off.getMemTotal(); 

  if (one_over_rowsums_d) mem += getNumStates() * sizeof(double);
  if (one_over_rowsums_f) mem += getNumStates() * sizeof(float);

  return mem;
}

// ******************************************************************

bool MCLib::Markov_chain::traverseOutgoing(GraphLib::BF_graph_traversal &t)
const
{
  while (t.hasNodesToExplore()) {
      long s = t.getNextToExplore();

      if (s<0 || s>=G_byrows_diag.getNumNodes()) {
        throw GraphLib::error(GraphLib::error::Bad_Index);
      }

      // Explore diagonal edges from s
      for (long z=G_byrows_diag.RowPointer(s); 
            z<G_byrows_diag.RowPointer(s+1); z++) 
      {
        if (t.visit(s, G_byrows_diag.ColumnIndex(z), G_byrows_diag.Label(z))) {
          return true;
        }
      }

      // Explore off-diagonal edges from s
      for (long z=G_byrows_off.RowPointer(s); 
            z<G_byrows_off.RowPointer(s+1); z++) 
      {
        if (t.visit(s, G_byrows_off.ColumnIndex(z), G_byrows_off.Label(z))) {
          return true;
        }
      }

  } // while
  return false;
}

// ******************************************************************

bool MCLib::Markov_chain::traverseIncoming(GraphLib::BF_graph_traversal &t)
const
{
  while (t.hasNodesToExplore()) {
      long s = t.getNextToExplore();

      if (s<0 || s>=G_bycols_diag.getNumNodes()) {
        throw GraphLib::error(GraphLib::error::Bad_Index);
      }

      // Explore diagonal edges from s
      for (long z=G_bycols_diag.RowPointer(s); 
            z<G_bycols_diag.RowPointer(s+1); z++) 
      {
        if (t.visit(s, G_bycols_diag.ColumnIndex(z), G_bycols_diag.Label(z))) {
          return true;
        }
      }

      // Explore off-diagonal edges from s
      for (long z=G_bycols_off.RowPointer(s); 
            z<G_bycols_off.RowPointer(s+1); z++) 
      {
        if (t.visit(s, G_bycols_off.ColumnIndex(z), G_bycols_off.Label(z))) {
          return true;
        }
      }

  } // while
  return false;
}

// ******************************************************************

long MCLib::Markov_chain::computePeriodOfClass(long c) const
{
  /*
    Algorithm adapted from discussions in chapter 7 of Stewart.  Idea:
      (1) Select a starting state (for us, the first one)
      (2) For each state, determine its distance from the start.
      (3) If there is an edge from i to j, with
              distance(i) >= distance(j),
          then add  
              distance(i) - distance(j) + 1
          to a set of integers.
      (4) The GCD of the resulting set gives the period.  
          Since GCD is associative, we don't need to construct
          the set of integers, just remember the GCD "so far".

    The following helper class magically does most of this.
  */

  // ======================================================================
  class BF_period : public GraphLib::BF_with_queue {
      public:
          // Constructor.
          //    first: index of first state in the recurrent class
          //    size:  total number of states in the recurrent class
          // Dists: initialize to -1 everywhere
          BF_period(long first, long size) 
          : BF_with_queue(size)
          {
              raw_dist = new long[size];
              raw_dist[0] = 0;
              for (long i=1; i<size; i++) {
                raw_dist[i] = -1;
              }
              distance = raw_dist-first;
              // note that distance[first] is the same as raw_dist[0]
              queuePush(first);
              first_pass = true;
#ifdef DEBUG_PERIOD
              cout << "Computing period, first pass.\n";
#endif
          }
          virtual ~BF_period() {
              delete[] raw_dist;
          }
          virtual bool visit(long src, long dest, const void* wt) {
              if (first_pass) {
                  if (distance[dest]>=0) return false;
                  distance[dest] = distance[src]+1;
                  queuePush(dest);
#ifdef DEBUG_PERIOD
                  cout << "  edge " << src << " -> " << dest << " updates distance\n";
                  cout << "      of " << dest << " to " << distance[dest] << "\n";
#endif
              } else {
                  long d = distance[src] - distance[dest] + 1;
                  if (d) {
                      if (period) period = GCD(period, d);
                      else        period = d;
#ifdef DEBUG_PERIOD
                  cout << "  edge " << src << " -> " << dest << " has d=" ;
                  cout << distance[src] << "-" << distance[dest] << "+1=" << d << "\n";
                  cout << "      updating period to " << period << "\n";
#endif
                  }
              }
              return false;
          }
          inline void secondPass(long first, long size) {
#ifdef DEBUG_PERIOD
              cout << "First pass completed, distances:\n  [" << distance[first];
              for (long i=1; i<size; i++) {
                cout << ", " << distance[first+i];
              }
              cout << "]\n";
#endif
              queueReset();
              first_pass = false;
              for (long i=0; i<size; i++) {
                queuePush(first+i);
              }
              period = 0;
          }
          inline long getPeriod() const { 
              return period;
          }
      private:
        long* raw_dist;
        long* distance;
        long period;
        bool first_pass;
  };  // class BF_distances
  // ======================================================================

  //
  // Special case: absorbing states
  //
  if (1==c) return 1;

  //
  // Check for invalid c's
  //
  if (c<1)  return 0;
  if (c>= stateClass.getNumClasses()) return 0;

  //
  // Easy case - if we have a self loop, then the period must be 1
  //
  if (one_over_rowsums_d) {
    for (long i=stateClass.firstNodeOfClass(c); i<=stateClass.lastNodeOfClass(c); i++) {
      if (one_over_rowsums_d[i] != 1.0) return 1;
    }
  }
  if (one_over_rowsums_f) {
    for (long i=stateClass.firstNodeOfClass(c); i<=stateClass.lastNodeOfClass(c); i++) {
      if (one_over_rowsums_f[i] != 1.0) return 1;
    }
  }

  // First pass: forward reachability search from
  // start state, track state distances.
  BF_period T(stateClass.firstNodeOfClass(c), stateClass.sizeOfClass(c));
  G_byrows_diag.traverse(T); 

  // Second pass: check all edges, and update period
  // based on (3) above.
  T.secondPass(stateClass.firstNodeOfClass(c), stateClass.sizeOfClass(c));
  G_byrows_diag.traverse(T); 

  return T.getPeriod();
}

// ******************************************************************

// STUFF WILL GO HERE


// ******************************************************************

void MCLib::Markov_chain::computeTTA(const LS_Vector &p0, double* p, 
    const LS_Options &opt, LS_Output &out) const
{
  //
  // Trivial case: no transient states
  //
  if (0==stateClass.sizeOfClass(0)) {
    out.status = LS_Success;
    out.num_iters = 0;
    out.relaxation = 0;
    out.precision = 0;
    return;
  }

  //
  // At least one transient state.  Have to do real work.
  // Check vectors.
  //
  if (0==p) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }
  if (0==p0.size) {
    throw MCLib::error(MCLib::error::Null_Vector);
  }

  //
  // Clear out p
  //
  for (long i=stateClass.firstNodeOfClass(0); i<=stateClass.lastNodeOfClass(0); i++) {
    p[i] = 0;
  } // for i

  //
  // Solve linear system of equations:
  //    n * Qtt = -p0
  // where n gives the expected time spent in each state.
  // Note: since we actually use p0, we need to negate n when we're done.

  if (double_graphs) {
    //
    // Set up matrix (shallow copies here)
    //
    LS_CRS_Matrix_double Qtt;
    graphToMatrix(G_bycols_diag, Qtt);
    Qtt.start = stateClass.firstNodeOfClass(0);
    Qtt.stop  = 1+stateClass.lastNodeOfClass(0);
    Qtt.one_over_diag = one_over_rowsums_d;

    //
    // Call the linear solver
    //
    Solve_Axb(Qtt, p, p0, opt, out);
  } else {
    //
    // Set up matrix (shallow copies here)
    //
    LS_CRS_Matrix_float Qtt;
    graphToMatrix(G_bycols_diag, Qtt);
    Qtt.start = stateClass.firstNodeOfClass(0);
    Qtt.stop  = 1+stateClass.lastNodeOfClass(0);
    Qtt.one_over_diag = one_over_rowsums_f;

    //
    // Call the linear solver
    //
    Solve_Axb(Qtt, p, p0, opt, out);
  }

  //
  // Negate our solution
  //
  for (long i=stateClass.firstNodeOfClass(0); i<=stateClass.lastNodeOfClass(0); i++) {
    p[i] = -p[i];
  } // for i
}

// ******************************************************************

void MCLib::Markov_chain::computeFirstRecurrentProbs(const LS_Vector &p0, 
    double* np, const LS_Options &opt, LS_Output &out) const
{
  for (long i=0; i<getNumStates(); i++) {
    np[i] = 0;
  }
  if (stateClass.sizeOfClass(0)) {
    // 
    // Determine time spent in each transient state
    //
    computeTTA(p0, np, opt, out);

    //
    // Multiply np by  Pta
    //

    if (double_graphs) {
      //
      // Set up matrix (shallow copies here)
      //
      LS_CRS_Matrix_double Qta;
      graphToMatrix(G_byrows_off, Qta);
      
      //
      // Multiply np += np * Qta 
      //
      Qta.VectorMatrixMultiply(np, np);
    } else {
      //
      // Set up matrix (shallow copies here)
      //
      LS_CRS_Matrix_float Qta;
      graphToMatrix(G_byrows_off, Qta);

      //
      // Multiply np += np * Qta 
      //
      Qta.VectorMatrixMultiply(np, np);
    }

  } // if there are transient states

  //
  // Add initial probabilities
  //
  if (p0.index) {
    //
    // p0 is sparse
    //
    if (p0.d_value) {
      for (long z=0; z<p0.size; z++) {
        if (p0.index[z] > stateClass.lastNodeOfClass(0)) {
          np[p0.index[z]] += p0.d_value[z];
        }
      }
    } else {
      DCASSERT(p0.f_value);
      for (long z=0; z<p0.size; z++) {
        if (p0.index[z] > stateClass.lastNodeOfClass(0)) {
          np[p0.index[z]] += p0.f_value[z];
        }
      }
    }
  } else {
    //
    // p0 is (truncated) full
    //
    if (p0.d_value) {
      for (long i=stateClass.firstNodeOfClass(1); i<p0.size; i++) {
        np[i] += p0.d_value[i];
      }
    } else {
      DCASSERT(p0.f_value);
      for (long i=stateClass.firstNodeOfClass(1); i<p0.size; i++) {
        np[i] += p0.f_value[i];
      }
    }
  }

  //
  // Normalize probs for recurrent states 
  //
  double total = 0.0;
  for (long i=stateClass.firstNodeOfClass(1); i<getNumStates(); i++) {
    total += np[i];
  }
  DCASSERT(total>0);
  if (0==total) return;   // Should be impossible
  for (long i=stateClass.firstNodeOfClass(1); i<getNumStates(); i++) {
    np[i] /= total;
  }
}

// ******************************************************************

