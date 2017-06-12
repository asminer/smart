
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
  cout << "Inside Markov_chain constructor.\n";
#endif
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
  // Build subgraphs and remove any self loops
  //

  if (G.isByRows()) {
    G.splitAndExport(TSCCinfo, false, Q_byrows_diag, Q_byrows_off, sw);
    Q_bycols_diag.transposeFrom(Q_byrows_diag);
    Q_bycols_off.transposeFrom(Q_byrows_off);
  } else {
    G.splitAndExport(TSCCinfo, false, Q_bycols_diag, Q_bycols_off, sw);
    Q_byrows_diag.transposeFrom(Q_bycols_diag);
    Q_byrows_off.transposeFrom(Q_bycols_off);
  }

  //
  // Determine rowsums from split graphs
  //
  zeroArray(rowsums, G.getNumNodes());
  double_graph_rowsums(Q_byrows_diag, rowsums);
  double_graph_rowsums(Q_byrows_off, rowsums);


  //
  // Uniformization constant
  //
  if (discrete) {
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
  DCASSERT(Q_byrows_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_byrows_off.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_bycols_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_bycols_off.getNumNodes() == G.getNumNodes());

#ifdef DEBUG_CONSTRUCTOR
  cout << "  Q_byrows_diag:\n";
  showGraph(Q_byrows_diag);
  cout << "  Q_byrows_off:\n";
  showGraph(Q_byrows_off);
  cout << "  Q_bycols_diag:\n";
  showGraph(Q_bycols_diag);
  cout << "  Q_bycols_off:\n";
  showGraph(Q_bycols_off);
  cout << "  rowsums:\n";
  cout << "    ";
  showArray(rowsums, Q_byrows_diag.getNumNodes());
#endif

  //
  // Build one_over_rowsums array, used for linear solvers
  //
  one_over_rowsums = new float[Q_byrows_diag.getNumNodes()];
  for (long i=0; i<Q_byrows_diag.getNumNodes(); i++) {
    one_over_rowsums[i] = rowsums[i] ? (1.0/rowsums[i]) : 0.0;
  }

  //
  // Cleanup
  //
  delete[] rowsums;

#ifdef DEBUG_CONSTRUCTOR
  cout << "  one_over_rowsums:\n";
  cout << "    ";
  showArray(one_over_rowsums, G.getNumNodes());
  cout << "Exiting Markov_chain constructor.\n";
#endif
}

// ******************************************************************

MCLib::Markov_chain::~Markov_chain()
{
  delete[] one_over_rowsums;
}

// ******************************************************************

size_t MCLib::Markov_chain::getMemTotal() const
{
  return 
    Q_byrows_diag.getNumNodes() * sizeof(float) + // for one_over_rowsums
    stateClass.getMemTotal() +
    Q_byrows_diag.getMemTotal() +
    Q_byrows_off.getMemTotal() +
    Q_bycols_diag.getMemTotal() +
    Q_bycols_off.getMemTotal(); 
}

// ******************************************************************

bool MCLib::Markov_chain::traverseOutgoing(GraphLib::BF_graph_traversal &t)
const
{
  while (t.hasNodesToExplore()) {
      long s = t.getNextToExplore();

      if (s<0 || s>=Q_byrows_diag.getNumNodes()) {
        throw GraphLib::error(GraphLib::error::Bad_Index);
      }

      // Explore diagonal edges from s
      for (long z=Q_byrows_diag.RowPointer(s); 
            z<Q_byrows_diag.RowPointer(s+1); z++) 
      {
        if (t.visit(s, Q_byrows_diag.ColumnIndex(z), Q_byrows_diag.Label(z))) {
          return true;
        }
      }

      // Explore off-diagonal edges from s
      for (long z=Q_byrows_off.RowPointer(s); 
            z<Q_byrows_off.RowPointer(s+1); z++) 
      {
        if (t.visit(s, Q_byrows_off.ColumnIndex(z), Q_byrows_off.Label(z))) {
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

      if (s<0 || s>=Q_bycols_diag.getNumNodes()) {
        throw GraphLib::error(GraphLib::error::Bad_Index);
      }

      // Explore diagonal edges from s
      for (long z=Q_bycols_diag.RowPointer(s); 
            z<Q_bycols_diag.RowPointer(s+1); z++) 
      {
        if (t.visit(s, Q_bycols_diag.ColumnIndex(z), Q_bycols_diag.Label(z))) {
          return true;
        }
      }

      // Explore off-diagonal edges from s
      for (long z=Q_bycols_off.RowPointer(s); 
            z<Q_bycols_off.RowPointer(s+1); z++) 
      {
        if (t.visit(s, Q_bycols_off.ColumnIndex(z), Q_bycols_off.Label(z))) {
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
  for (long i=stateClass.firstNodeOfClass(c); i<=stateClass.lastNodeOfClass(c); i++) {
    if (one_over_rowsums[i] != 1.0) return 1;
  }

  // First pass: forward reachability search from
  // start state, track state distances.
  BF_period T(stateClass.firstNodeOfClass(c), stateClass.sizeOfClass(c));
  Q_byrows_diag.traverse(T); 

  // Second pass: check all edges, and update period
  // based on (3) above.
  T.secondPass(stateClass.firstNodeOfClass(c), stateClass.sizeOfClass(c));
  Q_byrows_diag.traverse(T); 

  return T.getPeriod();
}

