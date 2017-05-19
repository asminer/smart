
/* $Id$ */

/* 
  RNG Library test: 
  generate a large sample of Exponential(1) variates,
  and create a continuous histogram.

*/

#include "rng.h"

#include <stdio.h>
#include <math.h>

// #define DEBUG

inline double Exponential(rng_stream* s)
{
  return -log(s->Uniform32());
}

inline double pdf(double x)
{
  return exp(-x);
}

const int SAMPLES = 100000000;
const int BINS = 1000;
const double RIGHT = 10.0;

const double WIDTH = RIGHT / BINS;

// bin i is the interval [i*width, (i+1)*width)

int main(int argc, char** argv)
{
  const int seed = 123456789;
  rng_manager* rm = RNG_MakeStreamManager();
  rng_stream* s = rm->NewStreamFromSeed(seed);
  if (0==s) {
    printf("Error: couldn't initialize stream from seed %d\n", seed);
    return 1;
  }

  int bin;
  int counts[BINS];
  int outliers = 0;
  for (bin=0; bin<BINS; bin++) counts[bin] = 0;

  printf("# Generating %d Exponential samples...\n", SAMPLES);

  for (int n=SAMPLES; n; n--) {
    double x = Exponential(s);
    bin = int(x / WIDTH);
#ifdef DEBUG
    printf("Sample: %lf bin: %d\n", x, bin);
#endif
    if (bin>=BINS) outliers++;
    else counts[bin]++;
  }

  printf("# Done generating.\n# There were %d outliers\n", outliers);
  printf("#\n# bin   midpoint   count   density   theory\n");

  for (bin=0; bin<BINS; bin++) {
    double mid = (bin+0.5)*WIDTH;
    double density = counts[bin];
    density /= WIDTH*SAMPLES;
    printf("%d  %lf  %d  %lf  %lf\n", bin, mid, counts[bin], density, pdf(mid));
  }

  // not strictly necessary, but good practice
  delete s;
  delete rm;

  return 0;
}
