
// $Id$

#include "lslib.h"

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <assert.h>

#include "revision.h"

// #define DEBUG

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 3;

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }

/// Debugging Assertion
#ifdef DEBUG
#define DEBUG_ASSERT(X)  assert(X);
#else
#define DEBUG_ASSERT(X)
#endif

// ******************************************************************
// *                      Iteration  debugging                      *
// ******************************************************************

void DebugIter(const char* which, long iters, double* x, long start, long stop)
{
  printf("Start of %s iteration %ld\n[ %f", which, iters, x[start]);
  for (long s=start+1; s<stop; s++) printf(", %f", x[s]);
  printf("]\n");
}

// ******************************************************************
// *                      LS_Matrix  debugging                      *
// ******************************************************************

void DebugMatrix(const LS_Matrix &A)
{
  printf("Using A matrix:\n");
  printf("  is_transposed: ");
  if (A.is_transposed) printf("true"); else printf("false");
  printf("\n  start: %ld\n  stop: %ld\n  rowptr: [", A.start, A.stop);
  for (long i=0; i<=A.stop; i++) {
    if (i) printf(", ");
    printf("%ld", A.rowptr[i]);
  }
  printf("]\n  colindex: [");
  for (long i=0; i<A.rowptr[A.stop]; i++) {
    if (i) printf(", ");
    printf("%ld", A.colindex[i]);
  }
  printf("]\n  f_value:  ");
  if (A.f_value) {
    printf("[");
    for (long i=0; i<A.rowptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%g", A.f_value[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  d_value:  ");
  if (A.d_value) {
    printf("[");
    for (long i=0; i<A.rowptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%lg", A.d_value[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  f_one_over_diag: ");
  if (A.f_one_over_diag) {
    printf("[");
    for (long i=0; i<A.stop; i++) {
      if (i) printf(", ");
      printf("%g", A.f_one_over_diag[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  d_one_over_diag: ");
  if (A.d_one_over_diag) {
    printf("[");
    for (long i=0; i<A.stop; i++) {
      if (i) printf(", ");
      printf("%g", A.d_one_over_diag[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}

// ******************************************************************
// *                      LS_Vector  debugging                      *
// ******************************************************************

void DebugVector(const LS_Vector &b)
{
  printf("Using b vector:\n");
  printf("  size: %ld\n", b.size);
  printf("  index: ");
  if (b.index) {
    printf("[%ld", b.index[0]);
    for (long i=1; i<b.size; i++) printf(", %ld", b.index[i]);
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  d_value: ");
  if (b.d_value) {
    printf("[%lg", b.d_value[0]);
    for (long i=1; i<b.size; i++) printf(", %lg", b.d_value[i]);
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  f_value: ");
  if (b.f_value) {
    printf("[%g", b.f_value[0]);
    for (long i=1; i<b.size; i++) printf(", %g", b.f_value[i]);
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}

// ******************************************************************
// *                       LS_Matrix  methods                       *
// ******************************************************************

void LS_Matrix::VectorMatrixMultiply(double* y, const double* x) const
{
  const long* rp = rowptr + start;
  const long* rpstop = rowptr + stop;
  const long* ci = colindex + rp[0];
  if (is_transposed) {
    // multiplication, matrix by columns
    if (d_value) {
      const double* v = d_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
            y[ci[0]] +=x[0] * v[0];
            ci++;
            v++;
        } // while ci
        x++;
      } // while rp
      return;
    }
    if (f_value) {
      const float* v = f_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
            y[ci[0]] +=x[0] * v[0];
            ci++;
            v++;
        } // while ci
        x++;
      } // while rp
      return;
    }
  } else {
    // multiplication, matrix by rows
    if (d_value) {
      const double* v = d_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[0] +=x[ci[0]] * v[0];
          ci++;
          v++;
        } // while ci
        y++;
      } // while rp
    }
    if (f_value) {
      const float* v = f_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[0] +=x[ci[0]] * v[0];
          ci++;
          v++;
        } // while ci
        y++;
      } // while rp
    }
  }
}

void LS_Matrix::VectorMatrixMultiply(double* y, const float* x) const
{
  const long* rp = rowptr + start;
  const long* rpstop = rowptr + stop;
  const long* ci = colindex + rp[0];
  if (is_transposed) {
    // multiplication, matrix by columns
    if (d_value) {
      const double* v = d_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[ci[0]] +=x[0] * v[0];
          ci++;
          v++;
        } // while ci
        x++;
      } // while rp
      return;
    }
    if (f_value) {
      const float* v = f_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[ci[0]] +=x[0] * v[0];
          ci++;
          v++;
        } // while ci
        x++;
      } // while rp
      return;
    }
  } else {
    // multiplication, matrix by rows
    if (d_value) {
      const double* v = d_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[0] +=x[ci[0]] * v[0];
          ci++;
          v++;
        } // while ci
        y++;
      } // while rp
    }
    if (f_value) {
      const float* v = f_value + rp[0];
      while (rp < rpstop) {
        rp++;
        const long* cstop = colindex + rp[0];
        while (ci < cstop) {
          y[0] +=x[ci[0]] * v[0];
          ci++;
          v++;
        } // while ci
        y++;
      } // while rp
    }
  }
}

