
/* $Id$ */

/* 
    RNG Library test: 
    generate a random sequence of (x,y) pairs

*/

#include "rng.h"

#include <stdio.h>
#include <stdlib.h>

void Usage(const char* name)
{
  printf("Usage: %s (opts) \n\n", name);
  printf("  Dumps pairs (x,y) to standard output, one pair per line.\n");
  printf("  The x and y coordinates are independent Uniform(0,1) RVs.\n");
  printf("\nRequired arguments:\n");
  printf("  -n number-of-points\n");
  printf("\nOptional arguments:\n");
  printf("  -l lower-bound (default is 0)\n");
  printf("  -u upper-bound (default is 1)\n");
  printf("\n");
}

int main(int argc, char** argv)
{
  int N = -1;
  double lower = 0.0;
  double upper = 1.0;
  int i;
  for (i=1; i<argc; i++) {
    if (argv[i][0] != '-') {
      Usage(argv[0]);
      return 1;
    }
    switch (argv[i][1]) {
      case 'n':
          i++;
          if (i>=argc) {
            Usage(argv[0]);
            return 1;
          }
          N = strtol(argv[i], NULL, 10);
          break;

      case 'l':
          i++;
          if (i>=argc) {
            Usage(argv[0]);
            return 1;
          }
          lower = strtod(argv[i], NULL);
          break;

      case 'u':
          i++;
          if (i>=argc) {
            Usage(argv[0]);
            return 1;
          }
          upper = strtod(argv[i], NULL);
          break;

      default:
          Usage(argv[0]);
          return 1;
    }
  }
  if (N<0) {
    Usage(argv[0]);
    return 1;
  }

  if (upper <= lower) {
    Usage(argv[0]);
    return 1;
  }

  rng_manager* rm = RNG_MakeStreamManager();
  const int seed = 123456789;
  rng_stream* s = rm->NewStreamFromSeed(seed);
  if (0==s) {
    printf("Error: couldn't initialize stream from seed %d\n", seed);
    return 1;
  }
  for (; N; ) {
    double x = s->Uniform32();
    double y = s->Uniform32();

    if (x<lower) continue;
    if (y<lower) continue;
    if (x>upper) continue;
    if (y>upper) continue;

    printf("%lf  %lf\n", x, y);
    fflush(stdout);
    N--;
  }

  // not strictly necessary, but good practice
  delete s;
  delete rm;
  return 0;
}
