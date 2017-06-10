
/**
  Implementation of Markov_chain class.
*/

#include "mclib.h"

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
  //
  // Build row sums
  //
  double* rowsums = new double[G.getNumNodes()];
  for (long i=0; i<G.getNumNodes(); i++) {
    rowsums[i] = 0;
  }
  G.addRowSums(rowsums);

  //
  // Normalize rows if we're a DTMC;
  // determine uniformization constant if we're a CTMC.
  //
  if (discrete) {
    G.divideRows(rowsums);
    uniformization_const = 1.0; 
  } else {
    uniformization_const = rowsums[0];
    for (long i=1; i<G.getNumNodes(); i++) {
      if (rowsums[i] > uniformization_const) {
        uniformization_const = rowsums[i];
      }
    }
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
  // Sanity checks
  //
  DCASSERT(Q_byrows_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_byrows_off.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_bycols_diag.getNumNodes() == G.getNumNodes());
  DCASSERT(Q_bycols_off.getNumNodes() == G.getNumNodes());

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
  // Translate from mcbase.cc line ~421

  // TBD
  return 0;
}

