
// $Id$

#include "lslib.h"

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <assert.h>

#include "revision.h"

#include "debug.hh"

#include "row_gs_ax0.hh"
#include "row_jac_ax0.hh"
#include "vmm_jac_ax0.hh"

#include "row_gs_axb.hh"
#include "row_jac_axb.hh"
#include "vmm_jac_axb.hh"

const int MAJOR_VERSION = 1;
const int MINOR_VERSION = 5;

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }


// ******************************************************************
// *                       LS_Matrix  methods                       *
// ******************************************************************

template <typename R1, typename R2>
inline void VMM_t(const long* rp, const long* rpstop, const long* colindex,
  const R1* v, double* y, const R2* x)
{
  const long* ci = colindex + rp[0];
  v += rp[0];

  // multiplication, matrix by columns

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
}


template <typename R1, typename R2>
inline void VMM_r(const long* rp, const long* rpstop, const long* colindex,
  const R1* v, double* y, const R2* x)
{
  const long* ci = colindex + rp[0];
  v += rp[0];

  // multiplication, matrix by rows
  
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


void LS_Matrix::VMM_transposed(double *y, const double* x) const
{
  if (d_value) 
    return VMM_t(rowptr + start, rowptr + stop, colindex, d_value, y, x);

  if (f_value)
    return VMM_t(rowptr + start, rowptr + stop, colindex, f_value, y, x);
}

void LS_Matrix::VMM_transposed(double *y, const float* x) const
{
  if (d_value) 
    return VMM_t(rowptr + start, rowptr + stop, colindex, d_value, y, x);

  if (f_value)
    return VMM_t(rowptr + start, rowptr + stop, colindex, f_value, y, x);
}

void LS_Matrix::VMM_regular(double *y, const double* x) const
{
  if (d_value)
    return VMM_r(rowptr + start, rowptr + stop, colindex, d_value, y, x);

  if (f_value)
    return VMM_r(rowptr + start, rowptr + stop, colindex, f_value, y, x);
}

void LS_Matrix::VMM_regular(double *y, const float* x) const
{
  if (d_value)
    return VMM_r(rowptr + start, rowptr + stop, colindex, d_value, y, x);

  if (f_value)
    return VMM_r(rowptr + start, rowptr + stop, colindex, f_value, y, x);
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

  inline long Start() const { return start; }
  inline long Stop() const { return stop; }

  // for convenience
  inline void Fill(const LS_Matrix &A);
  inline void FillOthers(const LS_Matrix &A) {
    is_transposed = A.is_transposed;
    start = A.start;
    stop = A.stop;
    rowptr = A.rowptr;
    colindex = A.colindex;
  }

  /*
      Compute y += (this without diagonals) * old
  */
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

  /*
      Compute y += (this without diagonals) * old
  */
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

  /*
      Compute y += (this without diagonals) * old
      Call this when we don't know how the matrix is stored
  */
  template <class REAL2>
  inline void Multiply(double *y, const REAL2* old) const {
    if (is_transposed)  MultiplyByCols(y, old);
    else                MultiplyByRows(y, old);
  }


  /*
      Compute x[i] *= 1 / diagonal[i], for all i 
  */
  inline void DivideDiag(double* x) const {
    for (long i=start; i<stop; i++) {
      x[i] *= one_over_diag[i];
    }
  }

  /*
      Compute x[i] *= a / diagonal[i], for all i 
  */
  inline void DivideDiag(double* x, double a) const {
    for (long i=start; i<stop; i++) {
      x[i] *= one_over_diag[i] * a;
    }
  }


  /*
      Compute sum += x * (row "index" of this matrix)
  */
  template <class REAL2>
  inline void ColumnDotProduct(long index, const REAL2* x, double &sum) const {
    DEBUG_ASSERT(is_transposed);
    long astop = rowptr[index+1];
    for (long a = rowptr[index]; a < astop; a++) {
      sum += x[colindex[a]] * value[a];
    }
  }

  template <class REAL2>
  inline void SolveRow(long index, const REAL2* x, double &sum) const {
    ColumnDotProduct(index, x, sum);
    sum *= one_over_diag[index];
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
  inline void FirstIndex(long &p, long start) const {
    for (p=0; p<size; p++) {
      if (index[p] >= start) return;
    }
  }
  inline double GetNegValue(long &p, long s) const {
    if (p<size) if (s == index[p]) {
      return -value[p++];
    }
    return 0;
  }
  inline void CopyNegativeToFull(double* x, long start, long stop) const {
    for (long i=0; i<size; i++) {
      if (index[i] >= stop) return;
      if (index[i] < start) continue;
      x[index[i]] = -value[i];
    }
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
  inline void FirstIndex(long &, long) const {
    // NO-OP
  }
  inline double GetNegValue(long &, long s) const {
    if (s < size) return -value[s];
    return 0;
  }
  inline void CopyNegativeToFull(double *x, long start, long stop) const {
    long istop = MIN(size, stop);
    for (long i=MAX(long(0), start); i<istop; i++) {
      x[i] = -value[i];
    }
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

LS_Abstract_Matrix::LS_Abstract_Matrix(long sta, long sto, long s)
{
  start = sta;
  stop = sto;
  size = s;
}

LS_Abstract_Matrix::~LS_Abstract_Matrix()
{
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
        out.status = LS_Not_Implemented;
        return;


    case LS_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(Asize * sizeof(float));
            if (NULL==fold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation) {
                New_VMMJacobi_Ax0<true>(A, x, fold, opts, out);
            } else {
                New_VMMJacobi_Ax0<false>(A, x, fold, opts, out);
            }
            /*
            if (Ais_transposed)
              ColVMJacobi_Ax0(A, x, fold, opts, out);
            else 
              RowVMJacobi_Ax0(A, x, fold, opts, out);
            */
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation) {
                New_VMMJacobi_Ax0<true>(A, x, dold, opts, out);
            } else {
                New_VMMJacobi_Ax0<false>(A, x, dold, opts, out);
            }
            /*
            if (Ais_transposed)
              ColVMJacobi_Ax0(A, x, dold, opts, out);
            else 
              RowVMJacobi_Ax0(A, x, dold, opts, out);
            */
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
            if (opts.use_relaxation) {
                New_RowJacobi_Ax0<true>(A, x, fold, opts, out);
            } else {
                New_RowJacobi_Ax0<false>(A, x, fold, opts, out);
            }
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation) {
                New_RowJacobi_Ax0<true>(A, x, dold, opts, out);
            } else {
                New_RowJacobi_Ax0<false>(A, x, dold, opts, out);
            }
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
            return;
        } 
        if (opts.use_relaxation) {
            New_RowGS_Ax0<true>(A, x, opts, out);
        } else {
            New_RowGS_Ax0<false>(A, x, opts, out);
        }
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
            if (opts.use_relaxation) {
                New_VMMJacobi_Axb<true>(A, x, b, fold, opts, out);
            } else {
                New_VMMJacobi_Axb<false>(A, x, b, fold, opts, out);
            }
            /*
            if (Ais_transposed)
              ColVMJacobi_Axb(A, x, b, fold, opts, out);
            else 
              RowVMJacobi_Axb(A, x, b, fold, opts, out);
            */
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation) {
                New_VMMJacobi_Axb<true>(A, x, b, dold, opts, out);
            } else {
                New_VMMJacobi_Axb<false>(A, x, b, dold, opts, out);
            }
            /*
            if (Ais_transposed)
                ColVMJacobi_Axb(A, x, b, dold, opts, out);
            else 
                RowVMJacobi_Axb(A, x, b, dold, opts, out);
                */
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
                New_RowJacobi_Axb<true>(A, x, b, fold, opts, out);
            else
                New_RowJacobi_Axb<false>(A, x, b, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(Asize * sizeof(double));
            if (NULL==dold) {
              out.status = LS_Out_Of_Memory;
              return;
            } 
            if (opts.use_relaxation)
                New_RowJacobi_Axb<true>(A, x, b, dold, opts, out);
            else
                New_RowJacobi_Axb<false>(A, x, b, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
        if (Ais_transposed) {
            out.status = LS_Wrong_Format;
        } else {
            if (opts.use_relaxation)
              New_RowGS_Axb<true>(A, x, b, opts, out);
            else
              New_RowGS_Axb<false>(A, x, b, opts, out);
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

void Solve_AxZero(const LS_Abstract_Matrix &A, double* x, const LS_Options &opts, LS_Output &out)
{
  bool ait = A.IsTransposed();
  long asize = A.GetSize();
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


void Solve_Axb(const LS_Abstract_Matrix &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  bool ait = A.IsTransposed();
  long asize = A.GetSize();
  Axb_VectorExpand(A, ait, asize, x, b, opts, out);
}

