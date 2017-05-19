
// $Id$

#ifndef DEBUG_HH
#define DEBUG_HH

// #define DEBUG

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
// *                  LS_Generic_Matrix  debugging                  *
// ******************************************************************

void DebugMatrix(const LS_Generic_Matrix &A)
{
  printf("Using A matrix:\n");
  printf("\n  start: %ld\n  stop: %ld\n", A.Start(), A.Stop());
  A.ShowDebugInfo();
}

// ******************************************************************
// *                    LS_CRS_Matrix  debugging                    *
// ******************************************************************

template <class REAL>
void DebugMatrix(const LS_CRS_Matrix<REAL> &A)
{
  printf("Using A matrix:\n");
  printf("\n  start: %ld\n  stop: %ld\n  row_ptr: [", A.start, A.stop);
  for (long i=0; i<=A.stop; i++) {
    if (i) printf(", ");
    printf("%ld", A.row_ptr[i]);
  }
  printf("]\n  col_ind: ");
  if (A.col_ind) {
    printf("[");
    for (long i=0; i<A.row_ptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%ld", A.col_ind[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  val: ");
  if (A.val) {
    printf("[");
    for (long i=0; i<A.row_ptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%lg", double(A.val[i]));
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  one_over_diag: ");
  if (A.one_over_diag) {
    printf("[");
    for (long i=0; i<A.stop; i++) {
      if (i) printf(", ");
      printf("%lg", double(A.one_over_diag[i]));
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}

// ******************************************************************
// *                    LS_CCS_Matrix  debugging                    *
// ******************************************************************

template <class REAL>
void DebugMatrix(const LS_CCS_Matrix<REAL> &A)
{
  printf("Using A matrix:\n");
  printf("\n  start: %ld\n  stop: %ld\n  col_ptr: [", A.start, A.stop);
  for (long i=0; i<=A.stop; i++) {
    if (i) printf(", ");
    printf("%ld", A.col_ptr[i]);
  }
  printf("]\n  row_ind: ");
  if (A.row_ind) {
    printf("[");
    for (long i=0; i<A.col_ptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%ld", A.row_ind[i]);
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  val: ");
  if (A.val) {
    printf("[");
    for (long i=0; i<A.col_ptr[A.stop]; i++) {
      if (i) printf(", ");
      printf("%lg", double(A.val[i]));
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  one_over_diag: ");
  if (A.one_over_diag) {
    printf("[");
    for (long i=0; i<A.stop; i++) {
      if (i) printf(", ");
      printf("%lg", double(A.one_over_diag[i]));
    }
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}


// ******************************************************************
// *                   LS_Sparse_Vector debugging                   *
// ******************************************************************

template <class REAL>
void DebugVector(const LS_Sparse_Vector<REAL> &b)
{
  printf("Using sparse b vector:\n");
  printf("  size: %ld\n", b.size);
  printf("  index: ");
  if (b.index) {
    printf("[%ld", b.index[0]);
    for (long i=1; i<b.size; i++) printf(", %ld", b.index[i]);
    printf("]");
  } else {
    printf("null");
  }
  printf("\n  value: ");
  if (b.value) {
    printf("[%lg", double(b.value[0]));
    for (long i=1; i<b.size; i++) printf(", %lg", double(b.value[i]));
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}

// ******************************************************************
// *                    LS_Full_Vector debugging                    *
// ******************************************************************

template <class REAL>
void DebugVector(const LS_Full_Vector<REAL> &b)
{
  printf("Using full b vector:\n");
  printf("  size: %ld\n", b.size);
  printf("  value: ");
  if (b.value) {
    printf("[%lg", double(b.value[0]));
    for (long i=1; i<b.size; i++) printf(", %lg", double(b.value[i]));
    printf("]");
  } else {
    printf("null");
  }
  printf("\n");
}

// ******************************************************************
// *                      LS_Matrix  debugging                      *
// ******************************************************************

#if 0

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
#endif


#endif

