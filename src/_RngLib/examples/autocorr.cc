
/*
    Test application for RNG:
    checks the autocorrelation of the raw Uniform(0,1) sequence.

    Function "acs" is (essentially) main taken from program "acs.c" 
    in "Discrete-event Simulation: A First Course" by Leemis & Park.
    The original documentation is below.
    
    Basically, instead of reading from a file, it "reads" directly
    from a function: Generate.

    Also, the maximum lag K is a function parameter rather than a constant.

    The main program here simply initializes the random number stream.
*/

#include "rng.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

const int MIDDLE_SAMPLES = 100000000;

rng_stream* s;

inline double Generate() { return s->Uniform32(); }

int acs(int K);  // defined below...

int main()
{
  rng_manager* rm = RNG_MakeStreamManager();

  const int K = 20;

  const char* info = rm->GetVersion();
  printf("Using %s\n", info);

  const long seed = 123456789;
  s = rm->NewStreamFromSeed(seed);
  if (0==s) {
    printf("Error: couldn't initialize stream from seed %ld\n", seed);
    return 1;
  }

  printf("Theoretical mean ..... %1.4f\n", 0.5);
  printf("Theoretical stdev .... %1.4f\n", sqrt(1.0/12.0));

  printf("\nGenerating %d samples...\n\n", MIDDLE_SAMPLES + K + 1);
  fflush(stdout);

  acs(K);

  delete s;
  delete rm;
  printf("Done\n");
  return 0;
}

/* -------------------------------------------------------------------------  
 * This program is based on a one-pass algorithm for the calculation of an  
 * array of autocorrelations r[1], r[2], ... r[K].  The key feature of this 
 * algorithm is the circular array 'hold' which stores the (K + 1) most 
 * recent data points and the associated index 'p' which points to the 
 * (rotating) head of the array. 
 * 
 * Data is read from a text file in the format 1-data-point-per-line (with 
 * no blank lines).  Similar to programs UVS and BVS, this program is
 * designed to be used with OS redirection. 
 * 
 * NOTE: the constant K (maximum lag) MUST be smaller than the # of data 
 * points in the text file, n.  Moreover, if the autocorrelations are to be 
 * statistically meaningful, K should be MUCH smaller than n. 
 *
 * Name              : acs.c  (AutoCorrelation Statistics) 
 * Author            : Steve Park & Dave Geyer 
 * Language          : ANSI C
 * Latest Revision   : 2-10-97 
 * Compile with      : gcc -lm acs.c
 * Execute with      : a.out < acs.dat
 * ------------------------------------------------------------------------- 
 */

int acs(int K)
{
  int SIZE = K+1;
  long   i = 0;                   /* data point index              */
  long   cms = 0;                 /* Count for middle samples      */
  double x;                       /* current x[i] data point       */
  double sum = 0.0;               /* sums x[i]                     */
  long   n;                       /* number of data points         */
  long   j;                       /* lag index                     */
  double* hold;                   /* K + 1 most recent data points */
  hold = (double*) malloc(SIZE * sizeof(double));
  long   p = 0;                   /* points to the head of 'hold'  */
  double* cosum;                  /* cosum[j] sums x[i] * x[i+j]   */
  cosum = (double*) calloc(SIZE, sizeof(double));  // set to zero?
  double mean;

  while (i < K+1) {               /* initialize the hold array with */
    x = Generate();
    sum     += x;
    hold[i]  = x;
    i++;
  }

  for (cms = MIDDLE_SAMPLES; cms; cms--) {
    for (j = 0; j < SIZE; j++)
      cosum[j] += hold[p] * hold[(p + j) % SIZE];
    x = Generate();
    sum    += x;
    hold[p] = x;
    p       = (p + 1) % SIZE;
    i++;
  }
  n = i;

  while (i < n + SIZE) {         /* empty the circular array */
    for (j = 0; j < SIZE; j++)
      cosum[j] += hold[p] * hold[(p + j) % SIZE];
    hold[p] = 0.0;
    p       = (p + 1) % SIZE;
    i++;
  } 

  mean = sum / n;
  for (j = 0; j <= K; j++)
    cosum[j] = (cosum[j] / (n - j)) - (mean * mean);

  printf("for %ld data points\n", n);
  printf("the mean is ... %1.4f\n", mean);
  printf("the stdev is .. %1.4f\n\n", sqrt(cosum[0]));
  printf("  j (lag)   r[j] (autocorrelation)\n");
  for (j = 1; j < SIZE; j++)
    printf("%3ld  %11.4f\n", j, cosum[j] / cosum[0]);

  free(hold);
  free(cosum);
  return (0);
}
