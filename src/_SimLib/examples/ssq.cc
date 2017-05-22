
/*  Model version of a single server queue.
*/

#include <stdio.h>
#include "sim-models.h"

#include "rngs.h"
#include "rvgs.h"

class sim_state {
public:
  int num_in_node;
};

class my_model : public sim_model {
public:
  my_model() : sim_model(2) { }
  virtual ~my_model() { }

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
    state->num_in_node = 0;
    return 0;
  }

  virtual int FillNextState(int evno, const sim_state* curr, sim_state* next) {
    if (NULL==curr) return 2;
    if (NULL==next) return 3;
    switch (evno) {
      case 0:  // arrival
          next->num_in_node = curr->num_in_node + 1;
          return 0;

      case 1: // departure
          next->num_in_node = curr->num_in_node - 1;
          return 0;
    }
    return 4;  
  }

  virtual bool IsEventEnabled(int evno, const sim_state* state) {
    if (NULL==state) return false;
    switch (evno) {
      case 0: // arrival, always enabled
          return true;

      case 1: // departure
          return (state->num_in_node > 0);
    }
    // unknown event, play it safe
    return false;
  }

  virtual double GenerateEventTime(int evno, const sim_state*) {
    switch (evno) {
      case 0:  // generate interarrival time
          return Exponential(2.0);

      case 1: // generate service time
          return Erlang(5, 0.3);
    }
    // unknown event
    return -1.0;
  }

  virtual double GetEventSpeed(int, const sim_state* ) {
    return 1.0;
  }

  virtual int EvaluateMeasures(const sim_state* state, int N, void*, double* vlist) {
    if (N!=3) return 5;
    if (NULL==vlist) return 6;
    // measure 0: average number in node
    vlist[0] = state->num_in_node;
    // measure 1: average number in queue
    vlist[1] = (state->num_in_node > 0) ? (state->num_in_node - 1) : 0.0;
    // measure 2: server utilization
    vlist[2] = (state->num_in_node > 0) ? 1.0 : 0.0;
    return 0;
  }
};


int main()
{
  printf("Creating ssq model\n");
  
  my_model mdl;
  
  printf("Simulating model\n");
  double avgs[3];
  int error = SIM_BatchMeans(&mdl, 3, NULL, avgs);

  if (error != 0) {
    printf("Got error code: %d\n", error);
  } else {
    printf("Success.  Got the following measures:\n\n");
    printf("Average number in node ......... %6.4f\n", avgs[0]);
    printf("Average number in queue ........ %6.4f\n", avgs[1]);
    printf("Average utilization ............ %6.4f\n", avgs[2]);
  }
  return 0;
}
