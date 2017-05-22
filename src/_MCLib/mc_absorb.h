
#ifndef MC_ABSORB
#define MC_ABSORB

#include "mcbase.h"
#include "../_GraphLib/graphlib.h"

// ******************************************************************
// *                                                                *
// *                        mc_absorb  class                        *
// *                                                                *
// ******************************************************************

class hypersparse_matrix;

// for building known absorbing chains.
class mc_absorb : public mc_base {
  long num_absorbing;
public:
  mc_absorb(bool disc, long ns, long na);
  virtual ~mc_absorb();
  virtual long addState();
  virtual long addAbsorbing();
  virtual bool addEdge(long from, long to, double v);
  virtual void finish(const finish_options &o, renumbering &r);
  virtual void clear();
  void clearKeepAbsorbing();

  /** For vanishing chains.
      Given a rate times a vector of times spent in each transient state,
      determines the rate of entering each absorbing state
      (a simple vector-matrix multiply)
      and adds the resulting rates as edges in a Markov chain
      (with states corresponding only to absorbing states).

      @param  n      Vector of rate multiplied by times.
      @param  row    Source state in new Markov chain
      @param  R      Markov chain to dump result.
                    If we determine that we reach (absorbing)
                    state \a a with rate \a r, then we add an edge
                    from state \a row to state \a a - (number of
                    transients) with rate \a r to Markov chain \a R.
  */
  void AbsorbingRatesToMCRow(const double* n, long row, Markov_chain* R) const;

  void AbsorbingRatesToRow(const double* n, long row, hypersparse_matrix* A) const;
protected:
  void checkAbsorbing(const finish_options &o);
};

#endif