// ******************************************************************
// *                    Internal matrix template                    *
// ******************************************************************

template <class REAL>
struct LS_Internal_Matrix {
  bool is_transposed;
  long start;
  long stop;
  const long* rowptr;
  const long* colindex;
  const REAL* value;
  const REAL* one_over_diag;

  // for convenience
  inline void Fill(const LS_Matrix &A);
  inline void FillOthers(const LS_Matrix &A) {
    is_transposed = A.is_transposed;
    start = A.start;
    stop = A.stop;
    rowptr = A.rowptr;
    colindex = A.colindex;
  }

  template <class REAL2>
  void MultiplyByRows(double* y, const REAL2* old) const {
    DEBUG_ASSERT(!is_transposed);
    const long* rp = rowptr + start;
    const long* rpstop = rowptr + stop;
    const long* ci = colindex + rp[0];
    const REAL* v = value + rp[0];
    y += start;
    while (rp < rpstop) {
      rp++;
      const long* cstop = colindex + rp[0];
      while (ci < cstop) {
        y[0] += old[ci[0]] * v[0];
        ci++;
        v++;
      } // inner while
      y++;
    } // outer while
  }

  template <class REAL2>
  void MultiplyByCols(double* y, const REAL2* old) const {
    DEBUG_ASSERT(is_transposed);
    const long* rp = rowptr + start;
    const long* rpstop = rowptr + stop;
    const long* ci = colindex + rp[0];
    const REAL* v = value + rp[0];
    old += start;
    while (rp < rpstop) {
      rp++;
      const long* cstop = colindex + rp[0];
      while (ci < cstop) {
        y[ci[0]] += old[0] * v[0];
        ci++;
        v++;
      } // inner while
      old++;
    } // outer while
  }
};

template<>
void LS_Internal_Matrix<float>::Fill(const LS_Matrix &A)
{
  FillOthers(A);
  value = A.f_value;
  one_over_diag = A.f_one_over_diag;
}

template<>
void LS_Internal_Matrix<double>::Fill(const LS_Matrix &A)
{
  FillOthers(A);
  value = A.d_value;
  one_over_diag = A.d_one_over_diag;
}

// ******************************************************************
// *                Internal sparse vector  template                *
// ******************************************************************

template <class REAL>
struct LS_Sparse_Vector {
  long size;
  const long* index;
  const REAL* value;
  // for convenience
  inline void Fill(const LS_Vector &v);
  inline void FillOthers(const LS_Vector &v) {
    size = v.size;
    index = v.index;
  }
};

template<>
void LS_Sparse_Vector<double>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.d_value;
}

template<>
void LS_Sparse_Vector<float>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.f_value;
}

// ******************************************************************
// *            Internal truncated full vector  template            *
// ******************************************************************

template <class REAL>
struct LS_Full_Vector {
  long size;
  const REAL* value;
  // for convenience
  inline void Fill(const LS_Vector &v);
  inline void FillOthers(const LS_Vector &v) {
    size = v.size;
  }
};

template<>
void LS_Full_Vector<double>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.d_value;
}

