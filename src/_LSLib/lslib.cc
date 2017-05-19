
#include "lslib.h"

#include <stdlib.h> 
#include <stdio.h> 
#include <string.h>
#include <assert.h>

/// Standard MIN "macro".
template <class T> inline T MIN(T X,T Y) { return ((X<Y)?X:Y); }
/// Standard MAX "macro".
template <class T> inline T MAX(T X,T Y) { return ((X>Y)?X:Y); }


#include "vectors.hh"
#include "debug.hh"

#include "row_gs_ax0.hh"
#include "row_jac_ax0.hh"
#include "vmm_jac_ax0.hh"

#include "row_gs_axb.hh"
#include "row_jac_axb.hh"
#include "vmm_jac_axb.hh"

const int MAJOR_VERSION = 2;
const int MINOR_VERSION = 1;

// ******************************************************************
// *                                                                *
// *                   LS_Generic_Matrix  methods                   *
// *                                                                *
// ******************************************************************

LS_Generic_Matrix::LS_Generic_Matrix(long sta, long sto, long siz)
{
  start = sta;
  stop = sto;
  size = siz;
  one_over_diag = 0;
}

LS_Generic_Matrix::~LS_Generic_Matrix()
{
}

void LS_Generic_Matrix::ShowDebugInfo() const
{
  printf("  LS_Generic_Matrix\n");
}

void LS_Generic_Matrix::RowDotProduct(long, const float*, double &) const
{
  throw LS_Wrong_Format;
}

