
// $Id$

#ifndef MC_LLM_H
#define MC_LLM_H

#include "stoch_llm.h"

namespace MCLib {
  class Markov_chain;
};

namespace StateLib {
  class state_db;
};

class model_enum;
class LS_Vector;
class expl_rss_only;
class exprman;

namespace StateLib {
  class state_db;
}

// ******************************************************************
// *                                                                *
// *                       markov_lldsm class                       *
// *                                                                *
// ******************************************************************

class LS_Options;
class timer;

class markov_lldsm : public stochastic_lldsm {
  static LS_Options* lsopts;
  // options, shared by all Markov-chain low-level models.
  static int solver;
  static const int GAUSS_SEIDEL = 0;
  static const int JACOBI       = 1;
  static const int ROW_JACOBI   = 2;
  static const int NUM_SOLVERS  = 3;
  static named_msg report;
  static int access;
  static const int BY_COLUMNS = 0;
  static const int BY_ROWS    = 1;
  friend void InitMCLibs(exprman* em);
public:
  markov_lldsm(bool discr);
  inline static bool storeByRows() { return BY_ROWS == access; }
protected:
  virtual ~markov_lldsm();
  static const LS_Options& getSolverOptions();
  
  void startTransientReport(double t) const;
  void stopTransientReport(long iters) const;
  void startSteadyReport() const;
  void stopSteadyReport(long iters) const;
  void startTTAReport() const;
  void stopTTAReport(long iters) const;
  void startAccumulatedReport(double t) const;
  void stopAccumulatedReport(long iters) const;
  
  static const char* getSolver();
private:
  timer* watch;
};

// ******************************************************************
// *                                                                *
// *                           Front  end                           *
// *                                                                *
// ******************************************************************

void InitMCLibs(exprman* em);

/** Build an enumerated Markov chain model.
      @param  init  Initial distribution
      @param  ss    Enumerated states (state space)
      @param  mc    Finished Markov chain

      @return A new low-level model.
*/
stochastic_lldsm* 
MakeEnumeratedMC(LS_Vector &init, model_enum* ss, MCLib::Markov_chain* mc);


/** Start an explicit Markov chain model.
      @param  dtmc  Is it a DTMC?  Otherwise, CTMC.
      @param  ss    State space

      @return A new low-level model, or 0 on error.
*/
stochastic_lldsm* StartExplicitMC(bool dtmc, StateLib::state_db* ss);

/** Get states from an explicit Markov chain model.
      @param  mc  Markov chain.

      @return The state space, or 0 on error (e.g., wrong type)
*/
StateLib::state_db* GrabExplicitMCStates(lldsm* mc);

/** Given a Markov chain model with only a state space,
    specify the rest of the Markov chain.
*/
void FinishExplicitMC(lldsm* m, LS_Vector &i, MCLib::Markov_chain* mc);

/** Given a Markov chain model with only a state space,
    specify the rest of the Markov chain.
    Phase type version: we must specify the accepting and trap states
    (or -1 for none).
*/
void FinishExplicitMC(lldsm* m, LS_Vector &i, long acc, long trap, MCLib::Markov_chain* mc);

#endif

