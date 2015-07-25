
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

#endif

