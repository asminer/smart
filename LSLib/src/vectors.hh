
// $Id$

#ifndef VECTORS_HH
#define VECTORS_HH

// Handy vector functions

inline void CopyToAuxOrSwap(float* xold, const double* x, long start, long stop)
{
  for (stop--; stop >= start; stop--) {
    xold[stop] = x[stop];
  }
}

inline void CopyToAuxOrSwap(double* &xold, double* &x, long, long)
{
  double* tmp = xold;
  xold = x;
  x = tmp;
}

#endif