void LS_Generic_Matrix::RowDotProduct(long, const double*, double &) const
{
  throw LS_Wrong_Format;
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
void Ax0_Solver(const MATRIX &A, double *x, const LS_Options &opts, LS_Output &out)
{
  if (opts.debug) {
    DebugMatrix(A);
  }
  float* fold;
  double* dold;
  switch (opts.method) {
    case LS_Power:
        out.status = LS_Not_Implemented;
        return;


    case LS_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(A.Size() * sizeof(float));
            if (NULL==fold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_VMMJacobi_Ax0<true>(A, x, fold, opts, out);
            } else {
                New_VMMJacobi_Ax0<false>(A, x, fold, opts, out);
            }
            free(fold);
        } else { 
            dold = (double*) malloc(A.Size() * sizeof(double));
            if (NULL==dold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_VMMJacobi_Ax0<true>(A, x, dold, opts, out);
            } else {
                New_VMMJacobi_Ax0<false>(A, x, dold, opts, out);
            }
            free(dold);
        } 
        return;    

    case LS_Row_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(A.Size() * sizeof(float));
            if (NULL==fold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_RowJacobi_Ax0<true>(A, x, fold, opts, out);
            } else {
                New_RowJacobi_Ax0<false>(A, x, fold, opts, out);
            }
            free(fold);
        } else { 
            dold = (double*) malloc(A.Size() * sizeof(double));
            if (NULL==dold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_RowJacobi_Ax0<true>(A, x, dold, opts, out);
            } else {
                New_RowJacobi_Ax0<false>(A, x, dold, opts, out);
            }
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
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


// ******************************************************************


// Decide which solver to call.
template <class MATRIX, class VECTOR>
void Axb_Solver(const MATRIX &A, double *x, const VECTOR &b, 
      const LS_Options &opts, LS_Output &out)
{
  if (opts.debug) {
    DebugMatrix(A);
    DebugVector(b);
  }
  float* fold;
  double* dold;
  switch (opts.method) {
    case LS_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(A.Size() * sizeof(float));
            if (NULL==fold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_VMMJacobi_Axb<true>(A, x, b, fold, opts, out);
            } else {
                New_VMMJacobi_Axb<false>(A, x, b, fold, opts, out);
            }
            free(fold);
        } else { 
            dold = (double*) malloc(A.Size() * sizeof(double));
            if (NULL==dold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation) {
                New_VMMJacobi_Axb<true>(A, x, b, dold, opts, out);
            } else {
                New_VMMJacobi_Axb<false>(A, x, b, dold, opts, out);
            }
            free(dold);
        } 
        return;    

    case LS_Row_Jacobi:
        if (opts.float_vectors) {
            fold = (float*) malloc(A.Size() * sizeof(float));
            if (NULL==fold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation)
                New_RowJacobi_Axb<true>(A, x, b, fold, opts, out);
            else
                New_RowJacobi_Axb<false>(A, x, b, fold, opts, out);
            free(fold);
        } else { 
            dold = (double*) malloc(A.Size() * sizeof(double));
            if (NULL==dold) throw LS_Out_Of_Memory;
            if (opts.use_relaxation)
                New_RowJacobi_Axb<true>(A, x, b, dold, opts, out);
            else
                New_RowJacobi_Axb<false>(A, x, b, dold, opts, out);
            free(dold);
        } 
        return;    

    case LS_Gauss_Seidel:
        if (opts.use_relaxation) {
            New_RowGS_Axb<true>(A, x, b, opts, out);
        } else {
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
void Axb_VectorExpand(const MATRIX &A, double *x, const LS_Vector &b, 
      const LS_Options &opts, LS_Output &out)
{
  if (b.index) {
      // SPARSE
      if (b.f_value) {
        LS_Sparse_Vector <float> Myb;
        Myb.Fill(b);
        Axb_Solver(A, x, Myb, opts, out);
      } else {
        LS_Sparse_Vector <double> Myb;
        Myb.Fill(b);
        Axb_Solver(A, x, Myb, opts, out);
      }
  } else {
      // FULL
      if (b.f_value) {
        LS_Full_Vector <float> Myb;
        Myb.Fill(b);
        Axb_Solver(A, x, Myb, opts, out);
      } else {
        LS_Full_Vector <double> Myb;
        Myb.Fill(b);
        Axb_Solver(A, x, Myb, opts, out);
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
  snprintf(buffer, sizeof(buffer), "Linear Solver Library, version %d.%d",
     MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number mechanism
}

// ******************************************************************

void Solve_AxZero(const LS_CRS_Matrix_float &A, double* x, const LS_Options &opts, LS_Output &out)
{
  try {
    Ax0_Solver(A, x, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  };
}

void Solve_AxZero(const LS_CRS_Matrix_double &A, double* x, const LS_Options &opts, LS_Output &out)
{
  try {
    Ax0_Solver(A, x, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  };
}

void Solve_AxZero(const LS_CCS_Matrix_float &A, double* x, const LS_Options &opts, LS_Output &out)
{
  try {
    Ax0_Solver(A, x, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  };
}

void Solve_AxZero(const LS_CCS_Matrix_double &A, double* x, const LS_Options &opts, LS_Output &out)
{
  try {
    Ax0_Solver(A, x, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  };
}

void Solve_AxZero(const LS_Generic_Matrix &A, double* x, const LS_Options &opts, LS_Output &out)
{
  try {
    Ax0_Solver(A, x, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  };
}

// ******************************************************************

void Solve_Axb(const LS_CRS_Matrix_float &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  try {
    Axb_VectorExpand(A, x, b, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  }
}

void Solve_Axb(const LS_CRS_Matrix_double &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  try {
    Axb_VectorExpand(A, x, b, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  }
}

void Solve_Axb(const LS_CCS_Matrix_float &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  try {
    Axb_VectorExpand(A, x, b, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  }
}

void Solve_Axb(const LS_CCS_Matrix_double &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  try {
    Axb_VectorExpand(A, x, b, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  }
}

void Solve_Axb(const LS_Generic_Matrix &A, double* x, const LS_Vector &b, const LS_Options &opts, LS_Output &out)
{
  try {
    Axb_VectorExpand(A, x, b, opts, out);
  }
  catch (LS_Error e) {
    out.status = e;
  }
}


