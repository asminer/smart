
/*! \file sim.h

    \brief Simulation Library Header File.

    Monte-carlo simulation is ready.

    
    CONVENTIONS:

      Functions that must be provided by the application
      (i.e., written by "the user") are prefixed by "SIMAPP_".
  
      Functions provided by the library are prefixed by "SIM_".

      Simulation functions have suffix "_S", "_W", or "_C",
      based upon how confidence intervals are constructed:

      "_S" functions iterate (generate more samples) until the specified
      half-width "precision" is achieved for the given confidence.

      "_W" functions determine the half-width for a fixed sample size
      for the given confidence.

      "_C" functions determine the level of confidence achieved for
      a fixed sample size and half-width "precision".

  

    TO DO:
    Allow Monte-carlo simulations to add samples?

    Hammer out interface for dynamic simulations (i.e., "models").

*/

#ifndef SIM_H
#define SIM_H

/** Get the name and version info of the library.
    The returned string should not be modified or free'd by the caller.
  
      @return    Information string.
*/
const char*  SIM_LibraryVersion();


/** Struct to return an observation from some experiment.
    The observation is somehow encoded into the "value".    
*/
struct sim_outcome {
  /// Did the experiment generate a valid result.
  bool is_valid;
  /// The result of the experiment.
  double value;
};


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
  /// Is this a valid result.
  bool is_valid;
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
};



/** A random experiment.
    
    To define a particular experiment, derive a class from this one,
    and implement the virtual function.
*/
class sim_experiment {
public:
  /// Constructor.
  sim_experiment() { }

  /// Destructor.
  virtual ~sim_experiment() { }
  
  /** Function to perform the experiment and measure outcomes.
      Each call to the function should perform one random
      experiment, and the measured results should be stored
      in \a olist, an array of dimension \a n.    
      It is assumed that each experiment (i.e., each function call)
      is independent of the others.
  */
  virtual void PerformExperiment(sim_outcome* olist, int n) = 0;
};





/** Monte carlo simulation with varying number of samples.

    Repeatedly performs an experiment to obtain a confidence 
    interval estimate for each of the outcomes of the experiment.

      @param  expt      Function to perform the experiment.

      @param  estlist   (Output) array of estimates, the measured
                        quantities of the experiment.

      @param  n         Number of measures (dimension of estlist array).

      @param  conf      Level of confidence (e.g., 0.95 for 95%)

      @param  prec      Desired half-width precision relative to
                        the point estimate.
*/
void SIM_MonteCarlo_S(sim_experiment* expt, sim_confintl* estlist, int n,
        float conf, double prec);

/** Monte carlo simulation with varying half-width.

    Repeatedly performs an experiment to obtain a confidence 
    interval estimate for each of the outcomes of the experiment.

      @param  expt      Function to perform the experiment.

      @param  estlist   (Output) array of estimates, the measured
                        quantities of the experiment.

      @param  n         Number of measures (dimension of estlist array).

      @param  iters     Number of iterations.

      @param  conf      Level of confidence (e.g., 0.95 for 95%)
*/
void SIM_MonteCarlo_W(sim_experiment* expt, sim_confintl* estlist, int n,
        long iters, float conf);

/** Monte carlo simulation with varying confidence.

    Repeatedly performs an experiment to obtain a confidence 
    interval estimate for each of the outcomes of the experiment.

      @param  expt      Function to perform the experiment.

      @param  estlist   (Output) array of estimates, the measured
                        quantities of the experiment.

      @param  n         Number of measures (dimension of estlist array).

      @param  iters     Number of iterations.

      @param  prec      Desired half-width precision relative to
                        the point estimate.
*/
void SIM_MonteCarlo_C(sim_experiment* expt, sim_confintl* estlist, int n,
        long iters, double prec);




#endif
