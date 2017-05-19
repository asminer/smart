
// $Id$

/*
    Classic algorithm to find all primes between 2 and a constant N,
    based on the sieve of Eratosthenes.
*/

// #define SHOW_PRIMES

#include <stdio.h>
#include "intset.h"

// find the smallest integer x such that x*x > n
long mysqrt(long n)
{
  // do a "binary search"
  long hi = n;
  long lo = 1;
  while (hi>lo) {
    long mid = (hi+lo)/2;
    long msq = mid*mid;
    if (msq == n) return mid+1;
    if (msq > n)
      hi = mid;
    else
      lo = mid+1;
  }
  return lo;
}

int main()
{
  long N;
  long n, i, stop;
  printf("Enter size of sieve: \n");
  scanf("%ld", &N);
  if (N<2) return 0;

  printf("Finding primes between 2 and %ld...\n", N);

  intset prime(N+1);
  prime.addAll();
  prime.removeElement(0);
  prime.removeElement(1);
  stop = mysqrt(N);

  for (n=2; n < stop; n++) 
    if (prime.contains(n))
      for (i=2*n; i<=N; i+=n)  // cross off 2n, 3n, 4n, ...
        prime.removeElement(i);

#ifdef SHOW_PRIMES
  printf("\nThe primes between 2 and %ld are\n\n", N);
#endif

  i = 0;
  for (n=2; n <= N; n++)
    if (prime.contains(n)) {
      i++;
#ifdef SHOW_PRIMES
      printf("%15d", n);
      if (i % 5 == 0) printf("\n");
#endif
    }
  printf("\n\nBy my count, there were %ld primes\n", i);

  return 0;
}
