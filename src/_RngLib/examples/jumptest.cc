
/* 
    RNG Library test: 
    compare the jump matrix to slow-but-sure stream advancement.

*/

#include "rng.h"

#include <stdio.h>

long long two2d(int d) 
{
  if (0==d) return 1;
  if (d%2 == 0) {
    long long x = two2d(d/2);
    return x*x;
  }
  long long x = two2d(d-1);
  return 2*x;
}

int main()
{
  rng_manager* rm = RNG_MakeStreamManager();
  const char* info;
  int d, min, max;
  info = rm->GetVersion();
  printf("Using %s\n", info);

  min = rm->MinimumJumpValue();
  max = rm->MaximumJumpValue();
  printf("Enter jump value d (between %d and %d): \n", min, max);
  scanf("%d", &d);

  rm->SetJumpValue(d);
  if (rm->GetJumpValue() != d) {
    printf("Error: couldn't set stream jump value to %d\n", d);
    return 1;
  }

  int seed;
  printf("Enter seed: \n");
  scanf("%d", &seed);

  rng_stream* s1;
  rng_stream* s2;

  s1 = rm->NewStreamFromSeed(seed);
  if (0==s1) {
    printf("Error: couldn't initialize stream from seed %d\n", seed);
    return 1;
  }

  printf("Jumping stream\n");

  s2 = rm->NewStreamByJumping(s1);
  if (0==s2) {
    printf("Error: couldn't jump stream\n");
    return 1;
  }
  long long steps = two2d(d);

  printf("Advancing by hand %lld steps\n", steps);
  if (steps>1000000000) printf("This might take a while...\n");
  for ( ; steps; steps--) {
    s1->RandomWord();
  }

  printf("Comparing streams\n");

  for(steps=0; steps<1024; steps++) {
    unsigned w1 = s1->RandomWord();
    unsigned w2 = s2->RandomWord();
    if (w1 != w2) {
      printf("Streams differ (words %u and %u)\n", w1, w2);
      return 1;
    }
  }

  delete s1;
  delete s2;
  delete rm;
  printf("Done\n");
  return 0;
}
