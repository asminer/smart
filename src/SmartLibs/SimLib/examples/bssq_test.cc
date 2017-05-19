
// $Id$

/* 
  Test of a simple bounded queue.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "tester.h"
#include "rng.h"

// #define THEORY_DEBUG

inline int MAX(int a, int b) { return (a > b) ? a : b; }
inline int MIN(int a, int b) { return (a > b) ? b : a; }

class sim_state {
public:
  int jobs;
};

class sms_model : public sim_model {
  int bound;
  double arrival_rate;
  double service_rate;
  rng_stream* s;
public:
  sms_model(rng_stream* myrngs, int B, double A, double S) : sim_model(2) { 
    bound = B;
    arrival_rate = A;
    service_rate = S;
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
    state->jobs = 0;
    return 0;
  }

  virtual int FillNextState(int evno, const sim_state* curr, sim_state* next) {
    switch (evno) {
      case 0:  // arrival
          next->jobs = MIN(curr->jobs+1, bound);
          return 0;

      case 1:  // departure
          next->jobs = MAX(curr->jobs-1, 0);
          return 0;
    }
    return 1;
  }

  virtual bool IsEventEnabled(int evno, const sim_state* state) {
    switch (evno) {
      case 0:  // arrival
          return state->jobs < bound;

      case 1:  // departure
          return state->jobs > 0;
    }
    return false;
  }

  virtual double GenerateEventTime(int evno, const sim_state*) {
    switch (evno) {
      case 0:  // arrival
          return Exponential(arrival_rate);

      case 1:  // departure
          return Exponential(service_rate);
    }
    return -1;
  }

  virtual double GetEventSpeed(int evno, const sim_state* state) {
    return 1.0;
  }

  virtual int EvaluateMeasures(const sim_state* state, int N, void*, double* vlist) {
    vlist[0] = state->jobs;    // number in node
    vlist[1] = MAX(0, state->jobs-1);  // number in queue
    vlist[2] = MIN(1, state->jobs);  // utilization
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

  int bound = 10;
  double lambda = 0.8;
  double mu = 1.0;

  printf("Enter node capacity:\n");
  scanf("%d", &bound);
  printf("Enter arrival rate:\n");
  scanf("%lf", &lambda);
  printf("Enter service rate:\n");
  scanf("%lf", &mu);

  double* probvector = new double[bound+1];
  double total = 0.0;
  if (lambda < mu) {
    probvector[0] = 1.0;
    for (int i=1; i<=bound; i++) 
        probvector[i] = probvector[i-1] * (lambda/mu);
    for (int i=bound; i>=0; i--) 
        total += probvector[i];
  } else {
    probvector[bound] = 1.0;
    for (int i=bound-1; i>=0; i--) 
        probvector[i] = probvector[i+1] * (mu/lambda);
    for (int i=0; i<=bound; i++) 
        total += probvector[i];
  }
  for (int i=0; i<=bound; i++) 
  probvector[i] /= total;

#ifdef THEORY_DEBUG
  printf("Theoretical probability vector:\n[%lf", probvector[0]);
  for (int i=1; i<=bound; i++) {
    printf(", %lf", probvector[i]);
  }
  printf("]\n");
#endif

  double theory[3];
  theory[0] = 0.0;
  theory[1] = 0.0;
  theory[2] = 1.0 - probvector[0];
  if (lambda < mu) {
    // probability vector is decreasing, add from the right
    for (int i=bound; i; i--) {
      theory[0] += i*probvector[i];
      theory[1] += (i-1)*probvector[i];
    }
  } else {
    // probability vector is increasing, add from the left
    for (int i=1; i<=bound; i++) {
      theory[0] += i*probvector[i];
      theory[1] += (i-1)*probvector[i];
    }
  }
  printf("Theoretical (steady-state) results:\n");
  printf("\tAverage number in node.... %lf\n", theory[0]);
  printf("\tAverage number in queue... %lf\n", theory[1]);
  printf("\tUtilization............... %lf\n", theory[2]);

  printf("Starting simulation\n");
  rng_manager* foo = RNG_MakeStreamManager();
  printf("Using %s\n", foo->GetVersion());
  sms_model bar(foo->NewStreamFromSeed(123456789), bound, lambda, mu);

  return TestSim(argc, argv, &bar, 3, theory); 
}
