
#include "tester.h"

#include <stdio.h>
#include <stdlib.h>

inline double ABS(double a) { return (a<0) ? -a : a; }
inline double MAX(double a, double b) { return (a>b) ? a : b; }
inline double MIN(double a, double b) { return (a<b) ? a : b; }

int WithinCI(sim_confintl& ci, double theory, bool noisy)
{
  int contains = 0;
  if (  (ci.average - ci.half_width <= theory) &&
        (ci.average + ci.half_width >= theory) )  contains = 1;
  
  if (noisy) {
    printf("%6.4f +- %6.4f (ac %7.4f) \t", ci.average, ci.half_width, ci.autocorrelation);
    if (contains)  printf("YES\n");
    else    printf(" NO\n");
  }
  return contains;
}

int Usage(const char* argv0)
{
  printf("\nUsage: %s (switches)\n\n", argv0);
  
  printf("Switches:\n");
  printf("\t-h\tDisplay this help and exit\n");
  printf("\t-n x\tSets number of batches to x\n");
  printf("\t-q\tQuiet, cancels -v\n");
  printf("\t-r x\tSets number of replications x\n");
  printf("\t-s x\tSets batch size to x\n");
  printf("\t-v\tVerbose, cancels -q\n");
  return 0;
}

int HelpScreen(int argc, const char** argv)
{
  for (int i=1; i<argc; i++) {
    if (argv[i][0] != '-') continue;
    if (argv[i][2] != 0)   continue;
    if ('h' == argv[i][1]) {
      Usage(argv[0]);
      return 1;
    }
  }
  return 0;
}

int TestSim(int argc, const char** argv, sim_model* mdl, int N, const double* theory)
{
  double confidence = 0.95;
  bool noisy = false;
  int batch_size = 256;
  int num_batches = 1024;
  int reps = 1;
  
  for (int i=1; i<argc; i++) {
    if (argv[i][0] != '-') continue;
    if (argv[i][2] != 0)   continue;
    if ('v' == argv[i][1])  noisy = true;
    if ('q' == argv[i][1])  noisy = false;
    if ('s' == argv[i][1]) if (i+1 < argc) {
      batch_size = atoi(argv[i+1]);
      i++;
    }
    if ('n' == argv[i][1]) if (i+1 < argc) {
      num_batches = atoi(argv[i+1]);
      i++;
    }
    if ('r' == argv[i][1]) if (i+1 < argc) {
      reps = atoi(argv[i+1]);
      i++;
    }
    if ('h' == argv[i][1])  return Usage(argv[0]);
  }

  if (noisy) {
    printf("Theory:\n");
    for (int j=0; j<N; j++) {
      printf("\tMeasure %2d: \t %6.4f\n", j, theory[j]);
    }
  }

  printf("Generating %d confidence intervals (%4.1lf%%), using %d batches of size %d\n", reps, confidence*100, num_batches, batch_size);

  sim_confintl* mlist = new sim_confintl[N];
  int* count = new int[N];
  double* maxac = new double[N];
  double* minac = new double[N];

  for (int j=0; j<N; j++) {
    count[j] = 0;
    maxac[j] = 0;
    minac[j] = 1;
  }

  int dots = 0;
  for (int i=0; i<reps; i++) {
    int code = SIM_BatchMeans(mdl, N, 0, mlist, num_batches, batch_size, confidence);
    if (code) {
      printf("Simulation error (code %d)\n", code);
      return code;
    }

    if (noisy) printf("Simulation run %d:\n", i+1);
    else if (i*80 > reps*dots) {
      fprintf(stderr, ".");
      dots++;
    }

    for (int j=0; j<N; j++) {
      if (noisy) printf("\tState %2d: \t", j);
      count[j] += WithinCI(mlist[j], theory[j], noisy);
      maxac[j] = MAX(maxac[j], ABS(mlist[j].autocorrelation));
      minac[j] = MIN(minac[j], ABS(mlist[j].autocorrelation));
    }
    // TO DO: check confidence interval against theoretical and count
  }
  if (!noisy) fprintf(stderr, "\n");

  delete[] mlist;
  delete[] count;
  delete[] maxac;
  delete[] minac;

  if (reps<1) return 0;

  printf("Actual confidence levels:\n");
  for (int j=0; j<N; j++) {
    double percent = count[j] * 100.0;
    percent /= reps;
    printf("\tMeasure %d: %2.2f \tMax AC %1.4f \tMin AC %1.4f\n", 
    j, percent, maxac[j], minac[j]);
  }

  return 0;
}
