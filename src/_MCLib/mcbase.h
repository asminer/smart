
#ifndef MCBASE_H
#define MCBASE_H

#include "mclib.h"
#include "../_LSLib/lslib.h"
#include "../_GraphLib/graphlib.h"
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
private:
  GraphLib::generic_graph::const_matrix rawQ;

protected:
  /// Options for internal computation of discrete distributions
  struct extra_distopts : public distopts {
      /// Fixed distribution
      double* fixed_dist;
      /// Size of fixed distribution array
      int fixed_dist_size;

      /// Variable distribution, will expand as necessary
      double* var_dist;
      /// Size of variable distribution array
      int var_dist_size;

      /// Distribution "precision"
      double epsilon;

      /// Should we save the error at each step?
      bool need_error;

      /// Error at each step, will expand as necessary
      double* error_dist;
      /// Size of the error_dist array
      int error_dist_size;

    public:
      extra_distopts(const distopts &d) : distopts(d) {
        fixed_dist = 0;
        fixed_dist_size = 0;
        var_dist = 0;
        var_dist_size = 0;
        epsilon = 0;
        need_error = 0;
        error_dist = 0;
        error_dist_size = 0;
      }
      void setFixed(double dist[], int N) {
        fixed_dist = dist;
        fixed_dist_size = N;
      }
      void setVariable(double eps) {
        epsilon = eps;
      }
  };
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
  virtual void reverseTransient(double t, double* p, transopts &opts) const;
  virtual void reverseTransient(int t, double* p, transopts &opts) const;
  virtual void accumulate(double t, const double* p0, double* n0t, transopts &opts) const;

  virtual void computeSteady(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const;
  virtual void computeTTA(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &out) const;
  virtual void computeClassProbs(const LS_Vector &p0, double* nc, const LS_Options &opt, LS_Output &out) const;
protected:
  void internalDiscreteDistTTA(const LS_Vector &p0, extra_distopts &opts, int c) const;
public:
  virtual void computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, double e, double* &dist, int &N) const;
  virtual double computeDiscreteDistTTA(const LS_Vector &p0, distopts &opts, int c, double dist[], int N) const;
  virtual void computeContinuousDistTTA(const LS_Vector &p0, distopts &opts, int c, double dt, double e, double* &dist, int &N) const;
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
  inline void baseFinish() {
    g->exportFinished(rawQ);
  }

  void irredSteady(double* p, const LS_Options &opt, LS_Output &lo) const;
  void reducSteady(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &lo) const;
  void reducTTA(const LS_Vector &p0, double* p, const LS_Options &opt, LS_Output &lo) const;

  void accDTMC(double t, const double* p0, double* n0t, transopts &opts) const;
  void accCTMC(double t, const double* p0, double* n0t, transopts &opts) const;

  static inline void fillFullVector(double* x, long n, const LS_Vector &p0) {
    for (long i=0; i<n; i++) x[i] = 0.0;
    if (p0.index) {
      // Sparse storage
      if (p0.d_value) {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] < n)
            x[p0.index[z]] += p0.d_value[z];
      } else {
        for (long z=0; z<p0.size; z++)
          if (p0.index[z] < n)
            x[p0.index[z]] += p0.f_value[z];
      }
    } else {
      // Full storage 
      if (p0.size < n) {
        n = p0.size;
      }
      if (p0.d_value) 
        for (long i=0; i<n; i++)
          x[i] += p0.d_value[i];
      else 
        for (long i=0; i<n; i++)
          x[i] += p0.f_value[i];
    }
  }

public:
  /// Export (transposed) Q matrix
  inline void exportQT(LS_CRS_Matrix_float &Q) const {
    if (!rawQ.is_transposed) throw MCLib::error(MCLib::error::Internal);
    Q.size = getNumStates();
    Q.val = (float*) rawQ.value;
    Q.col_ind = rawQ.colindex;
    Q.row_ptr = rawQ.rowptr;
    Q.one_over_diag = oneoverd;
  }

  /// Export (transposed) Q matrix
  inline void exportQT(LS_CCS_Matrix_float &Q) const {
    if (rawQ.is_transposed) throw MCLib::error(MCLib::error::Internal);
    Q.size = getNumStates();
    Q.val = (float*) rawQ.value;
    Q.row_ind = rawQ.colindex;
    Q.col_ptr = rawQ.rowptr;
    Q.one_over_diag = oneoverd;
  }

  template <class MATRIX>
  inline void useClass(MATRIX &Q, long c) const {
    Q.start = c ? stop_index[c-1] : 0;
    Q.stop = stop_index[c];
  }

  template <class MATRIX>
  inline void useAllButAbsorbing(MATRIX &Q) const {
    Q.start = 0;
    Q.stop = stop_index[num_classes];
  }

/*
  template <class MATRIX>
  inline void useEntire(MATRIX &Q) const {
    Q.start = 0;
    Q.stop = Q.size;
  }
  */

/*
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
    GraphLib::generic_graph::const_matrix m;
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
*/

private:

#if 0
  /*
      Nitty gritty details from here
  */

  // Let's try this out:

  template <class MATRIX>


  // Row storage

  void genericTransient(double t, double* p, transopts &opts, 
    void (mc_base::* s)(const LS_CRS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  void genericTransient(int t, double* p, transopts &opts, 
    void (mc_base::* s)(const LS_CRS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  int stepGeneric(int n, double q, double* p, double* aux, double delta, 
    void (mc_base::* s)(const LS_CRS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  void forwStep(const LS_CRS_Matrix_float &Qtt, double q, double* p, 
    double* aux, bool normalize) const;

  void backStep(const LS_CRS_Matrix_float &Qtt, double q, double* p, 
    double* aux, bool normalize) const;


  // Column storage

  void genericTransient(double t, double* p, transopts &opts, 
    void (mc_base::* s)(const LS_CCS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  void genericTransient(int t, double* p, transopts &opts, 
    void (mc_base::* s)(const LS_CCS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  int stepGeneric(int n, double q, double* p, double* aux, double delta, 
    void (mc_base::* s)(const LS_CCS_Matrix_float&, 
    double, double*, double*, bool) const
  ) const;

  void forwStep(const LS_CCS_Matrix_float &Qtt, double q, double* p, 
    double* aux, bool normalize) const;

  void backStep(const LS_CCS_Matrix_float &Qtt, double q, double* p, 
    double* aux, bool normalize) const;
#endif

};

#endif
