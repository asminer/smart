
// $Id$

#ifndef MCBASE_H
#define MCBASE_H

#include "mclib.h"
#include "lslib.h"
#include "graphlib.h"
#include <stdlib.h>

class hypersparse_matrix;

// ******************************************************************
// *                                                                *
// *                      row_normalizer class                      *
// *                                                                *
// ******************************************************************

class row_normalizer : public GraphLib::generic_graph::element_visitor {
  const double* rowsums;
public:
  row_normalizer(const double* rs);
  virtual bool visit(long from, long to, void* wt);
};

// ******************************************************************
// *                                                                *
// *                                                                *
// *                         mc_base  class                         *
// *                                                                *
// *                                                                *
// ******************************************************************

/**
  Reducible (finished) chain information and operations.
  All the different derived classes will simply provide
  a different mechanism to build this information;
  once finished, they all have this structure.

  There are zero or more transient states,
  one or more recurrent classes,
  and zero or more absorbing states.
  States are numbered in that order
  (i.e., all transients, then all recurrents of class 1,
  ..., then all absorbing states).

*/
class mc_base : public MCLib::Markov_chain {
protected:
  static const int MAX_NODE_ADD = 1024;

  /// Graph of edges, except transient to recurrent.
  GraphLib::merged_weighted_digraph <float> *g;

  /// Transient to recurrent edges.
  hypersparse_matrix* h;

  /// Row sums, used during construction of DTMCs
  double* rowsums;

  /// Current size of array \a rowsums.
  long rowsums_alloc;

  /// Stopping index per class (including transient)
  long* stop_index;

  /// Period per class, or 0
  long* period;

  /// Vector of one over diagonals, in convenient form for numerical solution. 
  float* oneoverd;  
 
  /// Dimension of vector \a oneoverd.
  long ood_size;

  /// magnitude of largest diagonal element
  double maxdiag;
public:
  /** Constructor.
      @param  disc  Are we discrete-time?
      @param  gs  Initial number of graph states.
      @param  ge  Initial number of graph edges.
  */
  mc_base(bool disc, long gs, long ge);
  virtual ~mc_base();

protected:
  inline void EnlargeRowsums(long ns) {
    if (0==rowsums_alloc) return;
    if (ns < rowsums_alloc) return;
    long a = (rowsums_alloc >= MAX_NODE_ADD) 
                ? rowsums_alloc + MAX_NODE_ADD 
                : rowsums_alloc * 2;
    if (a<=0) throw MCLib::error(MCLib::error::Out_Of_Memory);
    double* r = (double*) realloc(rowsums, a*sizeof(double));
    if (0==r) throw MCLib::error(MCLib::error::Out_Of_Memory);
    rowsums = r;
    rowsums_alloc = a;
  }

public:

  virtual long getNumArcs() const;

  virtual bool isEfficientByRows() const;
  virtual void transpose();

  virtual void traverseFrom(long i, GraphLib::generic_graph::element_visitor &x);
  virtual void traverseTo(long i, GraphLib::generic_graph::element_visitor &x);
  virtual void traverseEdges(GraphLib::generic_graph::element_visitor &x);

  virtual bool getForward(const intset& x, intset &y) const;
  virtual bool getBackward(const intset& y, intset &x) const;

  virtual long getFirstTransient() const;
  virtual long getNumTransient() const;
  virtual long getFirstAbsorbing() const;
  virtual long getNumAbsorbing() const;
  virtual long getFirstRecurrent(long c) const;
  virtual long getRecurrentSize(long c) const;
  virtual bool isAbsorbingState(long s) const;
  virtual bool isTransientState(long s) const;
  virtual long getClassOfState(long s) const;
  virtual bool isStateInClass(long st, long cl) const;
  virtual void computePeriodOfClass(long c);
  virtual long getPeriodOfClass(long c) const;
  virtual double getUniformizationConst() const;
  virtual void computeTransient(double t, double* p, transopts &opts) const;
  virtual void computeTransient(int t, double* p, transopts &opts) const;
  virtual void accumulate(double t, const double* p0, double* n0t, transopts &opts) const;

  virtual void computeSteady(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const;
  virtual void computeTTA(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const;
  virtual void computeClassProbs(const LS_Vector &p0, double* nc, const LS_Options &opt, LS_Output &out) const;
  virtual long randomWalk(rng_stream &rng, long &state, const intset* final,
                            long maxt, double q) const;
  virtual double randomWalk(rng_stream &rng, long &state, const intset* final,
                            double maxt) const;

  virtual long ReportMemTotal() const;
protected:
  inline void finalize(type t) {
    // assert(Unknown == our_type);
    // assert(false == finished);
    our_type = t;
    finished = true;
  }

  void irredSteady(double* p, const LS_Options &opt, LS_Output &lo) const;
  void reducSteady(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &lo) const;
  void reducTTA(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &lo) const;

  void accDTMC(double t, const double* p0, double* n0t, transopts &opts) const;
  void accCTMC(double t, const double* p0, double* n0t, transopts &opts) const;

  int stepForward(int n, double q, double* p, double* aux, double delta) const;

  void oneStep(const LS_Matrix &Qtt, double q, double* p, double* aux) const {
    // vector-matrix multiply
    for (long s=num_states-1; s>=0; s--) aux[s] = 0.0;
    Qtt.VectorMatrixMultiply(aux, p);
    if (h) h->VectorMatrixMultiply(aux, p);

    // adjust for diagonals
    if (Qtt.d_one_over_diag) {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.d_one_over_diag[s];
        aux[s] += d*p[s];
      }
    } else {
      for (long s=Qtt.stop-1; s>=0; s--) {
        double d = q - 1.0/Qtt.f_one_over_diag[s];
        aux[s] += d*p[s];
      }
    }

    // finally, adjust for absorbing states
    for (long s=Qtt.stop; s<num_states; s++) {
      aux[s] += q*p[s];
    }

    // normalize (right now we are off by a factor of q)
    double total = 0.0;
    for (long s=num_states-1; s>=0; s--)   total += aux[s];
    for (long s=num_states-1; s>=0; s--)  aux[s] /= total;
  }

public:
  inline void exportQtt(LS_Matrix &Qtt) const {
    Qtt.d_value = 0;
    Qtt.d_one_over_diag = 0;
    if (0==g) {
      Qtt.rowptr = 0;
      Qtt.colindex = 0;
      Qtt.f_value = 0;
      Qtt.f_one_over_diag = 0;
      return;
    }
    GraphLib::generic_graph::matrix m;
    g->exportFinished(m);
    Qtt.is_transposed = !m.is_transposed;
    Qtt.rowptr = m.rowptr;
    Qtt.colindex = m.colindex;
    Qtt.f_value = (float*) m.value;
    Qtt.f_one_over_diag = oneoverd;
  }

  inline void setClass(LS_Matrix &Qtt, long c) const {
    Qtt.start = c ? stop_index[c-1] : 0;
    Qtt.stop = stop_index[c];
  }
};

#endif
