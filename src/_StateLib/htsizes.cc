
// 
// Not part of the library, but used to determine
// hard-coded hash table sizes.
//

#include <stdio.h>

bool is_prime(long n)
{
  if (2==n) return true;
  if (3==n) return true;
  if (5==n) return true;
  if (7==n) return true;
  if (0==n%2) return false;
  if (0==n%3) return false;
  if (0==n%5) return false;
  if (0==n%7) return false;
  
  long one = 11;
  long three = 13;
  long seven = 17;
  long nine = 19;

  while (one * one <= n) {
    if (0==n%one)   return false;
    if (0==n%three) return false;
    if (0==n%seven) return false;
    if (0==n%nine)  return false;
    one += 10;
    three += 10;
    seven += 10;
    nine += 10;
  }

  return true;
}

long next_prime(long n) 
{
  while (!is_prime(n)) n++;
  return n;
}



int main()
{
  fprintf(stderr, "Hash table sequence of primes.\n");
  fprintf(stderr, "Each term will be prime, and at least n/d times larger than previous.\n");
  long term;
  long n;
  long d;
  fprintf(stderr, "Enter starting value:\n");
  scanf("%ld", &term);
  fprintf(stderr, "Enter n:\n");
  scanf("%ld", &n);
  fprintf(stderr, "Enter d:\n");
  scanf("%ld", &d);

  printf("Sequence of prime table sizes:\n");
  term = next_prime(term);
  printf("%ld\n", term);

  for (;;) {
    long tmp = term * n;
    if (tmp < term) return 0;
    term = tmp / d;
    term = next_prime(term);
    printf("%ld\n", term);
  }
  
}
