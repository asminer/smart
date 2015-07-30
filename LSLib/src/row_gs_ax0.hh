
// $Id$

#ifndef ROW_GS_AX0_HH
#define ROW_GS_AX0_HH

#include "lslib.h"
#include "debug.hh"
#include <math.h>

/**

    Workhorse for Row Gauss-Seidel solving Ax=0.

    The MATRIX class must provide the following methods:

      long Start()        : index of first row
      long Stop()         : one plus index of last row
      SolveRow(r, x, ans) : Row r times x added to ans,
                            then ans divided by row r diagonal


    @param  A     Matrix
    @param  x     Vector
    @param  opts  options
    @param  out   Output information
*/

template <bool RELAX, class MATRIX>
void New_RowGS_Ax0(
          const MATRIX &A,          // abstract matrix
          double *x,                // solution vector
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
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.Start(), A.Stop());
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    for (long s=A.Start(); s<A.Stop(); s++) {
      double tmp = 0.0;

      A.SolveRow(s, x, tmp);

      double delta;
      if (RELAX) {
        tmp = (tmp * opts.relaxation) + (x[s] * one_minus_omega);
      } 
      delta = tmp - x[s];
      x[s] = tmp;
      total += x[s];

      if (check) {
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
}

#endif

