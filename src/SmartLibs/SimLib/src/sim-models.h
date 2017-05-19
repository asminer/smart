
// $Id$

/*! \file sim-models.h
    
    Model simulation library.

    To define a model, derive a class from sim_model.

*/

#ifndef SIM_MODELS
#define SIM_MODELS

/** Struct to return a confidence interval.
    Used to measure the average of an outcome.

    The confidence interval obtained (if valid)
    is "average +- half_width".
    
    For completeness, the struct also contains
    the number of samples taken so far, and the confidence level.
    The variance is also included, in case we want to
    collect more samples.
*/
struct sim_confintl {
  /// midpoint of the interval.
  double average;
  /// variance of measurements.
  double variance;
  /// number of samples used to obtain this estimate
  long samples;
  /// Level of confidence, between 0 and 1 (e.g., 0.95 for 95%).
  float confidence;
  /// half width of interval (absolute).
  double half_width;
  /// autocorrelation for sample data with lag = 1 .
  double autocorrelation;
};

/** Abstract type.
    (Must be defined in the application.)
    State of a model.
    We use this definition to allow typechecking.
*/
class sim_state;

/** Abstract class for models.
    To simulate a model, you must derive a class from this one,
    and provide all the virtual functions.

    Conventions:

    - Events are indexed by integers between 0 and \a num_events - 1.

    - If several events are enabled at the same time, then the one
      with the smallest remaining firing time fires first.
*/
class sim_model {
protected:
  /// Number of events.
  int num_events;
public:
  /** Constructor.
        @param  ne  Number of events in the model.
  */
  sim_model(int ne) { num_events = ne; }


  /// Destructor.
  virtual ~sim_model() { }


  /// Return the number of events.
  inline int NumberOfEvents() const { return num_events; }


  /** Allocate memory to store a model's "state".

        @param  model  The model of interest.

        @return    NULL,  if the model is invalid, or not enough memory.
                          A pointer to a new sim_state object, otherwise.
  */
  virtual sim_state* CreateState() const = 0;
      

  /** Deallocate memory used to store a model's "state".

        @param  state   NULL, or a valid sim_state object.
                        The state will be destroyed and
                        the pointer set to NULL.
  */
  virtual void   DestroyState(sim_state* &state) const = 0;


  /** Fill \a state with the initial state of the model.

        @param  state  Object to be filled with state info.
  
        @return    0 on success, nonzero on failure.
  */
  virtual int   FillInitialState(sim_state* state) = 0;


  /** Determine the new state, from the current state, if an event occurs.

        @param  evno  Index of the event that occurs
                      (assumed to be enabled in the given state).
        @param  curr  The current state (input).
        @param  next  The state reached (output).

        @return    0 on success, nonzero on failure.
  */
  virtual int  FillNextState(int evno, const sim_state* curr, sim_state* next) = 0;


  /** Determine if an event is enabled in the current state.

        @param  evno    Index of the event.
        @param  state   Current state of the model.

        @return    true, if the event is enabled in the given state.
                  false, otherwise.
  */
  virtual bool  IsEventEnabled(int evno, const sim_state* state) = 0;
  

  /** Sample a time for an (enabled) event to occur.

        @param  evno    Index of the event.
        @param  state   Current state of the model.

        @return   Non-negative "firing time" of the event,
                  or a negative time for invalid event / state.
  */
  virtual double GenerateEventTime(int evno, const sim_state* state) = 0;

  
  /** Get the current "speed" of an event.

        @param  evno    Index of the event.
        @param  state   Current state of the model.

        @return     Non-negative "speed" of the event,
                    which determines how fast the firing
                    time of the event elapses.
  */
  virtual double GetEventSpeed(int evno, const sim_state* state) = 0;


  /** Evaluate measures in the given state.

        @param  state     Current state of the model.
        @param  N         Number of measures.
        @param  msrdata   Measure data, handled by derived class.
        @param  vlist     Array (dimension \a N) of values, the current value 
                          for each measure (output).

        @return    0 on success, nonzero on failure.
  */
  virtual int  EvaluateMeasures(const sim_state* state, 
        int N, void* msrdata, double* vlist) = 0;

};



/** Compute steady-state averages for the desired measures.

      @param  m         The model of interest.
      @param  N         Number of measures.
      @param  msrdata   Measures to compute.
      @param  estlist   (Output)  
                        Array (dimension \a N) of confidence intervals
                        to estimate steady-state measures.

      @return   0 on success, nonzero on failure.
                The failure code will match the code given
                by one of the sim_model virtual functions.
*/
int SIM_BatchMeans(sim_model* m, int N, void* msrdata, sim_confintl* estlist, int batchNum, int batchSize, float percentage);  

#endif
