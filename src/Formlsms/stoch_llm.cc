
// $Id$

#include "stoch_llm.h"

// ******************************************************************
// *                                                                *
// *                    stochastic_lldsm methods                    *
// *                                                                *
// ******************************************************************

stochastic_lldsm::stochastic_lldsm(model_type t) : checkable_lldsm(t)
{
  accept_prob = -1;
  trap_prob = -1;
  mtta = -2;
  vtta = -2;
}

long stochastic_lldsm::getAcceptingState() const
{
  return -1;
}

long stochastic_lldsm::getTrapState() const
{
  return -1;
}

void stochastic_lldsm::getNumClasses(result& x) const
{
  x.setInt(getNumClasses(false));
  if (x.getInt() < 0) {
    x.setNull();
  } 
}

long stochastic_lldsm::getNumClasses(bool show) const
{
  return bailOut(__FILE__, __LINE__, "Can't count TSCCs");
}

void stochastic_lldsm::getClass(long cl, intset &) const
{
  bailOut(__FILE__, __LINE__, "Can't find class of states");
}

bool stochastic_lldsm::isTransient(long st) const
{
  bailOut(__FILE__, __LINE__, "Can't check transient");
  return false;
}

bool stochastic_lldsm::getInitialDistribution(double* probs) const
{
  bailOut(__FILE__, __LINE__, "Can't get initial distribution");
  return false;
}

void stochastic_lldsm::buildInitialDistribution(long* &indexes, double* &probs, long &numinit) const
{
  bailOut(__FILE__, __LINE__, "Can't build (sparse) initial distribution");
}

long stochastic_lldsm::getOutgoingWeights(long from, long* to, double* w, long n) const
{
  bailOut(__FILE__, __LINE__, "Can't get outgoing weights");
  return 0;
}

bool stochastic_lldsm::computeTransient(double t, double*, double*, double*) const
{
  bailOut(__FILE__, __LINE__, "Can't compute transient");
  return false;
}

bool stochastic_lldsm::computeAccumulated(double t, const double*, double*, double*, double*) const
{
  bailOut(__FILE__, __LINE__, "Can't compute accumulated");
  return false;
}

bool stochastic_lldsm::computeSteadyState(double* probs) const
{
  bailOut(__FILE__, __LINE__, "Can't compute steady-state");
  return false;
}

bool stochastic_lldsm::computeTimeInStates(const double*, double*) const
{
  bailOut(__FILE__, __LINE__, "Can't compute time in states");
  return false;
}

bool stochastic_lldsm::computeClassProbs(const double*, double*) const
{
  bailOut(__FILE__, __LINE__, "Can't compute class probabilities");
  return false;
}

bool stochastic_lldsm::computeDiscreteTTA(double, double* &, int &) const
{
  bailOut(__FILE__, __LINE__, "Can't compute discrete TTA");
  return false;
}

bool stochastic_lldsm::computeContinuousTTA(double, double, double* &, int &) const
{
  bailOut(__FILE__, __LINE__, "Can't compute continuous TTA");
  return false;
}

bool stochastic_lldsm
::randomTTA(rng_stream &, long &, const stateset &, long, long &)
{
  bailOut(__FILE__, __LINE__, "Can't simulate discrete-time random walk");
  return false;
}

bool stochastic_lldsm
::randomTTA(rng_stream &, long &, const stateset &, double, double &)
{
  bailOut(__FILE__, __LINE__, "Can't simulate continuous-time random walk");
  return false;
}

