
// $Id$

#ifndef ROW_JAC_AXB_HH
#define ROW_JAC_AXB_HH

#include "lslib.h"
#include "debug.hh"
#include "vectors.hh"

/**

    Workhorse for Row Jacobi solving Ax=b.

    The MATRIX class must provide the following methods:

      long Start()        : index of first row
      long Stop()         : one plus index of last row
      SolveRow(r, x, ans) : Row r times x added to ans,
                            then ans divided by row r diagonal


    The VECTOR class must provide the following methods:
      
      void CopyNegativeToFull(double* x, long start, long stop);


    @param  A     Matrix
    @param  x     Vector
    @param  xold  Auxiliary vector
    @param  opts  options
    @param  out   Output information
*/

template <bool RELAX, class MATRIX, class VECTOR, class REAL>
void New_RowJacobi_Axb(
          const MATRIX &A,          // abstract matrix
          double *xnew,             // solution vector
          const VECTOR &b,          // constant vector (right side)
          REAL *xold,               // auxiliary vector
          const LS_Options &opts,   // solver options
          LS_Output &out            // performance results
)
{
  out.status = LS_No_Convergence;
  double one_minus_omega;
  if (RELAX) {
    out.relaxation = opts.relaxation;
    one_minus_omega = 1.0 - opts.relaxation;
  } else {
    out.relaxation = 1;
    one_minus_omega = 0;
  }
  double* x = xnew;
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.Start(), A.Stop());

    CopyToAuxOrSwap(xold, x, A.Start(), A.Stop());
    for (long s=A.Start(); s<A.Stop(); s++) x[s] = 0;
    b.CopyNegativeToFull(x, A.Start(), A.Stop());

    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for (long s=A.Start(); s<A.Stop(); s++) {

      A.SolveRow(s, xold, x[s]);

      if (RELAX) {
        x[s] = (x[s] * opts.relaxation) + (xold[s] * one_minus_omega);
      } 

      if (check) {
        double delta = x[s] - xold[s];
        if (opts.use_relative) if (x[s]) delta /= x[s];
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision) {
              if (iters < opts.max_iters) {
                check = false;
              }
            }
        }
      } // if check

    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break;
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;

  if (x != xnew) {
      // Solution and aux vectors are swapped; copy results over
      for (long s=A.Stop()-1; s>=A.Start(); s--) xnew[s] = x[s];
  }
}

#endif

