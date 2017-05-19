
// $Id$

/*  Extremely simple (monte carlo) simulation test:

    Rolling 6-sided dice

*/

#include "sim.h"

#include <stdio.h>
#include <stdlib.h>

// #define DEBUG


class my_expt : public sim_experiment {
int die;
public:
  my_expt() : sim_experiment() { die = 6; }
  virtual ~my_expt() { }

  virtual void PerformExperiment(sim_outcome* outlist, int n);
};


void my_expt::PerformExperiment(sim_outcome* outlist, int n)
{
#ifdef DEBUG
  if (n!=7) {
    printf("Error, expected exactly 7 measures, got %d instead\n", n);
    exit(0);
  }
#endif

  die = (die % 6) + 1;

  // set measures
  outlist[0].is_valid = true;
  outlist[0].value = die;
  int i;
  for (i=1; i<7; i++) {
    outlist[i].is_valid = true;
    outlist[i].value = (i==die) ? 1.0 : 0.0;
  }
}


void DumpResults(sim_confintl* measures, int num)
{
  printf("Results:\n");

  for (int i=0; i<num; i++) {
    if (0==i) printf("E[die face]:\n");
    else printf("Prob{die face = %d}:\n", i);
    if (measures[i].is_valid) {
      printf("%f +- %f  \t", measures[i].average, measures[i].half_width);
      printf("(%2.1f%% confidence)\n", 100*measures[i].confidence);
      printf("\t%ld samples taken;  ", measures[i].samples);
      printf("\tvariance: %f\n", measures[i].variance);
    } else {
      printf("\tinvalid\n");
    }
    printf("\n");
  }
}

int main()
{
  const char* version = SIM_LibraryVersion();
  printf("Using %s\n", version);

  my_expt foo;

  sim_confintl* measures = new sim_confintl[7];

  printf("Starting dice simulation, variable number of iterations\n");
  SIM_MonteCarlo_S(&foo, measures, 7, 0.99, 0.01);
  DumpResults(measures, 7);

  printf("Starting dice simulation, variable half-widths\n");
  SIM_MonteCarlo_W(&foo, measures, 7, 10000000, 0.99);
  DumpResults(measures, 7);

  printf("Starting dice simulation, variable confidence\n");
  SIM_MonteCarlo_C(&foo, measures, 7, 10000000, 0.01);
  DumpResults(measures, 7);

  delete[] measures;
}