template<>
void LS_Full_Vector<float>::Fill(const LS_Vector &v)
{
  FillOthers(v);
  value = v.f_value;
}

// ******************************************************************
// *                                                                *
// *                   LS_Abstract_Matrix methods                   *
// *                                                                *
// ******************************************************************

LS_Abstract_Matrix::LS_Abstract_Matrix(long s)
{
  size = s;
}

LS_Abstract_Matrix::~LS_Abstract_Matrix()
{
}

void LS_Abstract_Matrix::FirstRow(long& r) const
{
  r = 0;
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                                                                *
// *                             Ax = 0                             *
// *                                                                *
// *                                                                *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                                                                *
// *                        Row Gauss-Seidel                        *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL>
void RowGS_Ax0(const LS_Internal_Matrix <REAL> &A, 
    double *x, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_No_Convergence;
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    maxerror = 0;
    double total = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (long s=A.start; s<A.stop; s++) {
      const long* cstop = A.colindex + A.rowptr[s+1];
      double tmp = 0.0;
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (long s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break;
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1;
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL>
void RowGS_Ax0_w(const LS_Internal_Matrix <REAL> &A, 
    double *x, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_No_Convergence;
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    maxerror = 0;
    double total = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (long s=A.start; s<A.stop; s++) {
      const long* cstop = A.colindex + A.rowptr[s+1];
      double tmp = 0.0;
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (long s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break;
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

void RowGS_Ax0(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_No_Convergence;
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    A->FirstRow(s);
    for (; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      news = A->SolveRow(s, x, tmp);
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break;
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
}


// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

void RowGS_Ax0_w(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_No_Convergence;
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    A->FirstRow(s);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      news = A->SolveRow(s, x, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) {
      out.status = LS_Success;
      break; 
    }
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
}



// ******************************************************************
// *                                                                *
// *                           Row Jacobi                           *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Ax0(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) old[s] = x[s];
    double total = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      const long* cstop = A.colindex + A.rowptr[s+1];
      double tmp = 0.0;
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Ax0_w(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) old[s] = x[s];
    double total = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      const long* cstop = A.colindex + A.rowptr[s+1];
      double tmp = 0.0;
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

template <class REAL>
void RowJacobi_Ax0(LS_Abstract_Matrix* const &A, 
    double *x, REAL* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=A->GetSize(); s>=0; s--) old[s] = x[s];
    double total = 0;
    bool check = (iters >= opts.min_iters);
    A->FirstRow(s);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      news = A->SolveRow(s, old, tmp);
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

template <class REAL>
void RowJacobi_Ax0_w(LS_Abstract_Matrix* const &A, 
    double *x, REAL* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=A->GetSize(); s>=0; s--) old[s] = x[s];
    double total = 0;
    bool check = (iters >= opts.min_iters);
    A->FirstRow(s);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      news = A->SolveRow(s, old, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}



// ******************************************************************
// *                                                                *
// *                            VMJacobi                            *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       Explicit,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2>
void RowVMJacobi_Ax0(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A.MultiplyByRows(x, old);

    // complete the iteration    
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    if (opts.use_relaxation) {
      for(s=A.start; s<A.stop; s++) {
          double tmp = x[s];
          tmp *= A.one_over_diag[s] * opts.relaxation;
          tmp += one_minus_omega * old[s];
          if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
          } // if check
          total += (x[s] = tmp);
      } // for s
    } else {
      for(s=A.start; s<A.stop; s++) {
          double tmp = x[s];
          tmp *= A.one_over_diag[s];
          if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
          } // if check
          total += (x[s] = tmp);
      } // for s
    } // if relaxation
    if (total != 1.0) 
      for (s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, by columns                      *
// ******************************************************************

template <class REAL1, class REAL2>
void ColVMJacobi_Ax0(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A.MultiplyByCols(x, old);

    // Diagonal adjust
    for (s=A.stop-1; s>=A.start; s--) {
      x[s] *= A.one_over_diag[s] * opts.relaxation;
    } 

    // complete the iteration    
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    for(s=A.start; s<A.stop; s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A.stop-1; s>=A.start; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by rows                       *
// ******************************************************************

template <class REAL>
void RowVMJacobi_Ax0(LS_Abstract_Matrix* const &A, 
          double* x, 
          REAL* old,
          const LS_Options &opts, 
          LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=A->GetSize()-1; s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A->NoDiag_MultByRows(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by cols                       *
// ******************************************************************

template <class REAL>
void ColVMJacobi_Ax0(LS_Abstract_Matrix* const &A, 
          double* x, 
          REAL* old,
          const LS_Options &opts, 
          LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=A->GetSize()-1; s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A->NoDiag_MultByCols(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    double total = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      total += (x[s] = tmp);
    } // for s
    if (total != 1.0) 
      for (s=A->GetSize()-1; s>=0; s--) x[s] /= total;

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                                                                *
// *                          Power method                          *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       Explicit,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2>
void RowPower_Ax0(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_Not_Implemented;
}

// ******************************************************************
// *                      Explicit, by columns                      *
// ******************************************************************

template <class REAL1, class REAL2>
void ColPower_Ax0(const LS_Internal_Matrix <REAL1> &A, 
    double *x, REAL2* old, const LS_Options &opts, LS_Output &out)
{
  out.status = LS_Not_Implemented;
}

// ******************************************************************
// *                            Abstract                            *
// ******************************************************************

template <class REAL>
void Power_Ax0(LS_Abstract_Matrix* const &A, 
          double* x, 
          REAL* old,
          const LS_Options &opts, 
          LS_Output &out)
{
  out.status = LS_Not_Implemented;
}

template <class REAL>
inline void RowPower_Ax0(LS_Abstract_Matrix* const &A, 
          double* x, 
          REAL* old,
          const LS_Options &opts, 
          LS_Output &out)
{
  Power_Ax0(A, x, old, opts, out);
}

template <class REAL>
inline void ColPower_Ax0(LS_Abstract_Matrix* const &A, 
          double* x, 
          REAL* old,
          const LS_Options &opts, 
          LS_Output &out)
{
  Power_Ax0(A, x, old, opts, out);
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                                                                *
// *                        Ax = b, sparse b                        *
// *                                                                *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                        Row Gauss-Seidel                        *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2>
void RowGS_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  int bstart;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    int bptr = bstart;
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp = 0;
      if (bptr < b.size) if (s == b.index[bptr]) {
        tmp = -b.value[bptr];
        bptr++;
      }
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1.0;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2>
void RowGS_Axb_w(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2>  &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  int bstart;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    int bptr = bstart;
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp = 0.0;
      if (bptr < b.size) if (s == b.index[bptr]) {
    tmp = -b.value[bptr];
    bptr++;
      } 
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

template <class REAL>
void RowGS_Axb(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Sparse_Vector <REAL> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      // binary search through b vector
      int left = 0;
      int right = b.size;
      while (left < right) {
        int mid = (right+left)/2;
        if (b.index[mid]==s) {
          tmp = -b.value[mid];
          break;
        }
        if (b.index[mid] < s) {
          left = mid+1; 
        } else {
          right = mid;
        } 
      } // while
      news = A->SolveRow(s, x, tmp);
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}


// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

template <class REAL>
void RowGS_Axb_w(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Sparse_Vector <REAL> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = 0.0;
      // binary search through b vector
      int left = 0;
      int right = b.size;
      while (left < right) {
        int mid = (right+left)/2;
        if (b.index[mid]==s) {
          tmp = -b.value[mid];
          break;
        }
        if (b.index[mid] < s) {
          left = mid+1; 
        } else {
          right = mid;
        } 
      } // while
      news = A->SolveRow(s, x, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                                                                *
// *                           Row Jacobi                           *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  int bstart, bstop;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (bstop = b.size; bstop>0; bstop--) {
    if (b.index[bstop-1] < A.stop) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=bstart; s<bstop; s++)  x[b.index[s]] = -b.value[s];
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp = x[s];
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowJacobi_Axb_w(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  int bstart, bstop;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (bstop = b.size; bstop>0; bstop--) {
    if (b.index[bstop-1] < A.stop) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=bstart; s<bstop; s++)  x[b.index[s]] = -b.value[s];
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp = x[s];
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Sparse_Vector <REAL1> &b, REAL2* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=A->GetSize(); s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=0; s<b.size; s++)  x[b.index[s]] = -b.value[s];
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = x[s];
      news = A->SolveRow(s, old, tmp);
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters

  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Axb_w(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Sparse_Vector <REAL1> &b, REAL2* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=A->GetSize(); s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=0; s<b.size; s++)  x[b.index[s]] = -b.value[s];
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = x[s];
      news = A->SolveRow(s, old, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}



// ******************************************************************
// *                                                                *
// *                            VMJacobi                            *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       Explicit,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowVMJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  int bstart, bstop;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (bstop = b.size; bstop>0; bstop--) {
    if (b.index[bstop-1] < A.stop) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=bstart; s<bstop; s++)  x[b.index[s]] = -b.value[s];
   
    // Vector-matrix multiply
    A.MultiplyByRows(x, old);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    if (opts.use_relaxation) {
      for(s=A.start; s<A.stop; s++) {
          double tmp = x[s];
          tmp *= A.one_over_diag[s] * opts.relaxation;
          tmp += one_minus_omega * old[s];
          if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
          } // if check
        x[s] = tmp;
      } // for s
    } else {
      for(s=A.start; s<A.stop; s++) {
          double tmp = x[s] * A.one_over_diag[s];
          if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
          } // if check
          x[s] = tmp;
      } // for s
    } // if relaxation

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, by columns                      *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void ColVMJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Sparse_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  int bstart, bstop;
  for (bstart = 0; bstart < b.size; bstart++) {
    if (b.index[bstart] >= A.start) break;
  }
  for (bstop = b.size; bstop>0; bstop--) {
    if (b.index[bstop-1] < A.stop) break;
  }
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    for (s=A.stop-1; s>=A.start; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=bstart; s<bstop; s++)  x[b.index[s]] = -b.value[s];
   
    // Vector-matrix multiply
    A.MultiplyByCols(x, old);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=A.start; s<A.stop; s++) {
      double tmp = x[s];
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * old[s];
      if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2>
void RowVMJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double* x, 
    const LS_Sparse_Vector <REAL1> &b,
    REAL2* old,
    const LS_Options &opts, 
    LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=A->GetSize()-1; s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=0; s<b.size; s++)  x[b.index[s]] = -b.value[s];
   
    // Vector-matrix multiply
    A->NoDiag_MultByRows(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by cols                       *
// ******************************************************************

template <class REAL1, class REAL2>
void ColVMJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double* x, 
    const LS_Sparse_Vector <REAL1> &b,
    REAL2* old,
    const LS_Options &opts, 
    LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=A->GetSize()-1; s>=0; s--) {
      old[s] = x[s];
      x[s] = 0;
    }
    for (s=0; s<b.size; s++)  x[b.index[s]] = -b.value[s];
   
    // Vector-matrix multiply
    A->NoDiag_MultByCols(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
            double delta = tmp - old[s];
            if (opts.use_relative) if (tmp) delta /= tmp;
            if (delta<0) delta = -delta;
            if (delta > maxerror) {
                maxerror = delta;
                if (maxerror >= opts.precision)
                  if (iters < opts.max_iters)
                    check = false;
            }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                                                                *
// *                    Ax = b, truncated full b                    *
// *                                                                *
// *                                                                *
// *                                                                *
// ******************************************************************


// ******************************************************************
// *                                                                *
// *                        Row Gauss-Seidel                        *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2>
void RowGS_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1.0;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2>
void RowGS_Axb_w(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2>  &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += x[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

template <class REAL>
void RowGS_Axb(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Full_Vector <REAL> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      news = A->SolveRow(s, x, tmp);
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}


// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

template <class REAL>
void RowGS_Axb_w(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Full_Vector <REAL> &b, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Gauss-Seidel", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      news = A->SolveRow(s, x, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                                                                *
// *                           Row Jacobi                           *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                    Explicit,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) old[s] = x[s];
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = 1;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowJacobi_Axb_w(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, A.start, A.stop);
    long s;
    maxerror = 0;
    for (s=A.stop-1; s>=A.start; s--) old[s] = x[s];
    const long* ci = A.colindex + A.rowptr[A.start];
    const REAL1* v = A.value + A.rowptr[A.start];
    bool check = (iters >= opts.min_iters);
    for (s=A.start; s<A.stop; s++) {
      double tmp;
      if (s < b.size)   tmp = -b.value[s];  
      else              tmp = 0.0;
      const long* cstop = A.colindex + A.rowptr[s+1];
      while (ci < cstop) {
        tmp += old[ci[0]] * v[0];
        ci++;
        v++;
      }
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * x[s];
      if (check) {
        double delta = tmp - x[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                    Abstract,  no relaxation                    *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Full_Vector <REAL1> &b, REAL2* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=0; s<b.size; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A->GetSize(); s++) {
      old[s] = x[s];
      x[s] = 0;
    }
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = x[s];
      news = A->SolveRow(s, old, tmp);
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Abstract, relaxation                      *
// ******************************************************************

template <class REAL1, class REAL2>
void RowJacobi_Axb_w(LS_Abstract_Matrix* const &A, 
    double *x, const LS_Full_Vector <REAL1> &b, REAL2* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Row Jacobi", iters, x, 0, A->GetSize());
    long s, news;
    maxerror = 0;
    for (s=0; s<b.size; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A->GetSize(); s++) {
      old[s] = x[s];
      x[s] = 0;
    }
    A->FirstRow(s);
    bool check = (iters >= opts.min_iters);
    for ( ; s<A->GetSize(); s=news) {
      double tmp = x[s];
      news = A->SolveRow(s, old, tmp);
      tmp *= opts.relaxation;
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
          maxerror = delta;
          if (maxerror >= opts.precision)
            if (iters < opts.max_iters)
              check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}



// ******************************************************************
// *                                                                *
// *                            VMJacobi                            *
// *                                                                *
// ******************************************************************

// ******************************************************************
// *                       Explicit,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void RowVMJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    long bstop = MIN(b.size, A.stop);
    for (s=A.start; s<bstop; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A.stop; s++) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A.MultiplyByRows(x, old);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    if (opts.use_relaxation) {
      for(s=A.start; s<A.stop; s++) {
        double tmp = x[s];
        tmp *= A.one_over_diag[s] * opts.relaxation;
        tmp += one_minus_omega * old[s];
        if (check) {
          double delta = tmp - old[s];
          if (opts.use_relative) if (tmp) delta /= tmp;
          if (delta<0) delta = -delta;
          if (delta > maxerror) {
              maxerror = delta;
              if (maxerror >= opts.precision)
                if (iters < opts.max_iters)
                  check = false;
          }
        } // if check
        x[s] = tmp;
      } // for s
    } else {
      for(s=A.start; s<A.stop; s++) {
        double tmp = x[s] * A.one_over_diag[s];
        if (check) {
          double delta = tmp - old[s];
          if (opts.use_relative) if (tmp) delta /= tmp;
          if (delta<0) delta = -delta;
          if (delta > maxerror) {
              maxerror = delta;
              if (maxerror >= opts.precision)
                if (iters < opts.max_iters)
                  check = false;
          }
        } // if check
        x[s] = tmp;
      } // for s
    } // if relaxation

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                      Explicit, by columns                      *
// ******************************************************************

template <class REAL1, class REAL2, class REAL3>
void ColVMJacobi_Axb(const LS_Internal_Matrix <REAL1> &A, 
    double *x, const LS_Full_Vector <REAL2> &b, REAL3* old, 
    const LS_Options &opts, LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, A.start, A.stop);
    long s;
    long bstop = MIN(b.size, A.stop);
    for (s=A.start; s<bstop; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A.stop; s++) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A.MultiplyByCols(x, old);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=A.start; s<A.stop; s++) {
      double tmp = x[s];
      tmp *= A.one_over_diag[s] * opts.relaxation;
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by rows                       *
// ******************************************************************

template <class REAL1, class REAL2>
void RowVMJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double* x, 
    const LS_Full_Vector <REAL1> &b,
    REAL2* old,
    const LS_Options &opts, 
    LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=0; s<b.size; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A->GetSize(); s++) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A->NoDiag_MultByRows(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                       Abstract,  by cols                       *
// ******************************************************************

template <class REAL1, class REAL2>
void ColVMJacobi_Axb(LS_Abstract_Matrix* const &A, 
    double* x, 
    const LS_Full_Vector <REAL1> &b,
    REAL2* old,
    const LS_Options &opts, 
    LS_Output &out)
{
  long iters;
  double one_minus_omega = 1.0 - opts.relaxation;
  double maxerror = 0;
  for (iters=1; iters<=opts.max_iters; iters++) {
    if (opts.debug)  DebugIter("Jacobi", iters, x, 0, A->GetSize());
    long s;
    for (s=0; s<b.size; s++) {
      old[s] = x[s];
      x[s] = -b.value[s];
    }
    for (; s<A->GetSize(); s++) {
      old[s] = x[s];
      x[s] = 0;
    }
   
    // Vector-matrix multiply
    A->NoDiag_MultByCols(old, x);

    // diagonal adjust
    A->DivideDiag(x, opts.relaxation);

    // complete the iteration    
    maxerror = 0;
    bool check = (iters >= opts.min_iters);
    for(s=0; s<A->GetSize(); s++) {
      double tmp = x[s];
      tmp += one_minus_omega * old[s];
      if (check) {
        double delta = tmp - old[s];
        if (opts.use_relative) if (tmp) delta /= tmp;
        if (delta<0) delta = -delta;
        if (delta > maxerror) {
            maxerror = delta;
            if (maxerror >= opts.precision)
              if (iters < opts.max_iters)
                check = false;
        }
      } // if check
      x[s] = tmp;
    } // for s

    if (iters < opts.min_iters) continue;
    if (maxerror < opts.precision) break; 
  } // for iters
  out.num_iters = iters;
  out.precision = maxerror;
  out.relaxation = opts.relaxation;
  if (iters <= opts.max_iters) {
    out.status = LS_Success;
  } else {
    out.status = LS_No_Convergence;
  }
}

// ******************************************************************
// *                                                                *
// *                                                                *
// *                  template frontend  functions                  *
// *                                                                *
// *                                                                *
// ******************************************************************

// Decide which solver to call.
template <class MATRIX>
void Ax0_Solver(const MATRIX &A, bool Ais_transposed, long Asize,
    double *x, const LS_Options &opts, LS_Output &out)
{
  float* fold;
  double* dold;
  switch (opts.method) {
    case LS_Power:
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
              ColPower_Ax0(A, x, fold, opts, out);
            else 
              RowPower_Ax0(A, x, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
              ColPower_Ax0(A, x, dold, opts, out);
            else 
              RowPower_Ax0(A, x, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
              ColVMJacobi_Ax0(A, x, fold, opts, out);
            else 
              RowVMJacobi_Ax0(A, x, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
              ColVMJacobi_Ax0(A, x, dold, opts, out);
            else 
              RowVMJacobi_Ax0(A, x, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Row_Jacobi:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
            return;
        } 
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation)
              RowJacobi_Ax0_w(A, x, fold, opts, out);
            else
              RowJacobi_Ax0(A, x, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation)
              RowJacobi_Ax0_w(A, x, dold, opts, out);
            else
              RowJacobi_Ax0(A, x, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
            return;
        } 
        if (opts.use_relaxation)
            RowGS_Ax0_w(A, x, opts, out);
        else
            RowGS_Ax0(A, x, opts, out);
        return;    

    default:
        out.status = LS_Not_Implemented;
        return;
  } 
}


// Decide which solver to call.
template <class MATRIX, class VECTOR>
void Axb_Solver(const MATRIX &A, bool Ais_transposed, long Asize,
      double *x, const VECTOR &b, 
      const LS_Options &opts, LS_Output &out)
{
  float* fold;
  double* dold;
  switch (opts.method) {
    case LS_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
              ColVMJacobi_Axb(A, x, b, fold, opts, out);
            else 
              RowVMJacobi_Axb(A, x, b, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (Ais_transposed)
                ColVMJacobi_Axb(A, x, b, dold, opts, out);
            else 
                RowVMJacobi_Axb(A, x, b, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Row_Jacobi:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
            return;
        } 
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation)
                RowJacobi_Axb_w(A, x, b, fold, opts, out);
            else
                RowJacobi_Axb(A, x, b, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation)
                RowJacobi_Axb_w(A, x, b, dold, opts, out);
            else
                RowJacobi_Axb(A, x, b, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
        } else {
            if (opts.use_relaxation)
              RowGS_Axb_w(A, x, b, opts, out);
            else
              RowGS_Axb(A, x, b, opts, out);
        }
        return;    

    default:
        out.status = LS_Illegal_Method;
        return;
  } 
}


// Decide between sparse and full b vector.
template <class MATRIX>
void Axb_VectorExpand(const MATRIX &A, bool Ais_transposed, long Asize,
      double *x, const LS_Vector &b, 
      const LS_Options &opts, LS_Output &out)
{
  if (b.index) {
      // SPARSE
      if (b.f_value) {
        LS_Sparse_Vector <float> Myb;
        Myb.Fill(b);
        Axb_Solver(A, Ais_transposed, Asize, x, Myb, opts, out);
      } else {
        LS_Sparse_Vector <double> Myb;
        Myb.Fill(b);
        Axb_Solver(A, Ais_transposed, Asize, x, Myb, opts, out);
      }
  } else {
      // FULL
      if (b.f_value) {
        LS_Full_Vector <float> Myb;
        Myb.Fill(b);
        Axb_Solver(A, Ais_transposed, Asize, x, Myb, opts, out);
      } else {
        LS_Full_Vector <double> Myb;
        Myb.Fill(b);
        Axb_Solver(A, Ais_transposed, Asize, x, Myb, opts, out);
      }
  }
}


// ******************************************************************
// *                                                                *
// *                                                                *
// *                       frontend functions                       *
// *                                                                *
// *                                                                *
// ******************************************************************

const char* LS_LibraryVersion()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Linear Solver Library, version %d.%d.%d",
     MAJOR_VERSION, MINOR_VERSION,  REVISION_NUMBER);
  return buffer;
}

void Solve_AxZero(const LS_Matrix &A, double* x, const LS_Options &opts, LS_Output &out)
{
  if (opts.debug) {
    DebugMatrix(A);
  }
  bool ait = A.is_transposed;
  long asize = A.stop;
  // Try values as doubles
  if (A.d_value || 0==A.rowptr[asize]) if (A.d_one_over_diag) {
      LS_Internal_Matrix <double> MyA;
      MyA.Fill(A);
      Ax0_Solver(MyA, ait, asize, x, opts, out);
      return;
  } 
  // Try values as floats
  if (A.f_value || 0==A.rowptr[asize]) if (A.f_one_over_diag) {
      LS_Internal_Matrix <float> MyA;
      MyA.Fill(A);
      Ax0_Solver(MyA, ait, asize, x, opts, out);
      return;
  } 
  out.status = LS_Wrong_Format;
}

void Solve_AxZero(LS_Abstract_Matrix *A, double* x, const LS_Options &opts, LS_Output &out)
{
  bool ait = A->IsTransposed();
  long asize = A->GetSize();
  Ax0_Solver(A, ait, asize, x, opts, out);
}


void Solve_Axb(const LS_Matrix &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  if (opts.debug) {
    DebugMatrix(A);
    DebugVector(b);
  }
  bool ait = A.is_transposed;
  long asize = A.stop;
  // Try doubles
  if (A.d_value || 0==A.rowptr[asize]) if (A.d_one_over_diag) {
      LS_Internal_Matrix <double> MyA;
      MyA.Fill(A);
      Axb_VectorExpand(MyA, ait, asize, x, b, opts, out);
      return;
  }
  // Try floats
  if (A.f_value || 0==A.rowptr[asize]) if (A.f_one_over_diag) {
      LS_Internal_Matrix <float> MyA;
      MyA.Fill(A);
      Axb_VectorExpand(MyA, ait, asize, x, b, opts, out);
      return;
  }
  out.status = LS_Wrong_Format;
}


void Solve_Axb(LS_Abstract_Matrix* A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  bool ait = A->IsTransposed();
  long asize = A->GetSize();
  Axb_VectorExpand(A, ait, asize, x, b, opts, out);
}

