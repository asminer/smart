
// $Id$

#ifndef VMM_JAC_AX0_HH
#define VMM_JAC_AX0_HH

#include "lslib.h"
#include "debug.hh"
#include "vectors.hh"
#include <math.h>

/**

    Workhorse for vector-matrix multiplication Jacobi solving Ax=0.

    The MATRIX class must provide the following methods:

      long Start()        : index of first row
      long Stop()         : one plus index of last row
      Multiply(x, old)    : Compute matrix * old, store in x
      DivideDiag(x, a)    : x[i] *= a / diagonal[i], for all i


    @param  A     Matrix
    @param  xnew  Vector
    @param  xold  Auxiliary vector
    @param  opts  options
    @param  out   Output information
*/

template <bool RELAX, class MATRIX, class REAL>
void New_VMMJacobi_Ax0(
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
  double* x = xnew;
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.Start(), A.Stop());

    CopyToAuxOrSwap(xold, x, A.Start(), A.Stop());

    for (long s=A.Start(); s<A.Stop(); s++) x[s] = 0;

    A.MatrixVectorMultiply(x, xold);

    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    for (long s=A.Start(); s<A.Stop(); s++) {
      if (RELAX) {
        x[s] *= A.one_over_diag[s] * opts.relaxation;
        x[s] += one_minus_omega * xold[s];
      } else {
        x[s] *= A.one_over_diag[s];
      }
      total += x[s];

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
    if (total != 1.0) {
      total = 1.0 / total;
      for (long s=A.Stop()-1; s>=A.Start(); s--) x[s] *= total;
    }

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

