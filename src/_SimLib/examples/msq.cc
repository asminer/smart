
// $Id$

/*  Model version of a single queue with multiple servers.
*/

#include <stdio.h>
#include "sim-models.h"

#include "rngs.h"
#include "rvgs.h"


const int SERVERS = 4;    /* Number of servers */


class sim_state {
public:
  bool* busy_servers;
  int num_servers;
  int num_in_node;
public:
  sim_state(int N) {
    busy_servers = new bool[N];
    num_servers = N;
  }
  ~sim_state() { 
    delete[] busy_servers; 
  }
};

class my_model : public sim_model {
  int num_servers;
public:
  my_model(int N) : sim_model(N+1) { 
    num_servers = N;
  }
  virtual ~my_model() { }

  // required functions
  virtual sim_state* CreateState() const {
    return new sim_state(num_servers);
  } 

  virtual void DestroyState(sim_state* &x) const {
    delete x;
    x = NULL;
  }

  virtual int FillInitialState(sim_state* state) {
    if (NULL==state) return 1;
    state->num_in_node = 0;
    for (int i=0; i<num_servers; i++) state->busy_servers[i] = false;
    return 0;
  }

  virtual int FillNextState(int evno, const sim_state* curr, sim_state* next) {
    if (NULL==curr) return 2;
    if (NULL==next) return 3;

    if (0==evno) { // arrival
      next->num_in_node = curr->num_in_node + 1;
      int s = -1;
      for (int i=0; i<num_servers; i++) {
        next->busy_servers[i] = curr->busy_servers[i]; 
        if (! next->busy_servers[i]) s = i;
      }
      if (s>=0) next->busy_servers[s] = true;
      return 0;
    }
    if (evno>num_servers) return 4;

    // departure from server "evno-1"
    next->num_in_node = curr->num_in_node - 1;
    for (int i=0; i<num_servers; i++) {
      next->busy_servers[i] = curr->busy_servers[i]; 
    }
    if (next->num_in_node < num_servers) // server goes idle
      next->busy_servers[evno - 1] = false;
    return 0;
  }

  virtual bool IsEventEnabled(int evno, const sim_state* state) {
    if (NULL==state) return false;
    if (0==evno) return true;  
    if (evno > num_servers) return false;

    // departure is enabled if the server is busy
    return state->busy_servers[evno-1];
  }

  virtual double GenerateEventTime(int evno, const sim_state*) {
    if (0==evno) return Exponential(2.0);  // interarrival time
    // must be a service time
    return Uniform(2.0, 10.0);
  }

  virtual double GetEventSpeed(int, const sim_state* ) {
    return 1.0;
  }

  virtual int EvaluateMeasures(const sim_state* state, int N, void*, double* vlist) {
    if (N!=2+num_servers) return 5;
    if (NULL==vlist) return 6;
    // measure 0: average number in node
    vlist[0] = state->num_in_node;
    // measure 1: average number in queue
    vlist[1] = (state->num_in_node > 0) ? (state->num_in_node - 1) : 0.0;
    // measures 2..num_servers+1: server utilization
    for (int i=0; i<num_servers; i++) 
      vlist[2+i] = (state->busy_servers[i]) ? 1.0 : 0.0;
    return 0;
  }
};


int main()
{
  printf("Creating msq model with %d servers\n", SERVERS);
  
  my_model mdl(SERVERS);
  
  printf("Simulating model\n");
  double avgs[2+SERVERS];
  int error = SIM_BatchMeans(&mdl, 2+SERVERS, NULL, avgs);

  if (error != 0) {
    printf("Got error code: %d\n", error);
  } else {
    printf("Success.  Got the following measures:\n\n");
    printf("Average number in node ......... %6.4f\n", avgs[0]);
    printf("Average number in queue ........ %6.4f\n", avgs[1]);
    for (int i=0; i<SERVERS; i++) {
      printf("Server %3d utilization ............ %6.4f\n", i, avgs[2+i]);
    }
  }
  return 0;
}
