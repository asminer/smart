
#include "sim.h"

#include "normal.h"

#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// #define DEBUG

const int MAJOR_VERSION = 1;  // (significant) backend algorithm changes
const int MINOR_VERSION = 3;  // interface changes

// Global constants

const int MIN_SAMPLES = 100;

const int MAX_NONSTOP = 100000;

inline int MAX(int a, int b) { return (a>b) ? a : b; }

// ******************************************************************
// *                                                                *
// *                        helper functions                        *
// *                                                                *
// ******************************************************************


void MCSim(sim_experiment* expt, sim_confintl* estlist, int n, 
     sim_outcome* results, long iters)
{
#ifdef DEBUG
  printf("Starting %d MC sim iterations\n", iters); 
#endif

  for (; iters; iters--) {
    int m;
 
    // perform experiment
    expt->PerformExperiment(results, n);

    // update measures
    bool all_invalid = true;
    for (m=n-1; m>=0; m--) if (estlist[m].is_valid) {
      if (results[m].is_valid) {
        all_invalid = false;
        // welford's algorithm
        estlist[m].samples++;
        double d = results[m].value - estlist[m].average;
        double doveri = d/estlist[m].samples;
        estlist[m].variance += d * doveri * (estlist[m].samples-1);
        estlist[m].average += doveri;
      } else {
        // measure now invalid
        estlist[m].is_valid = false;
      }
    } // for valid measures

    if (all_invalid) return;
  } // for iters
}


inline int DecideIterations(double estimated_iters)
{
  if (estimated_iters < 2) return 1;
  estimated_iters *= 0.75;
  if (estimated_iters > MAX_NONSTOP) return MAX_NONSTOP;
  return int(estimated_iters);
}

// ******************************************************************
// *                                                                *
// *                       frontend functions                       *
// *                                                                *
// ******************************************************************

const char* SIM_LibraryVersion()
{
  static char buffer[100];
  snprintf(buffer, sizeof(buffer), "Simulation library version %d.%d", 
    MAJOR_VERSION, MINOR_VERSION);
  return buffer;

  // TBD - revision number?
}



void SIM_MonteCarlo_S(sim_experiment* expt, sim_confintl* estlist, int n,
        float conf, double prec)
{
  // allocate experiment outcomes
  sim_outcome* results = (sim_outcome*) malloc(n*sizeof(sim_outcome));

  // get critical value
  double alpha = 1.0 - double(conf);
  double tstar = idfStdNormal(1.0 - alpha/2.0);

#ifdef DEBUG
  printf("Confidence is: %f\n", conf);
  printf("Alpha is: %f\n", alpha);
  printf("Critical value is: %f\n", tstar);
#endif

  // initialize estimates
  int m;
  for (m=n-1; m>=0; m--) {
    estlist[m].is_valid = true;
    estlist[m].average = 0.0;
    estlist[m].variance = 0.0;
    estlist[m].samples = 0;
    estlist[m].confidence = conf;
  }

  int batch_size = MIN_SAMPLES;

  for ( ; ; ) {

    MCSim(expt, estlist, n, results, batch_size);
 
    // check if desired precision is reached
    batch_size = 0;
    for (m=n-1; m>=0; m--) if (estlist[m].is_valid) {
      double desired_hw = estlist[m].average * prec;
      double v = estlist[m].variance / estlist[m].samples;
      estlist[m].half_width = tstar * sqrt(v/(estlist[m].samples-1));
      if (estlist[m].half_width > desired_hw) {
        // precision not reached, estimate required number of iterations
        // ts * s / sqrt(i-1) <= dhw
        // ts * s / dhw <= sqrt(i-1)
        // v * ts * ts / (dhw * dhw) <= i-1  
        double needed_iters = v * tstar * tstar / (desired_hw * desired_hw);  
        int myiters = DecideIterations(needed_iters-estlist[m].samples);
        batch_size = MAX(batch_size, myiters);
      } 
    } // for valid measures

    if (batch_size < 1) break;
  } 

  // finalize measures
  for (m=n-1; m>=0; m--) if (estlist[m].is_valid) {
    // update stats
    estlist[m].variance /= estlist[m].samples;
  }

  // cleanup
  free(results);
}



void SIM_MonteCarlo_W(sim_experiment* expt, sim_confintl* estlist, int n,
        long iters, float conf)
{
  // allocate experiment outcomes
  sim_outcome* results = (sim_outcome*) malloc(n*sizeof(sim_outcome));

  // get critical value
  double alpha = 1.0 - double(conf);
  double tstar = idfStdNormal(1.0 - alpha/2.0);

#ifdef DEBUG
  printf("Confidence is: %f\n", conf);
  printf("Alpha is: %f\n", alpha);
  printf("Critical value is: %f\n", tstar);
#endif

  // initialize estimates
  int m;
  for (m=n-1; m>=0; m--) {
    estlist[m].is_valid = true;
    estlist[m].average = 0.0;
    estlist[m].variance = 0.0;
    estlist[m].samples = 0;
    estlist[m].confidence = conf;
  }

  MCSim(expt, estlist, n, results, iters);

  // finalize stats
  for (m=n-1; m>=0; m--) if (estlist[m].is_valid) {
    estlist[m].variance /= estlist[m].samples;
    estlist[m].half_width = tstar * sqrt(estlist[m].variance/(estlist[m].samples-1));
  } // for valid measures

  // cleanup
  free(results);
}


void SIM_MonteCarlo_C(sim_experiment* expt, sim_confintl* estlist, int n,
        long iters, double prec)
{
  // allocate experiment outcomes
  sim_outcome* results = (sim_outcome*) malloc(n*sizeof(sim_outcome));

  // initialize estimates
  int m;
  for (m=n-1; m>=0; m--) {
    estlist[m].is_valid = true;
    estlist[m].average = 0.0;
    estlist[m].variance = 0.0;
    estlist[m].samples = 0;
  }

  MCSim(expt, estlist, n, results, iters);

  // finalize stats
  for (m=n-1; m>=0; m--) if (estlist[m].is_valid) {
    estlist[m].variance /= estlist[m].samples;
    estlist[m].half_width = estlist[m].average * prec;

    double tstar = estlist[m].half_width * sqrt((estlist[m].samples-1)/estlist[m].variance);

    // find the level of confidence for this tstar
    double alpha = 2.0 * (1-cdfStdNormal(tstar));
    estlist[m].confidence = 1.0 - alpha;

#ifdef DEBUG
    printf("For desired critical value %f\n", tstar);
    printf("Alpha is: %f\n", alpha);
    printf("Confidence is: %f\n", estlist[m].confidence);
#endif

  } // for valid measures

  // cleanup
  free(results);
}


