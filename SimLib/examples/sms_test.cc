
// $Id$

/*  Test of a simple machine shop model.

    This model utilizes event "speeds".
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tester.h"
#include "rng.h"

inline int MAX(int a, int b) { return (a > b) ? a : b; }
inline int MIN(int a, int b) { return (a > b) ? b : a; }

class sim_state {
public:
  int num_failed;
};

class sms_model : public sim_model {
  int machines;
  int techies;
  double fail_rate;
  double repair_rate;
  rng_stream* s;
public:
  sms_model(rng_stream* myrngs, int M, int T, double F, double R) : sim_model(2) { 
    machines = M;
    techies = T;
    fail_rate = F;
    repair_rate = R;
    s = myrngs; 
  }
  virtual ~sms_model() { delete s; }

  // required functions

  virtual sim_state* CreateState() const {
    return new sim_state;
  } 

  virtual void DestroyState(sim_state* &x) const {
    delete x;
    x = NULL;
  }

  virtual int FillInitialState(sim_state* state) {
    if (NULL==state) return 1;
    state->num_failed = 0;
    return 0;
  }

  virtual int FillNextState(int evno, const sim_state* curr, sim_state* next) {
    switch (evno) {
      case 0:  // failure
          next->num_failed = curr->num_failed+1;
          return 0;

      case 1:  // repair
          next->num_failed = curr->num_failed-1;
          return 0;
    }
    return 1;
  }

  virtual bool IsEventEnabled(int evno, const sim_state* state) {
    switch (evno) {
      case 0:  // failure
          return state->num_failed < machines;

      case 1:  // repair
          return state->num_failed > 0;
    }
    return false;
  }

  virtual double GenerateEventTime(int evno, const sim_state*) {
    switch (evno) {
      case 0:  // failure
          return Exponential(fail_rate);

      case 1:  // repair
          return Exponential(repair_rate);
    }
    return -1;
  }

  virtual double GetEventSpeed(int evno, const sim_state* state) {
    switch (evno) {
      case 0:  // failure
          return machines - state->num_failed;

      case 1:  // repair
          return MIN(techies, state->num_failed);
    }
    return 1.0;
  }

  virtual int EvaluateMeasures(const sim_state* state, int N, void*, double* vlist) {
    vlist[0] = machines - state->num_failed;
    return 0;
  }

protected:
  inline double Exponential(double rate) {
    return -log(s->Uniform32()) / rate;
  }
};


int main(int argc, const char** argv)
{
  if (HelpScreen(argc, argv))  return 0;

  int machines = 10;
  int techies = 4;
  double lambda = 1.0;
  double mu = 1.25;
  printf("Enter number of machines:\n");
  scanf("%d", &machines);
  printf("Enter number of technicians:\n");
  scanf("%d", &techies);
  printf("Enter machine failure rate:\n");
  scanf("%lf", &lambda);
  printf("Enter machine repair rate:\n");
  scanf("%lf", &mu);

  double* probvector = new double[machines+1];
  probvector[0] = 1.0;
  for (int i=1; i<=machines; i++) {
    int up = machines + 1 - i;
    int working_techs = MIN(i, techies);
    probvector[i] = probvector[i-1] * ((up*lambda)/(working_techs*mu));
  }
  int low = 0;
  int high = machines;
  double total = 0.0;
  while (low <= high) {
    if (probvector[low] < probvector[high]) {
      total += probvector[low];
      low++;
    } else {
      total += probvector[high];
      high--;
    }
  } // while
  for (int i=0; i<=machines; i++) {
    probvector[i] /= total;
  }
/*
  printf("Theoretical probability vector:\n[%lf", probvector[0]);
  for (int i=1; i<=machines; i++) {
    printf(", %lf", probvector[i]);
  }
  printf("]\n");
*/
  for (int i=0; i<=machines; i++) {
    probvector[i] *= (machines - i);
  } 
  double avg_up = 0.0;
  low = 0;
  high = machines;
  while (low <= high) {
    if (probvector[low] < probvector[high]) {
      avg_up += probvector[low];
      low++;
    } else {
      avg_up += probvector[high];
      high--;
    }
  } // while
  
  printf("\nTheoretical average up: %lf\n\n", avg_up);

  printf("Starting simulation\n");
  rng_manager* foo = RNG_MakeStreamManager();
  printf("Using %s\n", foo->GetVersion());
  sms_model bar(foo->NewStreamFromSeed(123456789), machines, techies, lambda, mu);

  return TestSim(argc, argv, &bar, 1, &avg_up); 
}
