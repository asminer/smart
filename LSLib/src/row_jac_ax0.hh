
// $Id$

#ifndef ROW_JAC_AX0_HH
#define ROW_JAC_AX0_HH

#include "lslib.h"
#include "debug.hh"
#include "vectors.hh"
#include <math.h>

/**

    Workhorse for Row Jacobi solving Ax=0.

    The MATRIX class must provide the following methods:

      long Start()        : index of first row
      long Stop()         : one plus index of last row
      SolveRow(r, x, ans) : Row r times x added to ans,
                            then ans divided by row r diagonal


    @param  A     Matrix
    @param  x     Vector
    @param  xold  Auxiliary vector
    @param  opts  options
    @param  out   Output information
*/

template <bool RELAX, class MATRIX, class REAL>
void New_RowJacobi_Ax0(
          const MATRIX &A,          // abstract matrix
          double *xnew,             // solution vector
          REAL *xold,               // auxiliary vector
          const LS_Options &opts,   // solver options
          LS_Output &out            // performance results
)
{
  out.status = LS_No_Convergence;
  out.num_iters = 0;
#ifdef NAN
  out.precision = NAN;
#endif
  double one_minus_omega;
  if (RELAX) {
    out.relaxation = opts.relaxation;
    one_minus_omega = 1.0 - opts.relaxation;
  } else {
    out.relaxation = 1;
    one_minus_omega = 0;
  }
  long iters;
  double maxerror = 0;
  double total = 1;

  //
  // Copy xnew into xold
  //
  for (long s=A.Stop()-1; s>=A.Start(); s--) {
    xold[s] = xnew[s];
  }

  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, xnew, A.Start(), A.Stop());

    //
    // Compute the new vector and total
    //
    total = 0.0;
    for (long s=A.Start(); s<=A.Stop(); s++) {
      double tmp = 0.0;
      A.SolveRow(s, xold, tmp);

      if (RELAX) {
        tmp *= opts.relaxation;
        tmp += xold[s] * one_minus_omega;
      } 
      total += (xnew[s] = tmp);
    } // for s

    //
    // Determine current precision
    //
    maxerror = 0;
    if (iters >= opts.min_iters) {
      
      for (long s=A.Stop()-1; s>=A.Start(); s--) {
        double delta = xnew[s] - xold[s];
        if (opts.use_relative) if (xnew[s]) delta /= xnew[s];
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if ((maxerror >= opts.precision) && (iters < opts.max_iters)) {
              break;  
              // We know that we haven't achieved desired precision,
              // and we're not the last iteration, so there is no need
              // to determine the exact precision.
            }
        }
      } // for s

    } 

    //
    // Normalize and copy vector over
    //
    total = 1.0 / total;
    for (long s=A.Stop()-1; s>=A.Start(); s--) {
      xold[s] = xnew[s] * total;
    }

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break;
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;

  //
  // Normalize answer
  //
  for (long s=A.Stop()-1; s>=A.Start(); s--) {
    xnew[s] *= total;
  }
}

#endif

