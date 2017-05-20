
/*
  Implementation of the result class.
*/

#include "../include/defines.h"
#include "result.h"

#include "../ExprLib/exprman.h"
#include "../include/heap.h"

// #define DEBUG_SORTS

#ifdef DEBUG_MEM
#include <stdio.h>
#endif

// ******************************************************************
// *                         result methods                         *
// ******************************************************************

result::result()
{
#ifdef DEBUG_MEM
  printf("constructing result 0x%x\n", unsigned(this));
#endif
  other = 0;
  special = Normal;
}

result::result(long a)
{
#ifdef DEBUG_MEM
  printf("constructing result 0x%x\n", unsigned(this));
#endif
  special = Normal;
  ivalue = a;
  other = 0;
}

result::result(double a)
{
#ifdef DEBUG_MEM
  printf("constructing result 0x%x\n", unsigned(this));
#endif
  special = Normal;
  rvalue = a;
  other = 0;
}

result::result(shared_object* a)
{
#ifdef DEBUG_MEM
  printf("constructing result 0x%x\n", unsigned(this));
#endif
  special = Normal;
  other = a;
}

result::result(const result& x)
{
  constructFrom(x);
}

result::~result()
{
#ifdef DEBUG_MEM
  printf("destructing  result 0x%x\n", unsigned(this));
  if (other)  printf("\tother: 0x%x\n", unsigned(other));
#endif
  Delete(other);
}

void result::constructFrom(const result& x)
{
#ifdef DEBUG_MEM
  printf("initializing result 0x%x\n", unsigned(this));
#endif
  special = x.special;
  ivalue = x.ivalue;
  rvalue = x.rvalue;
  other = Share(x.other);
}

void result::operator= (const result& x)
{
  if (this != &x) {
#ifdef DEBUG_MEM
    printf("destroying   result 0x%x\n", unsigned(this));
    if (other)  printf("\tother 0x%x\n", unsigned(other));
#endif
    Delete(other);
    constructFrom(x);
  }
}

// ******************************************************************
// *                                                                *
// *                     code for  sortIntegers                     *
// *                                                                *
// ******************************************************************

class intresult_sorter {
    result* samples;
  public:
    intresult_sorter(result* A) {
      samples = A;
    };

    // for abstract heapsort
    inline int Compare(long i, long j) const {
      DCASSERT(samples);

      // Both finite
      if (samples[i].isNormal() && samples[j].isNormal()) {
        return samples[i].getInt() - samples[j].getInt();
      }

      // Both infinite
      if (samples[i].isInfinity() && samples[j].isInfinity()) {
        return samples[i].signInfinity() - samples[j].signInfinity();
      }

      if (samples[i].isInfinity()) {
        return samples[i].signInfinity();
      }

      if (samples[j].isInfinity()) {
        return -samples[i].signInfinity();
      }

      // NULL or unknown value(s) - behavior here is undefined
      return 0;
    }

    inline void Swap(long i, long j) const {
      SWAP(samples[i], samples[j]);
    }
};

void sortIntegers(result* A, int n)
{
  intresult_sorter foo(A);
  HeapSortAbstract(&foo, n);

#ifdef DEBUG_SORTS
  printf("Sorted integers: [");
  for (int i=0; i<n; i++) {
    if (i) printf(", ");
    if (A[i].isNormal()) {
      printf("%d", A[i].getInt());
      continue;
    }
    if (A[i].isInfinity()) {
      if (A[i].signInfinity()>0)
        printf("+oo");
      else
        printf("-oo");
      continue;
    }
    if (A[i].isUnknown()) {
      printf("?");
      continue;
    }
    if (A[i].isNull()) {
      printf("null");
      continue;
    }
    printf("unknown case");
  }
  printf("]\n");
#endif
}

// ******************************************************************
// *                                                                *
// *                       code for sortReals                       *
// *                                                                *
// ******************************************************************

class realresult_sorter {
    result* samples;
  public:
    realresult_sorter(result* A) {
      samples = A;
    };

    // for abstract heapsort
    inline int Compare(long i, long j) const {
      DCASSERT(samples);

      // Both finite
      if (samples[i].isNormal() && samples[j].isNormal()) {
        return SIGN(samples[i].getReal() - samples[j].getReal());
      }

      // Both infinite
      if (samples[i].isInfinity() && samples[j].isInfinity()) {
        return samples[i].signInfinity() - samples[j].signInfinity();
      }

      if (samples[i].isInfinity()) {
        return samples[i].signInfinity();
      }

      if (samples[j].isInfinity()) {
        return -samples[i].signInfinity();
      }

      // NULL or unknown value(s) - behavior here is undefined
      return 0;
    }

    inline void Swap(long i, long j) const {
      SWAP(samples[i], samples[j]);
    }
};

void sortReals(result* A, int n)
{
  realresult_sorter foo(A);
  HeapSortAbstract(&foo, n);

#ifdef DEBUG_SORTS
  printf("Sorted reals: [");
  for (int i=0; i<n; i++) {
    if (i) printf(", ");
    if (A[i].isNormal()) {
      printf("%f", A[i].getReal());
      continue;
    }
    if (A[i].isInfinity()) {
      if (A[i].signInfinity()>0)
        printf("+oo");
      else
        printf("-oo");
      continue;
    }
    if (A[i].isUnknown()) {
      printf("?");
      continue;
    }
    if (A[i].isNull()) {
      printf("null");
      continue;
    }
    printf("unknown case");
  }
  printf("]\n");
#endif
}

