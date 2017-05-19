
// $Id$

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "tester.h"
#include "rng.h"

class sim_state {
public:
  int which_state;
};

const int  oz_states = 3;

const double   oz_rates[9] = {  0.5, 0.25, 0.25,
                                0.5, 0, 0.5,
                                0.25, 0.25, 0.5 };

const double   oz_theory[3] = { 0.4, 0.2, 0.4 };

class chain : public sim_model {
  int states;
  const double* rates;
  rng_stream* s;
public:
  chain(rng_stream* myrngs, int ns, const double* r) : sim_model(ns*ns) { 
    s = myrngs; 
    states = ns;
    rates = r;
  }
  virtual ~chain() { delete s; }

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
    state->which_state = 0;
    return 0;
  }

  virtual int FillNextState(int evno, const sim_state* curr, sim_state* next) {
    if (evno < 0) return 1;
    if (evno >= states*states) return 2;
    if (evno / states != curr->which_state) return 3;
    next->which_state = evno % states;
    return 0;
  }

  virtual bool IsEventEnabled(int evno, const sim_state* state) {
    if (evno / states != state->which_state)  return false;
    return rates[evno] > 0;
  }

  virtual double GenerateEventTime(int evno, const sim_state*) {
    if (rates[evno] <= 0.0)  return -1;
    return Exponential(rates[evno]);
  }

  virtual double GetEventSpeed(int, const sim_state* ) {
    return 1.0;
  }

  virtual int EvaluateMeasures(const sim_state* state, int N, void*, double* vlist) {
    for (int i=0; i<N; i++)  vlist[i] = 0;
    if (state->which_state < N)  vlist[state->which_state] = 1;
    return 0;
  }

protected:
  inline double Exponential(double rate) {
    return -log(s->Uniform32()) / rate;
  }
};


int main(int argc, const char** argv)
{
  rng_manager* foo = RNG_MakeStreamManager();
  printf("Using %s\n", foo->GetVersion());
  chain oz(foo->NewStreamFromSeed(123456789), oz_states, oz_rates);

  return TestSim(argc, argv, &oz, oz_states, oz_theory); 
}
