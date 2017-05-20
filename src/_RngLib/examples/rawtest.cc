
#include "rng.h"

#include <stdio.h>

const int seed = 123456789;
const unsigned last = 633013247;

int main()
{
  rng_manager* rm = RNG_MakeStreamManager();
  rng_stream* s = rm->NewStreamFromSeed(seed);
  if (0==s) {
    printf("Error: couldn't initialize stream from seed %d\n", seed);
    return 1;
  }
  unsigned int x;
  for (int i=1000; i; i--) {
    x = s->RandomWord();
    printf("%u ", x);
  }
  printf("\n\n");
  delete s;
  delete rm;
  if (x != last) {
    printf("Stream has changed????\n");
  } else {
    printf("OK: last stream value matches.\n");
  }
  return 0;
}
